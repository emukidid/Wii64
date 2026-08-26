/**
 * glN64_GX - ZSortBOSS.cpp
 *
 * ZSortBOSS ucode: World Driver Championship, Stunt Racer 64.
 *
 * Ported from https://github.com/gonetz/GLideN64/blob/master/src/uCodes/ZSortBOSS.cpp
**/

#ifdef __GX__
#include <gccore.h>
#endif // __GX__

#include <math.h>
#include <algorithm>
#include "glN64.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "F3D.h"
#include "3DMath.h"
#include "OpenGL.h"
#include "VI.h"
#include "Combiner.h"
#include "Textures.h"
#include "DepthBuffer.h"
#include "GBI.h"
#include "ZSortBOSS.h"
#include "../gui/DEBUG.h"

#define CLAMP(x, lo, hi) ((x > hi) ? hi : ((x < lo) ? lo : x))
#define SATURATES8(x) ((x > 127) ? 127 : ((x < -128) ? -128 : x))

// We probably never need the non big endian but whatever
#ifndef _BIG_ENDIAN
# define ZSB_HW(i)	((i) ^ 1)
# define ZSB_BY(i)	((i) ^ 3)
#else // !_BIG_ENDIAN
# define ZSB_HW(i)	(i)
# define ZSB_BY(i)	(i)
#endif // _BIG_ENDIAN

#define	ZH_NULL		0
#define	ZH_TXTRI	4
#define	ZH_TXQUAD	8

#define DEFAULT		0
#define PROCESSED	1

struct ZSortBOSSState
{
	u32 maindl;
	u32 subdl;
	u32 updatemask[2];
	f32 view_scale[2];
	f32 view_trans[2];
	u32 rdpcmds[3];
	f32 invw_factor;
	u8 fogtable[256];
	s16 table[8][8];
	bool waiting_for_signal;
};

static struct ZSortBOSSState zsbState;

int ZSortBOSS_Calc_invw( int _w )
{
	if (_w == 0)
		return 0x7FFFFFFF;
	return 0x7FFFFFFF / _w;
}

void ZSortBOSS_RDPCMD( u32, u32 _w1 )
{
	u32 addr = RSP_SegmentToPhysical( _w1 ) >> 2;
	if (addr == 0)
		return;

	while (TRUE)
	{
		if (((addr << 2) + 4) > RDRAMSize)
			break;
		u32 w0 = ((u32*)RDRAM)[addr++];
		RSP.cmd = _SHIFTR( w0, 24, 8 );
		if (RSP.cmd == 0xDF) // G_ENDDL
		{
			break;
		}
		if (((addr << 2) + 4) > RDRAMSize)
			break;
		u32 w1 = ((u32*)RDRAM)[addr++];

		if (RSP.cmd == G_TEXRECT || RSP.cmd == G_TEXRECTFLIP)
		{
			if (((addr << 2) + 16) > RDRAMSize)
				break;
			addr++;
			u32 w2 = ((u32*)RDRAM)[addr++];
			addr++;
			u32 w3 = ((u32*)RDRAM)[addr++];

			if (RSP.cmd == G_TEXRECTFLIP)
				gDPTextureRectangleFlip( _FIXED2FLOAT( (u16)_SHIFTR( w1, 12, 12 ), 2 ),
										 _FIXED2FLOAT( (u16)_SHIFTR( w1,  0, 12 ), 2 ),
										 _FIXED2FLOAT( (u16)_SHIFTR( w0, 12, 12 ), 2 ),
										 _FIXED2FLOAT( (u16)_SHIFTR( w0,  0, 12 ), 2 ),
										 _SHIFTR( w1, 24,  3 ),
										 _FIXED2FLOAT( (s16)_SHIFTR( w2, 16, 16 ), 5 ),
										 _FIXED2FLOAT( (s16)_SHIFTR( w2,  0, 16 ), 5 ),
										 _FIXED2FLOAT( (s16)_SHIFTR( w3, 16, 16 ), 10 ),
										 _FIXED2FLOAT( (s16)_SHIFTR( w3,  0, 16 ), 10 ) );
			else
				gDPTextureRectangle( _FIXED2FLOAT( (u16)_SHIFTR( w1, 12, 12 ), 2 ),
									  _FIXED2FLOAT( (u16)_SHIFTR( w1,  0, 12 ), 2 ),
									  _FIXED2FLOAT( (u16)_SHIFTR( w0, 12, 12 ), 2 ),
									  _FIXED2FLOAT( (u16)_SHIFTR( w0,  0, 12 ), 2 ),
									  _SHIFTR( w1, 24,  3 ),
									  _FIXED2FLOAT( (s16)_SHIFTR( w2, 16, 16 ), 5 ),
									  _FIXED2FLOAT( (s16)_SHIFTR( w2,  0, 16 ), 5 ),
									  _FIXED2FLOAT( (s16)_SHIFTR( w3, 16, 16 ), 10 ),
									  _FIXED2FLOAT( (s16)_SHIFTR( w3,  0, 16 ), 10 ), NULL );
			continue;
		}

		GBI.cmd[RSP.cmd]( w0, w1 );
	}
}

void ZSortBOSS_EndMainDL( u32, u32 )
{
	if (zsbState.subdl == PROCESSED)
	{
		RSP.halt = TRUE;
		zsbState.maindl = DEFAULT;
		zsbState.subdl = DEFAULT;
		return;
	}

	zsbState.maindl = PROCESSED;

	if ((*REG.SP_STATUS & 0x80) == 0)
	{
		// wait for sig0
		RSP.PC[RSP.PCi] -= 8;
		RSP.infloop = TRUE;
		RSP.halt = TRUE;
	}
	else
	{
		// process sub dlist
		RSP.PCi = 1;
		*REG.SP_STATUS &= ~0x80; // clear sig0
	}
}

void ZSortBOSS_EndSubDL( u32, u32 )
{
	if (zsbState.maindl == PROCESSED)
	{
		RSP.halt = TRUE;
		zsbState.maindl = DEFAULT;
		zsbState.subdl = DEFAULT;
	}
	else
	{
		RSP.PCi = 0;
		zsbState.subdl = PROCESSED;
	}
}

void ZSortBOSS_WaitSignal( u32, u32 )
{
	if (!zsbState.waiting_for_signal)
	{
		*REG.SP_STATUS &= ~0x300; // clear sig1 | sig2
		*REG.SP_STATUS |= 0x400;  // set sig3
	}

	if ((*REG.SP_STATUS & 0x400))
	{
		// wait !sig3
		RSP.PC[RSP.PCi] -= 8;
		RSP.infloop = TRUE;
		RSP.halt = TRUE;
		zsbState.waiting_for_signal = true;
	}
	else
		zsbState.waiting_for_signal = false;
}

void ZSortBOSS_MoveWord( u32 _w0, u32 _w1 )
{
	if (((_w0 & 0xfff) == 0x10) && (RSP.nextCmd == 0x04)) // Next cmd is G_ZSBOSS_MOVEMEM
		zsbState.invw_factor = (f32)_w1;

	if ((_w0 & 0xfff) <= (4096 - 4))
		memcpy( (DMEM + (_w0 & 0xfff)), &_w1, sizeof(u32) );
}

void ZSortBOSS_ClearBuffer( u32, u32 )
{
	memset( (DMEM + 0xc20), 0, 512 );
}

static void ZSortBOSS_StoreMatrix( f32 mtx[4][4], u32 address )
{
	if ((address + 64) > RDRAMSize)
		return;

	struct _N64Matrix
	{
		s16 integer[4][4];
		u16 fraction[4][4];
	} *n64Mat = (struct _N64Matrix *)&RDRAM[address];

	for (u32 i = 0; i < 4; i++)
	{
		for (u32 j = 0; j < 4; j++)
		{
			s32 value = (s32)(mtx[i][j] * 65536.0f);
			n64Mat->fraction[i][ZSB_HW( j )] = (u16)(value & 0xFFFF);
			n64Mat->integer[i][ZSB_HW( j )] = (s16)(value >> 16);
		}
	}
}

static void ZSortBOSS_LoadMatrix( f32 mtx[4][4], u32 address, u32 *changed, u32 setBits, u32 clearBits = 0 )
{
	if ((address + 64) > RDRAMSize)
		return;
	RSP_LoadMatrix( mtx, address );
	*changed = (*changed & ~clearBits) | setBits;
}

void ZSortBOSS_MoveMem( u32 _w0, u32 _w1 )
{
	int flag = (_w0 >> 23) & 0x01;
	u32 len = 1 + ((_w0 >> 12) & 0x7ff);
	u32 addr = RSP_SegmentToPhysical( _w1 );

	// model matrix
	if ((_w0 & 0xfff) == 0x830)
	{
		ZSortBOSS_LoadMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], addr, &gSP.changed, CHANGED_MATRIX );
		return;
	}

	// projection matrix
	if ((_w0 & 0xfff) == 0x870)
	{
		ZSortBOSS_LoadMatrix( gSP.matrix.projection, addr, &gSP.changed, CHANGED_MATRIX );
		return;
	}

	// combined matrix
	if ((_w0 & 0xfff) == 0x8b0)
	{
		// Note the *clear* (upstream does the same): this hands the plugin a
		// ready-made combined matrix, so any pending CHANGED_MATRIX must be
		// dropped rather than left to make _gSPCombineMatrices() recompute
		// combined = projection * modelView over the top of it.
		if (flag == 0)
			ZSortBOSS_LoadMatrix( gSP.matrix.combined, addr, &gSP.changed, 0, CHANGED_MATRIX );
		else
			ZSortBOSS_StoreMatrix( gSP.matrix.combined, addr );
		return;
	}

	// VIEWPORT
	if ((_w0 & 0xfff) == 0x0)
	{
		if ((addr + 16) > RDRAMSize)
			return;
		u32 a = addr >> 1;

		const f32 scale_x = _FIXED2FLOAT( ((s16*)RDRAM)[ZSB_HW( a+0 )], 2 );
		const f32 scale_y = _FIXED2FLOAT( ((s16*)RDRAM)[ZSB_HW( a+1 )], 2 );
		const f32 scale_z = _FIXED2FLOAT( ((s16*)RDRAM)[ZSB_HW( a+2 )], 10 );
		const s16 fm = ((s16*)RDRAM)[ZSB_HW( a+3 )];
		const f32 trans_x = _FIXED2FLOAT( ((s16*)RDRAM)[ZSB_HW( a+4 )], 2 );
		const f32 trans_y = _FIXED2FLOAT( ((s16*)RDRAM)[ZSB_HW( a+5 )], 2 );
		const f32 trans_z = _FIXED2FLOAT( ((s16*)RDRAM)[ZSB_HW( a+6 )], 10 );
		const s16 fo = ((s16*)RDRAM)[ZSB_HW( a+7 )];
		gSPFogFactor( fm, fo );

		gSP.viewport.vscale[0] = scale_x;
		gSP.viewport.vscale[1] = scale_y;
		gSP.viewport.vscale[2] = scale_z;
		gSP.viewport.vtrans[0] = trans_x;
		gSP.viewport.vtrans[1] = trans_y;
		gSP.viewport.vtrans[2] = trans_z;

		gSP.viewport.x = gSP.viewport.vtrans[0] - gSP.viewport.vscale[0];
		gSP.viewport.y = gSP.viewport.vtrans[1] - gSP.viewport.vscale[1];
		gSP.viewport.width = gSP.viewport.vscale[0] * 2;
		gSP.viewport.height = gSP.viewport.vscale[1] * 2;
		gSP.viewport.nearz = gSP.viewport.vtrans[2] - gSP.viewport.vscale[2];
		gSP.viewport.farz = (gSP.viewport.vtrans[2] + gSP.viewport.vscale[2]);

		zsbState.view_scale[0] = scale_x * 4.0f;
		zsbState.view_scale[1] = scale_y * 4.0f;
		zsbState.view_trans[0] = trans_x * 4.0f;
		zsbState.view_trans[1] = trans_y * 4.0f;

		gSP.changed |= CHANGED_VIEWPORT;
		return;
	}

	const u32 dmemOffset = _w0 & 0xfff;
	if (((addr + len) > RDRAMSize) || ((dmemOffset + len) > 4096))
		return;

	if (dmemOffset == 0x730)
		memcpy( zsbState.fogtable, (RDRAM + addr), (len < sizeof(zsbState.fogtable)) ? len : sizeof(zsbState.fogtable) );

	if (flag == 0)
		memcpy( (DMEM + dmemOffset), (RDRAM + addr), len );
	else
		memcpy( (RDRAM + addr), (DMEM + dmemOffset), len );
}

static f32 *ZSortBOSS_GetMatrixById( u32 id )
{
	switch (id)
	{
		case 0x830: return (f32*)gSP.matrix.modelView[gSP.matrix.modelViewi];
		case 0x870: return (f32*)gSP.matrix.projection;
		case 0x8b0: return (f32*)gSP.matrix.combined;
	}
	return NULL;
}

void ZSortBOSS_MTXCAT( u32 _w0, u32 _w1 )
{
	f32 *s = ZSortBOSS_GetMatrixById( (_w1 >> 16) & 0xfff );
	f32 *t = ZSortBOSS_GetMatrixById( _w0 & 0xfff );
	f32 *d = ZSortBOSS_GetMatrixById( _w1 & 0xfff );

	if ((s == NULL) || (t == NULL) || (d == NULL))
		return;

	f32 tmp[4][4];
	memcpy( tmp, s, 64 );
	MultMatrix( tmp, (f32(*)[4])t );
	memcpy( d, tmp, 64 );
}

void ZSortBOSS_MultMPMTX( u32 _w0, u32 _w1 )
{
	if ((_w0 & 0xfff) != 0x8b0) // combined matrix
		return;

	int num = 1 + _SHIFTR( _w1, 24, 8 );
	int src = (_w1 >> 12) & 0xfff;
	int dst = _w1 & 0xfff;

	if (((u32)src + num * 6) > 4096 || ((u32)dst + num * (u32)sizeof(zSortVDest)) > 4096)
		return;

	s16 *saddr = (s16*)(DMEM + src);
	zSortVDest *daddr = (zSortVDest*)(DMEM + dst);
	int idx = 0;

	for (int i = 0; i < num; ++i)
	{
		s16 sx = saddr[ZSB_HW( idx + 0 )];
		s16 sy = saddr[ZSB_HW( idx + 1 )];
		s16 sz = saddr[ZSB_HW( idx + 2 )];
		idx += 3;
		f32 x = sx*gSP.matrix.combined[0][0] + sy*gSP.matrix.combined[1][0] + sz*gSP.matrix.combined[2][0] + gSP.matrix.combined[3][0];
		f32 y = sx*gSP.matrix.combined[0][1] + sy*gSP.matrix.combined[1][1] + sz*gSP.matrix.combined[2][1] + gSP.matrix.combined[3][1];
		f32 z = sx*gSP.matrix.combined[0][2] + sy*gSP.matrix.combined[1][2] + sz*gSP.matrix.combined[2][2] + gSP.matrix.combined[3][2];
		f32 w = sx*gSP.matrix.combined[0][3] + sy*gSP.matrix.combined[1][3] + sz*gSP.matrix.combined[2][3] + gSP.matrix.combined[3][3];

		zSortVDest v;
		memset( &v, 0, sizeof(v) );

		v.xi = (s16)x;
		v.yi = (s16)y;
		v.wi = (s16)w;

		v.invw = ZSortBOSS_Calc_invw( (int)(w * zsbState.invw_factor) );

		f32 invw = (w <= 0.f) ? zsbState.invw_factor : (1.f / w);

		f32 x_w = CLAMP( (x * invw), -zsbState.invw_factor, zsbState.invw_factor );
		f32 y_w = CLAMP( (y * invw), -zsbState.invw_factor, zsbState.invw_factor );

		v.sx = (s16)(zsbState.view_trans[0] + x_w * zsbState.view_scale[0]);
		v.sy = (s16)(zsbState.view_trans[1] + y_w * zsbState.view_scale[1]);

		int fog = (int)(w * _FIXED2FLOAT( gSP.fog.multiplier, 16 ) + gSP.fog.offset);
		fog = SATURATES8( fog );
		v.fog = zsbState.fogtable[fog + 128];

		v.cc = 0;
		if (x >= w) v.cc |= 0x10;
		if (y >= w) v.cc |= 0x20;
		if (z >= w) v.cc |= 0x40;
		if (x <= -w) v.cc |= 0x01;
		if (y <= -w) v.cc |= 0x02;
		if (z <= -w) v.cc |= 0x04;

		daddr[i] = v;
	}
}


static void ZSortBOSS_DrawScreenSpaceTriangle( u8 *_addr, u32 _vnum, u32 _textured )
{
	struct { f32 x, y, w, s, t, s1, t1; GXColor color; } v[4];

	for (u32 i = 0; i < _vnum; ++i)
	{
		u8 *vaddr = _addr + i * 16;

		// N64 layout of this 16-byte object vertex:
		//   0: sx  2: sy  4: r  5: g  6: b  7: a  8: s  10: t  12: invw(32)
		v[i].x = _FIXED2FLOAT( ((s16*)vaddr)[ZSB_HW( 0 )], 2 );
		v[i].y = _FIXED2FLOAT( ((s16*)vaddr)[ZSB_HW( 1 )], 2 );
		v[i].color.r = vaddr[ZSB_BY( 4 )];
		v[i].color.g = vaddr[ZSB_BY( 5 )];
		v[i].color.b = vaddr[ZSB_BY( 6 )];
		v[i].color.a = vaddr[ZSB_BY( 7 )];

		if (_textured)
		{
			if (gDP.otherMode.texturePersp != 0)
			{
				v[i].s = _FIXED2FLOAT( ((s16*)vaddr)[ZSB_HW( 4 )], 5 );
				v[i].t = _FIXED2FLOAT( ((s16*)vaddr)[ZSB_HW( 5 )], 5 );
			}
			else
			{
				v[i].s = _FIXED2FLOAT( ((s16*)vaddr)[ZSB_HW( 4 )], 6 );
				v[i].t = _FIXED2FLOAT( ((s16*)vaddr)[ZSB_HW( 5 )], 6 );
			}

			v[i].s1 = v[i].s;
			v[i].t1 = v[i].t;

			int invw = ((int*)vaddr)[3];
			int rgba = ((int*)vaddr)[1];

			if ((invw == rgba) || (invw < 0))
				v[i].w = 1.0f;
			else
				v[i].w = ZSortBOSS_Calc_invw( invw ) / zsbState.invw_factor;
		}
		else
		{
			v[i].s = 0.0f;
			v[i].t = 0.0f;
			v[i].s1 = 0.0f;
			v[i].t1 = 0.0f;
			v[i].w = 1.0f;
		}
	}

	OGL_UpdateStates();

	float ulx1 = std::max<float>( OGL.GXorigX + gDP.scissor.ulx * OGL.GXscaleX, 0 );
	float uly1 = std::max<float>( OGL.GXorigY + gDP.scissor.uly * OGL.GXscaleY, 0 );
	float lrx1 = std::max<float>( OGL.GXorigX + std::min<float>( gDP.scissor.lrx * OGL.GXscaleX, OGL.GXwidth ), 0 );
	float lry1 = std::max<float>( OGL.GXorigY + std::min<float>( gDP.scissor.lry * OGL.GXscaleY, OGL.GXheight ), 0 );
	GX_SetScissor( (u32)ulx1, (u32)uly1, (u32)(lrx1 - ulx1), (u32)(lry1 - uly1) );
	GX_SetCullMode( GX_CULL_NONE );
	GX_SetViewport( (f32)OGL.GXorigX, (f32)OGL.GXorigY, (f32)OGL.GXwidth, (f32)OGL.GXheight, 0.0f, 1.0f );

	Mtx44 proj;
	memset( proj, 0, sizeof(proj) );
	proj[0][0] = 1.0f;	// clip.x = x  (x is already ndc * w)
	proj[1][1] = 1.0f;	// clip.y = y
	proj[2][2] = 0.5f;
	proj[2][3] = 0.0f;
	GX_LoadProjectionMtx( proj, GX_PERSPECTIVE );
	GX_LoadPosMtxImm( OGL.GXmodelViewIdent, GX_PNMTX0 );

	if ((gDP.otherMode.depthSource == G_ZS_PRIM) || OGL.GXuseAlphaCompare)
		GX_SetZCompLoc( GX_FALSE );
	else
		GX_SetZCompLoc( GX_TRUE );

	if (_textured && combiner.usesT0 && cache.current[0] != NULL)
	{
		const bool fbTex0 = (cache.current[0]->frameBufferTexture != 0);
		for (u32 i = 0; i < _vnum; ++i)
		{
			if (fbTex0)
			{
				if (gSP.textureTile[0]->masks)
					v[i].s = (cache.current[0]->offsetS + (v[i].s * cache.current[0]->shiftScaleS * gSP.texture.scales - fmod( gSP.textureTile[0]->fuls, 1 << gSP.textureTile[0]->masks ))) * cache.current[0]->scaleS;
				else
					v[i].s = (cache.current[0]->offsetS + (v[i].s * cache.current[0]->shiftScaleS * gSP.texture.scales - gSP.textureTile[0]->fuls)) * cache.current[0]->scaleS;

				if (gSP.textureTile[0]->maskt)
					v[i].t = (cache.current[0]->offsetT + (v[i].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - fmod( gSP.textureTile[0]->fult, 1 << gSP.textureTile[0]->maskt ))) * cache.current[0]->scaleT;
				else
					v[i].t = (cache.current[0]->offsetT + (v[i].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[0]->fult)) * cache.current[0]->scaleT;
			}
			else
			{
				v[i].s = (v[i].s * cache.current[0]->shiftScaleS * gSP.texture.scales - gSP.textureTile[0]->fuls + cache.current[0]->offsetS) * cache.current[0]->scaleS;
				v[i].t = (v[i].t * cache.current[0]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[0]->fult + cache.current[0]->offsetT) * cache.current[0]->scaleT;
			}
		}
	}

	bool useTex1 = false;
	if (_textured && combiner.usesT1 && OGL.ARB_multitexture && cache.current[1] != NULL)
	{
		useTex1 = true;

		const bool fbTex1 = (cache.current[0] != NULL) && (cache.current[0]->frameBufferTexture != 0);
		for (u32 i = 0; i < _vnum; ++i)
		{
			if (fbTex1)
			{
				v[i].s1 = (cache.current[1]->offsetS + (v[i].s1 * cache.current[1]->shiftScaleS * gSP.texture.scales - gSP.textureTile[1]->fuls)) * cache.current[1]->scaleS;
				v[i].t1 = (cache.current[1]->offsetT + (v[i].t1 * cache.current[1]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[1]->fult)) * cache.current[1]->scaleT;
			}
			else
			{
				v[i].s1 = (v[i].s1 * cache.current[1]->shiftScaleS * gSP.texture.scales - gSP.textureTile[1]->fuls + cache.current[1]->offsetS) * cache.current[1]->scaleS;
				v[i].t1 = (v[i].t1 * cache.current[1]->shiftScaleT * gSP.texture.scalet - gSP.textureTile[1]->fult + cache.current[1]->offsetT) * cache.current[1]->scaleT;
			}
		}
	}

	GX_ClearVtxDesc();
	GX_SetVtxDesc( GX_VA_PTNMTXIDX, GX_PNMTX0 );
	GX_SetVtxDesc( GX_VA_TEX0MTXIDX, GX_TEXMTX0 );
	GX_SetVtxDesc( GX_VA_TEX1MTXIDX, GX_TEXMTX0 );
	GX_SetVtxDesc( GX_VA_TEX2MTXIDX, GX_TEXMTX0 );
	GX_SetVtxDesc( GX_VA_POS, GX_DIRECT );
	GX_SetVtxDesc( GX_VA_CLR0, GX_DIRECT );
	GX_SetVtxDesc( GX_VA_TEX0, GX_DIRECT );
	GX_SetVtxDesc( GX_VA_TEX1, GX_DIRECT );
	GX_SetVtxDesc( GX_VA_TEX2, GX_DIRECT );
	GX_SetVtxAttrFmt( GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
	GX_SetVtxAttrFmt( GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
	GX_SetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
	GX_SetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0 );
	GX_SetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0 );

	static const u32 stripIndices[6] = { 0, 1, 2, 1, 2, 3 };
	const u32 numEmit = (_vnum == 4) ? 6 : 3;
	const f32 ndcScaleX = 2.0f / (f32)VI.width;
	const f32 ndcScaleY = 2.0f / (f32)VI.height;
	GX_Begin( GX_TRIANGLES, GX_VTXFMT0, numEmit );
	for (u32 k = 0; k < numEmit; ++k)
	{
		const u32 idx = (_vnum == 4) ? stripIndices[k] : k;
		const f32 vw = (v[idx].w > 0.0f) ? v[idx].w : 1.0f;
		const f32 ndcX = v[idx].x * ndcScaleX - 1.0f;
		const f32 ndcY = 1.0f - v[idx].y * ndcScaleY;
		GX_Position3f32( ndcX * vw, ndcY * vw, -vw );
		GX_Color4u8( v[idx].color.r, v[idx].color.g, v[idx].color.b, v[idx].color.a );
		GX_TexCoord2f32( v[idx].s, v[idx].t );
		if (useTex1)
			GX_TexCoord2f32( v[idx].s1, v[idx].t1 );
		else
			GX_TexCoord2f32( 0.0f, 0.0f );
		GX_TexCoord2f32( 0.0f, 0.0f );
	}
	GX_End();

	OGL.GXupdateMtx = true;
	gDP.changed |= CHANGED_SCISSOR; // Restore scissor in OGL_UpdateStates() before drawing next geometry.
	OGL_UpdateCullFace();
	OGL_UpdateViewport();

	if (depthBuffer.current) depthBuffer.current->cleared = FALSE;
	gDP.colorImage.changed = TRUE;
	gDP.colorImage.height = (unsigned long)MAX( gDP.colorImage.height, gDP.scissor.lry );
}

static void ZSortBOSS_DrawObject( u8 *_addr, u32 _type )
{
	switch (_type)
	{
		case ZH_TXTRI:
			ZSortBOSS_DrawScreenSpaceTriangle( _addr, 3, 1 );
		break;
		case ZH_TXQUAD:
			ZSortBOSS_DrawScreenSpaceTriangle( _addr, 4, 1 );
		break;
	}
}

static u32 ZSortBOSS_LoadObject( u32 _zHeader )
{
	if ((_zHeader & 0xFFFFFFF8) + 32 > RDRAMSize)
		return 0;

	const u32 type = (_zHeader & 7) << 1;
	u8 *addr = RDRAM + (_zHeader & 0xFFFFFFF8);
	u32 w1;

	switch (type)
	{
		case ZH_NULL:
		case ZH_TXTRI:
		case ZH_TXQUAD:
		{
			w1 = ((u32*)addr)[1];
			if (w1 != zsbState.rdpcmds[0])
			{
				zsbState.rdpcmds[0] = w1;
				ZSortBOSS_RDPCMD( 0, w1 );
			}
			w1 = ((u32*)addr)[2];
			if (w1 != zsbState.rdpcmds[1])
			{
				ZSortBOSS_RDPCMD( 0, w1 );
				zsbState.rdpcmds[1] = w1;
			}
			w1 = ((u32*)addr)[3];
			if (w1 != zsbState.rdpcmds[2])
			{
				ZSortBOSS_RDPCMD( 0, w1 );
				zsbState.rdpcmds[2] = w1;
			}
			if (type != ZH_NULL)
				ZSortBOSS_DrawObject( addr + 16, type );
		}
		break;
	}
	return RSP_SegmentToPhysical( ((u32*)addr)[0] );
}

void ZSortBOSS_Obj( u32 _w0, u32 _w1 )
{
	u32 zHeader = RSP_SegmentToPhysical( _w0 );
	while (zHeader)
		zHeader = ZSortBOSS_LoadObject( zHeader );
	zHeader = RSP_SegmentToPhysical( _w1 );
	while (zHeader)
		zHeader = ZSortBOSS_LoadObject( zHeader );
}

void ZSortBOSS_TransposeMTX( u32, u32 _w1 )
{
	f32 *mtx = ZSortBOSS_GetMatrixById( _w1 & 0xfff );
	if (mtx == NULL)
		return;

	f32 (*m)[4] = (f32(*)[4])mtx;
	for (int i = 0; i < 3; i++)
		for (int j = i + 1; j < 3; j++)
		{
			f32 tmp = m[i][j];
			m[i][j] = m[j][i];
			m[j][i] = tmp;
		}
}

void ZSortBOSS_Lighting( u32 _w0, u32 _w1 )
{
	u32 num = 1 + (_w1 >> 24);
	u32 nsrs = _w0 & 0xfff;
	u32 tdest = _w1 & 0xfff;
	if ((tdest & 3) != 0)
		return;
	tdest >>= 1;

	if ((nsrs + num * 3) > 4096 || (tdest * 2 + num * 4) > 4096)
		return;

	for (u32 i = 0; i < num; i++)
	{
		f32 nx = _FIXED2FLOAT( ((s8*)DMEM)[ZSB_BY( nsrs + 0 )], 8 );
		f32 ny = _FIXED2FLOAT( ((s8*)DMEM)[ZSB_BY( nsrs + 1 )], 8 );
		f32 nz = _FIXED2FLOAT( ((s8*)DMEM)[ZSB_BY( nsrs + 2 )], 8 );
		nsrs += 3;

		f32 fLightDir[3] = { nx, ny, nz };
		f32 x = DotProduct( gSP.lookat.xyz[0], fLightDir );
		f32 y = DotProduct( gSP.lookat.xyz[1], fLightDir );
		f32 s = (x + 0.5f) * 1024.0f;
		f32 t = (y + 0.5f) * 1024.0f;

		((s16*)DMEM)[ZSB_HW( tdest + 0 )] = (s16)s;
		((s16*)DMEM)[ZSB_HW( tdest + 1 )] = (s16)t;
		tdest += 2;
	}
}

static void ZSortBOSS_TransformVectorNormalize( f32 vec[3], f32 mtx[4][4] )
{
	f32 vres[3];
	f32 recip = 256.f;

	vres[0] = mtx[0][0] * vec[0] + mtx[1][0] * vec[1] + mtx[2][0] * vec[2];
	vres[1] = mtx[0][1] * vec[0] + mtx[1][1] * vec[1] + mtx[2][1] * vec[2];
	vres[2] = mtx[0][2] * vec[0] + mtx[1][2] * vec[1] + mtx[2][2] * vec[2];

	f32 len = vres[0]*vres[0] + vres[1]*vres[1] + vres[2]*vres[2];

	if (len != 0.0f)
		recip = 1.f / sqrtf( len );
	if (recip > 256.f) recip = 256.f;

	vec[0] = vres[0] * recip;
	vec[1] = vres[1] * recip;
	vec[2] = vres[2] * recip;
}

void ZSortBOSS_TransformLights( u32 _w0, u32 _w1 )
{
	if ((_w0 & 0xfff) != 0x830) // model matrix
		return;

	int addr = _w1 & 0xfff;
	const s32 numLights = 1 - (s32)(_w1 >> 12);
	if (numLights < 0 || numLights >= 12)
		return;
	gSP.numLights = numLights;

	addr += (u32)gSP.numLights * 24;

	for (int i = 0; i < 2; i++)
	{
		gSP.lookat.xyz[i][0] = _FIXED2FLOAT( ((s8*)DMEM)[ZSB_BY( addr+16+0 )], 8 );
		gSP.lookat.xyz[i][1] = _FIXED2FLOAT( ((s8*)DMEM)[ZSB_BY( addr+16+1 )], 8 );
		gSP.lookat.xyz[i][2] = _FIXED2FLOAT( ((s8*)DMEM)[ZSB_BY( addr+16+2 )], 8 );
		ZSortBOSS_TransformVectorNormalize( gSP.lookat.xyz[i], gSP.matrix.modelView[gSP.matrix.modelViewi] );
		addr += 24;
	}
}

void ZSortBOSS_Audio1( u32 _w0, u32 _w1 )
{
	u32 addr = RSP_SegmentToPhysical( _w1 );
	if ((addr + 8) > RDRAMSize)
		return;
	u32 val = ((u32*)DMEM)[(_w0 & 0xfff) >> 2];
	((u32*)DMEM)[0] = val;
	memcpy( RDRAM + addr, DMEM, 0x8 );
}

void ZSortBOSS_Audio2( u32 _w0, u32 _w1 )
{
	int len = _w1 >> 24;

	// Written by a previous ZSortBOSS_MoveWord
	u32 dst = ((u32*)DMEM)[0x10 >> 2];

	f32 f1 = (f32)((_w0 >> 16) & 0xff) + (f32)(_w0 & 0xffff) / 65536.f;
	f32 f2 = (f32)((_w1 >> 16) & 0xff) + (f32)(_w1 & 0xffff) / 65536.f;

	u16 v11[2];
	v11[0] = ((u16*)DMEM)[ZSB_HW( 0x904 >> 1 )];
	v11[1] = ((u16*)DMEM)[ZSB_HW( (0x904 + 2) >> 1 )];

	for (int i = 0; i < len; i += 4)
	{
		for (int j = 0; j < 4; j++)
		{
			f32 intpart, fractpart;
			f32 val = i*f1 + j*f1 + f2;

			fractpart = fabsf( modff( val, &intpart ) );
			int index = ((int)intpart) << 1;

			s16 v9 = ((s16*)DMEM)[ZSB_HW( (0x30+index)>>1 )];
			s16 v10 = ((s16*)DMEM)[ZSB_HW( (0x32+index)>>1 )];
			s16 v12 = v10 - v9;

			if ((dst + 4) > 4096)
				return;
			s16 v13 = ((s16*)DMEM)[ZSB_HW( dst>>1 )];

			for (int k = 0; k < 2; k++)
			{
				s32 res1 = v12 * (u16)(fractpart * 65536.f);
				s32 res2 = v9 << 16;
				s16 res3 = (res2 + res1) >> 16;

				res1 = v11[k] * res3;
				res2 = v13 << 16;
				res3 = (res2 + res1) >> 16;

				((s16*)DMEM)[ZSB_HW( dst>>1 )] = res3;
				dst += 2;
			}
		}
	}
}

void ZSortBOSS_Audio3( u32 _w0, u32 _w1 )
{
	u32 addr = RSP_SegmentToPhysical( _w0 );
	if ((addr + 128) > RDRAMSize)
		return;

	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 8; j++)
			zsbState.table[i][j] = ((s16*)RDRAM)[ZSB_HW( (addr+(i<<4)+(j<<1))>>1 )];

	addr = RSP_SegmentToPhysical( _w1 );
	if ((addr + 8) > RDRAMSize)
		return;

	memcpy( DMEM, (RDRAM + addr), 0x8 );
	memcpy( (DMEM+8), &addr, sizeof(addr) );
}

void ZSortBOSS_Audio4( u32 _w0, u32 _w1 )
{
	u32 addr = RSP_SegmentToPhysical( _w1 );
	u32 src = ((_w0 & 0xf000) >> 12) + addr;
	s16 *dst = (s16*)(DMEM + 0x30);
	int len = (_w0 & 0xfff);

	// Written by a previous ZSortBOSS_MoveWord
	s16 v1 = ((s16*)DMEM)[ZSB_HW( 0>>1 )];
	s16 v2 = ((s16*)DMEM)[ZSB_HW( 2>>1 )];

	for (int l1 = len; l1 != 0; l1 -= 9)
	{
		if ((src + 9) > RDRAMSize)
			break;
		u32 r9 = ((u8*)RDRAM)[ZSB_BY( src )];
		src++;
		int index = (r9 & 0xf) << 1;

		if (index > 6)
			break;

		s16 c1 = 1 << ((r9 >> 4) & 0x1f);
		s16 c2 = 0x20;

		for (int l2 = 0; l2 < 2; l2++)
		{
			s32 a = ((s8*)RDRAM)[ZSB_BY( src + 0 )];
			s32 b = ((s8*)RDRAM)[ZSB_BY( src + 1 )];
			s32 c = ((s8*)RDRAM)[ZSB_BY( src + 2 )];
			s32 d = ((s8*)RDRAM)[ZSB_BY( src + 3 )];
			src += 4;

			s16 coeff[8];
			coeff[0] = a>>4;
			coeff[1] = (a<<28)>>28;
			coeff[2] = b>>4;
			coeff[3] = (b<<28)>>28;
			coeff[4] = c>>4;
			coeff[5] = (c<<28)>>28;
			coeff[6] = d>>4;
			coeff[7] = (d<<28)>>28;

			for (int i = 0; i < 8; i++)
			{
				s32 res1 = 0;

				for (int j = 0, k = i; j < i; j++, k--)
					res1 += zsbState.table[index+1][k-1] * coeff[j];

				res1 += coeff[i] * (s16)0x0800;
				res1 *= c1;

				s32 res2 = v1*zsbState.table[index][i] + v2*zsbState.table[index+1][i];

				dst[ZSB_HW( i )] = (res1*c2 + res2*c2) >> 16;
			}
			v1 = dst[ZSB_HW( 6 )];
			v2 = dst[ZSB_HW( 7 )];
			dst += 8;
		}
	}
}

// RDP Commands
static u32 ZSortBOSS_OtherModeFieldMask( u32 _w0 )
{
	u32 mask = 0xFFFFFFFFU << (31 - (_w0 & 0x1f));
	return mask >> ((_w0 >> 8) & 0x1f);
}

void ZSortBOSS_UpdateMask( u32 _w0, u32 _w1 )
{
	zsbState.updatemask[0] = _w0 | 0xff000000;
	zsbState.updatemask[1] = _w1;
}

void ZSortBOSS_SetOtherMode_L( u32 _w0, u32 _w1 )
{
	u32 mask = ZSortBOSS_OtherModeFieldMask( _w0 );
	gDP.otherMode.l = (gDP.otherMode.l & ~mask) | _w1;

	gDPSetOtherMode( _SHIFTR( gDP.otherMode.h, 0, 24 ), gDP.otherMode.l );
}

void ZSortBOSS_SetOtherMode_H( u32 _w0, u32 _w1 )
{
	u32 mask = ZSortBOSS_OtherModeFieldMask( _w0 );
	gDP.otherMode.h = (gDP.otherMode.h & ~mask) | _w1;

	gDPSetOtherMode( _SHIFTR( gDP.otherMode.h, 0, 24 ), gDP.otherMode.l );
}

void ZSortBOSS_SetOtherMode( u32 _w0, u32 _w1 )
{
	u32 h = (_w0 & zsbState.updatemask[0]) | (gDP.otherMode.h & ~zsbState.updatemask[0]);
	u32 l = (_w1 & zsbState.updatemask[1]) | (gDP.otherMode.l & ~zsbState.updatemask[1]);

	gDPSetOtherMode( _SHIFTR( h, 0, 24 ), l );
}

void ZSortBOSS_TriangleCommand( u32, u32 _w1 )
{
	gSP.texture.scales = 1.0f;
	gSP.texture.scalet = 1.0f;
	gSP.texture.level = (_w1 >> 3) & 0x7;
	gSP.texture.on = 1;
	gSP.texture.tile = _w1 & 0x7;

	gSP.textureTile[0] = &gDP.tiles[gSP.texture.tile];
	gSP.textureTile[1] = needReplaceTex1ByTex0() ? &gDP.tiles[gSP.texture.tile] : &gDP.tiles[(gSP.texture.tile < 7) ? (gSP.texture.tile + 1) : gSP.texture.tile];

	gSP.changed |= CHANGED_TEXTURE;

	gSPSetGeometryMode( G_SHADING_SMOOTH | G_SHADE );
}

void ZSortBOSS_FlushRDPCMDBuffer( u32, u32 )
{
}

void ZSortBOSS_Reserved( u32, u32 )
{
}

#define	G_ZSBOSS_ENDMAINDL			0x02
#define	G_ZSBOSS_MOVEMEM			0x04
#define	G_ZSBOSS_MTXCAT				0x0A
#define	G_ZSBOSS_MULT_MPMTX			0x0C
#define	G_ZSBOSS_MOVEWORD			0x06
#define	G_ZSBOSS_TRANSPOSEMTX		0x08
#define	G_ZSBOSS_RDPCMD				0x0E
#define	G_ZSBOSS_OBJ				0x10
#define	G_ZSBOSS_WAITSIGNAL			0x12
#define	G_ZSBOSS_LIGHTING			0x14
#define	G_ZSBOSS_RESERVED0			0x16
#define	G_ZSBOSS_TRANSFORMLIGHTS	0x18
#define	G_ZSBOSS_ENDSUBDL			0x1A
#define	G_ZSBOSS_AUDIO2				0x1C
#define	G_ZSBOSS_CLEARBUFFER		0x1E
#define	G_ZSBOSS_RESERVED1			0x20
#define	G_ZSBOSS_AUDIO3				0x22
#define	G_ZSBOSS_AUDIO4				0x24
#define	G_ZSBOSS_AUDIO1				0x26
#define	G_ZSBOSS_UPDATEMASK			0xDD
#define	G_ZSBOSS_TRIANGLECOMMAND	0xDE
#define	G_ZSBOSS_FLUSHRDPCMDBUFFER	0xDF
#define	G_ZSBOSS_RDPHALF_1			0xE1
#define	G_ZSBOSS_SETOTHERMODE_H		0xE3
#define	G_ZSBOSS_SETOTHERMODE_L		0xE2
#define	G_ZSBOSS_RDPSETOTHERMODE	0xEF
#define	G_ZSBOSS_RDPHALF_2			0xF1

u32 G_ZENDMAINDL, G_ZENDSUBDL, G_ZUPDATEMASK, G_ZSETOTHERMODE, G_ZFLUSHRDPCMDBUFFER;
u32 G_ZMOVEWORD, G_ZCLEARBUFFER, G_ZTRIANGLECOMMAND, G_ZTRANSPOSEMTX, G_ZTRANSFORMLIGHTS;
u32 G_ZAUDIO1, G_ZAUDIO2, G_ZAUDIO3, G_ZAUDIO4;
u32 G_ZOBJ, G_ZRDPCMD, G_ZWAITSIGNAL, G_ZMTXCAT, G_ZMULT_MPMTX, G_ZLIGHTING;

void ZSortBOSS_Init()
{
	// Set GeometryMode flags
	GBI_InitFlags( F3D );

	GBI.PCStackSize = 10;

	memset( &zsbState, 0, sizeof(zsbState) );
	zsbState.invw_factor = 10.0f;

	//			GBI Command				Command Value			Command Function
	GBI_SetGBI( G_SPNOOP,				F3D_SPNOOP,				F3D_SPNoOp );
	GBI_SetGBI( G_ZENDMAINDL,			G_ZSBOSS_ENDMAINDL,		ZSortBOSS_EndMainDL );
	GBI_SetGBI( G_MOVEMEM,				G_ZSBOSS_MOVEMEM,		ZSortBOSS_MoveMem );
	GBI_SetGBI( G_ZMOVEWORD,			G_ZSBOSS_MOVEWORD,		ZSortBOSS_MoveWord );
	GBI_SetGBI( G_ZTRANSPOSEMTX,		G_ZSBOSS_TRANSPOSEMTX,	ZSortBOSS_TransposeMTX );
	GBI_SetGBI( G_ZMTXCAT,				G_ZSBOSS_MTXCAT,		ZSortBOSS_MTXCAT );
	GBI_SetGBI( G_ZMULT_MPMTX,			G_ZSBOSS_MULT_MPMTX,	ZSortBOSS_MultMPMTX );
	GBI_SetGBI( G_ZRDPCMD,				G_ZSBOSS_RDPCMD,		ZSortBOSS_RDPCMD );
	GBI_SetGBI( G_ZOBJ,					G_ZSBOSS_OBJ,			ZSortBOSS_Obj );
	GBI_SetGBI( G_ZWAITSIGNAL,			G_ZSBOSS_WAITSIGNAL,	ZSortBOSS_WaitSignal );
	GBI_SetGBI( G_ZLIGHTING,			G_ZSBOSS_LIGHTING,		ZSortBOSS_Lighting );
	GBI_SetGBI( G_RESERVED0,			G_ZSBOSS_RESERVED0,		ZSortBOSS_Reserved );
	GBI_SetGBI( G_ZTRANSFORMLIGHTS,		G_ZSBOSS_TRANSFORMLIGHTS, ZSortBOSS_TransformLights );
	GBI_SetGBI( G_ZENDSUBDL,			G_ZSBOSS_ENDSUBDL,		ZSortBOSS_EndSubDL );
	GBI_SetGBI( G_ZAUDIO2,				G_ZSBOSS_AUDIO2,		ZSortBOSS_Audio2 );
	GBI_SetGBI( G_ZCLEARBUFFER,			G_ZSBOSS_CLEARBUFFER,	ZSortBOSS_ClearBuffer );
	GBI_SetGBI( G_RESERVED1,			G_ZSBOSS_RESERVED1,		ZSortBOSS_Reserved );
	GBI_SetGBI( G_ZAUDIO3,				G_ZSBOSS_AUDIO3,		ZSortBOSS_Audio3 );
	GBI_SetGBI( G_ZAUDIO4,				G_ZSBOSS_AUDIO4,		ZSortBOSS_Audio4 );
	GBI_SetGBI( G_ZAUDIO1,				G_ZSBOSS_AUDIO1,		ZSortBOSS_Audio1 );

	// RDP Commands
	GBI_SetGBI( G_ZUPDATEMASK,			G_ZSBOSS_UPDATEMASK,	ZSortBOSS_UpdateMask );
	GBI_SetGBI( G_ZTRIANGLECOMMAND,		G_ZSBOSS_TRIANGLECOMMAND, ZSortBOSS_TriangleCommand );
	GBI_SetGBI( G_ZFLUSHRDPCMDBUFFER,	G_ZSBOSS_FLUSHRDPCMDBUFFER, ZSortBOSS_FlushRDPCMDBuffer );
	GBI_SetGBI( G_RDPHALF_1,			G_ZSBOSS_RDPHALF_1,		F3D_RDPHalf_1 );
	GBI_SetGBI( G_SETOTHERMODE_L,		G_ZSBOSS_SETOTHERMODE_L, ZSortBOSS_SetOtherMode_L );
	GBI_SetGBI( G_SETOTHERMODE_H,		G_ZSBOSS_SETOTHERMODE_H, ZSortBOSS_SetOtherMode_H );
	GBI_SetGBI( G_ZSETOTHERMODE,		G_ZSBOSS_RDPSETOTHERMODE, ZSortBOSS_SetOtherMode );
	GBI_SetGBI( G_RDPHALF_2,			G_ZSBOSS_RDPHALF_2,		F3D_RDPHalf_2 );
}
