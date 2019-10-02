/**
 * Wii64 - Wrappers.c
 * Copyright (C) 2008, 2009, 2010 Mike Slegeir
 * 
 * Interface between emulator code and recompiled code
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
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
**/

#include <stdlib.h>
#include "../../gui/DEBUG.h"
#include "../Invalid_Code.h"
#include "../../gc_memory/memory.h"
#include "../../main/ROM-Cache.h"
#include "../interupt.h"
#include "../exception.h"
#include "../r4300.h"
#include "../Recomp-Cache.h"
#include "Recompile.h"
#include "Wrappers.h"

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

extern unsigned long instructionCount;
extern void (*interp_ops[64])(void);
inline unsigned long update_invalid_addr(unsigned long addr);
unsigned int dyna_check_cop1_unusable(unsigned int, int);
unsigned int dyna_mem(unsigned int, unsigned int, int, memType, unsigned int, int);

static PowerPC_instr* link_branch = NULL;
static PowerPC_func* last_func;

/* Recompiled code stack frame:
 *  $sp+12  |
 *  $sp+8   | old cr
 *  $sp+4   | old lr
 *  $sp	    | old sp
 */

inline u32 dyna_run(PowerPC_func* func, PowerPC_instr *code){
	
	PowerPC_instr* return_addr;

	register void* r3  __asm__("r3");
	register void* r28 __asm__("r28") = &r4300;
	register void* r29 __asm__("r29") = rdram;
	register void* r30 __asm__("r30") = func;
	register void* r31 __asm__("r31") = NULL;

	end_section(TRAMP_SECTION);

	__asm__ volatile(
		"mtctr  %4        \n"
		"bl     1f        \n"
		"mflr   %3        \n"
		"lwz    %2, 12(1) \n"
		"addi   1, 1, 8   \n"
		"b      2f        \n"
		"1:               \n"
		"stwu   1, -8(1)  \n"
		"mflr   0         \n"
		"stw    0, 12(1)  \n"
		"bctr             \n"
		"2:               \n"
		: "=r" (r3), "=r" (r30), "=r" (return_addr), "=r" (link_branch)
		: "r" (code), "r" (r28), "r" (r29), "r" (r30), "r" (r31)
		: "r0", "r2", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12",
		  "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11", "fr12", "fr13",
		  "lr", "ctr", "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7", "ca");

	last_func = r30;
	link_branch = link_branch == return_addr ? NULL : link_branch - 1;

	return (u32)r3;
}

void dynarec(unsigned int address){
	while(!r4300.stop){
		//print_gecko("PC: %08X cop0[9]: %08X\r\n",address,r4300.reg_cop0[9]);
#ifdef PROFILE
		refresh_stat();
		
		start_section(TRAMP_SECTION);
#endif
		unsigned long paddr = update_invalid_addr(address);
		/*
		sprintf(txtbuffer, "trampolining to 0x%08x\n", address);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		*/
		if(!paddr){ 
			link_branch = NULL;
			address = r4300.pc;
			paddr = update_invalid_addr(address);
		}
		
		if(!blocks[address>>12]){
			blocks[address>>12] = calloc(1, sizeof(PowerPC_block));
			blocks[address>>12]->start_address = address & ~0xFFF;
			init_block(blocks[address>>12]);
		} else if(invalid_code_get(address>>12)){
			invalidate_block(blocks[address>>12]);
		}

		PowerPC_func* func = find_func(&blocks[address>>12]->funcs, address);

		if(!func || !func->code_addr[(address-func->start_addr)>>2]){
			blocks[address>>12]->mips_code = (MIPS_instr*)fast_mem_access(paddr & ~0xFFF);
#ifdef PROFILE
			start_section(COMPILER_SECTION);
#endif
			//print_gecko("Recompile %08X\r\n",address);
			func = recompile_block(blocks[address>>12], address);
#ifdef PROFILE
			end_section(COMPILER_SECTION);
#endif
		} else {
#ifdef USE_RECOMP_CACHE
			RecompCache_Update(func);
#endif
		}

		int index = (address - func->start_addr)>>2;

		// Recompute the block offset
		PowerPC_instr *code = (PowerPC_instr *)func->code_addr[index];
		
		// Create a link if possible
		if(link_branch && !func_was_freed(last_func))
			RecompCache_Link(last_func, link_branch, func, code);
		clear_freed_funcs();

		r4300.pc = address = dyna_run(func, code);
		
		if(!r4300.noCheckInterrupt){
			r4300.last_pc = r4300.pc;
			// Check for interrupts
			if(r4300.next_interrupt <= Count){
				link_branch = NULL;
				gen_interupt();
				address = r4300.pc;
			}
		}
		r4300.noCheckInterrupt = 0;
	}
	r4300.pc = address;
}

unsigned int decodeNInterpret(MIPS_instr mips, unsigned int pc,
                              int isDelaySlot){
	r4300.delay_slot = isDelaySlot; // Make sure we set r4300.delay_slot properly
	r4300.pc = pc;
#ifdef PROFILE
	start_section(INTERP_SECTION);
#endif
	prefetch_opcode(mips);
	interp_ops[MIPS_GET_OPCODE(mips)]();
#ifdef PROFILE
	end_section(INTERP_SECTION);
#endif
	r4300.delay_slot = 0;

	if(r4300.pc != pc + 4) r4300.noCheckInterrupt = 1;

	return r4300.pc != pc + 4 ? r4300.pc : 0;
}

#ifdef COMPARE_CORE
int dyna_update_count(unsigned int pc, int isDelaySlot){
#else
int dyna_update_count(unsigned int pc){
#endif
	Count += ((pc - r4300.last_pc) >> 2) * count_per_op;
	r4300.last_pc = pc;

#ifdef COMPARE_CORE
	if(isDelaySlot){
		r4300.pc = pc;
		compare_core();
	}
#endif

	return r4300.next_interrupt - Count;
}

unsigned int dyna_check_cop1_unusable(unsigned int pc, int isDelaySlot){
	// Set state so it can be recovered after exception
	r4300.delay_slot = isDelaySlot;
	r4300.pc = pc;
	// Take a FP unavailable exception
	Cause = (11 << 2) | 0x10000000;
	exception_general();
	// Reset state
	r4300.delay_slot = 0;
	r4300.noCheckInterrupt = 1;
	// Return the address to trampoline to
	return r4300.pc;
}

void invalidate_func(unsigned int addr){
	if(!invalid_code_get(addr>>12)){
		PowerPC_func* func = find_func(&blocks[addr>>12]->funcs, addr);
		if(func)
			RecompCache_Free(func->start_addr);
	}
}

unsigned int dyna_mem(unsigned int addr, unsigned int value, int count,
                      memType type, unsigned int pc, int isDelaySlot){
	int i;

	//start_section(DYNAMEM_SECTION);
	r4300.pc = pc;
	r4300.delay_slot = isDelaySlot;

	switch(type){
		case MEM_LW:
			for(i = 0; i < count; i++){
				address = addr + i*4;
				read_word_in_memory();
				if(!address) break;
				r4300.gpr[value + i] = (signed long)word;
			}
			break;
		case MEM_LWU:
			for(i = 0; i < count; i++){
				address = addr + i*4;
				read_word_in_memory();
				if(!address) break;
				r4300.gpr[value + i] = word;
			}
			break;
		case MEM_LH:
			for(i = 0; i < count; i++){
				address = addr + i*2;
				read_hword_in_memory();
				if(!address) break;
				r4300.gpr[value + i] = (signed short)hword;
			}
			break;
		case MEM_LHU:
			for(i = 0; i < count; i++){
				address = addr + i*2;
				read_hword_in_memory();
				if(!address) break;
				r4300.gpr[value + i] = hword;
			}
			break;
		case MEM_LB:
			for(i = 0; i < count; i++){
				address = addr + i;
				read_byte_in_memory();
				if(!address) break;
				r4300.gpr[value + i] = (signed char)byte;
			}
			break;
		case MEM_LBU:
			for(i = 0; i < count; i++){
				address = addr + i;
				read_byte_in_memory();
				if(!address) break;
				r4300.gpr[value + i] = byte;
			}
			break;
		case MEM_LD:
			for(i = 0; i < count; i++){
				address = addr + i*8;
				read_dword_in_memory();
				if(!address) break;
				r4300.gpr[value + i] = dword;
			}
			break;
		case MEM_LWC1:
			for(i = 0; i < count; i++){
				address = addr + i*4;
				read_word_in_memory();
				if(!address) break;
				*((long*)r4300.fpr_single[value + i*2]) = word;
			}
			break;
		case MEM_LDC1:
			for(i = 0; i < count; i++){
				address = addr + i*8;
				read_dword_in_memory();
				if(!address) break;
				*((long long*)r4300.fpr_double[value + i*2]) = dword;
			}
			break;
		case MEM_LL:
			address = addr;
			read_word_in_memory();
			if(!address) break;
			r4300.gpr[value] = (signed long)word;
			r4300.llbit = 1;
			break;
		case MEM_LWL:
			address = addr & ~3;
			read_word_in_memory();
			if(!address) break;
			if((addr & 3) == 0){
				r4300.gpr[value] = (signed long)word;
			} else {
				u32 shift = (addr & 3) * 8;
				u32 mask = 0xFFFFFFFF << shift;
				r4300.gpr[value] = (r4300.gpr[value] & ~mask) | ((word << shift) & mask);
			}
			break;
		case MEM_LWR:
			address = addr & ~3;
			read_word_in_memory();
			if(!address) break;
			if((~addr & 3) == 0){
				r4300.gpr[value] = (signed long)word;
			} else {
				u32 shift = (~addr & 3) * 8;
				u32 mask = 0xFFFFFFFF >> shift;
				r4300.gpr[value] = (r4300.gpr[value] & ~mask) | ((word >> shift) & mask);
			}
			break;
		case MEM_LDL:
			address = addr & ~7;
			read_dword_in_memory();
			if(!address) break;
			if((addr & 7) == 0){
				r4300.gpr[value] = dword;
			} else {
				u32 shift = (addr & 7) * 8;
				u64 mask = 0xFFFFFFFFFFFFFFFFLL << shift;
				r4300.gpr[value] = (r4300.gpr[value] & ~mask) | ((dword << shift) & mask);
			}
			break;
		case MEM_LDR:
			address = addr & ~7;
			read_dword_in_memory();
			if(!address) break;
			if((~addr & 7) == 0){
				r4300.gpr[value] = dword;
			} else {
				u32 shift = (~addr & 7) * 8;
				u64 mask = 0xFFFFFFFFFFFFFFFFLL >> shift;
				r4300.gpr[value] = (r4300.gpr[value] & ~mask) | ((dword >> shift) & mask);
			}
			break;
		case MEM_SW:
			for(i = 0; i < count; i++){
				address = addr + i*4;
				word = r4300.gpr[value + i];
				write_word_in_memory();
			}
			invalidate_func(addr);
			break;
		case MEM_SH:
			for(i = 0; i < count; i++){
				address = addr + i*2;
				hword = r4300.gpr[value + i];
				write_hword_in_memory();
			}
			invalidate_func(addr);
			break;
		case MEM_SB:
			for(i = 0; i < count; i++){
				address = addr + i;
				byte = r4300.gpr[value + i];
				write_byte_in_memory();
			}
			invalidate_func(addr);
			break;
		case MEM_SD:
			for(i = 0; i < count; i++){
				address = addr + i*8;
				dword = r4300.gpr[value + i];
				write_dword_in_memory();
			}
			invalidate_func(addr);
			break;
		case MEM_SWC1:
			for(i = 0; i < count; i++){
				address = addr + i*4;
				word = *((long*)r4300.fpr_single[value + i*2]);
				write_word_in_memory();
			}
			invalidate_func(addr);
			break;
		case MEM_SDC1:
			for(i = 0; i < count; i++){
				address = addr + i*8;
				dword = *((long long*)r4300.fpr_double[value + i*2]);
				write_dword_in_memory();
			}
			invalidate_func(addr);
			break;
		case MEM_SC:
			if(r4300.llbit){
				address = addr;
				word = r4300.gpr[value];
				write_word_in_memory();
				invalidate_func(addr);
			}
			r4300.gpr[value] = !!r4300.llbit;
			r4300.llbit = 0;
			break;
		case MEM_SWL:
			address = addr & ~3;
			read_word_in_memory();
			if((addr & 3) == 0){
				word = r4300.gpr[value];
			} else {
				u32 shift = (addr & 3) * 8;
				u32 mask = 0xFFFFFFFF >> shift;
				word = (word & ~mask) | ((r4300.gpr[value] >> shift) & mask);
			}
			write_word_in_memory();
			invalidate_func(addr);
			break;
		case MEM_SWR:
			address = addr & ~3;
			read_word_in_memory();
			if((~addr & 3) == 0){
				word = r4300.gpr[value];
			} else {
				u32 shift = (~addr & 3) * 8;
				u32 mask = 0xFFFFFFFF << shift;
				word = (word & ~mask) | ((r4300.gpr[value] << shift) & mask);
			}
			write_word_in_memory();
			invalidate_func(addr);
			break;
		case MEM_SDL:
			address = addr & ~7;
			read_dword_in_memory();
			if((addr & 7) == 0){
				dword = r4300.gpr[value];
			} else {
				u32 shift = (addr & 7) * 8;
				u64 mask = 0xFFFFFFFFFFFFFFFFLL >> shift;
				dword = (dword & ~mask) | ((r4300.gpr[value] >> shift) & mask);
			}
			write_dword_in_memory();
			invalidate_func(addr);
			break;
		case MEM_SDR:
			address = addr & ~7;
			read_dword_in_memory();
			if((~addr & 7) == 0){
				dword = r4300.gpr[value];
			} else {
				u32 shift = (~addr & 7) * 8;
				u64 mask = 0xFFFFFFFFFFFFFFFFLL << shift;
				dword = (dword & ~mask) | ((r4300.gpr[value] << shift) & mask);
			}
			write_dword_in_memory();
			invalidate_func(addr);
			break;
		default:
			r4300.stop = 1;
			break;
	}
	r4300.delay_slot = 0;

	if(r4300.pc != pc) r4300.noCheckInterrupt = 1;
	//end_section(DYNAMEM_SECTION);
	return r4300.pc != pc ? r4300.pc : 0;
}

