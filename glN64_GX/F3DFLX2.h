/**
 * glN64_GX - F3DFLX2.h
 *
 * F-Zero X's custom vehicle-rendering microcode. It is the same as
 * F3DEX2 except for how it uses the G_MOVEMEM light table.
 * 
 * All credit goes to https://github.com/gonetz/GLideN64/blob/master/src/uCodes/F3DFLX2.cpp
**/

#ifndef F3DFLX2_H
#define F3DFLX2_H

void F3DFLX2_MoveMem( u32 w0, u32 w1 );
void F3DFLX2_Init();

#endif
