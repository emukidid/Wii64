/**
 * Mupen64 - macros.h
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

#ifndef MACROS_H
#define MACROS_H

#define sign_extended(a) a = (long long)((signed long)a)
#define sign_extendedb(a) a = (long long)((signed char)a)
#define sign_extendedh(a) a = (long long)((signed short)a)

#define rrt *PC->f.r.rt
#define rrd *PC->f.r.rd
#define rfs PC->f.r.nrd
#define rrs *PC->f.r.rs
#define rsa PC->f.r.sa
#define irt *PC->f.i.rt
#define ioffset PC->f.i.immediate
#define iimmediate PC->f.i.immediate
#define lsaddr (PC-1)->f.i.immediate+(*(PC-1)->f.i.rs)
#define lsrt *(PC-1)->f.i.rt
#define irs *PC->f.i.rs
#define ibase *PC->f.i.rs
#define jinst_index PC->f.j.inst_index
#define lfbase PC->f.lf.base
#define lfft PC->f.lf.ft
#define lfoffset PC->f.lf.offset
#define lslfaddr (PC-1)->f.lf.offset+r4300.gpr[(PC-1)->f.lf.base]
#define lslfft (PC-1)->f.lf.ft
#define cfft PC->f.cf.ft
#define cffs PC->f.cf.fs
#define cffd PC->f.cf.fd

// 32 bits macros
#define rrt32 LOW_WORD(*PC->f.r.rt)
#define rrd32 LOW_WORD(*PC->f.r.rd)
#define rrs32 LOW_WORD(*PC->f.r.rs)
#define irs32 LOW_WORD(*PC->f.i.rs)
#define irt32 LOW_WORD(*PC->f.i.rt)

#define check_PC \
if (PC->addr == actual->fin) \
{ \
printf("changement de block\n"); \
stop=1; \
}


//cop0 macros
#define Index r4300.reg_cop0[0]
#define Random r4300.reg_cop0[1]
#define EntryLo0 r4300.reg_cop0[2]
#define EntryLo1 r4300.reg_cop0[3]
#define Context r4300.reg_cop0[4]
#define PageMask r4300.reg_cop0[5]
#define Wired r4300.reg_cop0[6]
#define BadVAddr r4300.reg_cop0[8]
#define Count r4300.reg_cop0[9]
#define EntryHi r4300.reg_cop0[10]
#define Compare r4300.reg_cop0[11]
#define Status r4300.reg_cop0[12]
#define Cause r4300.reg_cop0[13]
#define EPC r4300.reg_cop0[14]
#define PRevID r4300.reg_cop0[15]
#define Config r4300.reg_cop0[16]
#define LLAddr r4300.reg_cop0[17]
#define WatchLo r4300.reg_cop0[18]
#define WatchHi r4300.reg_cop0[19]
#define XContext r4300.reg_cop0[20]
#define PErr r4300.reg_cop0[26]
#define CacheErr r4300.reg_cop0[27]
#define TagLo r4300.reg_cop0[28]
#define TagHi r4300.reg_cop0[29]
#define ErrorEPC r4300.reg_cop0[30]

#ifdef X86
#define set_rounding() __asm__ __volatile__("fldcw %0" : : "m" (rounding_mode))
#define set_trunc() __asm__ __volatile__("fldcw %0" : : "m" (trunc_mode))
#define set_round() __asm__ __volatile__("fldcw %0" : : "m" (round_mode))
#define set_ceil() __asm__ __volatile__("fldcw %0" : : "m" (ceil_mode))
#define set_floor() __asm__ __volatile__("fldcw %0" : : "m" (floor_mode))
#else
#define set_rounding() ((void) 0)
#define set_trunc() ((void) 0)
#define set_round() ((void) 0)
#define set_ceil() ((void) 0)
#define set_floor() ((void) 0)
#endif

#endif
