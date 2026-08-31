/**
 * glN64_GX - F5Rogue.h
 *
 * Star Wars Rogue Squadron ucode (F3DSWRS).
 *
 * Ported from https://github.com/gonetz/GLideN64/blob/master/src/uCodes/F5Rogue.cpp
 * Initial implementation ported from Lemmy's LemNemu plugin.
 * Microcode decoding: olivieryuyu
**/

#ifndef F5ROGUE_H
#define F5ROGUE_H

#include "Types.h"

#ifndef _BIG_ENDIAN
struct SWVertex
{
	s16 y, x;
	u16 flag;
	s16 z;
};
#else // !_BIG_ENDIAN
struct SWVertex
{
	s16 x, y, z;
	u16 flag;
};
#endif // _BIG_ENDIAN

void F5Rogue_Init();

#endif // F5ROGUE_H
