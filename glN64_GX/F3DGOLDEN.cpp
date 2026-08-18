/**
 * glN64_GX - F3DGOLDEN.cpp
 *
 * GoldenEye - It is the same as plain F3D except for two opcodes
 *
 * All credit goes to https://github.com/gonetz/GLideN64/blob/master/src/uCodes/F3DGOLDEN.cpp
**/

#ifdef __GX__
#include <gccore.h>
#endif // __GX__

#include "glN64.h"
#include "Debug.h"
#include "F3D.h"
#include "F3DGOLDEN.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"

static void F3DGOLDEN_TriX( u32 w0, u32 w1 )
{
	for (u32 i = 0; i < 4; i++)
	{
		if ((w1 >> (i * 8)) == 0)
			break;

		gSP1Triangle( _SHIFTR( w0, i * 4, 4 ), _SHIFTR( w1, i * 8, 4 ), _SHIFTR( w1, i * 8 + 4, 4 ) );
	}
}

void F3DGOLDEN_Init()
{
	F3D_Init();

	GBI.cmd[F3D_TRI4] = F3DGOLDEN_TriX;
	GBI.cmd[F3D_POPMTX] = F3D_MoveWord;
}
