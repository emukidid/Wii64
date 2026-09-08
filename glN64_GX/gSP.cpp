/**
 * glN64_GX - gSP.cpp
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009, 2010 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifdef __GX__
#include <gccore.h>
#include "../gui/DEBUG.h"
#endif // __GX__

#include <stdio.h>
#include <math.h>
#include "glN64.h"
#include "Debug.h"
#include "Types.h"
#include "RSP.h"
#include "GBI.h"
#include "gSP.h"
#include "gDP.h"
#include "3DMath.h"
#include "OpenGL.h"
#include "CRC.h"
#include <string.h>
#include "S2DEX.h"
#include "VI.h"
#include "DepthBuffer.h"
#include "RDP.h"
#include "FrameBuffer.h"
extern "C" {
#include "../main/gamehacks.h"
}
#ifndef __LINUX__
# include "Resource.h"
#else
#include <stdlib.h>
# ifndef min
#  define min(a,b) ((a) < (b) ? (a) : (b))
# endif
# ifndef max
#  define max(a,b) ((a) > (b) ? (a) : (b))
# endif
#endif // !__LINUX__

#ifdef DEBUG
extern u32 uc_crc, uc_dcrc;
extern char uc_str[256];
#endif

#define gSPFlushTriangles() \
	if ((OGL.numTriangles > 0) && \
		(RSP.nextCmd != G_TRI1) && \
		(RSP.nextCmd != G_TRI2) && \
		(RSP.nextCmd != G_TRI4) && \
		(RSP.nextCmd != G_QUAD) && \
		(RSP.nextCmd != G_DMA_TRI)) \
		OGL_DrawTriangles()

gSPInfo gSP;

f32 identityMatrix[4][4] =
{
	{ 1.0f, 0.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f }
};


void gSPLoadUcodeEx( u32 uc_start, u32 uc_dstart, u16 uc_dsize )
{
	RSP.PCi = 0;
	gSP.matrix.modelViewi = 0;
	gSP.changed |= CHANGED_MATRIX;
	gSP.status[0] = gSP.status[1] = gSP.status[2] = gSP.status[3] = 0;
	gSP.objRendermode = 0;

	if ((((uc_start & 0x1FFFFFFF) + 4096) > RDRAMSize) || (((uc_dstart & 0x1FFFFFFF) + uc_dsize) > RDRAMSize))
	{
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load ucode out of invalid address\n" );
			DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLoadUcodeEx( 0x%08X, 0x%08X, %i );\n", uc_start, uc_dstart, uc_dsize );
#endif
			return;
	}

	MicrocodeInfo *ucode = GBI_DetectMicrocode( uc_start, uc_dstart, uc_dsize );

	if (ucode->type != NONE) {
		GBI_MakeCurrent( ucode );
# ifdef __GX__
#ifdef SHOW_DEBUG
		sprintf(txtbuffer,"UCODE Detected: %s", MicrocodeTypes[ucode->type]);
		DEBUG_print(txtbuffer,DBG_RSPINFO); 
#endif
# endif // __GX__
	}
#ifdef SHOW_DEBUG	
	else
#ifdef RSPTHREAD
		SetEvent( RSP.threadMsg[RSPMSG_CLOSE] );
#else
# ifndef __GX__
		puts( "Warning: Unknown UCODE!!!" );
# else // !__GX__
		DEBUG_print((char*)"Warning: Unknown UCODE!!!",DBG_RSPINFO);
# endif // __GX__
#endif
#endif
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Unknown microcode: 0x%08X, 0x%08X, %s\n", uc_crc, uc_dcrc, uc_str );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLoadUcodeEx( 0x%08X, 0x%08X, %i );\n", uc_start, uc_dstart, uc_dsize );
#endif
}

#ifdef __GX__
static void _gSPUpdateCombW()
{
	if (OGL.numTriangles)
		OGL_DrawTriangles();

	guMtx44Inverse( gSP.matrix.combined, OGL.GXprojTemp );

	const f32 zTerm = fabsf( OGL.GXprojTemp[2][3] );
	if (zTerm != 0.0f &&
	    fabsf( OGL.GXprojTemp[0][3] ) <= zTerm * 1.0e-4f &&
	    fabsf( OGL.GXprojTemp[1][3] ) <= zTerm * 1.0e-4f)
	{
		OGL.GXcombW[2][3] = GXprojZScale / OGL.GXprojTemp[2][3];
		OGL.GXcombW[2][2] = -GXprojZOffset + (OGL.GXcombW[2][3] * OGL.GXprojTemp[3][3]);
		OGL.GXcombWok = true;
		OGL.GXuseCombW = true;
	}
	else
	{
		OGL.GXcombWok = false;
		OGL.GXuseCombW = false;
	}

	OGL.GXupdateMtx = true;
}
#endif // __GX__

static void _gSPCombineMatrices()
{
	CopyMatrix( gSP.matrix.combined, gSP.matrix.projection );
	MultMatrix( gSP.matrix.combined, gSP.matrix.modelView[gSP.matrix.modelViewi] );

	gSP.changed &= ~CHANGED_MATRIX;

#ifdef __GX__
	_gSPUpdateCombW();
#endif //__GX__
}

void gSPCombineMatrices( u32 mode )
{
	if (mode == 1)
		_gSPCombineMatrices();
}

void gSPProcessVertex( u32 v )
{
	f32 intensity;
	f32 r, g, b;

	if (gSP.changed & CHANGED_MATRIX)
		_gSPCombineMatrices();

	// Point lighting (Zelda OOT/MM) needs the vertex's eye-space position,
	// which requires the pre-projection object-space coordinates. Better save
	// them now, before they're overwritten by the combined-matrix transform.
	f32 objPos[4];
	bool bPointLighting = (gSP.geometryMode & G_LIGHTING) && (gSP.geometryMode & G_POINT_LIGHTING);
	if (bPointLighting)
	{
		objPos[0] = gSP.vertices[v].x;
		objPos[1] = gSP.vertices[v].y;
		objPos[2] = gSP.vertices[v].z;
		objPos[3] = gSP.vertices[v].w;
	}

	TransformVertex( &gSP.vertices[v].x, gSP.matrix.combined );

#ifdef __GX__
	OGL.GXnumVtxMP++;
#endif //__GX__

	//TODO: Properly implement this.
	if (gSP.matrix.billboard)
	{
		gSP.vertices[v].x += gSP.vertices[0].x;
		gSP.vertices[v].y += gSP.vertices[0].y;
		gSP.vertices[v].z += gSP.vertices[0].z;
		gSP.vertices[v].w += gSP.vertices[0].w;
#ifdef __GX__
# ifdef SHOW_DEBUG
		sprintf(txtbuffer,"gSP: Using billboard");
		DEBUG_print(txtbuffer,6); 
# endif
#endif // __GX__
	}

	//TODO: Investigate this
	if (!(gSP.geometryMode & G_ZBUFFER))
	{
		gSP.vertices[v].z = -gSP.vertices[v].w;
	}

	if (gSP.geometryMode & G_LIGHTING)
	{
		TransformVector( &gSP.vertices[v].nx, gSP.matrix.modelView[gSP.matrix.modelViewi] );
#ifndef __GX__
		Normalize( &gSP.vertices[v].nx );
#else //!__GX__
		guVecNormalize((guVector*) &gSP.vertices[v].nx,(guVector*) &gSP.vertices[v].nx );
#endif //__GX__

		r = gSP.lights[gSP.numLights].r;
		g = gSP.lights[gSP.numLights].g;
		b = gSP.lights[gSP.numLights].b;

		if (bPointLighting)
		{
			// Zelda OOT/MM: lights with ca != 0 are point lights, attenuated
			// by distance using the light's la/qa coefficients rather than a
			// plain directional dot product.
			f32 eyePos[4] = { objPos[0], objPos[1], objPos[2], objPos[3] };
			TransformVertex( eyePos, gSP.matrix.modelView[gSP.matrix.modelViewi] );

			for (int i = 0; i < gSP.numLights; i++)
			{
				if (gSP.lights[i].ca != 0.0f)
				{
					f32 dx = gSP.lights[i].posx - eyePos[0];
					f32 dy = gSP.lights[i].posy - eyePos[1];
					f32 dz = gSP.lights[i].posz - eyePos[2];

					f32 K = dx * dx + dy * dy + dz * dz * 2.0f;
					f32 KS = sqrtf( K );

					f32 dirx = dx, diry = dy, dirz = dz;
					if (KS != 0.0f)
					{
						dirx = 4.0f * dx / KS;
						diry = 4.0f * dy / KS;
						dirz = 4.0f * dz / KS;
					}
					if (dirx < -1.0f) dirx = -1.0f; else if (dirx > 1.0f) dirx = 1.0f;
					if (diry < -1.0f) diry = -1.0f; else if (diry > 1.0f) diry = 1.0f;
					if (dirz < -1.0f) dirz = -1.0f; else if (dirz > 1.0f) dirz = 1.0f;

					intensity = dirx * gSP.vertices[v].nx + diry * gSP.vertices[v].ny + dirz * gSP.vertices[v].nz;
					if (intensity < -1.0f) intensity = -1.0f; else if (intensity > 1.0f) intensity = 1.0f;

					f32 KSF = floorf( KS );
					f32 D = (KSF * gSP.lights[i].la * 2.0f + KSF * KSF * gSP.lights[i].qa * 0.125f) * FIXED2FLOATRECIP16 + 1.0f;
					intensity = (D != 0.0f) ? (intensity / D) : 0.0f;
				}
				else
				{
#ifndef __GX__
					intensity = DotProduct( &gSP.vertices[v].nx, &gSP.lights[i].x );
#else //!__GX__
					intensity = guVecDotProduct((guVector*) &gSP.vertices[v].nx,(guVector*) &gSP.lights[i].x );
#endif //__GX__
				}

				if (intensity > 0.0f)
				{
					r += gSP.lights[i].r * intensity;
					g += gSP.lights[i].g * intensity;
					b += gSP.lights[i].b * intensity;
				}
			}
		}
		else
		{
			for (int i = 0; i < gSP.numLights; i++)
			{
#ifndef __GX__
				intensity = DotProduct( &gSP.vertices[v].nx, &gSP.lights[i].x );
#else //!__GX__
				intensity = guVecDotProduct((guVector*) &gSP.vertices[v].nx,(guVector*) &gSP.lights[i].x );
#endif //__GX__

				if (intensity < 0.0f) intensity = 0.0f;

				r += gSP.lights[i].r * intensity;
				g += gSP.lights[i].g * intensity;
				b += gSP.lights[i].b * intensity;
			}
		}

#ifdef GLN64_SDLOG
		sprintf(txtbuffer,"gSPProcVert%d: Vert RGBA = %.2f, %.2f, %.2f, %.2f, Vert N_XYZ = %.2f, %.2f, %.2f, lighting RGB = %.2f, %.2f, %.2f, numLghts = %d\n", v, gSP.vertices[v].r, gSP.vertices[v].g, gSP.vertices[v].b, gSP.vertices[v].a, r, g, b, gSP.numLights);
		DEBUG_print(txtbuffer,DBG_SDGECKOPRINT);
#endif // GLN64_SDLOG

		gSP.vertices[v].r = min(1.0f, r);
		gSP.vertices[v].g = min(1.0f, g);
		gSP.vertices[v].b = min(1.0f, b);

		if (gSP.geometryMode & G_TEXTURE_GEN)
		{
			if ((GBI.current != NULL) && (GBI.current->type == F3DFLX2))
			{
				f32 intensity = (gSP.vertices[v].nx * gSP.lookat.xyz[0][0] +
								  gSP.vertices[v].ny * gSP.lookat.xyz[0][1] +
								  gSP.vertices[v].nz * gSP.lookat.xyz[0][2]) * 128.0f;
				s16 index = (s16)intensity;
#ifndef _BIG_ENDIAN
				gSP.vertices[v].a = RDRAM[(gSP.DMAIO_address + 128 + index) ^ 3] * 0.0039215689f;
#else // !_BIG_ENDIAN
				gSP.vertices[v].a = RDRAM[(gSP.DMAIO_address + 128 + index) ^ 0] * 0.0039215689f;
#endif // _BIG_ENDIAN
			}
			else
			{
				f32 x, y;

				if (gSP.lookat.enable)
				{
					x = gSP.vertices[v].nx * gSP.lookat.xyz[0][0] +
						gSP.vertices[v].ny * gSP.lookat.xyz[0][1] +
						gSP.vertices[v].nz * gSP.lookat.xyz[0][2];
					y = gSP.vertices[v].nx * gSP.lookat.xyz[1][0] +
						gSP.vertices[v].ny * gSP.lookat.xyz[1][1] +
						gSP.vertices[v].nz * gSP.lookat.xyz[1][2];
				}
				else
				{
					x = gSP.vertices[v].nx;
					y = gSP.vertices[v].ny;
				}

				if (gSP.geometryMode & G_TEXTURE_GEN_LINEAR)
				{
					if (x < -1.0f) x = -1.0f;
					if (x >  1.0f) x =  1.0f;
					if (y < -1.0f) y = -1.0f;
					if (y >  1.0f) y =  1.0f;
					gSP.vertices[v].s = acosf(-x) * 325.94931f;
					gSP.vertices[v].t = acosf(-y) * 325.94931f;
				}
				else // G_TEXTURE_GEN
				{
					gSP.vertices[v].s = (x + 1.0f) * 512.0f;
					gSP.vertices[v].t = (y + 1.0f) * 512.0f;
				}
			}
		}
	}
}

void gSPNoOp()
{
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_IGNORED, "gSPNoOp();\n" );
#endif
}

void gSPMatrix( u32 matrix, u8 param )
{
	f32 mtx[4][4];
	u32 address = RSP_SegmentToPhysical( matrix );

	if (address + 64 > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to load matrix from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPMatrix( 0x%08X, %s | %s | %s );\n",
			matrix,
			(param & G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_MODELVIEW",
			(param & G_MTX_LOAD) ? "G_MTX_LOAD" : "G_MTX_MUL",
			(param & G_MTX_PUSH) ? "G_MTX_PUSH" : "G_MTX_NOPUSH" );
#endif
		return;
	}

	RSP_LoadMatrix( mtx, address );

	if (param & G_MTX_PROJECTION)
	{
		if (param & G_MTX_LOAD)
			CopyMatrix( gSP.matrix.projection, mtx );
		else
			MultMatrix( gSP.matrix.projection, mtx );
	}
	else
	{
		if ((param & G_MTX_PUSH) && (gSP.matrix.modelViewi < (gSP.matrix.stackSize - 1)))
		{
			CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi + 1], gSP.matrix.modelView[gSP.matrix.modelViewi] );
			gSP.matrix.modelViewi++;
		}
#ifdef DEBUG
		else
			DebugMsg( DEBUG_ERROR | DEBUG_MATRIX, "// Modelview stack overflow\n" );
#endif

		if (param & G_MTX_LOAD)
			CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
		else
			MultMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
	}

	gSP.changed |= CHANGED_MATRIX;

#ifdef DEBUG
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[0][0], mtx[0][1], mtx[0][2], mtx[0][3] );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[1][0], mtx[1][1], mtx[1][2], mtx[1][3] );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[2][0], mtx[2][1], mtx[2][2], mtx[2][3] );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[3][0], mtx[3][1], mtx[3][2], mtx[3][3] );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPMatrix( 0x%08X, %s | %s | %s );\n",
		matrix,
		(param & G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_MODELVIEW",
		(param & G_MTX_LOAD) ? "G_MTX_LOAD" : "G_MTX_MUL",
		(param & G_MTX_PUSH) ? "G_MTX_PUSH" : "G_MTX_NOPUSH" );
#endif
}

void gSPDMAMatrix( u32 matrix, u8 index, u8 multiply )
{
	f32 mtx[4][4];
	u32 address = gSP.DMAOffsets.mtx + RSP_SegmentToPhysical( matrix );

	if (address + 64 > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to load matrix from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPDMAMatrix( 0x%08X, %i, %s );\n",
			matrix, index, multiply ? "TRUE" : "FALSE" );
#endif
		return;
	}

	RSP_LoadMatrix( mtx, address );

	gSP.matrix.modelViewi = index;

	if (multiply)
	{
		CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], gSP.matrix.modelView[0] );
		MultMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
	}
	else
		CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );

	CopyMatrix( gSP.matrix.projection, identityMatrix );

#ifdef __GX__
# ifdef SHOW_DEBUG
	sprintf(txtbuffer,"gSP: gSPDMAMatrix");
	DEBUG_print(txtbuffer,6);
# endif
#endif // __GX__

	gSP.changed |= CHANGED_MATRIX;
#ifdef DEBUG
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[0][0], mtx[0][1], mtx[0][2], mtx[0][3] );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[1][0], mtx[1][1], mtx[1][2], mtx[1][3] );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[2][0], mtx[2][1], mtx[2][2], mtx[2][3] );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
		mtx[3][0], mtx[3][1], mtx[3][2], mtx[3][3] );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPDMAMatrix( 0x%08X, %i, %s );\n",
		matrix, index, multiply ? "TRUE" : "FALSE" );
#endif
}

void gSPViewport( u32 v )
{
	u32 address = RSP_SegmentToPhysical( v );

	if ((address + 16) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load viewport from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPViewport( 0x%08X );\n", v );
#endif
		return;
	}

#ifndef _BIG_ENDIAN
	gSP.viewport.vscale[0] = _FIXED2FLOAT( *(s16*)&RDRAM[address +  2], 2 );
	gSP.viewport.vscale[1] = _FIXED2FLOAT( *(s16*)&RDRAM[address     ], 2 );
	gSP.viewport.vscale[2] = _FIXED2FLOAT( *(s16*)&RDRAM[address +  6], 10 ) + FIXED2FLOATRECIP10;
	gSP.viewport.vscale[3] = *(s16*)&RDRAM[address +  4];
	gSP.viewport.vtrans[0] = _FIXED2FLOAT( *(s16*)&RDRAM[address + 10], 2 );
	gSP.viewport.vtrans[1] = _FIXED2FLOAT( *(s16*)&RDRAM[address +  8], 2 );
	gSP.viewport.vtrans[2] = _FIXED2FLOAT( *(s16*)&RDRAM[address + 14], 10 ) + FIXED2FLOATRECIP10;
	gSP.viewport.vtrans[3] = *(s16*)&RDRAM[address + 12];
#else // !_BIG_ENDIAN -> This is to correct for big endian
	gSP.viewport.vscale[0] = _FIXED2FLOAT( *(s16*)&RDRAM[address     ], 2 );
	gSP.viewport.vscale[1] = _FIXED2FLOAT( *(s16*)&RDRAM[address +  2], 2 );
	gSP.viewport.vscale[2] = _FIXED2FLOAT( *(s16*)&RDRAM[address +  4], 10 ) + FIXED2FLOATRECIP10;
	gSP.viewport.vscale[3] = *(s16*)&RDRAM[address +  6];
	gSP.viewport.vtrans[0] = _FIXED2FLOAT( *(s16*)&RDRAM[address +  8], 2 );
	gSP.viewport.vtrans[1] = _FIXED2FLOAT( *(s16*)&RDRAM[address + 10], 2 );
	gSP.viewport.vtrans[2] = _FIXED2FLOAT( *(s16*)&RDRAM[address + 12], 10 ) + FIXED2FLOATRECIP10;
	gSP.viewport.vtrans[3] = *(s16*)&RDRAM[address + 14];
#endif // _BIG_ENDIAN


	if (gSP.viewport.vscale[1] < 0.0f && GBI.current != NULL && !GBI.current->negativeY)
		gSP.viewport.vscale[1] = -gSP.viewport.vscale[1];

	gSP.viewport.x		= gSP.viewport.vtrans[0] - gSP.viewport.vscale[0];
	gSP.viewport.y		= gSP.viewport.vtrans[1] - gSP.viewport.vscale[1];
	gSP.viewport.width	= gSP.viewport.vscale[0] * 2;
	gSP.viewport.height	= gSP.viewport.vscale[1] * 2;
	gSP.viewport.nearz	= gSP.viewport.vtrans[2] - gSP.viewport.vscale[2];
	gSP.viewport.farz	= gSP.viewport.vtrans[2] + gSP.viewport.vscale[2];

	gSP.changed |= CHANGED_VIEWPORT;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPViewport( 0x%08X );\n", v );
#endif
}

void gSPForceMatrix( u32 mptr )
{
	u32 address = RSP_SegmentToPhysical( mptr );

	if (address + 64 > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to load from invalid address" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPForceMatrix( 0x%08X );\n", mptr );
#endif
		return;
	}

	RSP_LoadMatrix( gSP.matrix.combined, RSP_SegmentToPhysical( mptr ) );

#ifdef __GX__
#ifdef SHOW_DEBUG
	sprintf(txtbuffer,"gSP: gSPForceMatrix");
	DEBUG_print(txtbuffer,7);
#endif

	_gSPUpdateCombW();
#endif //__GX__

	gSP.changed &= ~CHANGED_MATRIX;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPForceMatrix( 0x%08X );\n", mptr );
#endif
}

void gSPLight( u32 l, s32 n )
{
	n--;
	u32 address = RSP_SegmentToPhysical( l );

	if ((address + 16) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load light from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLight( 0x%08X, LIGHT_%i );\n",
			l, n );
#endif
		return;
	}

	Light *light = (Light*)&RDRAM[address];

	if (n >= 0 && n < 8)
	{
		gSP.lights[n].r = GXcastu8f32( light->r );
		gSP.lights[n].g = GXcastu8f32( light->g );
		gSP.lights[n].b = GXcastu8f32( light->b );

		gSP.lights[n].x = light->x;
		gSP.lights[n].y = light->y;
		gSP.lights[n].z = light->z;

		// Zelda OOT/MM point light fields (ca/la/qa attenuation + position).
		// ca overlays the same byte as ordinary lights' unused "type" byte:
		// a nonzero value here is what marks this record as a point light.
#ifndef _BIG_ENDIAN
		gSP.lights[n].ca = (f32)RDRAM[(address +  3) ^ 3];
		gSP.lights[n].la = (f32)RDRAM[(address +  7) ^ 3];
		gSP.lights[n].qa = (f32)RDRAM[(address + 14) ^ 3];
		gSP.lights[n].posx = (f32)(((s16*)RDRAM)[((address >> 1) + 4) ^ 1]);
		gSP.lights[n].posy = (f32)(((s16*)RDRAM)[((address >> 1) + 5) ^ 1]);
		gSP.lights[n].posz = (f32)(((s16*)RDRAM)[((address >> 1) + 6) ^ 1]);
#else //!_BIG_ENDIAN
		gSP.lights[n].ca = (f32)RDRAM[(address +  3) ^ 0];
		gSP.lights[n].la = (f32)RDRAM[(address +  7) ^ 0];
		gSP.lights[n].qa = (f32)RDRAM[(address + 14) ^ 0];
		gSP.lights[n].posx = (f32)(((s16*)RDRAM)[((address >> 1) + 4) ^ 0]);
		gSP.lights[n].posy = (f32)(((s16*)RDRAM)[((address >> 1) + 5) ^ 0]);
		gSP.lights[n].posz = (f32)(((s16*)RDRAM)[((address >> 1) + 6) ^ 0]);
#endif //_BIG_ENDIAN

#ifndef __GX__
		Normalize( &gSP.lights[n].x );
#else //!__GX__
		guVecNormalize((guVector*) &gSP.lights[n].x,(guVector*) &gSP.lights[n].x );
#endif //__GX__
	}

#ifdef DEBUG
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// x = %2.6f    y = %2.6f    z = %2.6f\n",
		_FIXED2FLOAT( light->x, 7 ), _FIXED2FLOAT( light->y, 7 ), _FIXED2FLOAT( light->z, 7 ) );
	DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// r = %3i    g = %3i   b = %3i\n",
		light->r, light->g, light->b );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLight( 0x%08X, LIGHT_%i );\n",
		l, n );
#endif
}

void gSPLightAcclaim( u32 l, s32 n )
{
	u32 addrByte = RSP_SegmentToPhysical( l );

	if ((addrByte + 16) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load light from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLightAcclaim( 0x%08X, LIGHT_%i );\n", l, n );
#endif
		return;
	}

	if (n >= 0 && n < 8)
	{
		const u32 addrShort = addrByte >> 1;
		gSP.lights[n].posx = (f32)(((s16*)RDRAM)[(addrShort + 0) ^ 0]);
		gSP.lights[n].posy = (f32)(((s16*)RDRAM)[(addrShort + 1) ^ 0]);
		gSP.lights[n].posz = (f32)(((s16*)RDRAM)[(addrShort + 2) ^ 0]);
		gSP.lights[n].ca   = (f32)(((s16*)RDRAM)[(addrShort + 5) ^ 0]);
		gSP.lights[n].la   = _FIXED2FLOAT( ((u16*)RDRAM)[(addrShort + 6) ^ 0], 16 );
		gSP.lights[n].qa   = (f32)(((u16*)RDRAM)[(addrShort + 7) ^ 0]);
		gSP.lights[n].r = GXcastu8f32( RDRAM[(addrByte + 6) ^ 0] );
		gSP.lights[n].g = GXcastu8f32( RDRAM[(addrByte + 7) ^ 0] );
		gSP.lights[n].b = GXcastu8f32( RDRAM[(addrByte + 8) ^ 0] );

	}
}

void gSPLookAt( u32 l, u32 n )
{
	u32 address = RSP_SegmentToPhysical( l );

	if ((address + sizeof( Light )) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load lookat from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLookAt( 0x%08X, LOOKAT_%i );\n", l, n );
#endif
		return;
	}

	Light *light = (Light*)&RDRAM[address];

	gSP.lookat.xyz[n][0] = light->x;
	gSP.lookat.xyz[n][1] = light->y;
	gSP.lookat.xyz[n][2] = light->z;

	gSP.lookat.enable = (n == 0) || (light->x != 0 || light->y != 0);

#ifndef __GX__
	Normalize( gSP.lookat.xyz[n] );
#else //!__GX__
	guVecNormalize((guVector*) gSP.lookat.xyz[n],(guVector*) gSP.lookat.xyz[n] );
#endif //__GX__

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLookAt( 0x%08X, LOOKAT_%i );\n", l, n );
#endif
}

void gSPVertex( u32 v, u32 n, u32 v0 )
{
	u32 address = RSP_SegmentToPhysical( v );

	if ((address + sizeof( Vertex ) * n) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPVertex( 0x%08X, %i, %i );\n",
			v, n, v0 );
#endif
		return;
	}

	Vertex *vertex = (Vertex*)&RDRAM[address];

	if ((n + v0) < SP_VERTEX_COUNT)
	{
		for (unsigned int i = v0; i < n + v0; i++)
		{
			gSP.vertices[i].x = vertex->x;
			gSP.vertices[i].y = vertex->y;
			gSP.vertices[i].z = vertex->z;
			gSP.vertices[i].flag = vertex->flag;
			gSP.vertices[i].s = _FIXED2FLOAT( vertex->s, 5 );
			gSP.vertices[i].t = _FIXED2FLOAT( vertex->t, 5 );

			if (gSP.geometryMode & G_LIGHTING)
			{
				gSP.vertices[i].nx = vertex->normal.x;
				gSP.vertices[i].ny = vertex->normal.y;
				gSP.vertices[i].nz = vertex->normal.z;
				gSP.vertices[i].a = GXcastu8f32( vertex->color.a );
			}
			else
			{
				gSP.vertices[i].r = GXcastu8f32( vertex->color.r );
				gSP.vertices[i].g = GXcastu8f32( vertex->color.g );
				gSP.vertices[i].b = GXcastu8f32( vertex->color.b );
				gSP.vertices[i].a = GXcastu8f32( vertex->color.a );
			}

#ifdef DEBUG
			DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_VERTEX, "// x = %6i    y = %6i    z = %6i \n",
				vertex->x, vertex->y, vertex->z );
			DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_VERTEX, "// s = %5.5f    t = %5.5f    flag = %i \n",
				vertex->s, vertex->t, vertex->flag );

			if (gSP.geometryMode & G_LIGHTING)
			{
				DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_VERTEX, "// nx = %2.6f    ny = %2.6f    nz = %2.6f\n",
					_FIXED2FLOAT( vertex->normal.x, 7 ), _FIXED2FLOAT( vertex->normal.y, 7 ), _FIXED2FLOAT( vertex->normal.z, 7 ) );
			}
			else
			{
				DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_VERTEX, "// r = %3u    g = %3u    b = %3u    a = %3u\n",
					vertex->color.r, vertex->color.g, vertex->color.b, vertex->color.a );
			}
#endif

			gSPProcessVertex( i );

			vertex++;
		}
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices past vertex buffer size\n" );
#endif

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPVertex( 0x%08X, %i, %i );\n",
		v, n, v0 );
#endif
}

static void calcF3DAMTexCoords( const Vertex *_vertex, SPVertex &_vtx )
{
	const u32 s0 = (u32)_vertex->s;
	const u32 t0 = (u32)_vertex->t;
	const u32 acum_0 = ((_SHIFTR( gSP.textureCoordScaleOrg, 0, 16 ) * t0) << 1) + 0x8000;
	const u32 acum_1 = ((_SHIFTR( gSP.textureCoordScale[1], 0, 16 ) * t0) << 1) + 0x8000;
	const u32 sres = ((_SHIFTR( gSP.textureCoordScaleOrg, 16, 16 ) * s0) << 1) + acum_0;
	const u32 tres = ((_SHIFTR( gSP.textureCoordScale[1], 16, 16 ) * s0) << 1) + acum_1;
	const s16 s = _SHIFTR( sres, 16, 16 ) + _SHIFTR( gSP.textureCoordScale[0], 16, 16 );
	const s16 t = _SHIFTR( tres, 16, 16 ) + _SHIFTR( gSP.textureCoordScale[0], 0, 16 );

	_vtx.s = _FIXED2FLOAT( s, 5 );
	_vtx.t = _FIXED2FLOAT( t, 5 );
}

void gSPF3DAMVertex( u32 v, u32 n, u32 v0 )
{
	u32 address = RSP_SegmentToPhysical( v );

	if ((address + sizeof( Vertex ) * n) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices from invalid address\n" );
#endif
		return;
	}

	Vertex *vertex = (Vertex*)&RDRAM[address];

	if ((n + v0) < SP_VERTEX_COUNT)
	{
		for (unsigned int i = v0; i < n + v0; i++)
		{
			gSP.vertices[i].x = vertex->x;
			gSP.vertices[i].y = vertex->y;
			gSP.vertices[i].z = vertex->z;
			gSP.vertices[i].flag = vertex->flag;
			calcF3DAMTexCoords( vertex, gSP.vertices[i] );

			if (gSP.geometryMode & G_LIGHTING)
			{
				gSP.vertices[i].nx = vertex->normal.x;
				gSP.vertices[i].ny = vertex->normal.y;
				gSP.vertices[i].nz = vertex->normal.z;
				gSP.vertices[i].a = GXcastu8f32( vertex->color.a );
			}
			else
			{
				gSP.vertices[i].r = GXcastu8f32( vertex->color.r );
				gSP.vertices[i].g = GXcastu8f32( vertex->color.g );
				gSP.vertices[i].b = GXcastu8f32( vertex->color.b );
				gSP.vertices[i].a = GXcastu8f32( vertex->color.a );
			}

			gSPProcessVertex( i );

			vertex++;
		}
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices past vertex buffer size\n" );
#endif
}

void gSPNIVertex( u32 v, u32 n, u32 v0 )
{
	u32 address = RSP_SegmentToPhysical( v );

	if ((address + sizeof( Vertex ) * n) > RDRAMSize)
	{
		return;
	}

	Vertex* vertex = (Vertex*)&RDRAM[address];

	if ((n + v0) < SP_VERTEX_COUNT)
	{
		for (unsigned int i = v0; i < n + v0; i++)
		{
			gSP.vertices[i].x = vertex->x;
			gSP.vertices[i].y = vertex->y;
			gSP.vertices[i].z = vertex->z;
			gSP.vertices[i].flag = 0;
			gSP.vertices[i].s = _FIXED2FLOAT( vertex->s, 5 );
			gSP.vertices[i].t = _FIXED2FLOAT( vertex->t, 5 );

			u8 *color = &RDRAM[gSP.vertexColorBase + (i << 1)];

			if (gSP.geometryMode & G_LIGHTING)
			{
				gSP.vertices[i].nx = (s8)color[0];
				gSP.vertices[i].ny = (s8)color[1];
				gSP.vertices[i].nz = (s8)vertex->flag;
				gSP.vertices[i].a = GXcastu8f32( vertex->color.a );
			}
			else
			{
				gSP.vertices[i].r = GXcastu8f32( vertex->color.r );
				gSP.vertices[i].g = GXcastu8f32( vertex->color.g );
				gSP.vertices[i].b = GXcastu8f32( vertex->color.b );
				gSP.vertices[i].a = GXcastu8f32( vertex->color.a );
			}

			gSPProcessVertex(i);

			if (gSP.geometryMode & G_LIGHTING)
			{
				gSP.vertices[i].r *= GXcastu8f32( vertex->color.r );
				gSP.vertices[i].g *= GXcastu8f32( vertex->color.g );
				gSP.vertices[i].b *= GXcastu8f32( vertex->color.b );
			}

			vertex++;
		}
	}
}

void gSPCIVertex( u32 v, u32 n, u32 v0 )
{
	u32 address = RSP_SegmentToPhysical( v );

	if ((address + sizeof( PDVertex ) * n) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPCIVertex( 0x%08X, %i, %i );\n",
			v, n, v0 );
#endif
		return;
	}

	PDVertex *vertex = (PDVertex*)&RDRAM[address];

	if ((n + v0) < SP_VERTEX_COUNT)
	{
		for (unsigned int i = v0; i < n + v0; i++)
		{
			gSP.vertices[i].x = vertex->x;
			gSP.vertices[i].y = vertex->y;
			gSP.vertices[i].z = vertex->z;
			gSP.vertices[i].flag = 0;
			gSP.vertices[i].s = _FIXED2FLOAT( vertex->s, 5 );
			gSP.vertices[i].t = _FIXED2FLOAT( vertex->t, 5 );

			u8 *color = &RDRAM[gSP.vertexColorBase + (vertex->ci & 0xff)];

#ifndef _BIG_ENDIAN
			if (gSP.geometryMode & G_LIGHTING)
			{
				gSP.vertices[i].nx = (s8)color[3];
				gSP.vertices[i].ny = (s8)color[2];
				gSP.vertices[i].nz = (s8)color[1];
				gSP.vertices[i].a = color[0] * 0.0039215689f;
			}
			else
			{
				gSP.vertices[i].r = color[3] * 0.0039215689f;
				gSP.vertices[i].g = color[2] * 0.0039215689f;
				gSP.vertices[i].b = color[1] * 0.0039215689f;
				gSP.vertices[i].a = color[0] * 0.0039215689f;
			}
#else // !_BIG_ENDIAN
			if (gSP.geometryMode & G_LIGHTING)
			{
				gSP.vertices[i].nx = (s8)color[0];
				gSP.vertices[i].ny = (s8)color[1];
				gSP.vertices[i].nz = (s8)color[2];
				gSP.vertices[i].a = GXcastu8f32( color[3] );
			}
			else
			{
				gSP.vertices[i].r = GXcastu8f32( color[0] );
				gSP.vertices[i].g = GXcastu8f32( color[1] );
				gSP.vertices[i].b = GXcastu8f32( color[2] );
				gSP.vertices[i].a = GXcastu8f32( color[3] );
			}
#endif // _BIG_ENDIAN

			gSPProcessVertex( i );

			vertex++;
		}
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices past vertex buffer size\n" );
#endif

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPCIVertex( 0x%08X, %i, %i );\n",
		v, n, v0 );
#endif
}

void gSPDMAVertex( u32 v, u32 n, u32 v0 )
{
	u32 address = gSP.DMAOffsets.vtx + RSP_SegmentToPhysical( v );

	if ((address + 10 * n) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPDMAVertex( 0x%08X, %i, %i );\n",
			v, n, v0 );
#endif
		return;
	}

	if ((n + v0) < SP_VERTEX_COUNT)
	{
		for (unsigned int i = v0; i < n + v0; i++)
		{
#ifndef _BIG_ENDIAN
			gSP.vertices[i].x = *(s16*)&RDRAM[address ^ 2];
			gSP.vertices[i].y = *(s16*)&RDRAM[(address + 2) ^ 2];
			gSP.vertices[i].z = *(s16*)&RDRAM[(address + 4) ^ 2];

			if (gSP.geometryMode & G_LIGHTING)
			{
				gSP.vertices[i].nx = *(s8*)&RDRAM[(address + 6) ^ 3];
				gSP.vertices[i].ny = *(s8*)&RDRAM[(address + 7) ^ 3];
				gSP.vertices[i].nz = *(s8*)&RDRAM[(address + 8) ^ 3];
				gSP.vertices[i].a = *(u8*)&RDRAM[(address + 9) ^ 3] * 0.0039215689f;
			}
			else
			{
				gSP.vertices[i].r = *(u8*)&RDRAM[(address + 6) ^ 3] * 0.0039215689f;
				gSP.vertices[i].g = *(u8*)&RDRAM[(address + 7) ^ 3] * 0.0039215689f;
				gSP.vertices[i].b = *(u8*)&RDRAM[(address + 8) ^ 3] * 0.0039215689f;
				gSP.vertices[i].a = *(u8*)&RDRAM[(address + 9) ^ 3] * 0.0039215689f;
			}
#else // !_BIG_ENDIAN -> This fixes an endian issue.
			gSP.vertices[i].x = *(s16*)&RDRAM[address ^ 0];
			gSP.vertices[i].y = *(s16*)&RDRAM[(address + 2) ^ 0];
			gSP.vertices[i].z = *(s16*)&RDRAM[(address + 4) ^ 0];

			if (gSP.geometryMode & G_LIGHTING)
			{
				gSP.vertices[i].nx = *(s8*)&RDRAM[(address + 6) ^ 0];
				gSP.vertices[i].ny = *(s8*)&RDRAM[(address + 7) ^ 0];
				gSP.vertices[i].nz = *(s8*)&RDRAM[(address + 8) ^ 0];
				gSP.vertices[i].a = GXcastu8f32( *(u8*)&RDRAM[(address + 9) ^ 0] );
			}
			else
			{
				gSP.vertices[i].r = GXcastu8f32( *(u8*)&RDRAM[(address + 6) ^ 0] );
				gSP.vertices[i].g = GXcastu8f32( *(u8*)&RDRAM[(address + 7) ^ 0] );
				gSP.vertices[i].b = GXcastu8f32( *(u8*)&RDRAM[(address + 8) ^ 0] );
				gSP.vertices[i].a = GXcastu8f32( *(u8*)&RDRAM[(address + 9) ^ 0] );
			}
#endif // _BIG_ENDIAN

			gSPProcessVertex( i );

			address += 10;
		}
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices past vertex buffer size\n" );
#endif

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPDMAVertex( 0x%08X, %i, %i );\n",
		v, n, v0 );
#endif
}

void gSPDisplayList( u32 dl )
{
	u32 address = RSP_SegmentToPhysical( dl );

	if ((address + 8) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load display list from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDisplayList( 0x%08X );\n",
			dl );
#endif
		return;
	}

	if (RSP.PCi < (GBI.PCStackSize - 1))
	{
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "\n" );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDisplayList( 0x%08X );\n",
		dl );
#endif
		RSP.PCi++;
		RSP.PC[RSP.PCi] = address;
		RSP.nextCmd = _SHIFTR( *(u32*)&RDRAM[address], 24, 8 );
	}
#ifdef DEBUG
	else
	{
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// PC stack overflow\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDisplayList( 0x%08X );\n",
			dl );
	}
#endif
}

void gSPDMADisplayList( u32 dl, u32 n )
{
	if ((dl + (n << 3)) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load display list from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDMADisplayList( 0x%08X, %i );\n",
			dl, n );
#endif
		return;
	}

	u32 curDL = RSP.PC[RSP.PCi];

	RSP.PC[RSP.PCi] = RSP_SegmentToPhysical( dl );

	while ((RSP.PC[RSP.PCi] - dl) < (n << 3))
	{
		if ((RSP.PC[RSP.PCi] + 8) > RDRAMSize)
		{
#ifdef DEBUG
			switch (Debug.level)
			{
				case DEBUG_LOW:
                    DebugMsg( DEBUG_LOW | DEBUG_ERROR, "ATTEMPTING TO EXECUTE RSP COMMAND AT INVALID RDRAM LOCATION\n" );
					break;
				case DEBUG_MEDIUM:
                    DebugMsg( DEBUG_MEDIUM | DEBUG_ERROR, "Attempting to execute RSP command at invalid RDRAM location\n" );
					break;
				case DEBUG_HIGH:
                    DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to execute RSP command at invalid RDRAM location\n" );
					break;
			}
#endif
			break;
		}

		u32 w0 = *(u32*)&RDRAM[RSP.PC[RSP.PCi]];
		u32 w1 = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 4];

#ifdef DEBUG
		DebugRSPState( RSP.PCi, RSP.PC[RSP.PCi], _SHIFTR( w0, 24, 8 ), w0, w1 );
		DebugMsg( DEBUG_LOW | DEBUG_HANDLED, "0x%08lX: CMD=0x%02lX W0=0x%08lX W1=0x%08lX\n", RSP.PC[RSP.PCi], _SHIFTR( w0, 24, 8 ), w0, w1 );
#endif

		RSP.PC[RSP.PCi] += 8;
		RSP.nextCmd = _SHIFTR( *(u32*)&RDRAM[RSP.PC[RSP.PCi]], 24, 8 );

		GBI.cmd[_SHIFTR( w0, 24, 8 )]( w0, w1 );
	}

	RSP.PC[RSP.PCi] = curDL;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDMADisplayList( 0x%08X, %i );\n",
		dl, n );
#endif
}

void gSPBranchList( u32 dl )
{
	u32 address = RSP_SegmentToPhysical( dl );

	if ((address + 8) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to branch to display list at invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchList( 0x%08X );\n",
			dl );
#endif
		return;
	}

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchList( 0x%08X );\n",
		dl );
#endif
	if(address == RSP.PC[RSP.PCi]-8) {	// Gauntlet Legends fix, display list branches to itself to idle the RSP
		RSP.infloop = TRUE;
		RSP.PC[RSP.PCi] -= 8;
		RSP.halt = TRUE;
		return;
	}
	else {
		RSP.PC[RSP.PCi] = address;
		RSP.nextCmd = _SHIFTR( *(u32*)&RDRAM[address], 24, 8 );
	}
}

void gSPBranchLessZ( u32 branchdl, u32 vtx, f32 zval )
{
	u32 address = RSP_SegmentToPhysical( branchdl );

	if ((address + 8) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Specified display list at invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchLessZ( 0x%08X, %i, %i );\n",
			branchdl, vtx, zval );
#endif
		return;
	}

	if (gSP.vertices[vtx].z <= zval)
		RSP.PC[RSP.PCi] = address;

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchLessZ( 0x%08X, %i, %i );\n",
			branchdl, vtx, zval );
#endif
}

void gSPBranchLessW( u32 branchdl, u32 vtx, f32 wval )
{
	u32 address = RSP_SegmentToPhysical( branchdl );

	if ((address + 8) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Specified display list at invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchLessW( 0x%08X, %i, %i );\n",
			branchdl, vtx, wval );
#endif
		return;
	}

	if (gSP.vertices[vtx].w < wval)
		RSP.PC[RSP.PCi] = address;

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchLessW( 0x%08X, %i, %i );\n",
			branchdl, vtx, wval );
#endif
}

void gSPDlistCount( u32 count, u32 v )
{
	u32 address = RSP_SegmentToPhysical( v );

	if ((address == 0) || ((address + 8) > RDRAMSize))
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to branch to display list at invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDlistCount( %i, 0x%08X );\n",
			count, v );
#endif
		return;
	}

	if (RSP.PCi >= (GBI.PCStackSize - 1))
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// ** DL stack overflow **\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDlistCount( %i, 0x%08X );\n",
			count, v );
#endif
		return;
	}

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDlistCount( %i, 0x%08X );\n",
		count, v );
#endif

	RSP.PCi++;								// go to the next PC in the stack
	RSP.PC[RSP.PCi] = address;				// jump to the address
	RSP.nextCmd = _SHIFTR( *(u32*)&RDRAM[address], 24, 8 );
	RSP.count = count + 1;
}

void gSPSetDMAOffsets( u32 mtxoffset, u32 vtxoffset )
{
	gSP.DMAOffsets.mtx = mtxoffset;
	gSP.DMAOffsets.vtx = vtxoffset;

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSetDMAOffsets( 0x%08X, 0x%08X );\n",
			mtxoffset, vtxoffset );
#endif
}

void gSPSetVertexColorBase( u32 base )
{
 	gSP.vertexColorBase = RSP_SegmentToPhysical( base );

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSetVertexColorBase( 0x%08X );\n",
			base );
#endif
}

void gSPSprite2DBase( u32 base )
{
}

void gSPCopyVertex( SPVertex *dest, SPVertex *src )
{
	dest->x = src->x;
	dest->y = src->y;
	dest->z = src->z;
	dest->w = src->w;
	dest->r = src->r;
	dest->g = src->g;
	dest->b = src->b;
	dest->a = src->a;
	dest->s = src->s;
	dest->t = src->t;
}

void gSPInterpolateVertex( SPVertex *dest, f32 percent, SPVertex *first, SPVertex *second )
{
	dest->x = first->x + percent * (second->x - first->x);
	dest->y = first->y + percent * (second->y - first->y);
	dest->z = first->z + percent * (second->z - first->z);
	dest->w = first->w + percent * (second->w - first->w);
	dest->r = first->r + percent * (second->r - first->r);
	dest->g = first->g + percent * (second->g - first->g);
	dest->b = first->b + percent * (second->b - first->b);
	dest->a = first->a + percent * (second->a - first->a);
	dest->s = first->s + percent * (second->s - first->s);
	dest->t = first->t + percent * (second->t - first->t);
}

void gSPTriangle( s32 v0, s32 v1, s32 v2 )
{
	if ((v0 < SP_VERTEX_COUNT) && (v1 < SP_VERTEX_COUNT) && (v2 < SP_VERTEX_COUNT))
	{
		OGL_AddTriangle( gSP.vertices, v0, v1, v2 );
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_TRIANGLE, "// Vertex index out of range\n" );
#endif

	if (depthBuffer.current) depthBuffer.current->cleared = FALSE;
	gDP.colorImage.changed = TRUE;
	gDP.colorImage.height = (unsigned long)(max( gDP.colorImage.height, gDP.scissor.lry ));
}

void gSP1Triangle( s32 v0, s32 v1, s32 v2 )
{
	gSPTriangle( v0, v1, v2 );

	gSPFlushTriangles();

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP1Triangle( %i, %i, %i, %i );\n",
		v0, v1, v2, flag );
#endif
}

void gSP2Triangles( s32 v00, s32 v01, s32 v02,
				    s32 v10, s32 v11, s32 v12 )
{
	gSPTriangle( v00, v01, v02 );
	gSPTriangle( v10, v11, v12 );

	gSPFlushTriangles();

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP2Triangles( %i, %i, %i, %i,\n",
		v00, v01, v02, flag0 );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "               %i, %i, %i, %i );\n",
		v10, v11, v12, flag1 );
#endif
}

void gSP4Triangles( s32 v00, s32 v01, s32 v02,
				    s32 v10, s32 v11, s32 v12,
					s32 v20, s32 v21, s32 v22,
					s32 v30, s32 v31, s32 v32 )
{
	gSPTriangle( v00, v01, v02 );
	gSPTriangle( v10, v11, v12 );
	gSPTriangle( v20, v21, v22 );
	gSPTriangle( v30, v31, v32 );

	gSPFlushTriangles();

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP4Triangles( %i, %i, %i,\n",
		v00, v01, v02 );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "               %i, %i, %i,\n",
		v10, v11, v12 );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "               %i, %i, %i,\n",
		v20, v21, v22 );
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "               %i, %i, %i );\n",
		v30, v31, v32 );
#endif
}

void gSPDMATriangles( u32 tris, u32 n )
{
	u32 address = RSP_SegmentToPhysical( tris );

	if (address + sizeof( DKRTriangle ) * n > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_TRIANGLE, "// Attempting to load triangles from invalid address\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSPDMATriangles( 0x%08X, %i );\n" );
#endif
		return;
	}

	DKRTriangle *triangles = (DKRTriangle*)&RDRAM[address];

	for (u32 i = 0; i < n; i++)
	{
		gSP.geometryMode &= ~G_CULL_BOTH;

		if (!(triangles->flag & 0x40))
		{
			if (gSP.viewport.vscale[0] > 0)
				gSP.geometryMode |= G_CULL_BACK;
			else
				gSP.geometryMode |= G_CULL_FRONT;
		}
		gSP.changed |= CHANGED_GEOMETRYMODE;
		
		gSP.vertices[triangles->v0].s = _FIXED2FLOAT( triangles->s0, 5 );
		gSP.vertices[triangles->v0].t = _FIXED2FLOAT( triangles->t0, 5 );

		gSP.vertices[triangles->v1].s = _FIXED2FLOAT( triangles->s1, 5 );
		gSP.vertices[triangles->v1].t = _FIXED2FLOAT( triangles->t1, 5 );

		gSP.vertices[triangles->v2].s = _FIXED2FLOAT( triangles->s2, 5 );
		gSP.vertices[triangles->v2].t = _FIXED2FLOAT( triangles->t2, 5 );

		gSPTriangle( triangles->v0, triangles->v1, triangles->v2 );

		triangles++;
	}

	gSPFlushTriangles();

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSPDMATriangles( 0x%08X, %i );\n",
			tris, n );
#endif
}

void gSP1Quadrangle( s32 v0, s32 v1, s32 v2, s32 v3 )
{
	gSPTriangle( v0, v1, v2 );
	gSPTriangle( v0, v2, v3 );

	gSPFlushTriangles();

#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP1Quadrangle( %i, %i, %i, %i );\n",
			v0, v1, v2, v3 );
#endif
}

void gSPPopMatrixN( u32 param, u32 num )
{
	if (gSP.matrix.modelViewi > num - 1)
	{
		gSP.matrix.modelViewi -= num;

		gSP.changed |= CHANGED_MATRIX;
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to pop matrix stack below 0\n" );

	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPPopMatrixN( %s, %i );\n",
		(param == G_MTX_MODELVIEW) ? "G_MTX_MODELVIEW" : 
	    (param == G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_INVALID",
		num );
#endif
}

void gSPPopMatrix( u32 param )
{
	if (gSP.matrix.modelViewi > 0)
	{
		gSP.matrix.modelViewi--;

		gSP.changed |= CHANGED_MATRIX;
	}
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to pop matrix stack below 0\n" );

	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPPopMatrix( %s );\n",
		(param == G_MTX_MODELVIEW) ? "G_MTX_MODELVIEW" : 
	    (param == G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_INVALID" );
#endif
}

void gSPSegment( s32 seg, s32 base )
{
	if (seg > 0xF)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load address into invalid segment\n",
			SegmentText[seg], base );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSegment( %s, 0x%08X );\n",
			SegmentText[seg], base );
#endif
		return;
	}
	gSP.segment[seg] = base;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSegment( %s, 0x%08X );\n",
		SegmentText[seg], base );
#endif
}

void gSPClipRatio( u32 r )
{
}

void gSPInsertMatrix( u32 where, u32 num )
{
	f32 fraction, integer;

	if (gSP.changed & CHANGED_MATRIX)
		_gSPCombineMatrices();

	if ((where & 0x3) || (where > 0x3C))
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Invalid matrix elements\n" );
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPInsertMatrix( 0x%02X, %i );\n",
			where, num );
#endif
		return;
	}

	if (where < 0x20)
	{
		fraction = modff( gSP.matrix.combined[0][where >> 1], &integer );
		gSP.matrix.combined[0][where >> 1] = (s16)_SHIFTR( num, 16, 16 ) + abs( (int)fraction );

		fraction = modff( gSP.matrix.combined[0][(where >> 1) + 1], &integer );
		gSP.matrix.combined[0][(where >> 1) + 1] = (s16)_SHIFTR( num, 0, 16 ) + abs( (int)fraction );
	}
	else
	{
		f32 newValue;

		fraction = modff( gSP.matrix.combined[0][(where - 0x20) >> 1], &integer );
		newValue = integer + GXcastu16f32( _SHIFTR( num, 16, 16 ) );

		// Make sure the sign isn't lost
		if ((integer == 0.0f) && (fraction != 0.0f))
			newValue = newValue * (fraction / abs( (int)fraction ));

		gSP.matrix.combined[0][(where - 0x20) >> 1] = newValue;

		fraction = modff( gSP.matrix.combined[0][((where - 0x20) >> 1) + 1], &integer );
		newValue = integer + GXcastu16f32( _SHIFTR( num, 0, 16 ) );

		// Make sure the sign isn't lost
		if ((integer == 0.0f) && (fraction != 0.0f))
			newValue = newValue * (fraction / abs( (int)fraction ));

		gSP.matrix.combined[0][((where - 0x20) >> 1) + 1] = newValue;
	}

#ifdef __GX__
#ifdef SHOW_DEBUG
	sprintf(txtbuffer,"gSP: gSPInsertMatrix");
	DEBUG_print(txtbuffer,6);
#endif

	_gSPUpdateCombW();
#endif //__GX__

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPInsertMatrix( %s, %i );\n",
		MWOMatrixText[where >> 2], num );
#endif
}

void gSPModifyVertex( u32 vtx, u32 where, u32 val )
{
	switch (where)
	{
		case G_MWO_POINT_RGBA:
			gSP.vertices[vtx].r = GXcastu8f32( _SHIFTR( val, 24, 8 ) );
			gSP.vertices[vtx].g = GXcastu8f32( _SHIFTR( val, 16, 8 ) );
			gSP.vertices[vtx].b = GXcastu8f32( _SHIFTR( val, 8, 8 ) );
			gSP.vertices[vtx].a = GXcastu8f32( _SHIFTR( val, 0, 8 ) );
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
				vtx, MWOPointText[(where - 0x10) >> 2], val );
#endif
			break;
		case G_MWO_POINT_ST:
			gSP.vertices[vtx].s = _FIXED2FLOAT( (s16)_SHIFTR( val, 16, 16 ), 5 ) / gSP.texture.scales;
			gSP.vertices[vtx].t = _FIXED2FLOAT( (s16)_SHIFTR( val, 0, 16 ), 5 ) / gSP.texture.scalet;
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
				vtx, MWOPointText[(where - 0x10) >> 2], val );
#endif
			break;
		case G_MWO_POINT_XYSCREEN:
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
				vtx, MWOPointText[(where - 0x10) >> 2], val );
#endif
			break;
		case G_MWO_POINT_ZSCREEN:
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
				vtx, MWOPointText[(where - 0x10) >> 2], val );
#endif
			break;
	}
}

void gSPNumLights( s32 n )
{
	if (n <= 8)
		gSP.numLights = n;
#ifdef DEBUG
	else
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Setting an invalid number of lights\n" );
#endif

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPNumLights( %i );\n",
		n );
#endif
}

void gSPLightColor( u32 lightNum, u32 packedColor )
{
	lightNum--;

	if (lightNum < 8)
	{
		gSP.lights[lightNum].r = GXcastu8f32( _SHIFTR( packedColor, 24, 8 ) );
		gSP.lights[lightNum].g = GXcastu8f32( _SHIFTR( packedColor, 16, 8 ) );
		gSP.lights[lightNum].b = GXcastu8f32( _SHIFTR( packedColor, 8, 8 ) );
	}
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLightColor( %i, 0x%08X );\n",
		lightNum, packedColor );
#endif
}

void gSPFogFactor( s16 fm, s16 fo )
{
    gSP.fog.multiplier = fm;
	gSP.fog.offset = fo;

#ifdef __GX__
	if (gSP.fog.multiplier == 0)
	{
		OGL.GXfogStartZ = 0.0f;
		OGL.GXfogEndZ = 0.0f;
	}
	else
	{
		OGL.GXfogStartZ = -((float)gSP.fog.offset / (float)gSP.fog.multiplier);
		OGL.GXfogEndZ = (256.0f - (float)gSP.fog.offset) / (float)gSP.fog.multiplier;
	}
#endif // __GX__

	gSP.changed |= CHANGED_FOGPOSITION;
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPFogFactor( %i, %i );\n", fm, fo );
#endif
}

void gSPPerspNormalize( u16 scale )
{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPPerspNormalize( %i );\n", scale );
#endif
}


static u32 _gSPDetailBaseTile( u32 tile )
{
	if ((gDP.otherMode.textureLOD == G_TL_LOD) &&
	    (gDP.otherMode.textureDetail == G_TD_DETAIL))
		return (tile + 1) & 7;

	return tile;
}

void gSPUpdateTextureTiles()
{
	const u32 base = _gSPDetailBaseTile( gSP.texture.tile );

	gSP.textureTile[0] = &gDP.tiles[base];
	gSP.textureTile[1] = needReplaceTex1ByTex0() ? &gDP.tiles[base]
	                                             : &gDP.tiles[(base + 1) & 7];
}

// From GLideN64 commit c28ea61b, "Fix for texture issues in Stunt Racer"
bool needReplaceTex1ByTex0()
{
	return (gDP.otherMode.textureLOD == G_TL_LOD) &&
	       (gDP.otherMode.textureDetail == G_TD_CLAMP) &&
	       (gSP.texture.level == 0);
}

void gSPTexture( f32 sc, f32 tc, s32 level, s32 tile, s32 on )
{
	gSP.texture.scales = sc;
	gSP.texture.scalet = tc;

	if (gSP.texture.scales == 0.0f) gSP.texture.scales = 1.0f;
	if (gSP.texture.scalet == 0.0f) gSP.texture.scalet = 1.0f;

	gSP.texture.level = level;
	gSP.texture.on = on;

	gSP.texture.tile = tile;
	gSPUpdateTextureTiles();

	gSP.changed |= CHANGED_TEXTURE;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gSPTexture( %f, %f, %i, %i, %i );\n",
		sc, tc, level, tile, on );
#endif
}

void gSPEndDisplayList()
{
	if (RSP.PCi > 0)
		RSP.PCi--;
	else
	{
#ifdef DEBUG
		DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// End of display list, halting execution\n" );
#endif
		RSP.halt = TRUE;
	}

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPEndDisplayList();\n\n" );
#endif
}

void gSPGeometryMode( u32 clear, u32 set )
{
	gSP.geometryMode = (gSP.geometryMode & ~clear) | set;

	gSP.changed |= CHANGED_GEOMETRYMODE;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPGeometryMode( %s%s%s%s%s%s%s%s%s%s, %s%s%s%s%s%s%s%s%s%s );\n",
		clear & G_SHADE ? "G_SHADE | " : "",
		clear & G_LIGHTING ? "G_LIGHTING | " : "",
		clear & G_SHADING_SMOOTH ? "G_SHADING_SMOOTH | " : "",
		clear & G_ZBUFFER ? "G_ZBUFFER | " : "",
		clear & G_TEXTURE_GEN ? "G_TEXTURE_GEN | " : "",
		clear & G_TEXTURE_GEN_LINEAR ? "G_TEXTURE_GEN_LINEAR | " : "",
		clear & G_CULL_FRONT ? "G_CULL_FRONT | " : "",
		clear & G_CULL_BACK ? "G_CULL_BACK | " : "",
		clear & G_FOG ? "G_FOG | " : "",
		clear & G_CLIPPING ? "G_CLIPPING" : "",
		set & G_SHADE ? "G_SHADE | " : "",
		set & G_LIGHTING ? "G_LIGHTING | " : "",
		set & G_SHADING_SMOOTH ? "G_SHADING_SMOOTH | " : "",
		set & G_ZBUFFER ? "G_ZBUFFER | " : "",
		set & G_TEXTURE_GEN ? "G_TEXTURE_GEN | " : "",
		set & G_TEXTURE_GEN_LINEAR ? "G_TEXTURE_GEN_LINEAR | " : "",
		set & G_CULL_FRONT ? "G_CULL_FRONT | " : "",
		set & G_CULL_BACK ? "G_CULL_BACK | " : "",
		set & G_FOG ? "G_FOG | " : "",
		set & G_CLIPPING ? "G_CLIPPING" : "" );
#endif
}

void gSPSetGeometryMode( u32 mode )
{
	gSP.geometryMode |= mode;

	gSP.changed |= CHANGED_GEOMETRYMODE;
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSetGeometryMode( %s%s%s%s%s%s%s%s%s%s );\n",
		mode & G_SHADE ? "G_SHADE | " : "",
		mode & G_LIGHTING ? "G_LIGHTING | " : "",
		mode & G_SHADING_SMOOTH ? "G_SHADING_SMOOTH | " : "",
		mode & G_ZBUFFER ? "G_ZBUFFER | " : "",
		mode & G_TEXTURE_GEN ? "G_TEXTURE_GEN | " : "",
		mode & G_TEXTURE_GEN_LINEAR ? "G_TEXTURE_GEN_LINEAR | " : "",
		mode & G_CULL_FRONT ? "G_CULL_FRONT | " : "",
		mode & G_CULL_BACK ? "G_CULL_BACK | " : "",
		mode & G_FOG ? "G_FOG | " : "",
		mode & G_CLIPPING ? "G_CLIPPING" : "" );
#endif
}

void gSPClearGeometryMode( u32 mode )
{
	gSP.geometryMode &= ~mode;

	gSP.changed |= CHANGED_GEOMETRYMODE;

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPClearGeometryMode( %s%s%s%s%s%s%s%s%s%s );\n",
		mode & G_SHADE ? "G_SHADE | " : "",
		mode & G_LIGHTING ? "G_LIGHTING | " : "",
		mode & G_SHADING_SMOOTH ? "G_SHADING_SMOOTH | " : "",
		mode & G_ZBUFFER ? "G_ZBUFFER | " : "",
		mode & G_TEXTURE_GEN ? "G_TEXTURE_GEN | " : "",
		mode & G_TEXTURE_GEN_LINEAR ? "G_TEXTURE_GEN_LINEAR | " : "",
		mode & G_CULL_FRONT ? "G_CULL_FRONT | " : "",
		mode & G_CULL_BACK ? "G_CULL_BACK | " : "",
		mode & G_FOG ? "G_FOG | " : "",
		mode & G_CLIPPING ? "G_CLIPPING" : "" );
#endif
}

void gSPLine3D( s32 v0, s32 v1, s32 flag )
{
	OGL_DrawLine( gSP.vertices, v0, v1, 1.5f );

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPLine3D( %i, %i, %i );\n", v0, v1, flag );
#endif
}

void gSPLineW3D( s32 v0, s32 v1, s32 wd, s32 flag )
{
	OGL_DrawLine( gSP.vertices, v0, v1, 1.5f + wd * 0.5f );
#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPLineW3D( %i, %i, %i, %i );\n", v0, v1, wd, flag );
#endif
}

// Stripped background rendering, ported from GLideN64 (S2DEX.cpp).
static void _s2dexRunCommand( u32 w0, u32 w1 )
{
	GBI.cmd[_SHIFTR( w0, 24, 8 )]( w0, w1 );
}

static void BgRect1CycStripped( u32 bgAddr )
{
	uObjScaleBg objBg = *(const uObjScaleBg*)&RDRAM[bgAddr];
	const u32 imagePtr = RSP_SegmentToPhysical( objBg.imagePtr );

	gDP.otherMode.cycleType = G_CYC_1CYCLE;
	gDP.changed |= CHANGED_CYCLETYPE;

	s32 E2_1;
	u16 F1_1;
	u16 P;
	s16 H2;

	// Part 1
	{
		// Step 1 & 2
		s16 Aw = objBg.frameW - ((((objBg.imageW << 10) / objBg.scaleW) - 1) & 0xFFFC);
		if (Aw < 0)
			Aw = 0;
		if ((objBg.imageFlip & G_BG_FLAG_FLIPS) != 0)
			objBg.frameX += Aw;

		s16 Bw = max( 0, gDP.scissor.xh - objBg.frameX );
		s16 Cw = max( 0, objBg.frameX + objBg.frameW - gDP.scissor.xl - Aw );
		if ((s16)objBg.frameW - Aw - Bw - Cw <= 0)
			return;

		s16 Dw = objBg.frameX + Bw;
		s16 Ew = objBg.frameX + objBg.frameW - Aw - Cw;

		s16 Ah = objBg.frameH - ((((objBg.imageH << 10) / objBg.scaleH) - 1) & 0xFFFC);
		if (Ah < 0)
			Ah = 0;

		s16 Bh = max( 0, gDP.scissor.yh - objBg.frameY );
		s16 Ch = max( 0, objBg.frameY + objBg.frameH - gDP.scissor.yl - Ah );
		if ((s16)objBg.frameH - Ah - Bh - Ch <= 0)
			return;

		s16 Dh = ((objBg.frameY + Bh) * 0x4000) >> 16;
		s16 Eh = ((objBg.frameH - Ah - Bh - Ch) * 0x4000) >> 16;

		*(u32*)(DMEM + 0x548) = (Dw << 16) | Ew;
		*(u32*)(DMEM + 0x54C) = (Dh << 16) | Eh;

		// Step 3
		u16 Fw = objBg.imageW << 3;
		if ((objBg.imageFlip & G_BG_FLAG_FLIPS) != 0)
			Bw = Cw;

		s16 Gw = ((objBg.scaleW * Bw * 0x0200) >> 16) + objBg.imageX;
		s32 Hw = Gw - Fw;
		u16 Fh = objBg.imageH << 3;
		s16 Gh = ((objBg.scaleH * Bh * 0x0200) >> 16) + objBg.imageY;

		while (Hw >= 0)
		{
			Gw -= Fw;
			Gh += 0x20;
			objBg.imageYorig += 0x20;
			Hw = Gw - Fw;
		}

		s32 Hh = Gh - Fh;
		while (Hh >= 0)
		{
			Gh -= Fh;
			objBg.imageYorig -= Fh;
			Hh = Gh - Fh;
		}

		s32 I = ((s32)Gh - objBg.imageYorig) << 5;
		s16 J = (objBg.scaleW * (objBg.frameW - Aw - Bw - Cw)) >> 7;
		u8 J_2 = 1;
		if (J + Gw + 0x0B < Fw)
			J_2 = 0;
		u8 K = (gSP.objRendermode & 0x08) >> 3;

		// Step 4
		static const u32 aSize[] = {
			0x01FF0080, 0x00FF0100, 0x007F0200, 0x003F0400
		};
		static const u32 aFormat[] = {
			0x04000400, 0x02000400, 0x04000000
		};
		const u16 *aFormat16 = (const u16*)aFormat;

		u16 L = aFormat16[objBg.imageFmt];
		u32 M = aSize[objBg.imageSiz];
		u16 N = ((objBg.frameW * objBg.scaleW) >> 7) + (K << 5);
		u16 O = N;
		if (N >= Fw)
			O = Fw;
		P = (((O + (M >> 16)) * (M & 0xFFFF)) >> 16) + 1;

		*(u32*)(DMEM + 0x550) = (K << 24) | (J_2 << 16) | P;
		*(s32*)(DMEM + 0x554) = I;

		// Step 5
		u16 Q = L / (P * 2) + K * 0xFFFF;
		s32 R = (0x100000 * Q) / objBg.scaleH;

		*(u32*)(DMEM + 0x558) = R;

		// Step 6
		s32 S = (((s64)I * 0x4000000 / objBg.scaleH) >> 16) & 0xFFFFFC00;
		s16 T = (s16)(((0xFFFFFFFF / R) * (s64)S) >> 0x20);
		s32 U = R * (T + 1);
		s16 V = T;
		if (U <= S)
			V += 1;
		s32 W = R * V;
		s32 Z = R - S + W;

		*(u32*)(DMEM + 0x55C) = Z;
		*(s32*)(DMEM + 0x560) = objBg.imageYorig;

		// Step 7
		u32 A1 = S - (W & 0xFFFFFC00);
		u32 B1 = (A1 * 0x0040) >> 16;
		u32 C1 = B1 * objBg.scaleH;
		u16 D1 = (u16)(((C1 * 0x0040) >> 16) & 0x0000FFFF);
		u16 E1 = Q - D1;
		u16 F1 = C1 & 0xFFFF;
		F1_1 = ((F1 << 11) >> 16) & 0x001F;

		// Step 8
		s16 A2 = (s16)(((u32)Q * (u32)V + (u32)D1 + ((objBg.imageYorig << 11) >> 16)) & 0xFFFF);
		u16 B2 = (objBg.imageH << 14) >> 16;
		s16 A2_1 = (A2 >= 0) ? A2 : A2 + B2;
		if (A2 - B2 >= 0)
			A2_1 -= B2;

		s16 C2 = (s16)((((s32)Gw * (M & 0xFFFF)) >> 16) << 3);
		s16 D2 = (s16)((((s32)Fw * (M & 0xFFFF)) >> 16) << 3);
		s32 E2 = A2_1 * D2 + C2;
		E2_1 = E2 + imagePtr;
		s32 F2 = E1 * D2;
		s32 G2 = Q * D2;

		H2 = Gw & (M >> 16);
		if ((objBg.imageFlip & G_BG_FLAG_FLIPS) != 0)
			H2 = (H2 + J) * 0xFFFF;

		u32 I2 = 0xFD100000 | ((D2 >> 1) - 1);
		u32 J2 = 0xF5100000 | (P << 9);
		u32 J2_1 = (J2 & 0xFF00FFFF) | (((objBg.imageFmt << 5) | (objBg.imageSiz << 3)) << 16);
		u32 K2 = (objBg.imagePal << 20) | 0x0007C1F0;
		u16 L2 = objBg.imageH >> 2;

		*(u32*)(DMEM + 0x564) = (C2 << 16) | D2;
		*(u32*)(DMEM + 0x568) = I2;
		*(u32*)(DMEM + 0x56C) = J2;
		*(u32*)(DMEM + 0x570) = (E1 << 16) | Q;
		*(u32*)(DMEM + 0x574) = F2;
		*(u32*)(DMEM + 0x578) = G2;
		*(u32*)(DMEM + 0x57C) = (L2 << 16) | A2_1;

		_s2dexRunCommand( J2, 0x27000000 );
		_s2dexRunCommand( J2_1, K2 );
		_s2dexRunCommand( (G_SETTILESIZE << 24), 0 );
	}

	// Part 2
	{
		u32 VV = *(u32*)(DMEM + 0x57C);
		s16 AA = _SHIFTR( VV, 16, 16 ) - _SHIFTR( VV, 0, 16 );
		VV = *(u32*)(DMEM + 0x570);
		s32 CC = VV >> 16;
		u32 DD = *(u32*)(DMEM + 0x55C);
		u32 EE = *(u32*)(DMEM + 0x558);
		VV = *(u32*)(DMEM + 0x54C);
		s32 FF = _SHIFTR( VV, 0, 16 );
		u16 JJ = _SHIFTR( VV, 16, 16 );
		u32 GG = *(u32*)(DMEM + 0x574);
		VV = *(u32*)(DMEM + 0x548);
		u32 HH = _SHIFTR( VV, 16, 16 ) << 0x0C;
		u32 II = _SHIFTR( VV, 0, 16 ) << 0x0C;

		u32 step = 2;
		s32 KK = 0;
		s16 LL = 0, MM = 0, NN = 0, RR = 0, AAA = 0;
		u32 SS = 0;
		BOOL stop = FALSE;

		while (!stop)
		{
			switch (step)
			{
			case 2:
				KK = DD >> 10;
				step = (KK > 0) ? 5 : 3;
				break;

			case 3:
				AA -= CC;
				if (AA > 0)
					E2_1 += GG;
				else
				{
					VV = *(u32*)(DMEM + 0x564);
					E2_1 = imagePtr + _SHIFTR( VV, 16, 16 ) + _SHIFTR( VV, 0, 16 ) * (-AA);
					VV = *(u32*)(DMEM + 0x57C);
					AA += _SHIFTR( VV, 16, 16 );
				}
				step = 4;
				break;

			case 4:
				DD += EE;
				VV = *(u32*)(DMEM + 0x570);
				CC = (s32)_SHIFTR( VV, 0, 16 );
				GG = *(u32*)(DMEM + 0x578);
				F1_1 = 0;
				step = 2;
				break;

			case 5:
				FF -= KK;
				DD &= 0x03FF;
				if (FF < 0)
				{
					CC += ((objBg.scaleH * FF) >> 10) + 1;
					KK += FF;
					VV = *(u32*)(DMEM + 0x570);
					if (CC - (s32)_SHIFTR( VV, 0, 16 ) > 0)
						CC = (s32)_SHIFTR( VV, 0, 16 );
				}
				step = 6;
				break;

			case 6:
				LL = JJ + KK;
				VV = *(u32*)(DMEM + 0x550);
				P = _SHIFTR( VV, 0, 16 );
				MM = CC + _SHIFTR( VV, 24, 8 );
				NN = AA - _SHIFTR( VV, 16, 8 );
				step = (NN - MM < 0) ? 7 : 77;
				break;

			case 7:
				RR = MM - AA;
				AAA = AA;
				if (RR > 0)
				{
					VV = *(u32*)(DMEM + 0x564);
					SS = imagePtr + _SHIFTR( VV, 16, 16 );
					if ((AAA & 1) != 0)
					{
						AAA--;
						RR++;
						SS -= _SHIFTR( VV, 0, 16 );
					}
					AAA *= P;
					_s2dexRunCommand( (*(u32*)(DMEM + 0x56C) | AAA), 0x27000000 );
					_s2dexRunCommand( (*(u32*)(DMEM + 0x568)), SS );
					_s2dexRunCommand( 0xF4000000, (((P + 0x6FF) << 16) | ((RR << 2) - 1)) );
				}
				step = 8;
				break;

			case 77:
				RR = MM;
				SS = E2_1;
				_s2dexRunCommand( *(u32*)(DMEM + 0x568), SS );
				_s2dexRunCommand( 0xF4000000, (((P + 0x6FF) << 16) | ((RR << 2) - 1)) );
				AA -= CC;
				E2_1 += GG;
				step = 11;
				break;

			case 8:
				VV = *(u32*)(DMEM + 0x550);
				if (_SHIFTR( VV, 16, 8 ) != 0)
				{
					SS = imagePtr;
					s16 BBB = NN;
					RR = NN & 1;
					VV = *(u32*)(DMEM + 0x564);
					if (RR != 0)
					{
						BBB--;
						SS -= _SHIFTR( VV, 0, 16 );
					}
					u32 CCC = E2_1 + BBB * _SHIFTR( VV, 0, 16 );
					RR++;
					u16 DDD = ((_SHIFTR( VV, 0, 16 ) - _SHIFTR( VV, 16, 16 )) * 0x2000) >> 16;
					u32 ZZZ = BBB * P;
					P -= DDD;
					AAA = ZZZ + DDD;
					_s2dexRunCommand( (*(u32*)(DMEM + 0x56C) | AAA), 0x27000000 );
					_s2dexRunCommand( (*(u32*)(DMEM + 0x568)), SS );
					_s2dexRunCommand( 0xF4000000, (((P + 0x6FF) << 16) | ((RR << 2) - 1)) );

					SS = CCC;
					AAA = ZZZ;
					P = DDD;
					_s2dexRunCommand( (*(u32*)(DMEM + 0x56C) | AAA), 0x27000000 );
					_s2dexRunCommand( (*(u32*)(DMEM + 0x568)), SS );
					_s2dexRunCommand( 0xF4000000, (((P + 0x6FF) << 16) | ((RR << 2) - 1)) );
				}
				step = 9;
				break;

			case 9:
				AA -= CC;
				if (NN <= 0)
					_s2dexRunCommand( *(u32*)(DMEM + 0x56C), 0x27000000 );
				else
				{
					VV = *(u32*)(DMEM + 0x550);
					P = _SHIFTR( VV, 0, 16 );
					SS = E2_1;
					RR = NN;
					AAA = 0;
					_s2dexRunCommand( (*(u32*)(DMEM + 0x56C) | AAA), 0x27000000 );
					_s2dexRunCommand( (*(u32*)(DMEM + 0x568)), SS );
					_s2dexRunCommand( 0xF4000000, (((P + 0x6FF) << 16) | ((RR << 2) - 1)) );
				}
				step = 10;
				break;

			case 10:
				if (AA > 0)
					E2_1 += GG;
				else
				{
					VV = *(u32*)(DMEM + 0x564);
					E2_1 = imagePtr + _SHIFTR( VV, 16, 16 ) + _SHIFTR( VV, 0, 16 ) * (-AA);
					VV = *(u32*)(DMEM + 0x57C);
					AA += _SHIFTR( VV, 16, 16 );
				}
				step = 11;
				break;

			case 11:
				{
					const u32 w0 = (G_TEXRECT << 24) | (LL << 2) | II;
					const u32 w1 = (JJ << 2) | HH;

					RDP_SetTexRectParams( (H2 << 16) | F1_1,
					                      (objBg.scaleW << 16) | objBg.scaleH );
					RDP_TexRect( w0, w1 );
				}

				if (FF <= 0)
					stop = TRUE;
				else
				{
					JJ = LL;
					DD = DD + EE;
					VV = *(u32*)(DMEM + 0x570);
					CC = _SHIFTR( VV, 0, 16 );
					GG = *(u32*)(DMEM + 0x578);
					F1_1 = 0;
					step = 2;
				}
				break;
			}
		}
	}
}

static void BgRectCopyStripped( u32 bgAddr )
{
	// Step 1
	uObjBg objBg = *(const uObjBg*)&RDRAM[bgAddr];
	const u32 imagePtr = RSP_SegmentToPhysical( objBg.imagePtr );

	gDP.otherMode.cycleType = G_CYC_COPY;
	gDP.changed |= CHANGED_CYCLETYPE;

	// Step 2
	s16 Aw = max( 0, objBg.frameX + objBg.frameW - gDP.scissor.xl );
	s16 Bw = min( 0, objBg.frameX - gDP.scissor.xh );
	s16 Cw = objBg.frameW + Bw - Aw;
	if (Cw <= 0)
		return;

	s16 Dw = (((objBg.imageX * 0x2000) >> 16) & 0xFFFC) - Bw;
	s16 Ew = objBg.frameX - Bw;

	s16 Ah = max( 0, objBg.frameY + objBg.frameH - gDP.scissor.yl );
	s16 Bh = min( 0, objBg.frameY - gDP.scissor.yh );
	s16 Ch = objBg.frameH + Bh - Ah;
	if (Ch <= 0)
		return;

	s16 Dh = (((objBg.imageY * 0x2000) >> 16) & 0xFFFC) - Bh;
	s16 Eh = objBg.frameY - Bh;

	s16 F = Dh - objBg.imageH;
	s16 G = (F >= 0) ? F : Dh;
	s16 H = (objBg.imageFlip != 0) ? Dw + Aw : Dw;

	// Step 3
	u32 I = (objBg.imageLoad == G_BGLT_LOADTILE) ? 0xFFFFFFFF : 0U;
	u32 J = objBg.tmemW << 9;
	u32 K = (G_SETTILE << 24) | 0x100000 | (J & I);
	u32 L = (objBg.imageFmt << 2) | objBg.imageSiz;
	L = (L << 0x13) | J;
	u32 M = (objBg.imagePal << 0x14) | 0x0007C1F0;

	_s2dexRunCommand( K, 0x27000000 );
	_s2dexRunCommand( (G_SETTILESIZE << 24), 0 );
	_s2dexRunCommand( ((G_SETTILE << 24) | L), M );

	// Step 4
	static const u32 aSize[] = {
		0x003F0800, 0x10000080, 0x001F1000, 0x20000100,
		0x000F2000, 0x40000200, 0x00074000, 0x80000400
	};
	const u16 *aSize16 = (const u16*)aSize;
	const u16 imageSzIdx = objBg.imageSiz << 2;

	u16 N0 = aSize16[0 + imageSzIdx];
	u16 N1 = aSize16[1 + imageSzIdx];
	u16 N2 = aSize16[2 + imageSzIdx];

	G = (G * 0x4000) >> 16;

	u16 O = (N0 & H) + Cw;
	u32 P = ((N1 * H) >> 16) + objBg.tmemSizeW * G;
	u16 Q = (N2 * O) >> 16;
	u32 R = (objBg.imageFlip != 0) ? (((1 - O) * 8) << 16) : (((N0 & H) * 8) << 16);
	u32 S = ((P >> 1) << 3) + imagePtr;
	u16 T = Ew + Cw - 1;

	u16 A1 = objBg.imageH & 0xFFFC;
	u16 A2 = G << 2;
	u16 A3 = (N1 * H) >> 16;
	u32 T0 = Ew << 0x0C;
	u16 T1 = Eh;
	u32 T2 = T << 0x0C;
	s16 AT = Ch;
	s16 U = A1 - A2;

	u32 V = 0, X = 0, Y = 0, Z = 0, AA = 0, w0 = 0, w1 = 0;
	u16 S5 = 0, BB = 0;
	u32 step = 4;
	BOOL stop = FALSE;

	while (!stop)
	{
		switch (step)
		{
		case 4:
			if (U <= 0)
				stop = TRUE;
			step = 5;
			break;

		case 5:
			if (A3 > 0)
				U -= 4;
			if (U > AT)
				U = AT;

			V = 0xE4000000 | T2;
			if (S2DEX_GetVersion() == S2DEX_VER_1_7)
				X = (objBg.imageLoad == G_BGLT_LOADTILE) ? (Q << 2) - 1 : objBg.tmemLoadSH;
			else
				X = (objBg.imageLoad == G_BGLT_LOADTILE) ? (Q << 2) : objBg.tmemLoadSH;
			X = (X | 0x7000) << 0x0C;
			Y = 0xFD100000 | ((objBg.tmemSizeW << 1) - 1);
			AT -= U;
			step = (U <= 0) ? 8 : 55;
			break;

		case 55:
			if (S2DEX_GetVersion() == S2DEX_VER_1_7)
				Z = (objBg.imageLoad == G_BGLT_LOADTILE) ? (objBg.tmemSize << 0x10) | objBg.tmemLoadSH : objBg.tmemSize;
			else
				Z = objBg.tmemSize;
			S5 = objBg.tmemH;
			AA = X | objBg.tmemLoadTH;
			step = 6;
			break;

		case 6:
			U -= S5;
			if (U < 0)
			{
				Z += objBg.tmemSizeW * U;
				S5 += U;
				AA = (objBg.imageLoad == G_BGLT_LOADTILE)
				   ? X | (S5 - 1)
				   : (((Z - 2) | 0xE000) << 0x0B) | objBg.tmemLoadTH;
			}
			step = 7;
			break;

		case 7:
			BB = T1 + S5 - 1;
			_s2dexRunCommand( Y, S );
			if (objBg.imageLoad == G_BGLT_LOADTILE)
				_s2dexRunCommand( (G_LOADTILE << 24), AA );
			else
				_s2dexRunCommand( (G_LOADBLOCK << 24), AA );

			w0 = V | BB;
			w1 = T0 | T1;
			RDP_SetTexRectParams( R, 0x10000400 );
			RDP_TexRect( w0, w1 );

			T1 = BB + 1;
			S += Z;
			if (U > 0)
				step = 6;
			else
			{
				if (AT <= 0)
					stop = TRUE;
				step = 8;
			}
			break;

		case 8:
			if (A3 > 0)
			{
				A3 >>= 1;
				_s2dexRunCommand( Y, S );
				_s2dexRunCommand( ((G_SETTILE << 24) | 0x35100000), 0x06000000 );
				_s2dexRunCommand( (G_LOADBLOCK << 24), (0x06000000 | (((((objBg.tmemSizeW >> 1) - A3) << 2) - 1) << 12)) );
				_s2dexRunCommand( Y, imagePtr );
				_s2dexRunCommand( ((G_SETTILE << 24) | 0x35100000 | ((objBg.tmemSizeW >> 1) - A3)), 0x06000000 );
				_s2dexRunCommand( (G_LOADBLOCK << 24), (0x06000000 | (((A3 << 2) - 1) << 12)) );

				w0 = V | T1;
				w1 = T0 | T1;
				RDP_SetTexRectParams( R, 0x10000400 );
				RDP_TexRect( w0, w1 );

				T1 += 4;
				AT -= 4;
				if (AT <= 0)
					stop = TRUE;
			}
			step = 9;
			break;

		case 9:
			S = imagePtr + (A3 << 3);
			U = AT;
			AT = 0;
			step = 55;
			break;
		}
	}
}

// Upstream's equivalent is config.graphics2D.bgMode == bgOnePiece for this:
//#define S2DEX_FORCE_ONE_PIECE_BG 1

// GLideN64 _useOnePieceBgCode()
static BOOL _useOnePieceBgCode( u32 address )
{
#ifdef S2DEX_FORCE_ONE_PIECE_BG
	return TRUE;
#else
	if (!OGL.frameBufferTextures)
		return FALSE;

	const uObjScaleBg *objBg = (const uObjScaleBg*)&RDRAM[address];
	const FrameBuffer *buffer = FrameBuffer_FindBuffer( RSP_SegmentToPhysical( objBg->imagePtr ) );

	return (buffer != NULL && buffer->size == objBg->imageSiz) ? TRUE : FALSE;
#endif
}

void gSPBgRect1Cyc( u32 bg )
{
	u32 address = RSP_SegmentToPhysical( bg );

	if (!_useOnePieceBgCode( address ))
	{
		BgRect1CycStripped( address );
		return;
	}

	uObjScaleBg *objScaleBg = (uObjScaleBg*)&RDRAM[address];

	gSP.bgImage.address = RSP_SegmentToPhysical( objScaleBg->imagePtr );
	gSP.bgImage.width = objScaleBg->imageW >> 2;
	gSP.bgImage.height = objScaleBg->imageH >> 2;
	gSP.bgImage.format = objScaleBg->imageFmt;
	gSP.bgImage.size = objScaleBg->imageSiz;
	gSP.bgImage.palette = objScaleBg->imagePal;
	gDP.textureMode = TEXTUREMODE_BGIMAGE;


	f32 imageX = _FIXED2FLOAT( objScaleBg->imageX, 5 );
	f32 imageY = _FIXED2FLOAT( objScaleBg->imageY, 5 );
	f32 imageW = objScaleBg->imageW >> 2;
	f32 imageH = objScaleBg->imageH >> 2;

	f32 frameX = _FIXED2FLOAT( objScaleBg->frameX, 2 );
	f32 frameY = _FIXED2FLOAT( objScaleBg->frameY, 2 );
	f32 frameW = _FIXED2FLOAT( objScaleBg->frameW, 2 );
	f32 frameH = _FIXED2FLOAT( objScaleBg->frameH, 2 );
	// Malformed display lists can send a zero scale (GLideN64's
	// 9eddde6e/af637370 fixes).
	// Clamp before converting so this can't divide by zero
	f32 scaleW = _FIXED2FLOAT( max( objScaleBg->scaleW, (u16)1 ), 10 );
	f32 scaleH = _FIXED2FLOAT( max( objScaleBg->scaleH, (u16)1 ), 10 );

	f32 frameX0 = frameX;
	f32 frameY0 = frameY;

	f32 frameX1 = frameX + min( (imageW - imageX) / scaleW, frameW );
	f32 frameY1 = frameY + min( (imageH - imageY) / scaleH, frameH );

	if (frameX1 < frameX0) frameX1 = frameX0;
	if (frameY1 < frameY0) frameY1 = frameY0;
	if (frameX1 > frameX0 + frameW) frameX1 = frameX0 + frameW;
	if (frameY1 > frameY0 + frameH) frameY1 = frameY0 + frameH;

	gDP.otherMode.cycleType = G_CYC_1CYCLE;
	gDP.changed |= CHANGED_CYCLETYPE;
	gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

	const f32 spanX[2][2] = { { frameX0, frameX1 }, { frameX1, frameX0 + frameW } };
	const f32 spanY[2][2] = { { frameY0, frameY1 }, { frameY1, frameY0 + frameH } };
	const f32 srcX[2] = { imageX, 0.0f };
	const f32 srcY[2] = { imageY, 0.0f };

	for (int iy = 0; iy < 2; iy++)
	{
		if (spanY[iy][1] <= spanY[iy][0])
			continue;

		for (int ix = 0; ix < 2; ix++)
		{
			if (spanX[ix][1] <= spanX[ix][0])
				continue;

			gDPTextureRectangle( spanX[ix][0], spanY[iy][0],
			                     spanX[ix][1], spanY[iy][1],
			                     0, srcX[ix], srcY[iy], scaleW, scaleH );
		}
	}

/*	u32 line = (u32)(frameS1 - frameS0 + 1) << objScaleBg->imageSiz >> 4;
	u16 loadHeight;
	if (objScaleBg->imageFmt == G_IM_FMT_CI)
		loadHeight = 256 / line;
	else
		loadHeight = 512 / line;
	
	gDPSetTile( objScaleBg->imageFmt, objScaleBg->imageSiz, line, 0, 7, objScaleBg->imagePal, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0 );
	gDPSetTile( objScaleBg->imageFmt, objScaleBg->imageSiz, line, 0, 0, objScaleBg->imagePal, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0 );
	gDPSetTileSize( 0, 0, 0, frameS1 * 4, frameT1 * 4 );
	gDPSetTextureImage( objScaleBg->imageFmt, objScaleBg->imageSiz, imageW, objScaleBg->imagePtr );

	gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

	for (u32 i = 0; i < frameT1 / loadHeight; i++)
	{
		//if (objScaleBg->imageLoad == G_BGLT_LOADTILE)
			gDPLoadTile( 7, frameS0 * 4, (frameT0 + loadHeight * i) * 4, frameS1 * 4, (frameT1 + loadHeight * (i + 1) * 4 );
		//else
		//{
//			gDPSetTextureImage( objScaleBg->imageFmt, objScaleBg->imageSiz, imageW, objScaleBg->imagePtr + (i + imageY) * (imageW << objScaleBg->imageSiz >> 1) + (imageX << objScaleBg->imageSiz >> 1) );
//			gDPLoadBlock( 7, 0, 0, (loadHeight * frameW << objScaleBg->imageSiz >> 1) - 1, 0 );
// 		}

		gDPTextureRectangle( frameX0, frameY0 + loadHeight * i, 
			frameX1, frameY0 + loadHeight * (i + 1) - 1, 0, 0, 0, 4, 1 );
	}*/
}

void gSPBgRectCopy( u32 bg )
{
	u32 address = RSP_SegmentToPhysical( bg );

	if (!_useOnePieceBgCode( address ))
	{
		BgRectCopyStripped( address );
		return;
	}

	uObjBg *objBg = (uObjBg*)&RDRAM[address];

	gSP.bgImage.address = RSP_SegmentToPhysical( objBg->imagePtr );
	gSP.bgImage.width = objBg->imageW >> 2;
	gSP.bgImage.height = objBg->imageH >> 2;
	gSP.bgImage.format = objBg->imageFmt;
	gSP.bgImage.size = objBg->imageSiz;
	gSP.bgImage.palette = objBg->imagePal;
	gDP.textureMode = TEXTUREMODE_BGIMAGE;

	u16 imageX = objBg->imageX >> 5;
	u16 imageY = objBg->imageY >> 5;

	s16 frameX = objBg->frameX / 4;
	s16 frameY = objBg->frameY / 4;
	u16 frameW = objBg->frameW >> 2;
	u16 frameH = objBg->frameH >> 2;
	
	gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

	gDPTextureRectangle( frameX, frameY, frameX + frameW - 1, frameY + frameH - 1, 0, imageX, imageY, 4, 1 );
}

// S2DEX G_MW_GENSTAT.  The status words gate G_SELECT_DL and the OBJ_LOADTXTR
// block/tile/TLUT skip test, so a game that sets them by MoveWord rather than
// through a texture load needs this to land somewhere.
void gSPSetStatus( u32 sid, u32 value )
{
	if (sid >= 4)
		return;

	gSP.status[sid] = value;
}

// Raw fixed-point copy of the object matrix.  gSP.objMatrix holds the same values
// as floats for gSPObjSprite(); the OBJ_RECTANGLE coordinate math below is done in
// integers exactly as the microcode does it, so it needs the originals.
static uObjMtx objMtx;

void S2DEX_ResetObjMtx()
{
	objMtx.A = 1 << 16;
	objMtx.B = 0;
	objMtx.C = 0;
	objMtx.D = 1 << 16;
	objMtx.X = 0;
	objMtx.Y = 0;
	objMtx.BaseScaleX = 1 << 10;
	objMtx.BaseScaleY = 1 << 10;
}

// Coordinate correctors, from GLideN64 (big endian adapted)
struct S2DEXCoordCorrector
{
	s16 A0, A1, A2, A3, B0, B2, B3, B5, B7;

	S2DEXCoordCorrector()
	{
		static const u32 CorrectorsA01[] = {
			0x00000000, 0x00100020, 0x00200040, 0x00300060,
			0x0000FFF4, 0x00100014, 0x00200034, 0x00300054
		};
		static const u32 CorrectorsA23[] = {
			0x0001FFFE, 0xFFFEFFFE, 0x00010000, 0x00000000
		};

		const s16 *A01 = (const s16*)CorrectorsA01;
		const s16 *A23 = (const s16*)CorrectorsA23;

		const u32 O1 = (gSP.objRendermode & (G_OBJRM_SHRINKSIZE_1 | G_OBJRM_SHRINKSIZE_2 | G_OBJRM_WIDEN)) >> 3;
		A0 = A01[0 + O1];
		A1 = A01[1 + O1];

		const u32 O2 = (gSP.objRendermode & (G_OBJRM_SHRINKSIZE_1 | G_OBJRM_BILERP)) >> 2;
		A2 = A23[0 + O2];
		A3 = A23[1 + O2];

		if (S2DEX_GetVersion() == S2DEX_VER_1_3)
		{
			static const u32 CorrectorsB03_v1_3[] = {
				0xFFFC0000, 0x00000000, 0x00000001, 0x00000000,
				0xFFFC0000, 0x00000000, 0x00000001, 0xFFFF0001,
				0xFFFC0000, 0x00030000, 0x00000001, 0x00000000,
				0xFFFC0000, 0x00030000, 0x00000001, 0xFFFF0000,
				0xFFFF0003, 0x0000FFF0, 0x00000001, 0x0000FFFF,
				0xFFFF0003, 0x0000FFF0, 0x00000001, 0xFFFFFFFF,
				0xFFFF0003, 0x0000FFF0, 0x00000000, 0x00000000,
				0xFFFF0003, 0x0000FFF0, 0x00000000, 0xFFFF0000
			};
			const s16 *B = (const s16*)CorrectorsB03_v1_3;
			const u32 O3 = (_SHIFTL( gSP.objRendermode, 3, 16 ) & (G_OBJRM_SHRINKSIZE_1 | G_OBJRM_SHRINKSIZE_2 | G_OBJRM_WIDEN)) >> 1;
			B0 = B[0 + O3];
			B2 = B[2 + O3];
			B3 = B[3 + O3];
			B5 = B[5 + O3];
			B7 = B[7 + O3];
		}
		else
		{
			static const u32 CorrectorsB03[] = {
				0xFFFC0000, 0x00000001, 0xFFFF0003, 0xFFF00000
			};
			const s16 *B = (const s16*)CorrectorsB03;
			const u32 O3 = (gSP.objRendermode & G_OBJRM_BILERP) >> 1;
			B0 = B[0 + O3];
			B2 = B[2 + O3];
			B3 = B[3 + O3];
			B5 = 0;
			B7 = 0;
		}
	}
};

static void _gSPObjRectCoords( const uObjSprite *objSprite, BOOL useMatrix,
                               f32 &ulx, f32 &uly, f32 &lrx, f32 &lry,
                               f32 &uls, f32 &ult, f32 &lrs, f32 &lrt )
{
	S2DEXCoordCorrector CC;
	s16 xh, xl, yh, yl;
	s16 sh, sl, th, tl;
	s16 stBase;

	const u16 spriteScaleW = MAX( objSprite->scaleW, (u16)1 );
	const u16 spriteScaleH = MAX( objSprite->scaleH, (u16)1 );
	u32 stScaleH = spriteScaleH;

	if (useMatrix)
	{
		const u32 baseScaleX = MAX( (u32)objMtx.BaseScaleX, (u32)1 );
		const u32 baseScaleY = MAX( (u32)objMtx.BaseScaleY, (u32)1 );
		const u32 scaleW = MAX( (baseScaleX * 0x40 * spriteScaleW) >> 16, (u32)1 );
		const u32 scaleH = MAX( (baseScaleY * 0x40 * spriteScaleH) >> 16, (u32)1 );

		if (S2DEX_GetVersion() == S2DEX_VER_1_3)
		{
			xh = (s16)(((((s64)objSprite->objX << 27) * (0x80007FFFU / baseScaleX)) >> 0x30) + objMtx.X + CC.A2) & CC.B0;
			xl = (s16)((s16)(((((s64)objSprite->imageW - CC.A1) << 8) * (0x80007FFFU / scaleW)) >> 0x20) & CC.B0) + xh;
			yh = (s16)(((((s64)objSprite->objY << 27) * (0x80007FFFU / baseScaleY)) >> 0x30) + objMtx.Y + CC.A2) & CC.B0;
			yl = (s16)((s16)(((((s64)objSprite->imageH - CC.A1) << 8) * (0x80007FFFU / scaleH)) >> 0x20) & CC.B0) + yh;
			stBase = CC.B3;
		}
		else
		{
			const s32 xhp = (s32)(((((s64)objSprite->objX << 16) * 0x0800) * (0x80007FFFU / baseScaleX)) >> 32) + (((objMtx.X + CC.A2) & CC.B0) << 16);
			xh = (s16)(xhp >> 16);
			const s32 xlp = xhp + (s32)(((((u64)objSprite->imageW - CC.A1) << 24) * (0x80007FFFU / scaleW)) >> 32);
			xl = (s16)(xlp >> 16);
			const s32 yhp = (s32)(((((s64)objSprite->objY << 16) * 0x0800) * (0x80007FFFU / baseScaleY)) >> 32) + (((objMtx.Y + CC.A2) & CC.B0) << 16);
			yh = (s16)(yhp >> 16);
			const s32 ylp = yhp + (s32)(((((u64)objSprite->imageH - CC.A1) << 24) * (0x80007FFFU / scaleH)) >> 32);
			yl = (s16)(ylp >> 16);
			stBase = CC.B2;
		}

		stScaleH = scaleH;
	}
	else
	{
		xh = (s16)((objSprite->objX + CC.A2) & CC.B0);
		xl = (s16)(((((u64)objSprite->imageW - CC.A1) << 24) * (0x80007FFFU / (u32)spriteScaleW)) >> 48) + xh;
		yh = (s16)((objSprite->objY + CC.A2) & CC.B0);
		yl = (s16)(((((u64)objSprite->imageH - CC.A1) << 24) * (0x80007FFFU / (u32)spriteScaleH)) >> 48) + yh;
		stBase = CC.B2;
	}

	sh = (s16)(CC.A0 + stBase);
	sl = (s16)(sh + objSprite->imageW + CC.A0 - CC.A1 - 1);
	th = (s16)(sh - (((yh & 3) * 0x0200 * stScaleH) >> 16));
	tl = (s16)(th + objSprite->imageH + CC.A0 - CC.A1 - 1);

	ulx = _FIXED2FLOAT( xh, 2 );
	lrx = _FIXED2FLOAT( xl, 2 );
	uly = _FIXED2FLOAT( yh, 2 );
	lry = _FIXED2FLOAT( yl, 2 );

	uls = _FIXED2FLOAT( sh, 5 );
	lrs = _FIXED2FLOAT( sl, 5 );
	ult = _FIXED2FLOAT( th, 5 );
	lrt = _FIXED2FLOAT( tl, 5 );

	if ((objSprite->imageFlags & G_BG_FLAG_FLIPS) != 0)
	{
		const f32 tmp = uls; uls = lrs; lrs = tmp;
	}
	if ((objSprite->imageFlags & G_BG_FLAG_FLIPT) != 0)
	{
		const f32 tmp = ult; ult = lrt; lrt = tmp;
	}
}

// YUV macro block support, from GLideN64, for Ogre Battle 64 (some backgrounds only).
static u16 _YUVtoRGBA( u8 y, u8 u, u8 v )
{
	f32 r = y + (1.370705f * (v - 128));
	f32 g = y - (0.698001f * (v - 128)) - (0.337633f * (u - 128));
	f32 b = y + (1.732446f * (u - 128));

	r *= 0.125f;
	g *= 0.125f;
	b *= 0.125f;

	if (r > 31.0f) r = 31.0f;
	if (g > 31.0f) g = 31.0f;
	if (b > 31.0f) b = 31.0f;
	if (r < 0.0f)  r = 0.0f;
	if (g < 0.0f)  g = 0.0f;
	if (b < 0.0f)  b = 0.0f;

	return (u16)((((u16)r) << 11) | (((u16)g) << 6) | (((u16)b) << 1) | 1);
}

static void _drawYUVImageToFrameBuffer( f32 fUlx, f32 fUly, f32 fLrx, f32 fLry )
{
	const u32 ulx = (u32)fUlx;
	const u32 uly = (u32)fUly;
	const u32 lrx = (u32)fLrx;
	const u32 lry = (u32)fLry;

	const u32 ciWidth  = gDP.colorImage.width;
	const u32 ciHeight = (u32)gDP.scissor.lry;

	if (ciWidth == 0 || ulx >= ciWidth || uly >= ciHeight)
		return;

	// A macro block is always 16x16; it may hang off the right or bottom edge.
	u32 width  = 16;
	u32 height = 16;
	if (lrx > ciWidth)
		width = ciWidth - ulx;
	if (lry > ciHeight)
		height = ciHeight - uly;

	const u32 srcBase = gDP.textureImage.address;
	const u32 dstBase = gDP.colorImage.address;

	if ((srcBase + 16 * 16 * 2) > RDRAMSize)
		return;

	for (u32 h = 0; h < 16; h++)
	{
		const u32 rowDst = dstBase + ((uly + h) * ciWidth + ulx) * 2;
		const u32 *src = (const u32*)&RDRAM[srcBase + h * 16 * 2];

		if (h >= height)
			break;
		if ((rowDst + width * 2) > RDRAMSize)
			break;

		u16 *dst = (u16*)&RDRAM[rowDst];

		for (u32 w = 0; w < 16; w += 2)
		{
			const u32 t = *(src++);	// two pixels per word

			if (w >= width)
				continue;

			const u8 y0 = (u8)(t & 0xFF);
			const u8 v  = (u8)((t >> 8) & 0xFF);
			const u8 y1 = (u8)((t >> 16) & 0xFF);
			const u8 u  = (u8)((t >> 24) & 0xFF);

			dst[w] = _YUVtoRGBA( y0, u, v );
			if ((w + 1) < width)
				dst[w + 1] = _YUVtoRGBA( y1, u, v );
		}
	}
}

// Tile setup shared by both OBJ_RECTANGLE forms (upstream gSPSetSpriteTile).
static void _gSPSetSpriteTile( const uObjSprite *objSprite )
{
	const u32 imageW = MAX( objSprite->imageW >> 5, (u32)1 );
	const u32 imageH = MAX( objSprite->imageH >> 5, (u32)1 );

	// A preceding BG command leaves gDP.textureMode at TEXTUREMODE_BGIMAGE, which
	// would route this sprite to TextureCache_UpdateBackground().  Upstream guards
	// the same way in gSPSetSpriteTile().
	gDP.textureMode = TEXTUREMODE_NORMAL;

	gDPSetTile( objSprite->imageFmt, objSprite->imageSiz, objSprite->imageStride,
	            objSprite->imageAdrs, 0, objSprite->imagePal,
	            G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0 );
	gDPSetTileSize( 0, 0, 0, (imageW - 1) << 2, (imageH - 1) << 2 );
	gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );
}

void gSPObjRectangle( u32 sp )
{
	const u32 address = RSP_SegmentToPhysical( sp );
	const uObjSprite *objSprite = (const uObjSprite*)&RDRAM[address];

	f32 ulx, uly, lrx, lry, uls, ult, lrs, lrt;
	_gSPObjRectCoords( objSprite, FALSE, ulx, uly, lrx, lry, uls, ult, lrs, lrt );

	_gSPSetSpriteTile( objSprite );

	const f32 spanX = lrx - ulx;
	const f32 spanY = lry - uly;
	const f32 dsdx = (spanX != 0.0f) ? (lrs - uls) / spanX : 1.0f;
	const f32 dtdy = (spanY != 0.0f) ? (lrt - ult) / spanY : 1.0f;

	gDPTextureRectangle( ulx, uly, lrx, lry, 0, uls, ult, dsdx, dtdy );
}

// OBJ_RECTANGLE_R: the same sprite rect, positioned and scaled through the object
// matrix set by gSPObjMatrix()/gSPObjSubMatrix().
void gSPObjRectangleR( u32 sp )
{
	const u32 address = RSP_SegmentToPhysical( sp );
	const uObjSprite *objSprite = (const uObjSprite*)&RDRAM[address];

	f32 ulx, uly, lrx, lry, uls, ult, lrs, lrt;
	_gSPObjRectCoords( objSprite, TRUE, ulx, uly, lrx, lry, uls, ult, lrs, lrt );

	// Ogre Battle 64 needs its YUV backgrounds decoded into the colour image.
	if (objSprite->imageFmt == G_IM_FMT_YUV &&
	    GetGameSpecificHack() == &hack_ogrebattle)
		_drawYUVImageToFrameBuffer( ulx, uly, lrx, lry );

	_gSPSetSpriteTile( objSprite );

	const f32 spanX = lrx - ulx;
	const f32 spanY = lry - uly;
	const f32 dsdx = (spanX != 0.0f) ? (lrs - uls) / spanX : 1.0f;
	const f32 dtdy = (spanY != 0.0f) ? (lrt - ult) / spanY : 1.0f;

	gDPTextureRectangle( ulx, uly, lrx, lry, 0, uls, ult, dsdx, dtdy );
}

void gSPObjLoadTxtr( u32 tx )
{
	u32 address = RSP_SegmentToPhysical( tx );
	uObjTxtr *objTxtr = (uObjTxtr*)&RDRAM[address];

	if (objTxtr->block.sid > 12)
		return;

	if ((gSP.status[objTxtr->block.sid >> 2] & objTxtr->block.mask) != objTxtr->block.flag)
	{
		switch (objTxtr->block.type)
		{
			case G_OBJLT_TXTRBLOCK:
				gDPSetTextureImage( G_IM_FMT_RGBA, G_IM_SIZ_16b,
				                    objTxtr->block.tsize + 1, objTxtr->block.image );
				gDPSetTile( G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, objTxtr->block.tmem,
				            G_TX_LOADTILE, 0,
				            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
				            0, 0, 0, 0 );
				gDPLoadBlock( G_TX_LOADTILE, 0, 0,
				              objTxtr->block.tsize << 2, objTxtr->block.tline );
				break;
			case G_OBJLT_TXTRTILE:
				gDPSetTextureImage( G_IM_FMT_RGBA, G_IM_SIZ_16b,
				                    objTxtr->tile.twidth + 1, objTxtr->tile.image );
				gDPSetTile( G_IM_FMT_RGBA, G_IM_SIZ_16b,
				            (objTxtr->tile.twidth + 1) >> 2, objTxtr->tile.tmem,
				            G_TX_LOADTILE, 0,
				            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
				            0, 0, 0, 0 );
				gDPLoadTile( G_TX_LOADTILE, 0, 0,
				             objTxtr->tile.twidth << 2, objTxtr->tile.theight );
				break;
			case G_OBJLT_TLUT:
				gDPSetTextureImage( G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, objTxtr->tlut.image );
				gDPSetTile( G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, objTxtr->tlut.phead,
				            G_TX_LOADTILE, 0,
				            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP,
				            0, 0, 0, 0 );
				gDPLoadTLUT( G_TX_LOADTILE, 0, 0, objTxtr->tlut.pnum << 2, 0 );
				break;
		}
		gSP.status[objTxtr->block.sid >> 2] = (gSP.status[objTxtr->block.sid >> 2] & ~objTxtr->block.mask) | (objTxtr->block.flag & objTxtr->block.mask);
	}
}

void gSPObjSprite( u32 sp )
{
	u32 address = RSP_SegmentToPhysical( sp );
	uObjSprite *objSprite = (uObjSprite*)&RDRAM[address];

	// Clamp against a zero scale from a malformed display list (GLideN64 commit 9eddde6e).
	f32 scaleW = _FIXED2FLOAT( max( objSprite->scaleW, (u16)1 ), 10 );
	f32 scaleH = _FIXED2FLOAT( max( objSprite->scaleH, (u16)1 ), 10 );
	f32 objX = _FIXED2FLOAT( objSprite->objX, 2 );
	f32 objY = _FIXED2FLOAT( objSprite->objY, 2 );
	u32 imageW = objSprite->imageW >> 5;
	u32 imageH = objSprite->imageH >> 5;

	f32 x0 = objX;
	f32 y0 = objY;
	f32 x1 = objX + imageW / scaleW - 1;
	f32 y1 = objY + imageH / scaleH - 1;

	gSP.vertices[0].x = gSP.objMatrix.A * x0 + gSP.objMatrix.B * y0 + gSP.objMatrix.X;
	gSP.vertices[0].y = gSP.objMatrix.C * x0 + gSP.objMatrix.D * y0 + gSP.objMatrix.Y;
	gSP.vertices[0].z = 0.0f;
	gSP.vertices[0].w = 1.0f;
	gSP.vertices[0].s = 0.0f;
	gSP.vertices[0].t = 0.0f;

	gSP.vertices[1].x = gSP.objMatrix.A * x1 + gSP.objMatrix.B * y0 + gSP.objMatrix.X;
	gSP.vertices[1].y = gSP.objMatrix.C * x1 + gSP.objMatrix.D * y0 + gSP.objMatrix.Y;
	gSP.vertices[1].z = 0.0f;
	gSP.vertices[1].w = 1.0f;
	gSP.vertices[1].s = imageW - 1;
	gSP.vertices[1].t = 0.0f;

	gSP.vertices[2].x = gSP.objMatrix.A * x1 + gSP.objMatrix.B * y1 + gSP.objMatrix.X;
	gSP.vertices[2].y = gSP.objMatrix.C * x1 + gSP.objMatrix.D * y1 + gSP.objMatrix.Y;
	gSP.vertices[2].z = 0.0f;
	gSP.vertices[2].w = 1.0f;
	gSP.vertices[2].s = imageW - 1;
	gSP.vertices[2].t = imageH - 1;

	gSP.vertices[3].x = gSP.objMatrix.A * x0 + gSP.objMatrix.B * y1 + gSP.objMatrix.X;
	gSP.vertices[3].y = gSP.objMatrix.C * x0 + gSP.objMatrix.D * y1 + gSP.objMatrix.Y;
	gSP.vertices[3].z = 0.0f;
	gSP.vertices[3].w = 1.0f;
	gSP.vertices[3].s = 0;
	gSP.vertices[3].t = imageH - 1;

	gDPSetTile( objSprite->imageFmt, objSprite->imageSiz, objSprite->imageStride, objSprite->imageAdrs, 0, objSprite->imagePal, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0 );
	gDPSetTileSize( 0, 0, 0, (imageW - 1) << 2, (imageH - 1) << 2 );
	gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

#ifndef __GX__
	glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	glOrtho( 0, VI.width, VI.height, 0, 0.0f, 32767.0f );
#else // !__GX__
	if (OGL.numTriangles)
		OGL_DrawTriangles();

	const BOOL objSpriteCombW = OGL.GXuseCombW;

	OGL.GXuseCombW = FALSE;
#endif // __GX__

	OGL_AddTriangle( gSP.vertices, 0, 1, 2 );
	OGL_AddTriangle( gSP.vertices, 0, 2, 3 );

#ifdef __GX__
	Mtx44 GXprojection;
	guOrtho( GXprojection, 0, VI.height, 0, VI.width, 1.0f, -1.0f );
	GX_LoadProjectionMtx( GXprojection, GX_ORTHOGRAPHIC );
	GX_LoadPosMtxImm( OGL.GXmodelViewIdent, GX_PNMTX0 );
	GX_SetViewport( (f32) OGL.GXorigX, (f32) OGL.GXorigY,
	                (f32) OGL.GXwidth, (f32) OGL.GXheight, 0.0f, 1.0f );

	OGL.GXuseCombW = FALSE;
	OGL.GXupdateMtx = FALSE;
#endif // __GX__

	OGL_DrawTriangles();

#ifndef __GX__
	glLoadIdentity();
#else // !__GX__
	OGL.GXuseCombW  = objSpriteCombW;
	OGL.GXupdateMtx = TRUE;
	gDP.changed |= CHANGED_SCISSOR;

	OGL_UpdateViewport();
#endif // __GX__

	if (depthBuffer.current) depthBuffer.current->cleared = FALSE;
	gDP.colorImage.changed = TRUE;
	gDP.colorImage.height = (unsigned long)(max( gDP.colorImage.height, gDP.scissor.lry ));
}

void gSPObjLoadTxSprite( u32 txsp )
{
	gSPObjLoadTxtr( txsp );
	gSPObjSprite( txsp + sizeof( uObjTxtr ) );
}

void gSPObjLoadTxRectR( u32 txsp )
{
	gSPObjLoadTxtr( txsp );
	gSPObjRectangleR( txsp + sizeof( uObjTxtr ) );
}

void gSPObjMatrix( u32 mtx )
{
	u32 address = RSP_SegmentToPhysical( mtx );
	uObjMtx *objMtx = (uObjMtx*)&RDRAM[address];

	gSP.objMatrix.A = _FIXED2FLOAT( objMtx->A, 16 );
	gSP.objMatrix.B = _FIXED2FLOAT( objMtx->B, 16 );
	gSP.objMatrix.C = _FIXED2FLOAT( objMtx->C, 16 );
	gSP.objMatrix.D = _FIXED2FLOAT( objMtx->D, 16 );
	gSP.objMatrix.X = _FIXED2FLOAT( objMtx->X, 2 );
	gSP.objMatrix.Y = _FIXED2FLOAT( objMtx->Y, 2 );
	gSP.objMatrix.baseScaleX = _FIXED2FLOAT( objMtx->BaseScaleX, 10 );
	gSP.objMatrix.baseScaleY = _FIXED2FLOAT( objMtx->BaseScaleY, 10 );

	::objMtx = *objMtx;
}

void gSPObjSubMatrix( u32 mtx )
{
	const u32 address = RSP_SegmentToPhysical( mtx );
	const uObjSubMtx *subMtx = (const uObjSubMtx*)&RDRAM[address];

	::objMtx.X = subMtx->X;
	::objMtx.Y = subMtx->Y;
	::objMtx.BaseScaleX = subMtx->BaseScaleX;
	::objMtx.BaseScaleY = subMtx->BaseScaleY;

	gSP.objMatrix.X = _FIXED2FLOAT( subMtx->X, 2 );
	gSP.objMatrix.Y = _FIXED2FLOAT( subMtx->Y, 2 );
	gSP.objMatrix.baseScaleX = _FIXED2FLOAT( subMtx->BaseScaleX, 10 );
	gSP.objMatrix.baseScaleY = _FIXED2FLOAT( subMtx->BaseScaleY, 10 );
}
