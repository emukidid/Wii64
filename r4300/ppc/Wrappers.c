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
#include "../ARAM-blocks.h"
#include "../../gc_memory/memory.h"
#include "../interupt.h"
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
unsigned int dyna_mem(unsigned int, unsigned int, memType, unsigned int, int);

int noCheckInterrupt = 0;

static PowerPC_instr* link_branch = NULL;
static PowerPC_func* last_func;

/* Recompiled code stack frame:
 *  $sp+12  |
 *  $sp+8   | old cr
 *  $sp+4   | old lr
 *  $sp	    | old sp
 */

inline unsigned int dyna_run(PowerPC_func* func, unsigned int (*code)(void)){
	unsigned int naddr;
	PowerPC_instr* return_addr;

	__asm__ volatile(
		// Create the stack frame for code
		"stwu	1, -16(1) \n"
		"mfcr	14        \n"
		"stw	14, 8(1)  \n"
		// Setup saved registers for code
		"mr	14, %0    \n"
		"mr	15, %1    \n"
		"mr	16, %2    \n"
		"addi	17, 0, 0  \n"
		"mr	18, %3    \n"
		"mr	19, %4    \n"
		:: "r" (&r4300),
		   "r" (((u32)(&rdram)&0x17FFFFF)),
		   "r" (func),
		   "r" (invalid_code),
#ifdef USE_EXPANSION
		   "r" (0x80800000)
#else
		   "r" (0x80400000)
#endif
		: "14", "15", "16", "17", "18", "19", "20");

#ifdef PROFILE
	end_section(TRAMP_SECTION);
#endif

	// naddr = code();
	__asm__ volatile(
		// Save the lr so the recompiled code won't have to
		"bl	4         \n"
		"mtctr	%4        \n"
		"mflr	4         \n"
		"addi	4, 4, 20  \n"
		"stw	4, 20(1)  \n"
		// Execute the code
		"bctrl           \n"
		"mr	%0, 3     \n"
		// Get return_addr, link_branch, and last_func
		"lwz	%2, 20(1) \n"
		"mflr	%1        \n"
		"mr	%3, 16    \n"
		// Pop the stack
		"lwz	1, 0(1)   \n"
		: "=r" (naddr), "=r" (link_branch), "=r" (return_addr),
		  "=r" (last_func)
		: "r" (code)
		: "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "16");

	link_branch = link_branch == return_addr ? NULL : link_branch - 1;
	
	return naddr;
}

void dynarec(unsigned int address){
	while(!r4300.stop){
#ifdef PROFILE
		refresh_stat();
		
		start_section(TRAMP_SECTION);
#endif
		PowerPC_block* dst_block = blocks_get(address>>12);
		unsigned long paddr = update_invalid_addr(address);
		/*
		sprintf(txtbuffer, "trampolining to 0x%08x\n", address);
		DEBUG_print(txtbuffer, DBG_USBGECKO);
		*/
		if(!paddr){ 
			address = paddr = update_invalid_addr(r4300.pc);
			dst_block = blocks_get(address>>12); 
		}
		
		if(!dst_block){
			/*sprintf(txtbuffer, "block at %08x doesn't exist\n", address&~0xFFF);
			DEBUG_print(txtbuffer, DBG_USBGECKO);*/
			dst_block = malloc(sizeof(PowerPC_block));
			blocks_set(address>>12, dst_block);
			//dst_block->code_addr     = NULL;
			dst_block->funcs         = NULL;
			dst_block->start_address = address & ~0xFFF;
			dst_block->end_address   = (address & ~0xFFF) + 0x1000;
			if((paddr >= 0xb0000000 && paddr < 0xc0000000) ||
			   (paddr >= 0x90000000 && paddr < 0xa0000000)){
				init_block(NULL, dst_block);
			} else {
				init_block(rdram+(((paddr-(address-dst_block->start_address)) & 0x1FFFFFFF)>>2),
						   dst_block);
			}
		} else if(invalid_code_get(address>>12)){
			invalidate_block(dst_block);
		}

		PowerPC_func* func = find_func(&dst_block->funcs, address);

		if(!func || !func->code_addr[(address-func->start_addr)>>2]){
			/*sprintf(txtbuffer, "code at %08x is not compiled\n", address);
			DEBUG_print(txtbuffer, DBG_USBGECKO);*/
			if((paddr >= 0xb0000000 && paddr < 0xc0000000) ||
			   (paddr >= 0x90000000 && paddr < 0xa0000000))
				dst_block->mips_code =
					ROMCache_pointer((paddr-(address-dst_block->start_address))&0x0FFFFFFF);
			start_section(COMPILER_SECTION);
			func = recompile_block(dst_block, address);
			end_section(COMPILER_SECTION);
		} else {
#ifdef USE_RECOMP_CACHE
			RecompCache_Update(func);
#endif
		}

		int index = (address - func->start_addr)>>2;

		// Recompute the block offset
		unsigned int (*code)(void);
		code = (unsigned int (*)(void))func->code_addr[index];
		
		// Create a link if possible
		if(link_branch && !func_was_freed(last_func))
			RecompCache_Link(last_func, link_branch, func, code);
		clear_freed_funcs();
		
		r4300.pc = address = dyna_run(func, code);

		if(!noCheckInterrupt){
			r4300.last_pc = r4300.pc;
			// Check for interrupts
			if(r4300.next_interrupt <= Count){
				gen_interupt();
				address = r4300.pc;
			}
		}
		noCheckInterrupt = 0;
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

	if(r4300.pc != pc + 4) noCheckInterrupt = 1;

	return r4300.pc != pc + 4 ? r4300.pc : 0;
}

#ifdef COMPARE_CORE
int dyna_update_count(unsigned int pc, int isDelaySlot){
#else
int dyna_update_count(unsigned int pc){
#endif
	Count += (pc - r4300.last_pc)/2;
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
	noCheckInterrupt = 1;
	// Return the address to trampoline to
	return r4300.pc;
}

void invalidate_func(unsigned int addr){
	PowerPC_block* block = blocks_get(addr>>12);
	PowerPC_func* func = find_func(&block->funcs, addr);
	if(func)
		RecompCache_Free(func->start_addr);
}

#define check_memory(address) \
	if(!invalid_code_get(address>>12)/* && \
	   blocks[address>>12]->code_addr[(address&0xfff)>>2]*/) \
		invalidate_func(address);

unsigned int dyna_mem(unsigned int value, unsigned int addr,
                      memType type, unsigned int pc, int isDelaySlot){
	//start_section(DYNAMEM_SECTION);
	r4300.pc = pc;
	r4300.delay_slot = isDelaySlot;

	switch(type){
		case MEM_LWR:
		{
			u32 rtype = addr & 3;
			addr &= 0xFFFFFFFC;
			if(likely((rtype == 3))) {
				r4300.gpr[value] = (long long)((long)read_word_in_memory(addr, 0));
			}
			else {
				unsigned long word = read_word_in_memory(addr, 0);
				switch(rtype) {
					case 0:
						r4300.gpr[value] = (long)(r4300.gpr[value] & 0xFFFFFFFFFFFFFF00LL) | ((word >> 24) & 0xFF);
						break;
					case 1:
						r4300.gpr[value] = (long)(r4300.gpr[value] & 0xFFFFFFFFFFFF0000LL) | ((word >> 16) & 0xFFFF);
						break;
					case 2:
						r4300.gpr[value] = (long)(r4300.gpr[value] & 0xFFFFFFFFFF000000LL) | ((word >> 8) & 0xFFFFFF);
						break;
				}
			}
		}
		break;
		case MEM_LWL:
		{
			u32 ltype = (addr) & 3;
			if(likely(!ltype)) {
				r4300.gpr[value] = (long long)((long)read_word_in_memory(addr, 0));
			}
			else {
				addr &= 0xFFFFFFFC;
				unsigned long word = read_word_in_memory(addr, 0);
				switch(ltype) {
					case 1:
						r4300.gpr[value] = (long long)((long)(r4300.gpr[value] & 0xFF) | (word << 8));
					break;
					case 2:
						r4300.gpr[value] = (long long)((long)(r4300.gpr[value] & 0xFFFF) | (word << 16));
					break;
					case 3:
						r4300.gpr[value] = (long long)((long)(r4300.gpr[value] & 0xFFFFFF) | (word << 24));
					break;
				}
			}
		}
		break;
		case MEM_LW:
			r4300.gpr[value] = (long long)((long)read_word_in_memory(addr, 0));
			break;
		case MEM_LWU:
			r4300.gpr[value] = (unsigned long long)((long)read_word_in_memory(addr, 0));
			break;
		case MEM_LH:
			r4300.gpr[value] = (long long)((short)read_hword_in_memory(addr, 0));
			break;
		case MEM_LHU:
			r4300.gpr[value] = (unsigned long long)((unsigned short)read_hword_in_memory(addr, 0));
			break;
		case MEM_LB:
			r4300.gpr[value] = (long long)((signed char)read_byte_in_memory(addr, 0));
			break;
		case MEM_LBU:
			r4300.gpr[value] = (unsigned long long)((unsigned char)read_byte_in_memory(addr, 0));
			break;
		case MEM_LD:
			r4300.gpr[value] = read_dword_in_memory(addr, 0);
			break;
		case MEM_LWC1:
			*((long*)r4300.fpr_single[value]) = (long)read_word_in_memory(addr, 0);
			break;
		case MEM_LDC1:
			*((long long*)r4300.fpr_double[value]) = read_dword_in_memory(addr, 0);
			break;
		case MEM_SW:
			write_word_in_memory(addr, value);
			check_memory(addr);
			break;
		case MEM_SH:
			write_hword_in_memory(addr, value);
			check_memory(addr);
			break;
		case MEM_SB:
			write_byte_in_memory(addr, value);
			check_memory(addr);
			break;
		case MEM_SD:
			write_dword_in_memory(addr, r4300.gpr[value]);
			check_memory(addr);
			break;
		case MEM_SWC1:
			write_word_in_memory(addr, *((long*)r4300.fpr_single[value]));
			check_memory(addr);
			break;
		case MEM_SDC1:
			write_dword_in_memory(addr, *((unsigned long long*)r4300.fpr_double[value]));
			check_memory(addr);
			break;
		default:
			r4300.stop = 1;
			break;
	}
	r4300.delay_slot = 0;

	if(r4300.pc != pc) noCheckInterrupt = 1;
	//end_section(DYNAMEM_SECTION);
	return r4300.pc != pc ? r4300.pc : 0;
}

unsigned int dyna_mem_write_byte(unsigned long value, unsigned int addr,
								unsigned long pc, int isDelaySlot) {
	r4300.pc = pc;
	r4300.delay_slot = isDelaySlot;
	write_byte_in_memory(addr, value);
	check_memory(addr);
	r4300.delay_slot = 0;
	if(r4300.pc != pc) noCheckInterrupt = 1;
	return r4300.pc != pc ? r4300.pc : 0;
}

unsigned int dyna_mem_write_hword(unsigned long value, unsigned int addr,
								unsigned long pc, int isDelaySlot) {
	r4300.pc = pc;
	r4300.delay_slot = isDelaySlot;
	write_hword_in_memory(addr, value);
	check_memory(addr);
	r4300.delay_slot = 0;
	if(r4300.pc != pc) noCheckInterrupt = 1;
	return r4300.pc != pc ? r4300.pc : 0;
}

unsigned int dyna_mem_write_word(unsigned long value, unsigned int addr,
								unsigned long pc, int isDelaySlot) {
	r4300.pc = pc;
	r4300.delay_slot = isDelaySlot;
	write_word_in_memory(addr, value);
	check_memory(addr);
	r4300.delay_slot = 0;
	if(r4300.pc != pc) noCheckInterrupt = 1;
	return r4300.pc != pc ? r4300.pc : 0;
}

