/**
 * Mupen64 - r4300.h
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

#ifndef R4300_H
#define R4300_H

#include <stdio.h>
#include <string.h>
#include "../main/rom.h"
#include "../gc_memory/tlb.h"
#include "recomp.h"

#ifndef _BIG_ENDIAN
#define LOW_WORD(reg) (*(long*)&(reg))
#else
#define LOW_WORD(reg) (*((long*)&(reg)+1))
#endif

typedef struct {
	unsigned long pc, last_pc;		//0 -> 8
	long long int gpr[32];			//8->264
	long long int hi, lo;			//264->280
	long long int local_gpr[2];		//280->296
	unsigned long reg_cop0[32];		//296->424
	long long int fpr_data[32];		//424->680
	double*       fpr_double[32];	//680->808
	float*        fpr_single[32];	//808->936
	long          fcr0, fcr31;		//936->944
	unsigned int  next_interrupt, cic_chip;	//944->952
	unsigned long delay_slot, skip_jump;	//952->960
	int           stop, llbit;				//960->968
	tlb	          tlb_e[32];				//968->2760
	int			  vi_field;			//2760
	unsigned long next_vi;			//2764
	unsigned int  nextLRU;			//2768
	int           noCheckInterrupt; //2772
	float         two16;
} R4300;
extern R4300 r4300 __attribute__((section(".sbss")));	//Why?

#define local_rs (r4300.local_gpr[0])
#define local_rt (r4300.local_gpr[1])
#define local_rs32 LOW_WORD(local_rs)
#define local_rt32 LOW_WORD(local_rt)

extern precomp_instr *PC;
#ifdef PPC_DYNAREC
#include "ppc/Recompile.h"
extern PowerPC_block **const blocks;
extern PowerPC_block *actual;
#else
extern precomp_block *blocks[0x100000], *actual;
#endif

extern unsigned long jump_target;
extern unsigned long dyna_interp;
extern unsigned long long int debug_count;
extern unsigned long dynacore;
extern unsigned long interpcore;
extern int rounding_mode, trunc_mode, round_mode, ceil_mode, floor_mode;
//extern char invalid_code[0x100000];
extern unsigned long jump_to_address;
extern int no_audio_delay;
extern int no_compiled_jump;

void cpu_init(void);
void cpu_deinit(void);
void go();
void pure_interpreter();
void compare_core();
inline void jump_to_func();
void update_count();
int check_cop1_unusable();
void set_fpr_pointers(int newStatus);

#ifndef PPC_DYNAREC
#define jump_to(a) { jump_to_address = a; jump_to_func(); }
#else
void jump_to(unsigned int);
#endif

// profiling

#define GFX_SECTION 1
#define AUDIO_SECTION 2
#define COMPILER_SECTION 3
#define IDLE_SECTION 4
#define TLB_SECTION 5
#define FP_SECTION 6
#define INTERP_SECTION 7
#define TRAMP_SECTION 8
#define FUNCS_SECTION 9
#define LINK_SECTION 10
#define UNLINK_SECTION 11
#define DYNAMEM_SECTION 12
#define ROMREAD_SECTION 13
#define BLOCKS_SECTION 14
#define NUM_SECTIONS 14

//#define PROFILE

#ifdef PROFILE

void start_section(int section_type);
void end_section(int section_type);
void refresh_stat();

#else

#define start_section(a)
#define end_section(a)
#define refresh_stat()

#endif

#endif

