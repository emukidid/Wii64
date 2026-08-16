/**
 * glN64_GX - F3DFLX2.cpp
 *
 * F-Zero X's custom vehicle-rendering microcode. It is the same as
 * F3DEX2 except for how it uses the G_MOVEMEM light table.
 * 
 * All credit goes to https://github.com/gonetz/GLideN64/blob/master/src/uCodes/F3DFLX2.cpp
**/

#ifdef __GX__
#include <gccore.h>
#endif // __GX__

#include "glN64.h"
#include "Debug.h"
#include "F3D.h"
#include "F3DEX.h"
#include "F3DEX2.h"
#include "F3DFLX2.h"
#include "N64.h"
#include "RSP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"

static
void F3DFLX2_LoadAlphaLight( u32 _a )
{
	u32 address = RSP_SegmentToPhysical( _a );

	if ((address + sizeof( Light )) > RDRAMSize)
	{
#ifdef DEBUG
		DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load alpha light from invalid address\n" );
#endif
		return;
	}

	const s16 *data = (const s16*)&RDRAM[address];

#ifndef _BIG_ENDIAN
	gSP.lookat.xyz[0][0] = _FIXED2FLOAT( data[4 ^ 1], 8 );
	gSP.lookat.xyz[0][1] = _FIXED2FLOAT( data[5 ^ 1], 8 );
	gSP.lookat.xyz[0][2] = _FIXED2FLOAT( data[6 ^ 1], 8 );
#else // !_BIG_ENDIAN
	gSP.lookat.xyz[0][0] = _FIXED2FLOAT( data[4], 8 );
	gSP.lookat.xyz[0][1] = _FIXED2FLOAT( data[5], 8 );
	gSP.lookat.xyz[0][2] = _FIXED2FLOAT( data[6], 8 );
#endif // _BIG_ENDIAN

	gSP.lookat.enable = TRUE;

#ifndef __GX__
	Normalize( gSP.lookat.xyz[0] );
#else //!__GX__
	guVecNormalize((guVector*) gSP.lookat.xyz[0],(guVector*) gSP.lookat.xyz[0] );
#endif //__GX__

#ifdef DEBUG
	DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "F3DFLX2_LoadAlphaLight( 0x%08X );\n", _a );
#endif
}

void F3DFLX2_MoveMem( u32 w0, u32 w1 )
{
	switch (_SHIFTR( w0, 0, 8 ))
	{
		case F3DEX2_MV_VIEWPORT:
			gSPViewport( w1 );
			break;
		case G_MV_MATRIX:
			gSPForceMatrix( w1 );

			// force matrix takes two commands
			RSP.PC[RSP.PCi] += 8;
			break;
		case G_MV_LIGHT:
		{
			u32 offset = _SHIFTR( w0, 8, 8 ) << 3;
			u32 n = offset / 24;

			if (n == 1)
				F3DFLX2_LoadAlphaLight( w1 );
			else if (n >= 2)
				gSPLight( w1, n - 1 );
		}
			break;
		default:
			F3DEX2_MoveMem( w0, w1 );
			break;
	}
}

void F3DFLX2_Init()
{
	F3DEX2_Init();

	GBI_SetGBI( G_MOVEMEM,				F3DEX2_MOVEMEM,				F3DFLX2_MoveMem );
}
