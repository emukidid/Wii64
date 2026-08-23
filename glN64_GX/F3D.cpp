/**
 * glN64_GX - F3D.cpp
 * Copyright (C) 2003 Orkin
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 *
**/

#ifdef __GX__
#include <gccore.h>
#include <math.h>
#endif // __GX__

#include "glN64.h"
#include "Debug.h"
#include "F3D.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"
#include "OpenGL.h"
#include "Combiner.h"
#include "Textures.h"

void F3D_SPNoOp( u32 w0, u32 w1 )
{
	gSPNoOp();
}

void F3D_Mtx( u32 w0, u32 w1 )
{
	if (_SHIFTR( w0, 0, 16 ) != 64)
	{
//		GBI_DetectUCode(); // Something's wrong
#ifdef DEBUG
	DebugMsg( DEBUG_MEDIUM | DEBUG_HIGH | DEBUG_ERROR, "G_MTX: address = 0x%08X    length = %i    params = 0x%02X\n", w1, _SHIFTR( w0, 0, 16 ), _SHIFTR( w0, 16, 8 ) );
#endif
		return;
	}

	gSPMatrix( w1, _SHIFTR( w0, 16, 8 ) );
}

void F3D_Reserved0( u32 w0, u32 w1 )
{
#ifdef DEBUG
	DebugMsg( DEBUG_MEDIUM | DEBUG_IGNORED | DEBUG_UNKNOWN, "G_RESERVED0: w0=0x%08lX w1=0x%08lX\n", w0, w1 );
#endif
}

void F3D_MoveMem( u32 w0, u32 w1 )
{
	switch (_SHIFTR( w0, 16, 8 ))
	{
		case F3D_MV_VIEWPORT://G_MV_VIEWPORT:
			gSPViewport( w1 );
			break;
		case G_MV_MATRIX_1:
			gSPForceMatrix( w1 );

			// force matrix takes four commands
			RSP.PC[RSP.PCi] += 24;
			break;
		case G_MV_L0:
			gSPLight( w1, LIGHT_1 );
			break;
		case G_MV_L1:
			gSPLight( w1, LIGHT_2 );
			break;
		case G_MV_L2:
			gSPLight( w1, LIGHT_3 );
			break;
		case G_MV_L3:
			gSPLight( w1, LIGHT_4 );
			break;
		case G_MV_L4:
			gSPLight( w1, LIGHT_5 );
			break;
		case G_MV_L5:
			gSPLight( w1, LIGHT_6 );
			break;
		case G_MV_L6:
			gSPLight( w1, LIGHT_7 );
			break;
		case G_MV_L7:
			gSPLight( w1, LIGHT_8 );
			break;
		case G_MV_LOOKATX:
			break;
		case G_MV_LOOKATY:
			break;
	}
}

void F3D_Vtx( u32 w0, u32 w1 )
{
	gSPVertex( w1, _SHIFTR( w0, 20, 4 ) + 1, _SHIFTR( w0, 16, 4 ) );
}

void F3D_Reserved1( u32 w0, u32 w1 )
{
}

void F3D_DList( u32 w0, u32 w1 )
{
	switch (_SHIFTR( w0, 16, 8 ))
	{
		case G_DL_PUSH:
			gSPDisplayList( w1 );
			break;
		case G_DL_NOPUSH:
			gSPBranchList( w1 );
			break;
	}
}

void F3D_Reserved2( u32 w0, u32 w1 )
{
}

void F3D_Reserved3( u32 w0, u32 w1 )
{
}

void F3D_Sprite2D_Base( u32 w0, u32 w1 )
{
	//gSPSprite2DBase( w1 );
	RSP.PC[RSP.PCi] += 8;
}

void F3D_Tri1( u32 w0, u32 w1 )
{
	gSP1Triangle( _SHIFTR( w1, 16, 8 ) / 10, 
		          _SHIFTR( w1, 8, 8 ) / 10, 
				  _SHIFTR( w1, 0, 8 ) / 10 );
}

void F3D_CullDL( u32 w0, u32 w1 )
{
}

void F3D_PopMtx( u32 w0, u32 w1 )
{
	gSPPopMatrix( w1 );
}

void F3D_MoveWord( u32 w0, u32 w1 )
{
	switch (_SHIFTR( w0, 0, 8 ))
	{
		case G_MW_MATRIX:
			gSPInsertMatrix( _SHIFTR( w0, 8, 16 ), w1 );
			break;
		case G_MW_NUMLIGHT:
			// w1 is a DMEM address just past the last light, so the count is
			// (w1 - 0x80000000) / 32 - 1. Both steps misbehave on a malformed
			// display list: the subtraction wraps when w1 is below the base,
			// and the shift yields zero for the 32 values at the base, after
			// which the -1 underflows to 0xFFFFFFFF.
			if (w1 >= 0x80000020)
				gSPNumLights( (s32)((w1 - 0x80000000) >> 5) - 1 );
#ifdef DEBUG
			else
				DebugMsg( DEBUG_MEDIUM | DEBUG_HIGH | DEBUG_ERROR, "// G_MW_NUMLIGHT: invalid light address 0x%08x\n", w1 );
#endif
			break;
		case G_MW_CLIP:
			gSPClipRatio( w1 );
			break;
		case G_MW_SEGMENT:
			gSPSegment( _SHIFTR( w0, 8, 16 ) >> 2, w1 & 0x00FFFFFF );
			break;
		case G_MW_FOG:
/*			u32 fm, fo, min, max;

			fm = _SHIFTR( w1, 16, 16 );
			fo = _SHIFTR( w1, 0, 16 );

			min = 500 - (fo * (128000 / fm)) / 256;
			max = (128000 / fm) + min;*/

			gSPFogFactor( (s16)_SHIFTR( w1, 16, 16 ), (s16)_SHIFTR( w1, 0, 16 ) );
			break;
		case G_MW_LIGHTCOL:
			switch (_SHIFTR( w0, 8, 16 ))
			{
				case F3D_MWO_aLIGHT_1:
					gSPLightColor( LIGHT_1, w1 );
					break;
				case F3D_MWO_aLIGHT_2:
					gSPLightColor( LIGHT_2, w1 );
					break;
				case F3D_MWO_aLIGHT_3:
					gSPLightColor( LIGHT_3, w1 );
					break;
				case F3D_MWO_aLIGHT_4:
					gSPLightColor( LIGHT_4, w1 );
					break;
				case F3D_MWO_aLIGHT_5:
					gSPLightColor( LIGHT_5, w1 );
					break;
				case F3D_MWO_aLIGHT_6:
					gSPLightColor( LIGHT_6, w1 );
					break;
				case F3D_MWO_aLIGHT_7:
					gSPLightColor( LIGHT_7, w1 );
					break;
				case F3D_MWO_aLIGHT_8:
					gSPLightColor( LIGHT_8, w1 );
					break;
			}
			break;
		case G_MW_POINTS:
			gSPModifyVertex( _SHIFTR( w0, 8, 16 ) / 40, _SHIFTR( w0, 0, 8 ) % 40, w1 );
			break;
		case G_MW_PERSPNORM:
			gSPPerspNormalize( w1 );
			break;
	}
}

void F3D_Texture( u32 w0, u32 w1 )
{
	gSPTexture( GXcastu16f32( _SHIFTR( w1, 16, 16 ) ), 
		        GXcastu16f32( _SHIFTR( w1, 0, 16 ) ), 
		        _SHIFTR( w0, 11, 3 ), 
				_SHIFTR( w0, 8, 3 ), 
				_SHIFTR( w0, 0, 8 ) );
}

void F3D_SetOtherMode_H( u32 w0, u32 w1 )
{
	switch (_SHIFTR( w0, 8, 8 ))
	{
		case G_MDSFT_PIPELINE:
			gDPPipelineMode( w1 >> G_MDSFT_PIPELINE );
			break;
		case G_MDSFT_CYCLETYPE:
			gDPSetCycleType( w1 >> G_MDSFT_CYCLETYPE );
			break;
		case G_MDSFT_TEXTPERSP:
			gDPSetTexturePersp( w1 >> G_MDSFT_TEXTPERSP );
			break;
		case G_MDSFT_TEXTDETAIL:
			gDPSetTextureDetail( w1 >> G_MDSFT_TEXTDETAIL );
			break;
		case G_MDSFT_TEXTLOD:
			gDPSetTextureLOD( w1 >> G_MDSFT_TEXTLOD );
			break;
		case G_MDSFT_TEXTLUT:
			gDPSetTextureLUT( w1 >> G_MDSFT_TEXTLUT );
			break;
		case G_MDSFT_TEXTFILT:
			gDPSetTextureFilter( w1 >> G_MDSFT_TEXTFILT );
			break;
		case G_MDSFT_TEXTCONV:
			gDPSetTextureConvert( w1 >> G_MDSFT_TEXTCONV );
			break;
		case G_MDSFT_COMBKEY:
			gDPSetCombineKey( w1 >> G_MDSFT_COMBKEY );
			break;
		case G_MDSFT_RGBDITHER:
			gDPSetColorDither( w1 >> G_MDSFT_RGBDITHER );
			break;
		case G_MDSFT_ALPHADITHER:
			gDPSetAlphaDither( w1 >> G_MDSFT_ALPHADITHER );
			break;
		default:
			u32 shift = _SHIFTR( w0, 8, 8 );
			u32 length = _SHIFTR( w0, 0, 8 );
			u32 mask = ((1 << length) - 1) << shift;

			gDP.otherMode.h &= ~mask;
			gDP.otherMode.h |= w1 & mask;

			gDP.changed |= CHANGED_CYCLETYPE;
			break;
	}
}

void F3D_SetOtherMode_L( u32 w0, u32 w1 )
{
	switch (_SHIFTR( w0, 8, 8 ))
	{
		case G_MDSFT_ALPHACOMPARE:
			gDPSetAlphaCompare( w1 >> G_MDSFT_ALPHACOMPARE );
			break;
		case G_MDSFT_ZSRCSEL:
			gDPSetDepthSource( w1 >> G_MDSFT_ZSRCSEL );
			break;
		case G_MDSFT_RENDERMODE:
			gDPSetRenderMode( w1 & 0xCCCCFFFF, w1 & 0x3333FFFF );
			break;
		default:
			u32 shift = _SHIFTR( w0, 8, 8 );
			u32 length = _SHIFTR( w0, 0, 8 );
			u32 mask = ((1 << length) - 1) << shift;

			gDP.otherMode.l &= ~mask;
			gDP.otherMode.l |= w1 & mask;

			gDP.changed |= CHANGED_RENDERMODE | CHANGED_ALPHACOMPARE;
			break;
	}
}

void F3D_EndDL( u32 w0, u32 w1 )
{
	gSPEndDisplayList();
}

void F3D_SetGeometryMode( u32 w0, u32 w1 )
{
	gSPSetGeometryMode( w1 );
}

void F3D_ClearGeometryMode( u32 w0, u32 w1 )
{
	gSPClearGeometryMode( w1 );
}

void F3D_Line3D( u32 w0, u32 w1 )
{
	// Hmmm...
}

void F3D_Quad( u32 w0, u32 w1 )
{
	gSP1Quadrangle( _SHIFTR( w1, 24, 8 ) / 10, _SHIFTR( w1, 16, 8 ) / 10, _SHIFTR( w1, 8, 8 ) / 10, _SHIFTR( w1, 0, 8 ) / 10 );
}


// GoldenEye 007 sky/water hackery (cause it's better than just a black sky, I guess...)

// Initially I'd brought across the clouds texture hack from Rice but decided I wanted something different here.
// After looking at certain sources, and GLideN64, I came up with a hack
// which the edge-coefficient decode below is based on (layout and fixed-point conversions ported from
// GLideN64's gDPLLETriangle() in gDP.cpp). It reconstructs 
// the same small vertex fan/strip that decoder would produce, and we take its
// bounding box as a plain rect, drawn as a translucent flat colour "fog" wash using
// the triangle's own decoded shade colour rather than the actual cloud/water texture.
// I felt like this felt more authentic than what Rice_GX does, but to each their own until it's done properly one day.


#define F3DGOLDEN_MAX_CMD_WORDS 44

static u32 f3dGoldenCmdWords[F3DGOLDEN_MAX_CMD_WORDS];
static u32 f3dGoldenCmdWordCount = 0;
static bool f3dGoldenCmdActive = false;

// Hacky sky clouds for GoldenEye, implement a manual zoom/scroll window (not positional)
// One day I'll do the whole LLE thing, but for now this looks better than no clouds (maybe :P)
static f32 f3dGoldenScrollS = 0.0f;
static f32 f3dGoldenScrollT = 0.0f;
static const f32 F3DGOLDEN_SCROLL_WINDOW = 16.0f; // zoomed-in slice/window
static f32 f3dGoldenTexWidth = 0.0f;
static f32 f3dGoldenTexHeight = 0.0f;
static const f32 F3DGOLDEN_SCROLL_SPEED_S = 0.01f;
static const f32 F3DGOLDEN_SCROLL_SPEED_T = 0.01f;

void F3DGOLDEN_NewFrame()
{
	if (f3dGoldenTexWidth > 0.0f && f3dGoldenTexHeight > 0.0f) {
		f3dGoldenScrollS = fmodf( f3dGoldenScrollS + F3DGOLDEN_SCROLL_SPEED_S, f3dGoldenTexWidth );
		f3dGoldenScrollT = fmodf( f3dGoldenScrollT + F3DGOLDEN_SCROLL_SPEED_T, f3dGoldenTexHeight );
	}
}

static inline s32 F3DGOLDEN_SignExtend( s32 v, s32 bits )
{
	s32 shift = 32 - bits;
	return (v << shift) >> shift;
}

static void F3DGOLDEN_DrawTriangleCommand( const u32 *pData, u32 wordCount )
{
	if (wordCount < 8)
		return;

	const s32 *pDataSigned = (const s32*) pData;

	u32 header = _SHIFTR( pData[0], 24, 8 );
	bool shade = (header == G_TRI_SHADE) || (header == G_TRI_SHADE_TXTR) ||
	             (header == G_TRI_SHADE_ZBUFF) || (header == G_TRI_SHADE_TXTR_ZBUFF);
	bool textured = (header == G_TRI_TXTR) || (header == G_TRI_SHADE_TXTR) ||
	                (header == G_TRI_TXTR_ZBUFF) || (header == G_TRI_SHADE_TXTR_ZBUFF);

	if (shade && wordCount < 24)
		return;
	if (textured && wordCount < 40)
		return;

	// Edge coefficients
	s32 yl = F3DGOLDEN_SignExtend( pDataSigned[0], 14 );
	s32 ym = F3DGOLDEN_SignExtend( pDataSigned[1] >> 16, 14 );
	s32 yh = F3DGOLDEN_SignExtend( pDataSigned[1], 14 );
	yh &= ~3;

	s32 xl = F3DGOLDEN_SignExtend( pDataSigned[2], 28 );
	s32 xh = F3DGOLDEN_SignExtend( pDataSigned[4], 28 );
	s32 xm = F3DGOLDEN_SignExtend( pDataSigned[6], 28 );
	s32 dxldy = F3DGOLDEN_SignExtend( pDataSigned[3], 30 );
	s32 dxhdy = F3DGOLDEN_SignExtend( pDataSigned[5], 30 );
	s32 dxmdy = F3DGOLDEN_SignExtend( pDataSigned[7], 30 );

	f32 xhf = _FIXED2FLOAT( (f32)(xh & ~1), 16 );
	f32 xmf = _FIXED2FLOAT( (f32)(xm & ~1), 16 );
	f32 yhf = (f32) yh;
	f32 ymf = (f32) ym;
	f32 ylf = (f32) yl;
	f32 hk = _FIXED2FLOAT( (f32)((dxhdy >> 2) & ~1), 16 );
	f32 mk = _FIXED2FLOAT( (f32)((dxmdy >> 2) & ~1), 16 );
	f32 hc = xhf - hk * yhf;
	f32 mc = xmf - mk * yhf;
	f32 xlf = _FIXED2FLOAT( (f32)(xl & ~1), 16 );
	f32 lk = _FIXED2FLOAT( (f32)((dxldy >> 2) & ~1), 16 );

	struct { f32 x, y; } vertices[8];
	u32 vtxCount = 0;

	// Reconstruct screen-space vertex positions from the edge coefficients
	f32 hkmk = hk - mk;
	if ((hkmk > -0.00000001f) && (hkmk < 0.00000001f))
	{
		auto *vtx = &vertices[vtxCount++];
		vtx->x = hk * yhf + hc;
		vtx->y = yhf * 0.25f;

		if (mc != hc)
		{
			vtx = &vertices[vtxCount++];
			vtx->x = mk * yhf + mc;
			vtx->y = yhf * 0.25f;
		}

		f32 xhym = (hk * ymf + hc);
		f32 xmym = (mk * ymf + mc);

		vtx = &vertices[vtxCount++];
		vtx->x = xhym;
		vtx->y = ymf * 0.25f;

		vtx = &vertices[vtxCount++];
		vtx->x = xmym;
		vtx->y = ymf * 0.25f;

		if (dxldy != dxmdy && ym < yl)
		{
			f32 lc = xlf - lk * ymf;
			f32 y4f = (lc - hc) / (hk - lk);
			vtx = &vertices[vtxCount++];
			vtx->x = hk * y4f + hc;
			vtx->y = y4f * 0.25f;
		}
	}
	else
	{
		f32 y0f = (mc - hc) / (hk - mk);

		auto *vtx = &vertices[vtxCount++];
		vtx->x = hk * y0f + hc;
		vtx->y = y0f * 0.25f;

		f32 y1f = ymf;
		f32 lc = xlf - lk * y1f;

		vtx = &vertices[vtxCount++];
		vtx->x = xlf;
		vtx->y = y1f * 0.25f;

		if (hk == lk)
		{
			f32 lrx = lk * ylf + lc;
			vtx = &vertices[vtxCount++];
			vtx->x = lrx;
			vtx->y = ylf * 0.25f - (vertices[1].y - vertices[0].y);

			vtx = &vertices[vtxCount++];
			vtx->x = lrx;
			vtx->y = ylf * 0.25f;
		}
		else if (mk == lk)
		{
			vtx = &vertices[vtxCount++];
			vtx->x = hk * ylf + hc;
			vtx->y = ylf * 0.25f;
		}
		else
		{
			f32 y2f = (yl == ym) ? (lc - mc) / (mk - lk) : (lc - hc) / (hk - lk);
			vtx = &vertices[vtxCount++];
			vtx->x = hk * y2f + hc;
			vtx->y = y2f * 0.25f;
		}
	}

	if (!shade)
		return;

	// Draw as a translucent flat colour fog wash instead of the actual cloud/water texture.
	s32 r = (s32)( (pData[8]  & 0xffff0000) | ((pData[12] >> 16) & 0x0000ffff) );
	s32 g = (s32)( ((pData[8]  << 16) & 0xffff0000) | (pData[12] & 0x0000ffff) );
	s32 b = (s32)( (pData[9]  & 0xffff0000) | ((pData[13] >> 16) & 0x0000ffff) );
	s32 rc = r << 2; rc = (rc > 0x3ff0000) ? 0x3ff0000 : ((rc < 0) ? 0 : rc);
	s32 gc = g << 2; gc = (gc > 0x3ff0000) ? 0x3ff0000 : ((gc < 0) ? 0 : gc);
	s32 bc = b << 2; bc = (bc > 0x3ff0000) ? 0x3ff0000 : ((bc < 0) ? 0 : bc);
	f32 rf = _FIXED2FLOAT( (f32)(rc >> 18), 8 );
	f32 gf = _FIXED2FLOAT( (f32)(gc >> 18), 8 );
	f32 bf = _FIXED2FLOAT( (f32)(bc >> 18), 8 );

	// Bounding box of the reconstructed vertices, used as a plain screen-space rect.
	f32 minX = vertices[0].x, maxX = vertices[0].x;
	f32 minY = vertices[0].y, maxY = vertices[0].y;
	for (u32 vi = 1; vi < vtxCount; vi++)
	{
		if (vertices[vi].x < minX) minX = vertices[vi].x;
		if (vertices[vi].x > maxX) maxX = vertices[vi].x;
		if (vertices[vi].y < minY) minY = vertices[vi].y;
		if (vertices[vi].y > maxY) maxY = vertices[vi].y;
	}

	// Deliberately translucent for a hazy "fog" look rather than a solid colour
	const f32 FOG_ALPHA = 0.45f;
	// Slightly darken the decoded shade colour because it felt off before
	const f32 FOG_DARKEN = 0.8f;

	f32 whiteness = rf;
	if (gf < whiteness) whiteness = gf;
	if (bf < whiteness) whiteness = bf;
	const f32 WHITE_ALPHA_REDUCTION = 0.5f;
	const f32 WHITE_DARKEN_EXTRA = 0.35f;
	f32 fogAlpha = FOG_ALPHA * (1.0f - whiteness * WHITE_ALPHA_REDUCTION);
	f32 fogDarken = FOG_DARKEN * (1.0f - whiteness * WHITE_DARKEN_EXTRA);

	f32 color[4] = { rf * fogDarken, gf * fogDarken, bf * fogDarken, fogAlpha };

	if (textured)
	{
		// Sample a small, fixed-size sub-window of the cloud/water texture and stretch it evenly across the whole visible rect
		f32 rectWidth = maxX - minX;
		f32 rectHeight = maxY - minY;
		if (rectWidth < 1.0f) rectWidth = 1.0f;
		if (rectHeight < 1.0f) rectHeight = 1.0f;

		f32 dsdx = F3DGOLDEN_SCROLL_WINDOW / rectWidth;
		f32 dtdy = F3DGOLDEN_SCROLL_WINDOW / rectHeight;

		// Fog and texture are baked into this single draw
		gDPTextureRectangle( minX, minY, maxX, maxY, gSP.texture.tile, f3dGoldenScrollS, f3dGoldenScrollT, dsdx, dtdy, color );

		// Grab the real texture size now
		if (combiner.usesT0 && cache.current[0])
		{
			f3dGoldenTexWidth = (f32) cache.current[0]->width;
			f3dGoldenTexHeight = (f32) cache.current[0]->height;
		}
	}
	else
	{
		OGL_DrawRect( (int) minX, (int) minY, (int) maxX, (int) maxY, color );
	}
}

static void F3DGOLDEN_BeginTriangleCommand( u32 w1 )
{
	f3dGoldenCmdWords[0] = w1;
	f3dGoldenCmdWordCount = 1;
	f3dGoldenCmdActive = true;
}

static void F3DGOLDEN_AppendTriangleWord( u32 w1 )
{
	if (f3dGoldenCmdWordCount < F3DGOLDEN_MAX_CMD_WORDS)
		f3dGoldenCmdWords[f3dGoldenCmdWordCount++] = w1;
}

static void F3DGOLDEN_EndTriangleCommand( u32 w1 )
{
	F3DGOLDEN_AppendTriangleWord( w1 );
	f3dGoldenCmdActive = false;
	F3DGOLDEN_DrawTriangleCommand( f3dGoldenCmdWords, f3dGoldenCmdWordCount );
}

void F3D_RDPHalf_1( u32 w0, u32 w1 )
{
	if (GBI.current->type == F3DGOLDEN)
	{
		if (f3dGoldenCmdActive)
		{
			F3DGOLDEN_AppendTriangleWord( w1 );
			return;
		}

		u32 top = _SHIFTR( w1, 24, 8 );
		if ((top >= G_TRI_FILL) && (top <= G_TRI_SHADE_TXTR_ZBUFF))
		{
			F3DGOLDEN_BeginTriangleCommand( w1 );
			return;
		}
	}

	gDP.half_1 = w1;
}

void F3D_RDPHalf_2( u32 w0, u32 w1 )
{
	if (f3dGoldenCmdActive)
	{
		F3DGOLDEN_EndTriangleCommand( w1 );
		return;
	}

	gDP.half_2 = w1;
}

void F3D_RDPHalf_Cont( u32 w0, u32 w1 )
{
	if (f3dGoldenCmdActive)
		F3DGOLDEN_AppendTriangleWord( w1 );
}

void F3D_Tri4( u32 w0, u32 w1 )
{
	gSP4Triangles( _SHIFTR( w0,  0, 4 ), _SHIFTR( w1,  0, 4 ), _SHIFTR( w1,  4, 4 ),
		           _SHIFTR( w0,  4, 4 ), _SHIFTR( w1,  8, 4 ), _SHIFTR( w1, 12, 4 ),
				   _SHIFTR( w0,  8, 4 ), _SHIFTR( w1, 16, 4 ), _SHIFTR( w1, 20, 4 ),
				   _SHIFTR( w0, 12, 4 ), _SHIFTR( w1, 24, 4 ), _SHIFTR( w1, 28, 4 ) );
}

void F3D_Init()
{
	// Set GeometryMode flags
	GBI_InitFlags( F3D );

	GBI.PCStackSize = 10;

	//          GBI Command             Command Value			Command Function
	GBI_SetGBI( G_SPNOOP,				F3D_SPNOOP,				F3D_SPNoOp );
	GBI_SetGBI( G_MTX,					F3D_MTX,				F3D_Mtx );
	GBI_SetGBI( G_RESERVED0,			F3D_RESERVED0,			F3D_Reserved0 );
	GBI_SetGBI( G_MOVEMEM,				F3D_MOVEMEM,			F3D_MoveMem );
	GBI_SetGBI( G_VTX,					F3D_VTX,				F3D_Vtx );
	GBI_SetGBI( G_RESERVED1,			F3D_RESERVED1,			F3D_Reserved1 );
	GBI_SetGBI( G_DL,					F3D_DL,					F3D_DList );
	GBI_SetGBI( G_RESERVED2,			F3D_RESERVED2,			F3D_Reserved2 );
	GBI_SetGBI( G_RESERVED3,			F3D_RESERVED3,			F3D_Reserved3 );
	GBI_SetGBI( G_SPRITE2D_BASE,		F3D_SPRITE2D_BASE,		F3D_Sprite2D_Base );

	GBI_SetGBI( G_TRI1,					F3D_TRI1,				F3D_Tri1 );
	GBI_SetGBI( G_CULLDL,				F3D_CULLDL,				F3D_CullDL );
	GBI_SetGBI( G_POPMTX,				F3D_POPMTX,				F3D_PopMtx );
	GBI_SetGBI( G_MOVEWORD,				F3D_MOVEWORD,			F3D_MoveWord );
	GBI_SetGBI( G_TEXTURE,				F3D_TEXTURE,			F3D_Texture );
	GBI_SetGBI( G_SETOTHERMODE_H,		F3D_SETOTHERMODE_H,		F3D_SetOtherMode_H );
	GBI_SetGBI( G_SETOTHERMODE_L,		F3D_SETOTHERMODE_L,		F3D_SetOtherMode_L );
	GBI_SetGBI( G_ENDDL,				F3D_ENDDL,				F3D_EndDL );
	GBI_SetGBI( G_SETGEOMETRYMODE,		F3D_SETGEOMETRYMODE,	F3D_SetGeometryMode );
	GBI_SetGBI( G_CLEARGEOMETRYMODE,	F3D_CLEARGEOMETRYMODE,	F3D_ClearGeometryMode );
	GBI_SetGBI( G_QUAD,					F3D_QUAD,				F3D_Quad );
	GBI_SetGBI( G_RDPHALF_1,			F3D_RDPHALF_1,			F3D_RDPHalf_1 );
	GBI_SetGBI( G_RDPHALF_2,			F3D_RDPHALF_2,			F3D_RDPHalf_2 );
	GBI_SetGBI( G_RDPHALF_CONT,			F3D_RDPHALF_CONT,		F3D_RDPHalf_Cont );
	GBI_SetGBI( G_TRI4,					F3D_TRI4,				F3D_Tri4 );
}

