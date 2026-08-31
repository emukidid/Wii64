/**
 * glN64_GX - F3DBETA.h
 *
 * Early Fast3D. Vertex and triangle indices are scaled by 5 rather than 10,
 * G_PERSPNORM is a standalone opcode, and TRI2/RDPHALF sit at their own values.
 *
 * Star Wars: Shadows of the Empire, Wave Race 64 (U)
 *
 * All credit goes to https://github.com/gonetz/GLideN64/blob/master/src/uCodes/F3DBETA.cpp
**/

#ifndef F3DBETA_H
#define F3DBETA_H
#include "Types.h"

#define F3DBETA_PERSPNORM	0xB4
#define F3DBETA_RDPHALF_1	0xB3
#define F3DBETA_RDPHALF_2	0xB2
#define F3DBETA_TRI2		0xB1

void F3DBETA_Init();

void F3DBETA_Perpnorm( u32 w0, u32 w1 );

#endif
