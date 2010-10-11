/**
 * Mupen64 hle rsp - hle.c
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#ifndef HLE_H
#define HLE_H

#include "Rsp_#1.1.h"

#ifdef _BIG_ENDIAN
#define S 0
#define S16 0
#define S8 0
#else
#define S 1
#define S16 2
#define S8 3
#endif

// types
typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

typedef signed char			s8;
typedef signed short		s16;
typedef signed int			s32;
typedef signed long long	s64;

//#define ACMD_SIZE               32
/*
 * Audio flags
 */

#define A_INIT			0x01
#define A_CONTINUE		0x00
#define A_LOOP          0x02
#define A_OUT           0x02
#define A_LEFT			0x02
#define	A_RIGHT			0x00
#define A_VOL			0x04
#define A_RATE			0x00
#define A_AUX			0x08
#define A_NOAUX			0x00
#define A_MAIN			0x00
#define A_MIX			0x10

extern RSP_INFO rsp;

typedef struct
{
   u32 type;
   u32 flags;

   u32 ucode_boot;
   u32 ucode_boot_size;

   u32 ucode;
   u32 ucode_size;

   u32 ucode_data;
   u32 ucode_data_size;

   u32 dram_stack;
   u32 dram_stack_size;

   u32 output_buff;
   u32 output_buff_size;

   u32 data_ptr;
   u32 data_size;

   u32 yield_data_ptr;
   u32 yield_data_size;
} OSTask_t;

void jpg_uncompress(OSTask_t *task);
/*void ucode1(OSTask_t *task);
void ucode2(OSTask_t *task);
void ucode3(OSTask_t *task);
void init_ucode1();*/
void init_ucode2();

extern u32 inst1, inst2;
extern u16 AudioInBuffer, AudioOutBuffer, AudioCount;
extern u16 AudioAuxA, AudioAuxC, AudioAuxE;
extern u32 loopval; // Value set by A_SETLOOP : Possible conflict with SETVOLUME???
//extern u32 UCData, UDataLen;

#endif
