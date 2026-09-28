/**
 * glN64_GX - DepthCopy.cpp
 *
 * Wii64 homepage: https://www.github.com/emukidid/wii64
**/

#include <gccore.h>
#include <string.h>
#include "DepthCopy.h"
#include "N64.h"
#include "gDP.h"
#include "gSP.h"
#include "VI.h"
#include "OpenGL.h"
#include "Config.h"
#include "FrameBuffer.h"

#ifdef HW_RVL
#include "../gc_memory/MEM2.h"

extern "C" int getVmodeAA();
extern GXRModeObj *rmode;

// The copy is: GX_TF_Z16 of the EFB's depth, downsized, so each texel is one N64 pixel. 
// A Z16 copy of a Z24 EFB seems to hold z24 >> 8 with its two bytes swapped, which is all the table below uses.
static u8  * const dcBuf = (u8*)DEPTHCOPY_BUF_LO;

// N64 depth word for every texel value, indexed by the texel
#define DC_LUT_ENTRIES (DEPTHCOPY_LUT_SIZE / 2)
static u16 * const dcLut = (u16*)DEPTHCOPY_LUT_LO;

// Not sure if these really help considering it's in MEM2?
#define DC_PREFETCH( p )	__asm__ volatile( "dcbt 0,%0" : : "r"(p) )
#define DC_CLAIM( p )		__asm__ volatile( "dcbz 0,%0" : : "r"(p) : "memory" )

static struct
{
	BOOL cleared;			// the current display list cleared the depth buffer
	BOOL depthWritten;		// and wrote to the depth image as a colour image
	u32  clearedAddress;	// the depth image that clear was made to
	BOOL pending;			// a copy is queued for the next display list to convert
	u32  address;			// the N64 depth image it is for
	u32  width, height;		// in N64 pixels, which is also the copy's size in texels
	f32  nearz, farz;		// gSP.viewport's, which the EFB depth is relative to
	u16  clearWord;			// what a pixel nothing was drawn on gets
	BOOL lutValid;
	f32  lutNear, lutFar;
	u16  lutClear;
} dc;

static inline u16 _dcEncode( u32 value )
{
	u32 e = __builtin_clz( ~(value << 14) );
	if (e > 7)
		e = 7;

	const u32 shift = (e < 6) ? 6 - e : 0;

	return (u16)((e << 13) | (((value >> shift) & 0x7FF) << 2));
}

static inline u32 _dcZ24( f32 v )
{
	const f64 z = (f64)v * 16777215.0 + 0.5;

	return (z <= 0.0) ? 0 : ((z >= 16777215.0) ? 0xFFFFFF : (u32)z);
}

static void _dcBuildLut( f32 nearz, f32 farz, u16 clearWord )
{
	const f32 range = (farz != nearz) ? (farz - nearz) : 1.0f;
	const u32 z0 = _dcZ24( nearz );
	const u32 k = (u32)((261632.0 / (16777215.0 * (f64)range)) * 4294967296.0);

	const u32 zFar = _dcZ24( farz );

	for (u32 idx = 0; idx < DC_LUT_ENTRIES; idx++)
	{
		// The texel is z24 >> 8, bytes swapped; the middle of its 256 z24s stands for it.
		const u32 z = ((idx & 0xFF) << 16) | (idx & 0xFF00) | 0x80;

		// A bucket that reaches the far plane is a clear: with the viewport's far at 1.0
		// that is only the top one, whose middle is short of 0xFFFFFF. It has to come out
		// as the clear word exactly.
		if ((z | 0xFF) >= zFar)
		{
			dcLut[idx] = clearWord;
			continue;
		}

		u32 value = (z > z0) ? (u32)(((u64)(z - z0) * k) >> 32) : 0;
		if (value > 261632)
			value = 261632;

		dcLut[idx] = _dcEncode( value );
	}

	dc.lutValid = TRUE;
	dc.lutNear = nearz;
	dc.lutFar = farz;
	dc.lutClear = clearWord;
}

static void _dcConvert( u16 *dst, u32 width, u32 height )
{
	const u32 tilesPerRow = width >> 2;
	const u32 claim = ((((u32)dst) & 31) == 0) && ((width & 15) == 0);
	const u16 *tile = (const u16*)dcBuf;

	for (u32 ty = 0; ty < (height >> 2); ty++)
	{
		u16 *rowStart = dst + (ty << 2) * width;

		for (u32 tx = 0; tx < tilesPerRow; tx++, tile += 16)
		{
			DC_PREFETCH( tile + 16 * 4 );

			u16 *out = rowStart + (tx << 2);
			const BOOL claimLine = claim && ((tx & 3) == 0);

			for (u32 r = 0; r < 4; r++, out += width)
			{
				if (claimLine)
					DC_CLAIM( out );

				const u16 *t = tile + (r << 2);
				out[0] = dcLut[t[0]];
				out[1] = dcLut[t[1]];
				out[2] = dcLut[t[2]];
				out[3] = dcLut[t[3]];
			}
		}
	}
}

void DepthCopy_Reset()
{
	memset( &dc, 0, sizeof(dc) );
}

BOOL DepthCopy_Active()
{
	// expensive, only do it on games we want to
	if ((config.generalEmulation.hacks & hack_copyDepthToRDRAM) == 0)
		return FALSE;

	// too hard, I couldn't work it out under AA :(
	if (getVmodeAA())
		return FALSE;

	// The GP's box filter halves the copy, so the EFB has to hold the frame at exactly
	// 2x the size and tiles are 4x4 and output lines 16 pixels wide.
	if (OGL.GXscaleX != 2.0f || OGL.GXscaleY != 2.0f)
		return FALSE;

	if (VI.width == 0 || VI.height == 0 || (VI.width & 15) != 0 || (VI.height & 3) != 0)
		return FALSE;

	// Bigger than the copy buffer?
	if ((VI.width * VI.height * 2) > DEPTHCOPY_BUF_SIZE)
		return FALSE;

	return TRUE;
}

// Writes the clear value over the whole depth image. What the game saw before the copy
// existed, so a frame whose copy is skipped reads as unoccluded rather than stale.
static void _dcFillClear( u32 address, u32 width, u32 height, u16 word )
{
	const u32 count = width * height;

	if (address == 0 || address + count * 2 > RDRAMSize)
		return;

	u16 *p = (u16*)&RDRAM[address];
	for (u32 i = 0; i < count; i++)
		p[i] = word;

	FrameBuffer_RestampMarkers( address, address + count * 2 - 1 );
}

void DepthCopy_BeginDList()
{
	dc.cleared = FALSE;
	dc.depthWritten = FALSE;

	if (!dc.pending)
		return;

	dc.pending = FALSE;

	// Settings may have changed since the copy was queued.
	if (!DepthCopy_Active() || VI.width != dc.width || VI.height != dc.height)
		return;

	const u32 bytes = dc.width * dc.height * 2;
	if (dc.address + bytes > RDRAMSize)
		return;

	// Normally finished anyway, the GP has had the rest of the previous frame to do it.
	GX_DrawDone();
	DCInvalidateRange( dcBuf, bytes );

	if (!dc.lutValid || dc.lutNear != dc.nearz || dc.lutFar != dc.farz || dc.lutClear != dc.clearWord)
		_dcBuildLut( dc.nearz, dc.farz, dc.clearWord );

	u16 *dst = (u16*)&RDRAM[dc.address];
	_dcConvert( dst, dc.width, dc.height );

	FrameBuffer_RestampMarkers( dc.address, dc.address + bytes - 1 );
}

void DepthCopy_EndDList()
{
	const BOOL cleared = dc.cleared;
	const BOOL depthWritten = dc.depthWritten;

	dc.cleared = FALSE;
	dc.depthWritten = FALSE;

	const BOOL active = DepthCopy_Active();
	BOOL skip = !active;

	if (!skip)
	{
		if (!cleared)
			skip = TRUE;	// the list never cleared the depth buffer
		else if (depthWritten)
			skip = TRUE;	// the depth image was drawn to as a colour image
		else if (gDP.depthImageAddress == 0)
			skip = TRUE;	// no depth image set
		else if (gDP.depthImageAddress + VI.width * VI.height * 2 > RDRAMSize)
			skip = TRUE;	// the depth image runs past RDRAM
	}

	if (skip)
	{
		// The clear this list made was kept out of RDRAM while the copy was running, and
		// there is no copy to put there instead so fall back to what it would have written.
		if (active && cleared)
			_dcFillClear( dc.clearedAddress, VI.width, VI.height, (u16)(DepthClearColor >> 16) );
		return;
	}

	const u32 width = VI.width;
	const u32 height = VI.height;

	// Whatever is still batched or waiting belongs in the depth being copied.
	if (OGL.numTriangles)
		OGL_DrawTriangles();
	OGL_ApplyPendingClears();

	GX_SetTexCopySrc( OGL.GXorigX, OGL.GXorigY, width << 1, height << 1 );
	GX_SetTexCopyDst( width, height, GX_TF_Z16, GX_TRUE );
	GX_SetCopyFilter( GX_FALSE, NULL, GX_FALSE, NULL );
	GX_CopyTex( dcBuf, GX_FALSE );
	GX_PixModeSync();
	GX_SetCopyFilter( rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter );

	dc.pending   = TRUE;
	dc.address   = gDP.depthImageAddress;
	dc.width     = width;
	dc.height    = height;
	dc.nearz     = gSP.viewport.nearz;
	dc.farz      = gSP.viewport.farz;
	dc.clearWord = (u16)(DepthClearColor >> 16);
}

void DepthCopy_NoteDepthClear()
{
	dc.cleared = TRUE;
	dc.clearedAddress = gDP.depthImageAddress;
}

void DepthCopy_NoteDepthImageWritten()
{
	dc.depthWritten = TRUE;
}

#else // HW_RVL

// Not for GameCube, at least not now anyway, maybe someday.
void DepthCopy_Reset() {}
BOOL DepthCopy_Active() { return FALSE; }
void DepthCopy_BeginDList() {}
void DepthCopy_EndDList() {}
void DepthCopy_NoteDepthClear() {}
void DepthCopy_NoteDepthImageWritten() {}

#endif // HW_RVL
