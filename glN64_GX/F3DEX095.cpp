/**
 * glN64_GX - F3DEX095.cpp
 *
 * F3DEX 0.95 / F3DLX 0.95 - identical to F3DEX except that G_CULLDL is still
 * at the plain F3D opcode. See GLideN64 issue #2774.
 *
 * Mario Kart 64
 *
 * All credit goes to https://github.com/gonetz/GLideN64/blob/master/src/uCodes/F3DEX095.cpp
**/

#ifdef __GX__
#include <gccore.h>
#endif // __GX__

#include "glN64.h"
#include "Debug.h"
#include "F3D.h"
#include "F3DEX.h"
#include "F3DEX095.h"
#include "gSP.h"
#include "GBI.h"

void F3DEX095_Init()
{
	F3DEX_Init();
	GBI_SetGBI( G_CULLDL, F3D_CULLDL, F3D_CullDL );
}
