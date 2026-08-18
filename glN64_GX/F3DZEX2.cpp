/**
 * glN64_GX - F3DZEX2.cpp
 *
 * Zelda OOT/MM ucode, it is the same as F3DEX2 except opcode 0x04 is G_BRANCH_W
 *
 * All credit goes to https://github.com/gonetz/GLideN64/blob/master/src/uCodes/F3DZEX2.cpp
**/

#ifdef __GX__
#include <gccore.h>
#endif // __GX__

#include "glN64.h"
#include "Debug.h"
#include "F3D.h"
#include "F3DEX.h"
#include "F3DEX2.h"
#include "F3DZEX2.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"

static void F3DZEX2_Branch_W( u32 w0, u32 w1 )
{
	gSPBranchLessW( gDP.half_1, _SHIFTR( w0, 1, 7 ), (f32)w1 );
}

void F3DZEX2_Init()
{
	F3DEX2_Init();
	GBI.cmd[F3DEX2_BRANCH_Z] = F3DZEX2_Branch_W;
}
