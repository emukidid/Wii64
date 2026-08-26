/**
 * glN64_GX - ZSortBOSS.h
 *
 * ZSortBOSS ucode: World Driver Championship, Stunt Racer 64.
 * Ported from https://github.com/gonetz/GLideN64/blob/master/src/uCodes/ZSortBOSS.h
**/

#ifndef ZSORTBOSS_H
#define ZSORTBOSS_H

#ifndef _BIG_ENDIAN
struct zSortVDest
{
	s16 sy;
	s16 sx;
	s32 invw;
	s16 yi;
	s16 xi;
	s16 wi;
	u8 fog;
	u8 cc;
};
#else // !_BIG_ENDIAN
struct zSortVDest
{
	s16 sx;
	s16 sy;
	s32 invw;
	s16 xi;
	s16 yi;
	u8 cc;
	u8 fog;
	s16 wi;
};
#endif // _BIG_ENDIAN

typedef f32 M44[4][4];

void ZSortBOSS_Init();

#endif // ZSORTBOSS_H
