/**
 * glN64_GX - Turbo3D.cpp
 *
 * Turbo3D, used in Dark Rift, basically its own display list format.
 *
 * All credit goes to https://github.com/gonetz/GLideN64/blob/master/src/uCodes/Turbo3D.cpp
**/

#include <gccore.h>

#include <string.h>

#include "glN64.h"
#include "Debug.h"
#include "Turbo3D.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"
#include "OpenGL.h"
#include "VI.h"

/******************Turbo3D microcode*************************/

#define GT_FLAG_NOMTX		0x01	/* don't load the matrix */
#define GT_FLAG_NO_XFM		0x02	/* load vtx, use verbatim */
#define GT_FLAG_XFM_ONLY	0x04	/* xform vtx, write to *TriN */

struct T3DGlobState
{
#ifndef _BIG_ENDIAN
	u16 pad0;
	u16 perspNorm;
#else
	u16 perspNorm;
	u16 pad0;
#endif
	u32 flag;
	u32 othermode0;
	u32 othermode1;
	u32 segBases[16];
	
#ifndef _BIG_ENDIAN
	s16 vsacle1;
	s16 vsacle0;
	s16 vsacle3;
	s16 vsacle2;
	s16 vtrans1;
	s16 vtrans0;
	s16 vtrans3;
	s16 vtrans2;
#else
	s16 vsacle0;
	s16 vsacle1;
	s16 vsacle2;
	s16 vsacle3;
	s16 vtrans0;
	s16 vtrans1;
	s16 vtrans2;
	s16 vtrans3;
#endif
	u32 rdpCmds;
};

struct T3DState
{
	u32 renderState;	/* render state */
	u32 textureState;	/* texture state */
#ifndef _BIG_ENDIAN
	u8 flag;
	u8 triCount;	/* how many tris? */
	u8 vtxV0;		/* where to load verts? */
	u8 vtxCount;	/* how many verts? */
#else
	u8 vtxCount;
	u8 vtxV0;
	u8 triCount;
	u8 flag;
#endif
	u32 rdpCmds;	/* ptr (segment address) to RDP DL */
	u32 othermode0;
	u32 othermode1;
};

struct T3DTriN
{
#ifndef _BIG_ENDIAN
	u8	flag, v2, v1, v0;	/* flag is which one for flat shade */
#else
	u8	v0, v1, v2, flag;
#endif
};

struct VtxOut
{
#ifndef _BIG_ENDIAN
	s16 yscrn;	/* x,y screen coordinates are SSSS10.2 */
	s16 xscrn;
#else
	s16 xscrn;
	s16 yscrn;
#endif
	s32 zscrn;	/* z screen is S15.16 */
#ifndef _BIG_ENDIAN
	s16 t;
	s16 s;
#else
	s16 s;
	s16 t;
#endif
	union
	{
		struct
		{
#ifndef _BIG_ENDIAN
			u8 a;
			u8 b;
			u8 g;
			u8 r;
#else
			u8 r;
			u8 g;
			u8 b;
			u8 a;
#endif
		} color;
		struct
		{
#ifndef _BIG_ENDIAN
			s8 a;
			s8 z;	// b
			s8 y;	// g
			s8 x;	// r
#else
			s8 x;
			s8 y;
			s8 z;
			s8 a;
#endif
		} normal;
	};
};

static bool Turbo3D_RangeValid( u32 addr, u32 size )
{
	return (addr != 0) && (size <= RDRAMSize) && ((addr + size) <= RDRAMSize);
}

static void Turbo3D_ProcessRDP( u32 _cmds )
{
	const u32 addr = RSP_SegmentToPhysical( _cmds );
	if (addr == 0)
		return;

	const u32 savedPC = RSP.PC[RSP.PCi];
	RSP.PC[RSP.PCi] = addr;

	while (Turbo3D_RangeValid( RSP.PC[RSP.PCi], 8 ))
	{
		const u32 w0 = *(u32*)&RDRAM[RSP.PC[RSP.PCi]];
		const u32 w1 = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 4];
		if ((w0 + w1) == 0)
			break;

		RSP.cmd = _SHIFTR( w0, 24, 8 );
		// Advance before dispatching, like _ProcessDList(), so a handler that
		// reads further words finds the PC where it expects it
		RSP.PC[RSP.PCi] += 8;
		GBI.cmd[RSP.cmd]( w0, w1 );
	}

	RSP.PC[RSP.PCi] = savedPC;
}

static void Turbo3D_LoadGlobState( u32 pgstate )
{
	const u32 addr = RSP_SegmentToPhysical( pgstate );
	if (!Turbo3D_RangeValid( addr, sizeof( T3DGlobState ) ))
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Turbo3D_LoadGlobState: state at 0x%08X lies outside RDRAM\n", pgstate );
#endif
		return;
	}

	T3DGlobState gstate;
	memcpy( &gstate, RDRAM + addr, sizeof( gstate ) );

	gDPSetOtherMode( _SHIFTR( gstate.othermode0, 0, 24 ),	// mode0
					 gstate.othermode1 );					// mode1

	for (s32 s = 0; s < 16; ++s)
		gSPSegment( s, gstate.segBases[s] & 0x00FFFFFF );

	gSPViewport( pgstate + 80 );

	Turbo3D_ProcessRDP( gstate.rdpCmds );
}

static void Turbo3D_LoadObject( u32 pstate, u32 pvtx, u32 ptri )
{
	u32 addr = RSP_SegmentToPhysical( pstate );
	if (!Turbo3D_RangeValid( addr, sizeof( T3DState ) ))
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Turbo3D_LoadObject: state at 0x%08X lies outside RDRAM\n", pstate );
#endif
		return;
	}

	T3DState ostate;
	memcpy( &ostate, RDRAM + addr, sizeof( ostate ) );

	const u32 tile = ostate.textureState & 7;
	gSP.texture.tile = tile;
	gSP.textureTile[0] = &gDP.tiles[tile];
	gSP.textureTile[1] = &gDP.tiles[(tile + 1) & 7];
	gSP.texture.scales = 1.0f;
	gSP.texture.scalet = 1.0f;

	gDPSetOtherMode( _SHIFTR( ostate.othermode0, 0, 24 ),	// mode0
					 ostate.othermode1 );					// mode1

	if (ostate.flag != GT_FLAG_NOMTX)
		gSPForceMatrix( pstate + sizeof( T3DState ) );

	gSPClearGeometryMode( G_LIGHTING | G_FOG );
	gSPSetGeometryMode( ostate.renderState | G_SHADING_SMOOTH | G_SHADE | G_ZBUFFER | G_CULL_BACK );

	if (pvtx != 0)
		gSPVertex( pvtx, ostate.vtxCount, ostate.vtxV0 );

	Turbo3D_ProcessRDP( ostate.rdpCmds );

	if (ptri == 0)
		return;

	addr = RSP_SegmentToPhysical( ptri );
	// triCount is a u8, so the whole triangle list is at most 255 * 4 bytes.
	// Validate it once instead of per iteration.
	if (!Turbo3D_RangeValid( addr, ostate.triCount * sizeof( T3DTriN ) ))
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Turbo3D_LoadObject: triangle list at 0x%08X lies outside RDRAM\n", ptri );
#endif
		return;
	}

	if (ostate.flag != GT_FLAG_NO_XFM)
	{
		for (u32 t = 0; t < ostate.triCount; ++t)
		{
			T3DTriN tri;
			memcpy( &tri, RDRAM + addr, sizeof( tri ) );
			addr += 4;
			gSPTriangle( tri.v0, tri.v1, tri.v2 );
		}
		if (OGL.numTriangles)
			OGL_DrawTriangles();
		return;
	}


	const u32 vtxAddr = RSP_SegmentToPhysical( pvtx );
	if (!Turbo3D_RangeValid( vtxAddr, ostate.vtxCount * sizeof( VtxOut ) ))
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Turbo3D_LoadObject: vertex list at 0x%08X lies outside RDRAM\n", pvtx );
#endif
		return;
	}

	const f32 ndcScaleX = 2.0f / (f32)VI.width;
	const f32 ndcScaleY = 2.0f / (f32)VI.height;
	for (u32 i = 0; i < ostate.vtxCount; ++i)
	{
		VtxOut vertex;
		memcpy( &vertex, RDRAM + vtxAddr + i * sizeof( VtxOut ), sizeof( vertex ) );
		gSP.vertices[i].x = _FIXED2FLOAT( vertex.xscrn, 2 ) * ndcScaleX - 1.0f;
		gSP.vertices[i].y = 1.0f - _FIXED2FLOAT( vertex.yscrn, 2 ) * ndcScaleY;
		gSP.vertices[i].z = _FIXED2FLOAT( vertex.zscrn, 16 );
		gSP.vertices[i].w = 1.0f;
	}

#ifdef __GX__
	OGL.GXuseCombW = false;		// identity orthographic projection
	OGL.GXupdateMtx = true;
#endif // __GX__

	for (u32 t = 0; t < ostate.triCount; ++t)
	{
		T3DTriN tri;
		memcpy( &tri, RDRAM + addr, sizeof( tri ) );
		addr += 4;

		if (tri.v0 >= ostate.vtxCount || tri.v1 >= ostate.vtxCount ||
			tri.v2 >= ostate.vtxCount)
			continue;

		gSPTriangle( tri.v0, tri.v1, tri.v2 );
	}
	if (OGL.numTriangles)
		OGL_DrawTriangles();

	gSP.changed |= CHANGED_MATRIX;	// let the next object rebuild the projection
}

void RunTurbo3D()
{
	static const u32 T3D_COMMAND_SIZE = 16;

	while (!RSP.halt)
	{
		if (!Turbo3D_RangeValid( RSP.PC[RSP.PCi], T3D_COMMAND_SIZE ))
		{
#ifdef DEBUG
			DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// RunTurbo3D: display list ran past the end of RDRAM at 0x%08X\n", RSP.PC[RSP.PCi] );
#endif
			RSP.halt = 1;
			break;
		}

		const u32 pc = RSP.PC[RSP.PCi];
		const u32 pgstate = *(u32*)&RDRAM[pc];
		const u32 pstate  = *(u32*)&RDRAM[pc +  4];
		const u32 pvtx    = *(u32*)&RDRAM[pc +  8];
		const u32 ptri    = *(u32*)&RDRAM[pc + 12];

		if (pstate == 0)
		{
			RSP.halt = 1;
			break;
		}

		if (pgstate != 0)
			Turbo3D_LoadGlobState( pgstate );
		Turbo3D_LoadObject( pstate, pvtx, ptri );

		// Go to the next instruction
		RSP.PC[RSP.PCi] += T3D_COMMAND_SIZE;
	}
}
