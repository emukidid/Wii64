/**
 * Wii64 - MIPS-to-PPC.c
 * Copyright (C) 2007, 2008, 2009, 2010 Mike Slegeir
 * 
 * Convert MIPS code into PPC (take 2 1/2)
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

/* TODO: Optimize idle branches (generate a call to gen_interrupt)
         Optimize instruction scheduling & reduce branch instructions
 */
#include <string.h>
#include "MIPS-to-PPC.h"
#include "Register-Cache.h"
#include "Interpreter.h"
#include "Wrappers.h"
#include "../../gc_memory/memory.h"
#include <math.h>
#include "../Invalid_Code.h"

#include <assert.h>

#ifdef USE_EXPANSION
#define RAM_TOP 0x80800000
#else
#define RAM_TOP 0x80400000
#endif

// Prototypes for functions used and defined in this file
static void genCallInterp(MIPS_instr);
#define JUMPTO_REG  0
#define JUMPTO_OFF  1
#define JUMPTO_ADDR 2
#define JUMPTO_REG_SIZE  2
#define JUMPTO_OFF_SIZE  11
#define JUMPTO_ADDR_SIZE 11
static void genJumpTo(unsigned int loc, unsigned int type);
static void genUpdateCount(int checkCount);
static void genCheckFP(void);
void genRecompileLoad(memType type, MIPS_instr mips);
void genCallDynaMem(memType type, int base, short immed);
void genRecompileStore(memType type, MIPS_instr mips);
void RecompCache_Update(PowerPC_func*);
static int inline mips_is_jump(MIPS_instr);
void jump_to(unsigned int);
void check_interupt();

double __floatdidf(long long);
float __floatdisf(long long);
long long __fixdfdi(double);
long long __fixsfdi(float);

#define CANT_COMPILE_DELAY() \
	((get_src_pc()&0xFFF) == 0xFFC && \
	 (get_src_pc() <  0x80000000 || \
	  get_src_pc() >= 0xC0000000))

static inline unsigned short extractUpper16(unsigned int address){
	unsigned int addr = (unsigned int)address;
	return (addr>>16) + ((addr>>15)&1);
}

static inline short extractLower16(unsigned int address){
	unsigned int addr = (unsigned int)address;
	return addr&0x8000 ? (addr&0xffff)-0x10000 : addr&0xffff;
}

static int FP_need_check;

// Variable to indicate whether the next recompiled instruction
//   is a delay slot (which needs to have its registers flushed)
//   and the current instruction
static int delaySlotNext, isDelaySlot;
// This should be called before the jump is recompiled
static inline int check_delaySlot(void){
	if(peek_next_src() == 0){ // MIPS uses 0 as a NOP
		get_next_src();   // Get rid of the NOP
		return 0;
	} else {
		if(mips_is_jump(peek_next_src())) return CONVERT_WARNING;
		delaySlotNext = 1;
		convert(); // This just moves the delay slot instruction ahead of the branch
		return 1;
	}
}

#define MIPS_REG_HI 32
#define MIPS_REG_LO 33

// Initialize register mappings
void start_new_block(void){
	invalidateRegisters();
	// Check if the previous instruction was a branch
	//   and thus whether this block begins with a delay slot
	unget_last_src();
	if(mips_is_jump(get_next_src())) delaySlotNext = 2;
	else delaySlotNext = 0;
}
void start_new_mapping(void){
	flushRegisters();
	FP_need_check = 1;
	reset_code_addr();
}

static inline int signExtend(int value, int size){
	int signMask = 1 << (size-1);
	int negMask = 0xffffffff << (size-1);
	if(value & signMask) value |= negMask;
	return value;
}

static void genCmp64(int cr, int _ra, int _rb){

	if(getRegisterMapping(_ra) == MAPPING_32 ||
	   getRegisterMapping(_rb) == MAPPING_32){
		// Here we cheat a little bit: if either of the registers are mapped
		// as 32-bit, only compare the 32-bit values
		int ra = mapRegister(_ra), rb = mapRegister(_rb);
		
		GEN_CMP(ra, rb, 4);
	} else {
		RegMapping ra = mapRegister64(_ra), rb = mapRegister64(_rb);
		
		GEN_CMP(ra.hi, rb.hi, 4);
		// Skip low word comparison if high words are mismatched
		GEN_BNE(4, 2, 0, 0);
		// Compare low words if r4300.hi words don't match
		GEN_CMPL(ra.lo, rb.lo, 4);
	}
}

static void genCmpi64(int cr, int _ra, short immed){
	
	if(getRegisterMapping(_ra) == MAPPING_32){
		// If we've mapped this register as 32-bit, don't bother with 64-bit
		int ra = mapRegister(_ra);
		
		GEN_CMPI(ra, immed, 4);
	} else {
		RegMapping ra = mapRegister64(_ra);
		
		GEN_CMPI(ra.hi, (immed&0x8000) ? ~0 : 0, 4);
		// Skip low word comparison if high words are mismatched
		GEN_BNE(4, 2, 0, 0);
		// Compare low words if r4300.hi words don't match
		GEN_CMPLI(ra.lo, immed, 4);
	}
}

typedef enum { NONE=0, EQ, NE, LT, GT, LE, GE } condition;
// Branch a certain offset (possibly conditionally, linking, or likely)
//   offset: N64 instructions from current N64 instruction to branch
//   cond: type of branch to execute depending on cr 7
//   link: if nonzero, branch and link
//   likely: if nonzero, the delay slot will only be executed when cond is true
static int branch(int offset, condition cond, int link, int likely, int fp){

	int likely_id;
	// Condition codes for bc (and their negations)
	int bo, bi, nbo;
	switch(cond){
		case EQ:
			bo = 0xc, nbo = 0x4, bi = 18;
			break;
		case NE:
			bo = 0x4, nbo = 0xc, bi = 18;
			break;
		case LT:
			bo = 0xc, nbo = 0x4, bi = 16;
			break;
		case GE:
			bo = 0x4, nbo = 0xc, bi = 16;
			break;
		case GT:
			bo = 0xc, nbo = 0x4, bi = 17;
			break;
		case LE:
			bo = 0x4, nbo = 0xc, bi = 17;
			break;
		default:
			bo = 0x14; nbo = 0x4; bi = 19;
			break;
	}

	flushRegisters();

	if(link){
		// Set LR to next instruction
		int lr = mapRegisterNew(MIPS_REG_LR);
		// lis	lr, pc@ha(0)
		GEN_LIS(lr, extractUpper16(get_src_pc()+8));
		// la	lr, pc@l(lr)
		GEN_ADDI(lr, lr, extractLower16(get_src_pc()+8));
		flushRegisters();
	}

	if(likely){
		if(fp){
			// This has to be done here because comparisons can happen in the delay slot
			GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
			GEN_RLWINM(0, 0, 9, 31, 31);
			GEN_CMPI(0, 0, 4);
		}
		// b[!cond] <past delay to update_count>
		likely_id = add_jump_special(0);
		GEN_BC(likely_id, 0, 0, nbo, bi);
	}

	// Check the delay slot, and note how big it is
	PowerPC_instr* preDelay = get_curr_dst();
	check_delaySlot();
	int delaySlot = get_curr_dst() - preDelay;

#ifdef COMPARE_CORE
	GEN_LI(4, 0, 1);
	if(likely){
		GEN_B(2, 0, 0);
		GEN_LI(4, 0, 0);

		set_jump_special(likely_id, delaySlot+2+1);
	}
#else
	if(likely) set_jump_special(likely_id, delaySlot+1);
#endif

	genUpdateCount(1); // Sets cr2 to (r4300.next_interrupt ? Count)

	if(fp && !likely){
		// This has to be done here because comparisons can happen in the delay slot
		GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
		GEN_RLWINM(0, 0, 9, 31, 31);
		GEN_CMPI(0, 0, 4);
	}

#ifndef INTERPRET_BRANCH
	// If we're jumping out, we need to trampoline using genJumpTo
	if(is_j_out(offset, 0)){
#endif // INTEPRET_BRANCH

		// b[!cond] <past jumpto & delay>
		//   Note: if there's a delay slot, I will branch to the branch over it
		GEN_BC(JUMPTO_OFF_SIZE+1, 0, 0, nbo, bi);

		genJumpTo(offset, JUMPTO_OFF);

		// The branch isn't taken, but we need to check interrupts
		// Load the address of the next instruction
		GEN_LIS(3, extractUpper16(get_src_pc()+4));
		GEN_ADDI(3, 3, extractLower16(get_src_pc()+4));
		// If taking the interrupt, return to the trampoline
		GEN_BLELR(2, 0);

#ifndef INTERPRET_BRANCH
	} else {
		// r4300.last_pc = naddr
		if(cond != NONE){
			GEN_BC(4, 0, 0, bo, bi);
			GEN_LIS(3, extractUpper16(get_src_pc()+4));
			GEN_ADDI(3, 3, extractLower16(get_src_pc()+4));
			GEN_B(3, 0, 0);
		}
		GEN_LIS(3, extractUpper16(get_src_pc() + (offset<<2)));
		GEN_ADDI(3, 3, extractLower16(get_src_pc() + (offset<<2)));
		GEN_STW(3, 0+R4300OFF_LADDR, DYNAREG_R4300);

		// If taking the interrupt, return to the trampoline
		GEN_BLELR(2, 0);
		// The actual branch
#if 0
		// FIXME: Reenable this when blocks are small enough to BC within
		//          Make sure that pass2 uses BD/LI as appropriate
		GEN_BC(add_jump(offset, 0, 0), 0, 0, bo, bi);
#else
		GEN_BC(2, 0, 0, nbo, bi);
		GEN_B(add_jump(offset, 0, 0), 0, 0);
#endif

	}
#endif // INTERPRET_BRANCH

	// Let's still recompile the delay slot in place in case its branched to
	// Unless the delay slot is in the next block, in which case there's nothing to skip
	//   Testing is_j_out with an offset of 0 checks whether the delay slot is out
	if(delaySlot){
		if(is_j_dst() && !is_j_out(0, 0)){
			// Step over the already executed delay slot if the branch isn't taken
			// b delaySlot+1
			GEN_B(delaySlot+1, 0, 0);
			unget_last_src();
			delaySlotNext = 2;
		}
	} else nop_ignored();

#ifdef INTERPRET_BRANCH
	return INTERPRETED;
#else // INTERPRET_BRANCH
	return CONVERT_SUCCESS;
#endif
}


static int (*gen_ops[64])(MIPS_instr);

int convert(void){
	int needFlush = delaySlotNext;
	isDelaySlot = (delaySlotNext == 1);
	delaySlotNext = 0;

	MIPS_instr mips = get_next_src();
	int result = gen_ops[MIPS_GET_OPCODE(mips)](mips);

	if(needFlush) flushRegisters();
	return result;
}

static int NI(){
	return CONVERT_ERROR;
}

// -- Primary Opcodes --

static int J(MIPS_instr mips){
	unsigned int naddr = (MIPS_GET_LI(mips)<<2)|((get_src_pc()+4)&0xf0000000);

	if(naddr == get_src_pc() || CANT_COMPILE_DELAY()){
		// J_IDLE || virtual delay
		genCallInterp(mips);
		return INTERPRETED;
	}

	flushRegisters();
	reset_code_addr();

	// Check the delay slot, and note how big it is
	PowerPC_instr* preDelay = get_curr_dst();
	check_delaySlot();
	int delaySlot = get_curr_dst() - preDelay;

#ifdef COMPARE_CORE
	GEN_LI(4, 0, 1);
#endif
	// Sets cr2 to (r4300.next_interrupt ? Count)
	genUpdateCount(1);

#ifdef INTERPRET_J
	genJumpTo(MIPS_GET_LI(mips), JUMPTO_ADDR);
#else // INTERPRET_J
	// If we're jumping out, we can't just use a branch instruction
	if(is_j_out(MIPS_GET_LI(mips), 1)){
		genJumpTo(MIPS_GET_LI(mips), JUMPTO_ADDR);
	} else {
		// r4300.last_pc = naddr
		GEN_LIS(3, extractUpper16(naddr));
		GEN_ADDI(3, 3, extractLower16(naddr));
		GEN_STW(3, 0+R4300OFF_LADDR, DYNAREG_R4300);

		// if(r4300.next_interrupt <= Count) return;
		GEN_BLELR(2, 0);

		// Even though this is an absolute branch
		//   in pass 2, we generate a relative branch
		GEN_B(add_jump(MIPS_GET_LI(mips), 1, 0), 0, 0);
	}
#endif

	// Let's still recompile the delay slot in place in case its branched to
	if(delaySlot){ if(is_j_dst()){ unget_last_src(); delaySlotNext = 2; } }
	else nop_ignored();

#ifdef INTERPRET_J
	return INTERPRETED;
#else // INTERPRET_J
	return CONVERT_SUCCESS;
#endif
}

static int JAL(MIPS_instr mips){
	unsigned int naddr = (MIPS_GET_LI(mips)<<2)|((get_src_pc()+4)&0xf0000000);

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	flushRegisters();
	reset_code_addr();

	// Check the delay slot, and note how big it is
	PowerPC_instr* preDelay = get_curr_dst();
	check_delaySlot();
	int delaySlot = get_curr_dst() - preDelay;

#ifdef COMPARE_CORE
	GEN_LI(4, 0, 1);
#endif
	// Sets cr2 to (r4300.next_interrupt ? Count)
	genUpdateCount(1);

	// Set LR to next instruction
	int lr = mapRegisterNew(MIPS_REG_LR);
	// lis	lr, pc@ha(0)
	GEN_LIS(lr, extractUpper16(get_src_pc()+4));
	// la	lr, pc@l(lr)
	GEN_ADDI(lr, lr, extractLower16(get_src_pc()+4));

	flushRegisters();

#ifdef INTERPRET_JAL
	genJumpTo(MIPS_GET_LI(mips), JUMPTO_ADDR);
#else // INTERPRET_JAL
	// If we're jumping out, we can't just use a branch instruction
	if(is_j_out(MIPS_GET_LI(mips), 1)){
		genJumpTo(MIPS_GET_LI(mips), JUMPTO_ADDR);
	} else {
		// r4300.last_pc = naddr
		GEN_LIS(3, extractUpper16(naddr));
		GEN_ADDI(3, 3, extractLower16(naddr));
		GEN_STW(3, 0+R4300OFF_LADDR, DYNAREG_R4300);

		/// if(r4300.next_interrupt <= Count) return;
		GEN_BLELR(2, 0);

		// Even though this is an absolute branch
		//   in pass 2, we generate a relative branch
		GEN_B(add_jump(MIPS_GET_LI(mips), 1, 0), 0, 0);
	}
#endif

	// Let's still recompile the delay slot in place in case its branched to
	if(delaySlot){ if(is_j_dst()){ unget_last_src(); delaySlotNext = 2; } }
	else nop_ignored();

#ifdef INTERPRET_JAL
	return INTERPRETED;
#else // INTERPRET_JAL
	return CONVERT_SUCCESS;
#endif
}

static int BEQ(MIPS_instr mips){

	if((MIPS_GET_IMMED(mips) == 0xffff &&
	    MIPS_GET_RA(mips) == MIPS_GET_RB(mips)) ||
	   CANT_COMPILE_DELAY()){
		// BEQ_IDLE || virtual delay
		genCallInterp(mips);
		return INTERPRETED;
	}
	
	genCmp64(4, MIPS_GET_RA(mips), MIPS_GET_RB(mips));

	return branch(signExtend(MIPS_GET_IMMED(mips),16), EQ, 0, 0, 0);
}

static int BNE(MIPS_instr mips){

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmp64(4, MIPS_GET_RA(mips), MIPS_GET_RB(mips));

	return branch(signExtend(MIPS_GET_IMMED(mips),16), NE, 0, 0, 0);
}

static int BLEZ(MIPS_instr mips){

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(4, MIPS_GET_RA(mips), 0);

	return branch(signExtend(MIPS_GET_IMMED(mips),16), LE, 0, 0, 0);
}

static int BGTZ(MIPS_instr mips){

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(4, MIPS_GET_RA(mips), 0);

	return branch(signExtend(MIPS_GET_IMMED(mips),16), GT, 0, 0, 0);
}

static int ADDIU(MIPS_instr mips){
	int _rs = MIPS_GET_RS(mips), _rt = MIPS_GET_RT(mips);
	int rs = mapRegister(_rs);
	int rt = mapConstantNew(_rt, isRegisterConstant(_rs));
	
	int immediate = MIPS_GET_IMMED(mips);
	immediate |= (immediate&0x8000) ? 0xffff0000 : 0;
	setRegisterConstant(_rt, getRegisterConstant(_rs) + immediate);
	
	GEN_ADDI(rt, rs, MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
}

static int ADDI(MIPS_instr mips){
	return ADDIU(mips);
}

static int SLTI(MIPS_instr mips){
#ifdef INTERPRET_SLTI
	genCallInterp(mips);
	return INTERPRETED;
#else
	// FIXME: Do I need to worry about 64-bit values?
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegisterNew( MIPS_GET_RT(mips) );
	int tmp = (rs == rt) ? mapRegisterTemp() : rt;

	// tmp = immed (sign extended)
	GEN_ADDI(tmp, 0, MIPS_GET_IMMED(mips));
	// carry = rs < immed ? 0 : 1 (unsigned)
	GEN_SUBFC(0, tmp, rs);
	// rt = ~(rs ^ immed)
	GEN_EQV(rt, tmp, rs);
	// rt = sign(rs) == sign(immed) ? 1 : 0
	GEN_SRWI(rt, rt, 31);
	// rt += carry
	GEN_ADDZE(rt, rt);
	// rt &= 1 ( = (sign(rs) == sign(immed)) xor (rs < immed (unsigned)) )
	GEN_RLWINM(rt, rt, 0, 31, 31);

	if(rs == rt) unmapRegisterTemp(tmp);

	return CONVERT_SUCCESS;
#endif
}

static int SLTIU(MIPS_instr mips){
#ifdef INTERPRET_SLTIU
	genCallInterp(mips);
	return INTERPRETED;
#else
	// FIXME: Do I need to worry about 64-bit values?
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegisterNew( MIPS_GET_RT(mips) );

	// r0 = EXTS(immed)
	GEN_ADDI(0, 0, MIPS_GET_IMMED(mips));
	// carry = rs < immed ? 0 : 1
	GEN_SUBFC(rt, 0, rs);
	// rt = carry - 1 ( = rs < immed ? -1 : 0 )
	GEN_SUBFE(rt, rt, rt);
	// rt = !carry ( = rs < immed ? 1 : 0 )
	GEN_NEG(rt, rt);

	return CONVERT_SUCCESS;
#endif
}

static int ANDI(MIPS_instr mips){
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegisterNew( MIPS_GET_RT(mips) );

	GEN_ANDI(rt, rs, MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
}

static int ORI(MIPS_instr mips){
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64New( MIPS_GET_RT(mips) );

	GEN_OR(rt.hi, rs.hi, rs.hi);
	GEN_ORI(rt.lo, rs.lo, MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
}

static int XORI(MIPS_instr mips){
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64New( MIPS_GET_RT(mips) );

	GEN_OR(rt.hi, rs.hi, rs.hi);
	GEN_XORI(rt.lo, rs.lo, MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
}

static int LUI(MIPS_instr mips){
	int rt = mapConstantNew( MIPS_GET_RT(mips), 1 );
	setRegisterConstant(MIPS_GET_RT(mips), MIPS_GET_IMMED(mips) << 16);
	
	GEN_LIS(rt, MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
}

static int BEQL(MIPS_instr mips){

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmp64(4, MIPS_GET_RA(mips), MIPS_GET_RB(mips));

	return branch(signExtend(MIPS_GET_IMMED(mips),16), EQ, 0, 1, 0);
}

static int BNEL(MIPS_instr mips){

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmp64(4, MIPS_GET_RA(mips), MIPS_GET_RB(mips));

	return branch(signExtend(MIPS_GET_IMMED(mips),16), NE, 0, 1, 0);
}

static int BLEZL(MIPS_instr mips){

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(4, MIPS_GET_RA(mips), 0);

	return branch(signExtend(MIPS_GET_IMMED(mips),16), LE, 0, 1, 0);
}

static int BGTZL(MIPS_instr mips){

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(4, MIPS_GET_RA(mips), 0);

	return branch(signExtend(MIPS_GET_IMMED(mips),16), GT, 0, 1, 0);
}

static int DADDIU(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DADDIU)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DADDIU

	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64New( MIPS_GET_RT(mips) );

	// Add the immediate to the LSW
	GEN_ADDIC(rt.lo, rs.lo, MIPS_GET_IMMED(mips));
	// Add the MSW with the sign-extension and the carry
	if(MIPS_GET_IMMED(mips)&0x8000){
		GEN_ADDME(rt.hi, rs.hi);
	} else {
		GEN_ADDZE(rt.hi, rs.hi);
	}
	return CONVERT_SUCCESS;
#endif
}

static int DADDI(MIPS_instr mips){
	return DADDIU(mips);
}

static int LDL(MIPS_instr mips){
#ifdef INTERPRET_LDL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LDL
	return CONVERT_ERROR;
#endif
}

static int LDR(MIPS_instr mips){
#ifdef INTERPRET_LDR
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LDR
	return CONVERT_ERROR;
#endif
}

static int LB(MIPS_instr mips){
#ifdef INTERPRET_LB
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LB
	genRecompileLoad(MEM_LB, mips);
	return CONVERT_SUCCESS;
#endif
}

static int LH(MIPS_instr mips){
#ifdef INTERPRET_LH
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LH
	genRecompileLoad(MEM_LH, mips);
	return CONVERT_SUCCESS;
#endif
}

static int LWL(MIPS_instr mips){
#ifdef INTERPRET_LWL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LWL
	int isConstant = isRegisterConstant( MIPS_GET_RS(mips) );
	int isPhysical = 1, isVirtual = 1;
	if(isConstant){
		int constant = getRegisterConstant( MIPS_GET_RS(mips) );
		int immediate = MIPS_GET_IMMED(mips);
		immediate |= (immediate&0x8000) ? 0xffff0000 : 0;
		
		if((constant + immediate) > (int)RAM_TOP)
			isPhysical = 0;
		else
			isVirtual = 0;
	}

	flushRegisters();
	reset_code_addr();
	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr
	invalidateRegisters();

	GEN_ADDI(base, base, MIPS_GET_IMMED(mips));	// addr = rs + immed
#ifdef FASTMEM
	if(isPhysical && isVirtual){
		// If base in physical memory
		GEN_CMP(base, DYNAREG_MEM_TOP, 1);
		GEN_BGE(1, 9, 0, 0);
		// Fast case LWL when it's not actually mis-aligned, it's just a LW()
		GEN_RLWINM(0, base, 0, 30, 31);	// r0 = addr & 3
		GEN_CMPI(0, 0, 1);
		GEN_BNE(1, 6, 0, 0);
	}
	if(isPhysical) {
		// Create a mapping for this value
		int value = mapRegisterNew( MIPS_GET_RT(mips) );
		// Perform the actual load
		GEN_LWZX(value, DYNAREG_RDRAM, base);
		flushRegisters();
	}
	
	PowerPC_instr* preCall;
	int not_fastmem_id = 0;
	if(isPhysical && isVirtual){
		// Skip over else
		not_fastmem_id = add_jump_special(1);
		GEN_B(not_fastmem_id, 0, 0);
		preCall = get_curr_dst();
	}
#endif // FASTMEM
	if(isVirtual) {
		// load into rt
		GEN_LI(rd, 0, MIPS_GET_RT(mips));
		genCallDynaMem(MEM_LWL, base, 0);
	}
#ifdef FASTMEM
	if(isPhysical && isVirtual){
		int callSize = get_curr_dst() - preCall;
		set_jump_special(not_fastmem_id, callSize+1);
	}
#endif

	return CONVERT_SUCCESS;
#endif
}

static int LW(MIPS_instr mips){
#ifdef INTERPRET_LW
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LW
	genRecompileLoad(MEM_LW, mips);
	return CONVERT_SUCCESS;
#endif
}

static int LBU(MIPS_instr mips){
#ifdef INTERPRET_LBU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LBU
	genRecompileLoad(MEM_LBU, mips);
	return CONVERT_SUCCESS;
#endif
}

static int LHU(MIPS_instr mips){
#ifdef INTERPRET_LHU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LHU
	genRecompileLoad(MEM_LHU, mips);
	return CONVERT_SUCCESS;
#endif
}

static int LWR(MIPS_instr mips){
#ifdef INTERPRET_LWR
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LWR
	int isConstant = isRegisterConstant( MIPS_GET_RS(mips) );
	int isPhysical = 1, isVirtual = 1;
	if(isConstant){
		int constant = getRegisterConstant( MIPS_GET_RS(mips) );
		int immediate = MIPS_GET_IMMED(mips);
		immediate |= (immediate&0x8000) ? 0xffff0000 : 0;
		
		if((constant + immediate) > (int)RAM_TOP)
			isPhysical = 0;
		else
			isVirtual = 0;
	}

	flushRegisters();
	reset_code_addr();
	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr
	invalidateRegisters();

	GEN_ADDI(base, base, MIPS_GET_IMMED(mips));	// addr = rs + immed
#ifdef FASTMEM
	if(isPhysical && isVirtual){
		// If base in physical memory
		GEN_CMP(base, DYNAREG_MEM_TOP, 1);
		GEN_BGE(1, 10, 0, 0);
		// Fast case LWR when it's not actually mis-aligned, it's just a LW()
		GEN_RLWINM(0, base, 0, 30, 31);	// r0 = addr & 3
		GEN_CMPI(0, 3, 1);
		GEN_BNE(1, 7, 0, 0);
	}
	if(isPhysical) {
		GEN_RLWINM(base, base, 0, 0, 29);	// addr &= 0xFFFFFFFC

		// Create a mapping for this value
		int value = mapRegisterNew( MIPS_GET_RT(mips) );
		// Perform the actual load
		GEN_LWZX(value, DYNAREG_RDRAM, base);
		flushRegisters();
	}

	PowerPC_instr* preCall;
	int not_fastmem_id = 0;
	if(isPhysical && isVirtual){
		// Skip over else
		not_fastmem_id = add_jump_special(1);
		GEN_B(not_fastmem_id, 0, 0);
		preCall = get_curr_dst();
	}
#endif // FASTMEM
	if(isVirtual) {
		// load into rt
		GEN_LI(rd, 0, MIPS_GET_RT(mips));
		genCallDynaMem(MEM_LWR, base, 0);
	}
#ifdef FASTMEM
	if(isPhysical && isVirtual){
		int callSize = get_curr_dst() - preCall;
		set_jump_special(not_fastmem_id, callSize+1);
	}
#endif

	return CONVERT_SUCCESS;
#endif
}

static int LWU(MIPS_instr mips){
#ifdef INTERPRET_LWU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LWU
	int isConstant = isRegisterConstant( MIPS_GET_RS(mips) );
	int isPhysical = 1, isVirtual = 1;
	if(isConstant){
		int constant = getRegisterConstant( MIPS_GET_RS(mips) );
		int immediate = MIPS_GET_IMMED(mips);
		immediate |= (immediate&0x8000) ? 0xffff0000 : 0;
		
		if((constant + immediate) > (int)RAM_TOP)
			isPhysical = 0;
		else
			isVirtual = 0;
	}

	flushRegisters();
	reset_code_addr();
	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr
	invalidateRegisters();

#ifdef FASTMEM
	if(isPhysical && isVirtual){
		// If base in physical memory
		GEN_CMP(base, DYNAREG_MEM_TOP, 1);
		GEN_BGE(1, 7, 0, 0);
	}

	if(isPhysical) {
		// Add rdram pointer
		GEN_ADD(base, DYNAREG_RDRAM, base);
		// Create a mapping for this value
		RegMapping value = mapRegister64New( MIPS_GET_RT(mips) );
		// Perform the actual load
		GEN_LWZ(value.lo, MIPS_GET_IMMED(mips), base);
		// Zero out the upper word
		GEN_LI(value.hi, 0, 0);
		flushRegisters();
	}

	PowerPC_instr* preCall;
	int not_fastmem_id = 0;
	if(isPhysical && isVirtual){
		// Skip over else
		not_fastmem_id = add_jump_special(1);
		GEN_B(not_fastmem_id, 0, 0);
		preCall = get_curr_dst();
	}
#endif // FASTMEM

	if(isVirtual) {
		// load into rt
		GEN_LI(rd, 0, MIPS_GET_RT(mips));
		genCallDynaMem(MEM_LWU, base, MIPS_GET_IMMED(mips));
	}
#ifdef FASTMEM
	if(isPhysical && isVirtual){
		int callSize = get_curr_dst() - preCall;
		set_jump_special(not_fastmem_id, callSize+1);
	}
#endif
	return CONVERT_SUCCESS;
#endif
}

static int SB(MIPS_instr mips){
#ifdef INTERPRET_SB
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SB
	genRecompileStore(MEM_SB, mips);
	return CONVERT_SUCCESS;
#endif
}

static int SH(MIPS_instr mips){
#ifdef INTERPRET_SH
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SH
	genRecompileStore(MEM_SH, mips);
	return CONVERT_SUCCESS;
#endif
}

static int SWL(MIPS_instr mips){
#ifdef INTERPRET_SWL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SWL
	return CONVERT_ERROR;
#endif
}

static int SW(MIPS_instr mips){
#ifdef INTERPRET_SW
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SW
	genRecompileStore(MEM_SW, mips);
	return CONVERT_SUCCESS;
#endif
}

static int SDL(MIPS_instr mips){
#ifdef INTERPRET_SDL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SDL
	return CONVERT_ERROR;
#endif
}

static int SDR(MIPS_instr mips){
#ifdef INTERPRET_SDR
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SDR
	return CONVERT_ERROR;
#endif
}

static int SWR(MIPS_instr mips){
#ifdef INTERPRET_SWR
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SWR
	return CONVERT_ERROR;
#endif
}

static int LD(MIPS_instr mips){
#ifdef INTERPRET_LD
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LD

	flushRegisters();
	reset_code_addr();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

#ifdef FASTMEM
	// If base in physical memory
	GEN_CMP(base, DYNAREG_MEM_TOP, 1);
	GEN_BGE(1, 7, 0, 0);

	// Add rdram pointer
	GEN_ADD(base, DYNAREG_RDRAM, base);
	// Create a mapping for this value
	RegMapping value = mapRegister64New( MIPS_GET_RT(mips) );
	// Perform the actual load
	GEN_LWZ(value.lo, MIPS_GET_IMMED(mips)+4, base);
	GEN_LWZ(value.hi, MIPS_GET_IMMED(mips), base);
	// Write the value out to reg
	flushRegisters();
	// Skip over else
	int not_fastmem_id = add_jump_special(1);
	GEN_B(not_fastmem_id, 0, 0);
	PowerPC_instr* preCall = get_curr_dst();
#endif // FASTMEM

	// load into rt
	GEN_LI(rd, 0, MIPS_GET_RT(mips));
	genCallDynaMem(MEM_LD, base, MIPS_GET_IMMED(mips));

#ifdef FASTMEM
	int callSize = get_curr_dst() - preCall;
	set_jump_special(not_fastmem_id, callSize+1);
#endif
	return CONVERT_SUCCESS;
#endif
}

static int SD(MIPS_instr mips){
#ifdef INTERPRET_SD
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SD

	flushRegisters();
	reset_code_addr();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

	// store from rt
	GEN_LI(rd, 0, MIPS_GET_RT(mips));
	genCallDynaMem(MEM_SD, base, MIPS_GET_IMMED(mips));
	return CONVERT_SUCCESS;
#endif
}

static int LWC1(MIPS_instr mips){
#ifdef INTERPRET_LWC1
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LWC1

	int isConstant = isRegisterConstant( MIPS_GET_RS(mips) );
	int isPhysical = 1, isVirtual = 1;
	if(isConstant){
		int constant = getRegisterConstant( MIPS_GET_RS(mips) );
		int immediate = MIPS_GET_IMMED(mips);
		immediate |= (immediate&0x8000) ? 0xffff0000 : 0;
		
		if((constant + immediate) > (int)RAM_TOP)
			isPhysical = 0;
		else
			isVirtual = 0;
	}

	flushRegisters();
	reset_code_addr();
	genCheckFP();
	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr
	int addr = mapRegisterTemp(); // r5 = fpr_addr
	invalidateRegisters();
	
#ifdef FASTMEM
	if(isVirtual && isPhysical) {
		// If base in physical memory
		GEN_CMP(base, DYNAREG_MEM_TOP, 1);
		GEN_BGE(1, 6, 0, 0);
	}
	if(isPhysical) {
		// Add rdram pointer
		GEN_ADD(base, DYNAREG_RDRAM, base);
		// Perform the actual load
		GEN_LWZ(rd, MIPS_GET_IMMED(mips), base);
		// addr = r4300.fpr_single[frt]
		GEN_LWZ(addr, (MIPS_GET_RT(mips)*4)+R4300OFF_FPR_32, DYNAREG_R4300);
		// *addr = frs
		GEN_STW(rd, 0, addr);
	}
	PowerPC_instr* preCall;
	int not_fastmem_id = 0;
	if(isPhysical && isVirtual){
		// Skip over else
		not_fastmem_id = add_jump_special(1);
		GEN_B(not_fastmem_id, 0, 0);
		preCall = get_curr_dst();
	}
#endif // FASTMEM
	if(isVirtual) {
		// load into frt
		GEN_LI(rd, 0, MIPS_GET_RT(mips));
		genCallDynaMem(MEM_LWC1, base, MIPS_GET_IMMED(mips));
	}
#ifdef FASTMEM
	if(isVirtual && isPhysical) {
		int callSize = get_curr_dst() - preCall;
		set_jump_special(not_fastmem_id, callSize+1);
	}
#endif

	return CONVERT_SUCCESS;
#endif
}

static int LDC1(MIPS_instr mips){
#ifdef INTERPRET_LDC1
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LDC1

	flushRegisters();
	reset_code_addr();
	
	genCheckFP();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr
	int addr = mapRegisterTemp(); // r5 = fpr_addr

	invalidateRegisters();

#ifdef FASTMEM
	// If base in physical memory
	GEN_CMP(base, DYNAREG_MEM_TOP, 1);
	GEN_BGE(1, 8, 0, 0);
	
	// Add rdram pointer
	GEN_ADD(base, DYNAREG_RDRAM, base);
	// Perform the actual load
	GEN_LWZ(rd, MIPS_GET_IMMED(mips), base);
	GEN_LWZ(6, MIPS_GET_IMMED(mips)+4, base);
	// addr = r4300.fpr_double[frt]
	GEN_LWZ(addr, (MIPS_GET_RT(mips)*4)+R4300OFF_FPR_64, DYNAREG_R4300);
	// *addr = frs
	GEN_STW(rd, 0, addr);
	GEN_STW(6, 4, addr);
	// Skip over else
	int not_fastmem_id = add_jump_special(1);
	GEN_B(not_fastmem_id, 0, 0);
	PowerPC_instr* preCall = get_curr_dst();
#endif // FASTMEM

	// load into frt
	GEN_LI(rd, 0, MIPS_GET_RT(mips));
	genCallDynaMem(MEM_LDC1, base, MIPS_GET_IMMED(mips));

#ifdef FASTMEM
	int callSize = get_curr_dst() - preCall;
	set_jump_special(not_fastmem_id, callSize+1);
#endif

	return CONVERT_SUCCESS;
#endif
}

static int SWC1(MIPS_instr mips){
#ifdef INTERPRET_SWC1
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SWC1
	genRecompileStore(MEM_SWC1, mips);
	return CONVERT_SUCCESS;
#endif
}

static int SDC1(MIPS_instr mips){
#ifdef INTERPRET_SDC1
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SDC1

	flushRegisters();
	reset_code_addr();

	genCheckFP();

	int rd = mapRegisterTemp(); // r3 = rd
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr

	invalidateRegisters();

	// store from rt
	GEN_LI(rd, 0, MIPS_GET_RT(mips));
	genCallDynaMem(MEM_SDC1, base, MIPS_GET_IMMED(mips));

	return CONVERT_SUCCESS;
#endif
}

static int CACHE(MIPS_instr mips){
	return CONVERT_ERROR;
}

static int LL(MIPS_instr mips){
#ifdef INTERPRET_LL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_LL
	return CONVERT_ERROR;
#endif
}

static int SC(MIPS_instr mips){
#ifdef INTERPRET_SC
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SC
	return CONVERT_ERROR;
#endif
}

// -- Special Functions --

static int SLL(MIPS_instr mips){
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );

	GEN_SLWI(rd, rt, MIPS_GET_SA(mips));
	return CONVERT_SUCCESS;
}

static int SRL(MIPS_instr mips){
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );

	GEN_SRWI(rd, rt, MIPS_GET_SA(mips));
	return CONVERT_SUCCESS;
}

static int SRA(MIPS_instr mips){
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );

	GEN_SRAWI(rd, rt, MIPS_GET_SA(mips));
	return CONVERT_SUCCESS;
}

static int SLLV(MIPS_instr mips){
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );

	GEN_RLWINM(0, rs, 0, 27, 31); // Mask the lower 5-bits of rs
	GEN_SLW(rd, rt, 0);
	return CONVERT_SUCCESS;
}

static int SRLV(MIPS_instr mips){
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );

	GEN_RLWINM(0, rs, 0, 27, 31); // Mask the lower 5-bits of rs
	GEN_SRW(rd, rt, 0);
	return CONVERT_SUCCESS;
}

static int SRAV(MIPS_instr mips){
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );

	GEN_RLWINM(0, rs, 0, 27, 31); // Mask the lower 5-bits of rs
	GEN_SRAW(rd, rt, 0);
	return CONVERT_SUCCESS;
}

static int JR(MIPS_instr mips){
	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}
	
	flushRegisters();
	reset_code_addr();
	
	GEN_STW(mapRegister(MIPS_GET_RS(mips)),
			(REG_LOCALRS*8+4)+R4300OFF_GPR, DYNAREG_R4300);
	invalidateRegisters();

	// Check the delay slot, and note how big it is
	PowerPC_instr* preDelay = get_curr_dst();
	check_delaySlot();
	int delaySlot = get_curr_dst() - preDelay;

#ifdef COMPARE_CORE
	GEN_LI(4, 0, 1);
#endif
	genUpdateCount(0);

#ifdef INTERPRET_JR
	genJumpTo(REG_LOCALRS, JUMPTO_REG);
#else // INTERPRET_JR
	// TODO: jr
#endif

	// Let's still recompile the delay slot in place in case its branched to
	if(delaySlot){ if(is_j_dst()){ unget_last_src(); delaySlotNext = 2; } }
	else nop_ignored();

#ifdef INTERPRET_JR
	return INTERPRETED;
#else // INTERPRET_JR
	return CONVER_ERROR;
#endif
}

static int JALR(MIPS_instr mips){
	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	flushRegisters();
	reset_code_addr();
	
	GEN_STW(mapRegister(MIPS_GET_RS(mips)),
			(REG_LOCALRS*8+4)+R4300OFF_GPR, DYNAREG_R4300);
	invalidateRegisters();

	// Check the delay slot, and note how big it is
	PowerPC_instr* preDelay = get_curr_dst();
	check_delaySlot();
	int delaySlot = get_curr_dst() - preDelay;

#ifdef COMPARE_CORE
	GEN_LI(4, 0, 1);
#endif
	genUpdateCount(0);

	// Set LR to next instruction
	int rd = mapRegisterNew(MIPS_GET_RD(mips));
	// lis	lr, pc@ha(0)
	GEN_LIS(rd, extractUpper16(get_src_pc()+4));
	// la	lr, pc@l(lr)
	GEN_ADDI(rd, rd, extractLower16(get_src_pc()+4));

	flushRegisters();

#ifdef INTERPRET_JALR
	genJumpTo(REG_LOCALRS, JUMPTO_REG);
#else // INTERPRET_JALR
	// TODO: jalr
#endif

	// Let's still recompile the delay slot in place in case its branched to
	if(delaySlot){ if(is_j_dst()){ unget_last_src(); delaySlotNext = 2; } }
	else nop_ignored();

#ifdef INTERPRET_JALR
	return INTERPRETED;
#else // INTERPRET_JALR
	return CONVERT_ERROR;
#endif
}

static int SYSCALL(MIPS_instr mips){
#ifdef INTERPRET_SYSCALL
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SYSCALL
	return CONVERT_ERROR;
#endif
}

static int BREAK(MIPS_instr mips){
#ifdef INTERPRET_BREAK
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_BREAK
	return CONVERT_ERROR;
#endif
}

static int SYNC(MIPS_instr mips){
#ifdef INTERPRET_SYNC
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_SYNC
	return CONVERT_SUCCESS;
#endif
}

static int MFHI(MIPS_instr mips){
#ifdef INTERPRET_HILO
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_HILO

	RegMapping hi = mapRegister64( MIPS_REG_HI );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	// mr rd, hi
	GEN_OR(rd.lo, hi.lo, hi.lo);
	GEN_OR(rd.hi, hi.hi, hi.hi);
	return CONVERT_SUCCESS;
#endif
}

static int MTHI(MIPS_instr mips){
#ifdef INTERPRET_HILO
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_HILO

	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping hi = mapRegister64New( MIPS_REG_HI );

	// mr hi, rs
	GEN_OR(hi.lo, rs.lo, rs.lo);
	GEN_OR(hi.hi, rs.hi, rs.hi);
	return CONVERT_SUCCESS;
#endif
}

static int MFLO(MIPS_instr mips){
#ifdef INTERPRET_HILO
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_HILO

	RegMapping lo = mapRegister64( MIPS_REG_LO );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	// mr rd, lo
	GEN_OR(rd.lo, lo.lo, lo.lo);
	GEN_OR(rd.hi, lo.hi, lo.hi);
	return CONVERT_SUCCESS;
#endif
}

static int MTLO(MIPS_instr mips){
#ifdef INTERPRET_HILO
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_HILO

	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping lo = mapRegister64New( MIPS_REG_LO );

	// mr lo, rs
	GEN_OR(lo.lo, rs.lo, rs.lo);
	GEN_OR(lo.hi, rs.hi, rs.hi);
	return CONVERT_SUCCESS;
#endif
}

static int MULT(MIPS_instr mips){
#ifdef INTERPRET_MULT
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_MULT
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int hi = mapRegisterNew( MIPS_REG_HI );
	int lo = mapRegisterNew( MIPS_REG_LO );

	// Don't multiply if they're using r0
	if(MIPS_GET_RS(mips) && MIPS_GET_RT(mips)){
		// mullw lo, rs, rt
		GEN_MULLW(lo, rs, rt);
		// mulhw hi, rs, rt
		GEN_MULHW(hi, rs, rt);
	} else {
		// li lo, 0
		GEN_LI(lo, 0, 0);
		// li hi, 0
		GEN_LI(hi, 0, 0);
	}
	return CONVERT_SUCCESS;
#endif
}

static int MULTU(MIPS_instr mips){
#ifdef INTERPRET_MULTU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_MULTU
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int hi = mapRegisterNew( MIPS_REG_HI );
	int lo = mapRegisterNew( MIPS_REG_LO );

	// Don't multiply if they're using r0
	if(MIPS_GET_RS(mips) && MIPS_GET_RT(mips)){
		// mullw lo, rs, rt
		GEN_MULLW(lo, rs, rt);
		// mulhwu hi, rs, rt
		GEN_MULHWU(hi, rs, rt);
	} else {
		// li lo, 0
		GEN_LI(lo, 0, 0);
		// li hi, 0
		GEN_LI(hi, 0, 0);
	}
	return CONVERT_SUCCESS;
#endif
}

static int DIV(MIPS_instr mips){
#ifdef INTERPRET_DIV
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DIV
	// This instruction computes the quotient and remainder
	//   and stores the results in lo and hi respectively
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int hi = mapRegisterNew( MIPS_REG_HI );
	int lo = mapRegisterNew( MIPS_REG_LO );

	// Don't divide if they're using r0
	if(MIPS_GET_RS(mips) && MIPS_GET_RT(mips)){
		// divw lo, rs, rt
		GEN_DIVW(lo, rs, rt);
		// This is how you perform a mod in PPC
		// divw lo, rs, rt
		// NOTE: We already did that
		// mullw hi, lo, rt
		GEN_MULLW(hi, lo, rt);
		// subf hi, hi, rs
		GEN_SUBF(hi, hi, rs);
	}
	return CONVERT_SUCCESS;
#endif
}

static int DIVU(MIPS_instr mips){
#ifdef INTERPRET_DIVU
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DIVU
	// This instruction computes the quotient and remainder
	//   and stores the results in lo and hi respectively
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int hi = mapRegisterNew( MIPS_REG_HI );
	int lo = mapRegisterNew( MIPS_REG_LO );

	// Don't divide if they're using r0
	if(MIPS_GET_RS(mips) && MIPS_GET_RT(mips)){
		// divwu lo, rs, rt
		GEN_DIVWU(lo, rs, rt);
		// This is how you perform a mod in PPC
		// divw lo, rs, rt
		// NOTE: We already did that
		// mullw hi, lo, rt
		GEN_MULLW(hi, lo, rt);
		// subf hi, hi, rs
		GEN_SUBF(hi, hi, rs);
	}
	return CONVERT_SUCCESS;
#endif
}

static int DSLLV(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DSLLV)
	genCallInterp(mips);
	return INTERPRETED;
#else  // INTERPRET_DW || INTERPRET_DSLLV

	int rs = mapRegister( MIPS_GET_RS(mips) );
	int sa = mapRegisterTemp();
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	// Mask off the shift amount (0x3f)
	GEN_RLWINM(sa, rs, 0, 26, 31);
	// Shift the MSW
	GEN_SLW(rd.hi, rt.hi, sa);
	// Calculate 32-sh
	GEN_SUBFIC(0, sa, 32);
	// Extract the bits that will be shifted out the LSW (sh < 32)
	GEN_SRW(0, rt.lo, 0);
	// Insert the bits into the MSW
	GEN_OR(rd.hi, rd.hi, 0);
	// Calculate sh-32
	GEN_ADDI(0, sa, -32);
	// Extract the bits that will be shifted out the LSW (sh > 31)
	GEN_SLW(0, rt.lo, 0);
	// Insert the bits into the MSW
	GEN_OR(rd.hi, rd.hi, 0);
	// Shift the LSW
	GEN_SLW(rd.lo, rt.lo, sa);

	unmapRegisterTemp(sa);

	return CONVERT_SUCCESS;
#endif
}

static int DSRLV(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DSRLV)
	genCallInterp(mips);
	return INTERPRETED;
#else  // INTERPRET_DW || INTERPRET_DSRLV

	int rs = mapRegister( MIPS_GET_RS(mips) );
	int sa = mapRegisterTemp();
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	// Mask off the shift amount (0x3f)
	GEN_RLWINM(sa, rs, 0, 26, 31);
	// Shift the LSW
	GEN_SRW(rd.lo, rt.lo, sa);
	// Calculate 32-sh
	GEN_SUBFIC(0, sa, 32);
	// Extract the bits that will be shifted out the MSW (sh < 32)
	GEN_SLW(0, rt.hi, 0);
	// Insert the bits into the LSW
	GEN_OR(rd.lo, rd.lo, 0);
	// Calculate sh-32
	GEN_ADDI(0, sa, -32);
	// Extract the bits that will be shifted out the MSW (sh > 31)
	GEN_SRW(0, rt.hi, 0);
	// Insert the bits into the LSW
	GEN_OR(rd.lo, rd.lo, 0);
	// Shift the MSW
	GEN_SRW(rd.hi, rt.hi, sa);
	unmapRegisterTemp(sa);
	return CONVERT_SUCCESS;
#endif
}

static int DSRAV(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DSRAV)
	genCallInterp(mips);
	return INTERPRETED;
#else  // INTERPRET_DW || INTERPRET_DSRAV

	int rs = mapRegister( MIPS_GET_RS(mips) );
	int sa = mapRegisterTemp();
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	// Mask off the shift amount (0x3f)
	GEN_RLWINM(sa, rs, 0, 26, 31);
	// Check whether the shift amount is < 32
	GEN_CMPI(sa, 32, 1);
	// Shift the LSW
	GEN_SRW(rd.lo, rt.lo, sa);
	// Skip over this code if sh >= 32
	GEN_BGE(1, 5, 0, 0);
	// Calculate 32-sh
	GEN_SUBFIC(0, sa, 32);
	// Extract the bits that will be shifted out the MSW (sh < 32)
	GEN_SLW(0, rt.hi, 0);
	// Insert the bits into the LSW
	GEN_OR(rd.lo, rd.lo, 0);
	// Skip over the else
	GEN_B(4, 0, 0);
	// Calculate sh-32
	GEN_ADDI(0, sa, -32);
	// Extract the bits that will be shifted out the MSW (sh > 31)
	GEN_SRAW(0, rt.hi, 0);
	// Insert the bits into the LSW
	GEN_OR(rd.lo, rd.lo, 0);
	// Shift the MSW
	GEN_SRAW(rd.hi, rt.hi, sa);
	unmapRegisterTemp(sa);
	return CONVERT_SUCCESS;
#endif
}

static int DMULT(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DMULT)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DMULT
	return CONVERT_ERROR;
#endif
}

static int DMULTU(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DMULTU)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DMULTU
	return CONVERT_ERROR;
#endif
}

static int DDIV(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DDIV)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DDIV
	return CONVERT_ERROR;
#endif
}

static int DDIVU(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DDIVU)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DDIVU
	return CONVERT_ERROR;
#endif
}

static int DADDU(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DADDU)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DADDU

	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_ADDC(rd.lo, rs.lo, rt.lo);
	GEN_ADDE(rd.hi, rs.hi, rt.hi);
	return CONVERT_SUCCESS;
#endif
}

static int DADD(MIPS_instr mips){
	return DADDU(mips);
}

static int DSUBU(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DSUBU)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSUBU

	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_SUBFC(rd.lo, rt.lo, rs.lo);
	GEN_SUBFE(rd.hi, rt.hi, rs.hi);
	return CONVERT_SUCCESS;
#endif
}

static int DSUB(MIPS_instr mips){
	return DSUBU(mips);
}

static int DSLL(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DSLL)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSLL

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );
	int sa = MIPS_GET_SA(mips);

	if(sa){
		// Shift MSW left by SA
		GEN_SLWI(rd.hi, rt.hi, sa);
		// Extract the bits shifted out of the LSW
		GEN_RLWINM(0, rt.lo, sa, 32-sa, 31);
		// Insert those bits into the MSW
		GEN_OR(rd.hi, rd.hi, 0);
		// Shift LSW left by SA
		GEN_SLWI(rd.lo, rt.lo, sa);
	} else {
		// Copy over the register
		GEN_ADDI(rd.hi, rt.hi, 0);
		GEN_ADDI(rd.lo, rt.lo, 0);
	}
	return CONVERT_SUCCESS;
#endif
}

static int DSRL(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DSRL)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSRL

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );
	int sa = MIPS_GET_SA(mips);

	if(sa){
		// Shift LSW right by SA
		GEN_SRWI(rd.lo, rt.lo, sa);
		// Extract the bits shifted out of the MSW
		GEN_RLWINM(0, rt.hi, 32-sa, 0, sa-1);
		// Insert those bits into the LSW
		GEN_OR(rd.lo, rd.lo, 0);
		// Shift MSW right by SA
		GEN_SRWI(rd.hi, rt.hi, sa);
	} else {
		// Copy over the register
		GEN_ADDI(rd.hi, rt.hi, 0);
		GEN_ADDI(rd.lo, rt.lo, 0);
	}
	return CONVERT_SUCCESS;
#endif
}

static int DSRA(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DSRA)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSRA

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );
	int sa = MIPS_GET_SA(mips);

	if(sa){
		// Shift LSW right by SA
		GEN_SRWI(rd.lo, rt.lo, sa);
		// Extract the bits shifted out of the MSW
		GEN_RLWINM(0, rt.hi, 32-sa, 0, sa-1);
		// Insert those bits into the LSW
		GEN_OR(rd.lo, rd.lo, 0);
		// Shift (arithmetically) MSW right by SA
		GEN_SRAWI(rd.hi, rt.hi, sa);
	} else {
		// Copy over the register
		GEN_ADDI(rd.hi, rt.hi, 0);
		GEN_ADDI(rd.lo, rt.lo, 0);
	}
	return CONVERT_SUCCESS;
#endif
}

static int DSLL32(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DSLL32)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSLL32

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );
	int sa = MIPS_GET_SA(mips);

	// Shift LSW into MSW and by SA
	GEN_SLWI(rd.hi, rt.lo, sa);
	// Clear out LSW
	GEN_ADDI(rd.lo, 0, 0);
	return CONVERT_SUCCESS;
#endif
}

static int DSRL32(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DSRL32)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSRL32

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );
	int sa = MIPS_GET_SA(mips);

	// Shift MSW into LSW and by SA
	GEN_SRWI(rd.lo, rt.hi, sa);
	// Clear out MSW
	GEN_ADDI(rd.hi, 0, 0);
	return CONVERT_SUCCESS;
#endif
}

static int DSRA32(MIPS_instr mips){
#if defined(INTERPRET_DW) || defined(INTERPRET_DSRA32)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_DW || INTERPRET_DSRA32

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );
	int sa = MIPS_GET_SA(mips);

	// Shift (arithmetically) MSW into LSW and by SA
	GEN_SRAWI(rd.lo, rt.hi, sa);
	// Fill MSW with sign of MSW
	GEN_SRAWI(rd.hi, rt.hi, 31);
	return CONVERT_SUCCESS;
#endif
}

static int ADDU(MIPS_instr mips){
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );

	GEN_ADD(mapRegisterNew( MIPS_GET_RD(mips) ), rs, rt);
	return CONVERT_SUCCESS;
}

static int ADD(MIPS_instr mips){
	return ADDU(mips);
}

static int SUBU(MIPS_instr mips){
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );

	GEN_SUB(mapRegisterNew( MIPS_GET_RD(mips) ), rs, rt);
	return CONVERT_SUCCESS;
}

static int SUB(MIPS_instr mips){
	return SUBU(mips);
}

static int AND(MIPS_instr mips){
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_AND(rd.hi, rs.hi, rt.hi);
	GEN_AND(rd.lo, rs.lo, rt.lo);
	return CONVERT_SUCCESS;
}

static int OR(MIPS_instr mips){
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_OR(rd.hi, rs.hi, rt.hi);
	GEN_OR(rd.lo, rs.lo, rt.lo);
	return CONVERT_SUCCESS;
}

static int XOR(MIPS_instr mips){
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_XOR(rd.hi, rs.hi, rt.hi);
	GEN_XOR(rd.lo, rs.lo, rt.lo);
	return CONVERT_SUCCESS;
}

static int NOR(MIPS_instr mips){
	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	RegMapping rs = mapRegister64( MIPS_GET_RS(mips) );
	RegMapping rd = mapRegister64New( MIPS_GET_RD(mips) );

	GEN_NOR(rd.hi, rs.hi, rt.hi);
	GEN_NOR(rd.lo, rs.lo, rt.lo);
	return CONVERT_SUCCESS;
}

static int SLT(MIPS_instr mips){
#ifdef INTERPRET_SLT
	genCallInterp(mips);
	return INTERPRETED;
#else
	// FIXME: Do I need to worry about 64-bit values?
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );

	// carry = rs < rt ? 0 : 1 (unsigned)
	GEN_SUBFC(0, rt, rs);
	// rd = ~(rs ^ rt)
	GEN_EQV(rd, rt, rs);
	// rd = sign(rs) == sign(rt) ? 1 : 0
	GEN_SRWI(rd, rd, 31);
	// rd += carry
	GEN_ADDZE(rd, rd);
	// rt &= 1 ( = (sign(rs) == sign(rt)) xor (rs < rt (unsigned)) )
	GEN_RLWINM(rd, rd, 0, 31, 31);
	return CONVERT_SUCCESS;
#endif
}

static int SLTU(MIPS_instr mips){
#ifdef INTERPRET_SLTU
	genCallInterp(mips);
	return INTERPRETED;
#else
	// FIXME: Do I need to worry about 64-bit values?
	int rt = mapRegister( MIPS_GET_RT(mips) );
	int rs = mapRegister( MIPS_GET_RS(mips) );
	int rd = mapRegisterNew( MIPS_GET_RD(mips) );
	// carry = rs < rt ? 0 : 1
	GEN_SUBFC(rd, rt, rs);
	// rd = carry - 1 ( = rs < rt ? -1 : 0 )
	GEN_SUBFE(rd, rd, rd);
	// rd = !carry ( = rs < rt ? 1 : 0 )
	GEN_NEG(rd, rd);
	return CONVERT_SUCCESS;
#endif
}

static int TEQ(MIPS_instr mips){
#ifdef INTERPRET_TRAPS
	genCallInterp(mips);
	return INTERPRETED;
#else
	return CONVERT_ERROR;
#endif
}

static int (*gen_special[64])(MIPS_instr) =
{
   SLL , NI   , SRL , SRA , SLLV   , NI    , SRLV  , SRAV  ,
   JR  , JALR , NI  , NI  , SYSCALL, BREAK , NI    , SYNC  ,
   MFHI, MTHI , MFLO, MTLO, DSLLV  , NI    , DSRLV , DSRAV ,
   MULT, MULTU, DIV , DIVU, DMULT  , DMULTU, DDIV  , DDIVU ,
   ADD , ADDU , SUB , SUBU, AND    , OR    , XOR   , NOR   ,
   NI  , NI   , SLT , SLTU, DADD   , DADDU , DSUB  , DSUBU ,
   NI  , NI   , NI  , NI  , TEQ    , NI    , NI    , NI    ,
   DSLL, NI   , DSRL, DSRA, DSLL32 , NI    , DSRL32, DSRA32
};

static int SPECIAL(MIPS_instr mips){
	return gen_special[MIPS_GET_FUNC(mips)](mips);
}

// -- RegImmed Instructions --

// Since the RegImmed instructions are very similar:
//   BLTZ, BGEZ, BLTZL, BGEZL, BLZAL, BGEZAL, BLTZALL, BGEZALL
//   It's less work to handle them all in one function
static int REGIMM(MIPS_instr mips){
	int which = MIPS_GET_RT(mips);
	int cond   = which & 1; // t = GE, f = LT
	int likely = which & 2;
	int link   = which & 16;

	if(MIPS_GET_IMMED(mips) == 0xffff || CANT_COMPILE_DELAY()){
		// REGIMM_IDLE || virtual delay
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCmpi64(4, MIPS_GET_RA(mips), 0);

	return branch(signExtend(MIPS_GET_IMMED(mips),16),
	              cond ? GE : LT, link, likely, 0);
}

// -- COP0 Instructions --

static int TLBR(MIPS_instr mips){
#ifdef INTERPRET_TLBR
	genCallInterp(mips);
	return INTERPRETED;
#else
	return CONVERT_ERROR;
#endif
}

static int TLBWI(MIPS_instr mips){
#ifdef INTERPRET_TLBWI
	genCallInterp(mips);
	return INTERPRETED;
#else
	return CONVERT_ERROR;
#endif
}

static int TLBWR(MIPS_instr mips){
#ifdef INTERPRET_TLBWR
	genCallInterp(mips);
	return INTERPRETED;
#else
	return CONVERT_ERROR;
#endif
}

static int TLBP(MIPS_instr mips){
#ifdef INTERPRET_TLBP
	genCallInterp(mips);
	return INTERPRETED;
#else
	return CONVERT_ERROR;
#endif
}

static int ERET(MIPS_instr mips){
#ifdef INTERPRET_ERET
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_ERET

	flushRegisters();

 #ifdef COMPARE_CORE
 	GEN_LI(4, 0, 0);
	// Hack: we count an extra instruction here; it's not worth working around
	unget_last_src();
 #endif
	genUpdateCount(0);
#ifdef COMPARE_CORE
	get_next_src();
#endif
	// Load Status
	GEN_LWZ(3, (12*4)+R4300OFF_COP0, DYNAREG_R4300);
	// Load upper address of llbit
	GEN_LIS(4, extractUpper16((unsigned int)&r4300.llbit));
	// Status & 0xFFFFFFFD
	GEN_RLWINM(3, 3, 0, 31, 29);
	// llbit = 0
	GEN_STW(DYNAREG_ZERO, extractLower16((unsigned int)&r4300.llbit), 4);
	// Store updated Status
	GEN_STW(3, (12*4)+R4300OFF_COP0, DYNAREG_R4300);
	// check_interupt()
	GEN_B(add_jump((unsigned long)(&check_interupt), 1, 1), 0, 1);
	// Load the old LR
	GEN_LWZ(0, DYNAOFF_LR, 1);
	// r4300.pc = EPC
	GEN_LWZ(3, (14*4)+R4300OFF_COP0, DYNAREG_R4300);
	// Restore the LR
	GEN_MTLR(0);
	// Return to trampoline with EPC
	GEN_BLR(0);
	return CONVERT_SUCCESS;
#endif
}

static int (*gen_tlb[64])(MIPS_instr) =
{
   NI  , TLBR, TLBWI, NI, NI, NI, TLBWR, NI,
   TLBP, NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI,
   ERET, NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI,
   NI  , NI  , NI   , NI, NI, NI, NI   , NI
};

static int MFC0(MIPS_instr mips){
#ifdef INTERPRET_MFC0
	genCallInterp(mips);
	return INTERPRETED;
#else
	int rt = mapRegisterNew(MIPS_GET_RT(mips));
	// *rt = r4300.reg_cop0[rd]
	GEN_LWZ(rt, (MIPS_GET_RD(mips)*4)+R4300OFF_COP0, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int MTC0(MIPS_instr mips){
#ifdef INTERPRET_MTC0
	genCallInterp(mips);
	return INTERPRETED;
#else
	
	int rt = MIPS_GET_RT(mips), rrt;
	int rd = MIPS_GET_RD(mips);
	int tmp;
	
	switch(rd){
	case 0: // Index
		rrt = mapRegister(rt);
		// r0 = rt & 0x8000003F
		GEN_RLWINM(0, rrt, 0, 26, 0);
		// r4300.reg_cop0[rd] = r0
		GEN_STW(0, (rd*4)+R4300OFF_COP0, DYNAREG_R4300);
		return CONVERT_SUCCESS;
	
	case 2: // EntryLo0
	case 3: // EntryLo1
		rrt = mapRegister(rt);
		// r0 = rt & 0x3FFFFFFF
		GEN_RLWINM(0, rrt, 0, 2, 31);
		// r4300.reg_cop0[rd] = r0
		GEN_STW(0, (rd*4)+R4300OFF_COP0, DYNAREG_R4300);
		return CONVERT_SUCCESS;
	
	case 4: // Context
		rrt = mapRegister(rt), tmp = mapRegisterTemp();
		// tmp = r4300.reg_cop0[rd]
		GEN_LWZ(tmp, (rd*4)+R4300OFF_COP0, DYNAREG_R4300);
		// r0 = rt & 0xFF800000
		GEN_RLWINM(0, rrt, 0, 0, 8);
		// tmp &= 0x007FFFF0
		GEN_RLWINM(tmp, tmp, 0, 9, 27);
		// tmp |= r0
		GEN_OR(tmp, tmp, 0);
		// r4300.reg_cop0[rd] = tmp
		GEN_STW(tmp, (rd*4)+R4300OFF_COP0, DYNAREG_R4300);
		return CONVERT_SUCCESS;
	
	case 5: // PageMask
		rrt = mapRegister(rt);
		// r0 = rt & 0x01FFE000
		GEN_RLWINM(0, rrt, 0, 7, 18);
		// r4300.reg_cop0[rd] = r0
		GEN_STW(0, (rd*4)+R4300OFF_COP0, DYNAREG_R4300);
		return CONVERT_SUCCESS;
	
	case 6: // Wired
		rrt = mapRegister(rt);
		// r0 = 31
		GEN_ADDI(0, 0, 31);
		// r4300.reg_cop0[rd] = rt
		GEN_STW(rrt, (rd*4)+R4300OFF_COP0, DYNAREG_R4300);
		// r4300.reg_cop0[1] = r0
		GEN_STW(0, (1*4)+R4300OFF_COP0, DYNAREG_R4300);
		return CONVERT_SUCCESS;
	
	case 10: // EntryHi
		rrt = mapRegister(rt);
		// r0 = rt & 0xFFFFE0FF
		GEN_RLWINM(0, rrt, 0, 24, 18);
		// r4300.reg_cop0[rd] = r0
		GEN_STW(0, (rd*4)+R4300OFF_COP0, DYNAREG_R4300);
		return CONVERT_SUCCESS;
	
	case 13: // Cause
		rrt = mapRegister(rt);
		// TODO: Ensure that rrt == 0?
		// r4300.reg_cop0[rd] = rt
		GEN_STW(rrt, (rd*4)+R4300OFF_COP0, DYNAREG_R4300);
		return CONVERT_SUCCESS;
	
	case 14: // EPC
	case 16: // Config
	case 18: // WatchLo
	case 19: // WatchHi
		rrt = mapRegister(rt);
		// r4300.reg_cop0[rd] = rt
		GEN_STW(rrt, (rd*4)+R4300OFF_COP0, DYNAREG_R4300);
		return CONVERT_SUCCESS;
	
	case 28: // TagLo
		rrt = mapRegister(rt);
		// r0 = rt & 0x0FFFFFC0
		GEN_RLWINM(0, rrt, 0, 4, 25);
		// r4300.reg_cop0[rd] = r0
		GEN_STW(0, (rd*4)+R4300OFF_COP0, DYNAREG_R4300);
		return CONVERT_SUCCESS;
	
	case 29: // TagHi
		// r4300.reg_cop0[rd] = 0
		GEN_STW(DYNAREG_ZERO, (rd*4)+R4300OFF_COP0, DYNAREG_R4300);
		return CONVERT_SUCCESS;
	
	case 1: // Random
	case 8: // BadVAddr
	case 15: // PRevID
	case 27: // CacheErr
		// Do nothing
		return CONVERT_SUCCESS;
	
	case 9: // Count
	case 11: // Compare
	case 12: // Status
	default:
		genCallInterp(mips);
		return INTERPRETED;
	}
	
#endif
}

static int TLB(MIPS_instr mips){
#ifdef INTERPRET_TLB
	genCallInterp(mips);
	return INTERPRETED;
#else
	return gen_tlb[mips&0x3f](mips);
#endif
}

static int (*gen_cop0[32])(MIPS_instr) =
{
   MFC0, NI, NI, NI, MTC0, NI, NI, NI,
   NI  , NI, NI, NI, NI  , NI, NI, NI,
   TLB , NI, NI, NI, NI  , NI, NI, NI,
   NI  , NI, NI, NI, NI  , NI, NI, NI
};

static int COP0(MIPS_instr mips){
#ifdef INTERPRET_COP0
	genCallInterp(mips);
	return INTERPRETED;
#else
	return gen_cop0[MIPS_GET_RS(mips)](mips);
#endif
}

// -- COP1 Instructions --

static int MFC1(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_MFC1)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP

	genCheckFP();

	int fs = MIPS_GET_FS(mips);
	int rt = mapRegisterNew( MIPS_GET_RT(mips) );
	flushFPR(fs);

	// rt = r4300.fpr_single[fs]
	GEN_LWZ(rt, (fs*4)+R4300OFF_FPR_32, DYNAREG_R4300);
	// rt = *rt
	GEN_LWZ(rt, 0, rt);

	return CONVERT_SUCCESS;
#endif
}

static int DMFC1(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_DMFC1)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP

	genCheckFP();

	int fs = MIPS_GET_FS(mips);
	RegMapping rt = mapRegister64New( MIPS_GET_RT(mips) );
	int addr = mapRegisterTemp();
	flushFPR(fs);

	// addr = r4300.fpr_double[fs]
	GEN_LWZ(addr, (fs*4)+R4300OFF_FPR_64, DYNAREG_R4300);
	// rt[r4300.hi] = *addr
	GEN_LWZ(rt.hi, 0, addr);
	// rt[r4300.lo] = *(addr+4)
	GEN_LWZ(rt.lo, 4, addr);

	unmapRegisterTemp(addr);

	return CONVERT_SUCCESS;
#endif
}

static int CFC1(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_CFC1)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_CFC1

	genCheckFP();

	if(MIPS_GET_FS(mips) == 31){
		int rt = mapRegisterNew( MIPS_GET_RT(mips) );

		GEN_LWZ(rt, 0+R4300OFF_FCR31, DYNAREG_R4300);
	} else if(MIPS_GET_FS(mips) == 0){
		int rt = mapRegisterNew( MIPS_GET_RT(mips) );

		GEN_LI(rt, 0, 0x511);
	}

	return CONVERT_SUCCESS;
#endif
}

static int MTC1(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_MTC1)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP

	genCheckFP();

	int rt = mapRegister( MIPS_GET_RT(mips) );
	int fs = MIPS_GET_FS(mips);
	int addr = mapRegisterTemp();
	invalidateFPR(fs);

	// addr = r4300.fpr_single[fs]
	GEN_LWZ(addr, (fs*4)+R4300OFF_FPR_32, DYNAREG_R4300);
	// *addr = rt
	GEN_STW(rt, 0, addr);

	unmapRegisterTemp(addr);

	return CONVERT_SUCCESS;
#endif
}

static int DMTC1(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_DMTC1)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP

	genCheckFP();

	RegMapping rt = mapRegister64( MIPS_GET_RT(mips) );
	int fs = MIPS_GET_FS(mips);
	int addr = mapRegisterTemp();
	invalidateFPR(fs);

	GEN_LWZ(addr, (fs*4)+R4300OFF_FPR_64, DYNAREG_R4300);
	GEN_STW(rt.hi, 0, addr);
	GEN_STW(rt.lo, 4, addr);

	unmapRegisterTemp(addr);

	return CONVERT_SUCCESS;
#endif
}

static int CTC1(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_CTC1)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_CTC1

	genCheckFP();

	if(MIPS_GET_FS(mips) == 31){
		int rt = mapRegister( MIPS_GET_RT(mips) );
		GEN_STW(rt, 0+R4300OFF_FCR31, DYNAREG_R4300);
	}
	return CONVERT_SUCCESS;
#endif
}

static int BC(MIPS_instr mips){
#if defined(INTERPRET_BC)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_BC

	if(CANT_COMPILE_DELAY()){
		genCallInterp(mips);
		return INTERPRETED;
	}

	genCheckFP();

	int cond   = mips & 0x00010000;
	int likely = mips & 0x00020000;

	return branch(signExtend(MIPS_GET_IMMED(mips),16), cond?NE:EQ, 0, likely, 1);
#endif
}

// -- Floating Point Arithmetic --
static int ADD_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_ADD)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_ADD

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FADD(fd, fs, ft, dbl);
	return CONVERT_SUCCESS;
#endif
}

static int SUB_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_SUB)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_SUB

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FSUB(fd, fs, ft, dbl);
	return CONVERT_SUCCESS;
#endif
}

static int MUL_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_MUL)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_MUL

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FMUL(fd, fs, ft, dbl);
	return CONVERT_SUCCESS;
#endif
}

static int DIV_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_DIV)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_DIV

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FDIV(fd, fs, ft, dbl);
	return CONVERT_SUCCESS;
#endif
}

static int SQRT_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_SQRT)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_SQRT

	genCheckFP();

	flushRegisters();
	mapFPR( MIPS_GET_FS(mips), dbl ); // maps to f1 (FP argument)
	invalidateRegisters();

	// call sqrt
	GEN_B(add_jump(dbl ? (unsigned long)(&sqrt) : (unsigned long)(&sqrtf), 1, 1), 0, 1);

	mapFPRNew( MIPS_GET_FD(mips), dbl ); // maps to f1 (FP return)

	// Load old LR
	GEN_LWZ(0, DYNAOFF_LR, 1);
	// Restore LR
	GEN_MTLR(0);
	return CONVERT_SUCCESS;
#endif
}

static int ABS_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_ABS)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_ABS

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FABS(fd, fs);
	return CONVERT_SUCCESS;
#endif
}

static int MOV_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_MOV)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_MOV

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FMR(fd, fs);
	return CONVERT_SUCCESS;
#endif
}

static int NEG_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_NEG)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_NEG

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );

	GEN_FNEG(fd, fs);
	return CONVERT_SUCCESS;
#endif
}

// -- Floating Point Rounding/Conversion --
#define PPC_ROUNDING_NEAREST 0
#define PPC_ROUNDING_TRUNC   1
#define PPC_ROUNDING_CEIL    2
#define PPC_ROUNDING_FLOOR   3
static void set_rounding(int rounding_mode){
	GEN_MTFSFI(7, rounding_mode);
}

static void set_rounding_reg(int fs){
	GEN_MTFSF(1, fs);
}

static int ROUND_L_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_ROUND_L)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_ROUND_L
	
	genCheckFP();

	flushRegisters();
	int fd = MIPS_GET_FD(mips);
	mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR( MIPS_GET_FS(mips) );

	// round
	GEN_B(add_jump(dbl ? (unsigned long)(&round) : (unsigned long)(&roundf), 1, 1), 0, 1);
	// convert
	GEN_B(add_jump(dbl ? (unsigned long)(&__fixdfdi) : (unsigned long)(&__fixsfdi), 1, 1), 0, 1);
	
	int addr = 5; // Use r5 for the addr (to not clobber r3/r4)
	// addr = r4300.fpr_double[fd]
	GEN_LWZ(addr, (fd*4)+R4300OFF_FPR_64, DYNAREG_R4300);
	// Load old LR
	GEN_LWZ(0, DYNAOFF_LR, 1);
	// stw r3, 0(addr)
	GEN_STW(3, 0, addr);
	// Restore LR
	GEN_MTLR(0);
	// stw r4, 4(addr)
	GEN_STW(4, 4, addr);
	return CONVERT_SUCCESS;
#endif
}

static int TRUNC_L_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_TRUNC_L)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_TRUNC_L

	genCheckFP();

	flushRegisters();
	int fd = MIPS_GET_FD(mips);
	mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR( MIPS_GET_FS(mips) );

	// convert
	GEN_B(add_jump(dbl ? (unsigned long)(&__fixdfdi) : (unsigned long)(&__fixsfdi), 1, 1), 0, 1);
	
	int addr = 5; // Use r5 for the addr (to not clobber r3/r4)
	// addr = r4300.fpr_double[fd]
	GEN_LWZ(addr, (fd*4)+R4300OFF_FPR_64, DYNAREG_R4300);
	// Load old LR
	GEN_LWZ(0, DYNAOFF_LR, 1);
	// stw r3, 0(addr)
	GEN_STW(3, 0, addr);
	// Restore LR
	GEN_MTLR(0);
	// stw r4, 4(addr)
	GEN_STW(4, 4, addr);
	return CONVERT_SUCCESS;
#endif
}

static int CEIL_L_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_CEIL_L)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_CEIL_L

	genCheckFP();

	flushRegisters();
	int fd = MIPS_GET_FD(mips);
	mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR( MIPS_GET_FS(mips) );

	// ceil
	GEN_B(add_jump(dbl ? (unsigned long)(&ceil) : (unsigned long)(&ceilf), 1, 1), 0, 1);
	// convert
	GEN_B(add_jump(dbl ? (unsigned long)(&__fixdfdi) : (unsigned long)(&__fixsfdi), 1, 1), 0, 1);
	
	int addr = 5; // Use r5 for the addr (to not clobber r3/r4)
	// addr = r4300.fpr_double[fd]
	GEN_LWZ(addr, (fd*4)+R4300OFF_FPR_64, DYNAREG_R4300);
	// Load old LR
	GEN_LWZ(0, DYNAOFF_LR, 1);
	// stw r3, 0(addr)
	GEN_STW(3, 0, addr);
	// Restore LR
	GEN_MTLR(0);
	// stw r4, 4(addr)
	GEN_STW(4, 4, addr);
	return CONVERT_SUCCESS;
#endif
}

static int FLOOR_L_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_FLOOR_L)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_FLOOR_L

	genCheckFP();

	flushRegisters();
	int fd = MIPS_GET_FD(mips);
	mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR( MIPS_GET_FS(mips) );

	// round
	GEN_B(add_jump(dbl ? (unsigned long)(&floor) : (unsigned long)(&floorf), 1, 1), 0, 1);
	// convert
	GEN_B(add_jump(dbl ? (unsigned long)(&__fixdfdi) : (unsigned long)(&__fixsfdi), 1, 1), 0, 1);
	
	int addr = 5; // Use r5 for the addr (to not clobber r3/r4)
	// addr = r4300.fpr_double[fd]
	GEN_LWZ(addr, (fd*4)+R4300OFF_FPR_64, DYNAREG_R4300);
	// Load old LR
	GEN_LWZ(0, DYNAOFF_LR, 1);
	// stw r3, 0(addr)
	GEN_STW(3, 0, addr);
	// Restore LR
	GEN_MTLR(0);
	// stw r4, 4(addr)
	GEN_STW(4, 4, addr);
	return CONVERT_SUCCESS;
#endif
}

static int ROUND_W_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_ROUND_W)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_ROUND_W

	genCheckFP();

	set_rounding(PPC_ROUNDING_NEAREST); // TODO: Presume its already set?

	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR(fd);
	int addr = mapRegisterTemp();

	// fctiw f0, fs
	GEN_FCTIW(0, fs);
	// addr = r4300.fpr_single[fd]
	GEN_LWZ(addr, (fd*4)+R4300OFF_FPR_32, DYNAREG_R4300);
	// stfiwx f0, 0, addr
	GEN_STFIWX(0, 0, addr);

	unmapRegisterTemp(addr);
	return CONVERT_SUCCESS;
#endif
}

static int TRUNC_W_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_TRUNC_W)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_TRUNC_W

	genCheckFP();

	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR(fd);
	int addr = mapRegisterTemp();

	// fctiwz f0, fs
	GEN_FCTIWZ(0, fs);
	// addr = r4300.fpr_single[fd]
	GEN_LWZ(addr, (fd*4)+R4300OFF_FPR_32, DYNAREG_R4300);
	// stfiwx f0, 0, addr
	GEN_STFIWX(0, 0, addr);

	unmapRegisterTemp(addr);
	return CONVERT_SUCCESS;
#endif
}

static int CEIL_W_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_CEIL_W)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_CEIL_W

	genCheckFP();

	set_rounding(PPC_ROUNDING_CEIL);

	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR(fd);
	int addr = mapRegisterTemp();

	// fctiw f0, fs
	GEN_FCTIW(0, fs);
	// addr = r4300.fpr_single[fd]
	GEN_LWZ(addr, (fd*4)+R4300OFF_FPR_32, DYNAREG_R4300);
	// stfiwx f0, 0, addr
	GEN_STFIWX(0, 0, addr);

	unmapRegisterTemp(addr);
	
	set_rounding(PPC_ROUNDING_NEAREST);
	return CONVERT_SUCCESS;
#endif
}

static int FLOOR_W_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_FLOOR_W)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_FLOOR_W

	genCheckFP();

	set_rounding(PPC_ROUNDING_FLOOR);

	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR(fd);
	int addr = mapRegisterTemp();

	// fctiw f0, fs
	GEN_FCTIW(0, fs);
	// addr = r4300.fpr_single[fd]
	GEN_LWZ(addr, (fd*4)+R4300OFF_FPR_32, DYNAREG_R4300);
	// stfiwx f0, 0, addr
	GEN_STFIWX(0, 0, addr);

	unmapRegisterTemp(addr);
	
	set_rounding(PPC_ROUNDING_NEAREST);
	return CONVERT_SUCCESS;
#endif
}

static int CVT_S_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_CVT_S)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_CVT_S

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), 0 );

	GEN_FMR(fd, fs);
	return CONVERT_SUCCESS;
#endif
}

static int CVT_D_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_CVT_D)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_CVT_D

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int fd = mapFPRNew( MIPS_GET_FD(mips), 1 );

	GEN_FMR(fd, fs);
	return CONVERT_SUCCESS;
#endif
}

static int CVT_W_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_CVT_W)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_CVT_W

	genCheckFP();

	// Set rounding mode according to r4300.fcr31
	GEN_LFD(0, -4+R4300OFF_FCR31, DYNAREG_R4300);

	// FIXME: Here I have the potential to disable IEEE mode
	//          and enable inexact exceptions
	set_rounding_reg(0);

	int fd = MIPS_GET_FD(mips);
	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR(fd);
	int addr = mapRegisterTemp();

	// fctiw f0, fs
	GEN_FCTIW(0, fs);
	// addr = r4300.fpr_single[fd]
	GEN_LWZ(addr, (fd*4)+R4300OFF_FPR_32, DYNAREG_R4300);
	// stfiwx f0, 0, addr
	GEN_STFIWX(0, 0, addr);

	unmapRegisterTemp(addr);
	
	set_rounding(PPC_ROUNDING_NEAREST);
	return CONVERT_SUCCESS;
#endif
}

static int CVT_L_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_CVT_L)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_CVT_L

	genCheckFP();

	flushRegisters();
	int fd = MIPS_GET_FD(mips);
	mapFPR( MIPS_GET_FS(mips), dbl );
	invalidateFPR( MIPS_GET_FS(mips) );

	// FIXME: I'm fairly certain this will always trunc
	// convert
	GEN_B(add_jump(dbl ? (unsigned long)(&__fixdfdi) : (unsigned long)(&__fixsfdi), 1, 1), 0, 1);
	
	int addr = 5; // Use r5 for the addr (to not clobber r3/r4)
	// addr = r4300.fpr_double[fd]
	GEN_LWZ(addr, (fd*4)+R4300OFF_FPR_64, DYNAREG_R4300);
	// Load old LR
	GEN_LWZ(0, DYNAOFF_LR, 1);
	// stw r3, 0(addr)
	GEN_STW(3, 0, addr);
	// Restore LR
	GEN_MTLR(0);
	// stw r4, 4(addr)
	GEN_STW(4, 4, addr);
	return CONVERT_SUCCESS;
#endif
}

// -- Floating Point Comparisons --
static int C_F_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_F)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_F

	genCheckFP();

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_UN_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_UN)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_UN

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// bord cr0, 2 (past setting cond)
	GEN_BC(2, 0, 0, 0x4, 3);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_EQ_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_EQ)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_EQ

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// bne cr0, 2 (past setting cond)
	GEN_BNE(0, 2, 0, 0);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_UEQ_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_UEQ)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_UEQ

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// cror cr0[eq], cr0[eq], cr0[un]
	GEN_CROR(2, 2, 3);
	// bne cr0, 2 (past setting cond)
	GEN_BNE(0, 2, 0, 0);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_OLT_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_OLT)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_OLT

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// bge cr0, 2 (past setting cond)
	GEN_BGE(0, 2, 0, 0);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_ULT_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_ULT)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_ULT

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// cror cr0[lt], cr0[lt], cr0[un]
	GEN_CROR(0, 0, 3);
	// bge cr0, 2 (past setting cond)
	GEN_BGE(0, 2, 0, 0);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_OLE_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_OLE)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_OLE

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// cror cr0[gt], cr0[gt], cr0[un]
	GEN_CROR(1, 1, 3);
	// bgt cr0, 2 (past setting cond)
	GEN_BGT(0, 2, 0, 0);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_ULE_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_ULE)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_ULE

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// bgt cr0, 2 (past setting cond)
	GEN_BGT(0, 2, 0, 0);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_SF_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_SF)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_SF

	genCheckFP();

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_NGLE_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_NGLE)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_NGLE

	genCheckFP();

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_SEQ_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_SEQ)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_SEQ

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// bne cr0, 2 (past setting cond)
	GEN_BNE(0, 2, 0, 0);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_NGL_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_NGL)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_NGL

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// bne cr0, 2 (past setting cond)
	GEN_BNE(0, 2, 0, 0);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_LT_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_LT)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_LT

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// bge cr0, 2 (past setting cond)
	GEN_BGE(0, 2, 0, 0);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_NGE_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_NGE)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_NGE

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// bge cr0, 2 (past setting cond)
	GEN_BGE(0, 2, 0, 0);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_LE_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_LE)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_LE

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// bgt cr0, 2 (past setting cond)
	GEN_BGT(0, 2, 0, 0);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int C_NGT_FP(MIPS_instr mips, int dbl){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_C_NGT)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_C_NGT

	genCheckFP();

	int fs = mapFPR( MIPS_GET_FS(mips), dbl );
	int ft = mapFPR( MIPS_GET_FT(mips), dbl );

	// lwz r0, 0(&fcr31)
	GEN_LWZ(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	// fcmpu cr0, fs, ft
	GEN_FCMPU(fs, ft, 0);
	// and r0, r0, 0xff7fffff (clear cond)
	GEN_RLWINM(0, 0, 0, 9, 7);
	// bgt cr0, 2 (past setting cond)
	GEN_BGT(0, 2, 0, 0);
	// oris r0, r0, 0x0080 (set cond)
	GEN_ORIS(0, 0, 0x0080);
	// stw r0, 0(&fcr31)
	GEN_STW(0, 0+R4300OFF_FCR31, DYNAREG_R4300);
	return CONVERT_SUCCESS;
#endif
}

static int (*gen_cop1_fp[64])(MIPS_instr, int) =
{
   ADD_FP    ,SUB_FP    ,MUL_FP   ,DIV_FP    ,SQRT_FP   ,ABS_FP    ,MOV_FP   ,NEG_FP    ,
   ROUND_L_FP,TRUNC_L_FP,CEIL_L_FP,FLOOR_L_FP,ROUND_W_FP,TRUNC_W_FP,CEIL_W_FP,FLOOR_W_FP,
   NI        ,NI        ,NI       ,NI        ,NI        ,NI        ,NI       ,NI        ,
   NI        ,NI        ,NI       ,NI        ,NI        ,NI        ,NI       ,NI        ,
   CVT_S_FP  ,CVT_D_FP  ,NI       ,NI        ,CVT_W_FP  ,CVT_L_FP  ,NI       ,NI        ,
   NI        ,NI        ,NI       ,NI        ,NI        ,NI        ,NI       ,NI        ,
   C_F_FP    ,C_UN_FP   ,C_EQ_FP  ,C_UEQ_FP  ,C_OLT_FP  ,C_ULT_FP  ,C_OLE_FP ,C_ULE_FP  ,
   C_SF_FP   ,C_NGLE_FP ,C_SEQ_FP ,C_NGL_FP  ,C_LT_FP   ,C_NGE_FP  ,C_LE_FP  ,C_NGT_FP
};

static int S(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_S)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_S
	return gen_cop1_fp[ MIPS_GET_FUNC(mips) ](mips, 0);
#endif
}

static int D(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_D)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_D
	return gen_cop1_fp[ MIPS_GET_FUNC(mips) ](mips, 1);
#endif
}

static int CVT_FP_W(MIPS_instr mips, int dbl){
	genCheckFP();

	int fs = MIPS_GET_FS(mips);
	flushFPR(fs);
	int fd = mapFPRNew( MIPS_GET_FD(mips), dbl );
	int tmp = mapRegisterTemp();

	// Get the integer value into a GPR
	// tmp = fpr32[fs]
	GEN_LWZ(tmp, (fs*4)+R4300OFF_FPR_32, DYNAREG_R4300);
	// tmp = *tmp (src)
	GEN_LWZ(tmp, 0, tmp);

	// lis r0, 0x4330
	GEN_LIS(0, 0x4330);
	// stw r0, -8(r1)
	GEN_STW(0, -8, 1);
	// lis r0, 0x8000
	GEN_LIS(0, 0x8000);
	// stw r0, -4(r1)
	GEN_STW(0, -4, 1);
	// xor r0, src, 0x80000000
	GEN_XOR(0, tmp, 0);
	// lfd f0, -8(r1)
	GEN_LFD(0, -8, 1);
	// stw r0 -4(r1)
	GEN_STW(0, -4, 1);
	// lfd fd, -8(r1)
	GEN_LFD(fd, -8, 1);
	// fsub fd, fd, f0
	GEN_FSUB(fd, fd, 0, dbl);

	unmapRegisterTemp(tmp);
	return CONVERT_SUCCESS;
}

static int W(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_W)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_W

	int func = MIPS_GET_FUNC(mips);

	if(func == MIPS_FUNC_CVT_S_) return CVT_FP_W(mips, 0);
	if(func == MIPS_FUNC_CVT_D_) return CVT_FP_W(mips, 1);
	else return CONVERT_ERROR;
#endif
}

static int CVT_FP_L(MIPS_instr mips, int dbl){
	genCheckFP();

	flushRegisters();
	int fs = MIPS_GET_FS(mips);
	mapFPRNew( MIPS_GET_FD(mips), dbl ); // f1
	int hi = mapRegisterTemp(); // r3
	int lo = mapRegisterTemp(); // r4

	// Get the long value into GPRs
	// lo = fpr64[fs]
	GEN_LWZ(lo, (fs*4)+R4300OFF_FPR_64, DYNAREG_R4300);
	// hi = *lo (hi word)
	GEN_LWZ(hi, 0, lo);
	// lo = *(lo+4) (lo word)
	GEN_LWZ(lo, 4, lo);

	// convert
	GEN_B(add_jump(dbl ? (unsigned long)(&__floatdidf) : (unsigned long)(&__floatdisf), 1, 1), 0, 1);
	
	// Load old LR
	GEN_LWZ(0, DYNAOFF_LR, 1);
	// Restore LR
	GEN_MTLR(0);
	
	unmapRegisterTemp(hi);
	unmapRegisterTemp(lo);

	return CONVERT_SUCCESS;
}

static int L(MIPS_instr mips){
#if defined(INTERPRET_FP) || defined(INTERPRET_FP_L)
	genCallInterp(mips);
	return INTERPRETED;
#else // INTERPRET_FP || INTERPRET_FP_L

	int func = MIPS_GET_FUNC(mips);

	if(func == MIPS_FUNC_CVT_S_) return CVT_FP_L(mips, 0);
	if(func == MIPS_FUNC_CVT_D_) return CVT_FP_L(mips, 1);
	else return CONVERT_ERROR;
#endif
}

static int (*gen_cop1[32])(MIPS_instr) =
{
   MFC1, DMFC1, CFC1, NI, MTC1, DMTC1, CTC1, NI,
   BC  , NI   , NI  , NI, NI  , NI   , NI  , NI,
   S   , D    , NI  , NI, W   , L    , NI  , NI,
   NI  , NI   , NI  , NI, NI  , NI   , NI  , NI
};

static int COP1(MIPS_instr mips){
	return gen_cop1[MIPS_GET_RS(mips)](mips);
}

static int (*gen_ops[64])(MIPS_instr) =
{
   SPECIAL, REGIMM, J   , JAL  , BEQ , BNE , BLEZ , BGTZ ,
   ADDI   , ADDIU , SLTI, SLTIU, ANDI, ORI , XORI , LUI  ,
   COP0   , COP1  , NI  , NI   , BEQL, BNEL, BLEZL, BGTZL,
   DADDI  , DADDIU, LDL , LDR  , NI  , NI  , NI   , NI   ,
   LB     , LH    , LWL , LW   , LBU , LHU , LWR  , LWU  ,
   SB     , SH    , SWL , SW   , SDL , SDR , SWR  , CACHE,
   LL     , LWC1  , NI  , NI   , NI  , LDC1, NI   , LD   ,
   SC     , SWC1  , NI  , NI   , NI  , SDC1, NI   , SD
};



static void genCallInterp(MIPS_instr mips){
	flushRegisters();
	reset_code_addr();
	// Pass in whether this instruction is in the delay slot
	GEN_LI(5, 0, isDelaySlot ? 1 : 0);
	// Load our argument into r3 (mips)
	GEN_LIS(3, extractUpper16(mips));
	// Load the current PC as the second arg
	GEN_LIS(4, extractUpper16(get_src_pc()));
	// Load the lower halves of mips and PC
	GEN_ADDI(3, 3, extractLower16(mips));
	GEN_ADDI(4, 4, extractLower16(get_src_pc()));
	// Branch to decodeNInterpret
	GEN_B(add_jump((unsigned long)(&decodeNInterpret), 1, 1), 0, 1);
	// Load the old LR
	GEN_LWZ(0, DYNAOFF_LR, 1);
	// Check if the PC changed
	GEN_CMPI(3, 0, 6);
	// Restore the LR
	GEN_MTLR(0);
	// if decodeNInterpret returned an address
	//   jumpTo it
	GEN_BNELR(6, 0);

	if(mips_is_jump(mips)) delaySlotNext = 2;
}

static void genJumpTo(unsigned int loc, unsigned int type){

	if(type == JUMPTO_REG){
		// Load the register as the return value
		GEN_LWZ(3, (loc*8+4)+R4300OFF_GPR, DYNAREG_R4300);
	} else {
		// Calculate the destination address
		loc <<= 2;
		if(type == JUMPTO_OFF) loc += get_src_pc();
		else loc |= get_src_pc() & 0xf0000000;
		// Create space to load destination func*
		GEN_ORI(0, 0, 0);
		GEN_ORI(0, 0, 0);
		// Move func* into r3 as argument
		GEN_ADDI(3, DYNAREG_FUNC, 0);
		// Call RecompCache_Update(func)
		GEN_B(add_jump((unsigned long)(&RecompCache_Update), 1, 1), 0, 1);
		// Restore LR
		GEN_LWZ(0, DYNAOFF_LR, 1);
		GEN_MTLR(0);
		// Load the address as the return value
		GEN_LIS(3, extractUpper16(loc));
		GEN_ADDI(3, 3, extractLower16(loc));
		// Since we could be linking, return on interrupt
		GEN_BLELR(2, 0);
		// Store r4300.last_pc for linking
		GEN_STW(3, 0+R4300OFF_LADDR, DYNAREG_R4300);
	}

	GEN_BLR((type != JUMPTO_REG));
}

// Updates Count, and sets cr2 to (r4300.next_interrupt ? Count)
static void genUpdateCount(int checkCount){
#ifndef COMPARE_CORE
	// Dynarec inlined code equivalent:
	int tmp = mapRegisterTemp();
	// lis    tmp, pc >> 16
	GEN_LIS(tmp, extractUpper16(get_src_pc()+4));
	// lwz    r0,  0(&r4300.last_pc)     // r0 = r4300.last_pc
	GEN_LWZ(0, 0+R4300OFF_LADDR, DYNAREG_R4300);
	// addi    tmp, tmp, pc & 0xffff  // tmp = pc
	GEN_ADDI(tmp, tmp, extractLower16(get_src_pc()+4));
	// stw    tmp, 0(&r4300.last_pc)     // r4300.last_pc = pc
	GEN_STW(tmp, 0+R4300OFF_LADDR, DYNAREG_R4300);
	// subf   r0,  r0, tmp           // r0 = pc - r4300.last_pc
	GEN_SUBF(0, 0, tmp);
	// lwz    tmp, 9*4(r4300.reg_cop0)     // tmp = Count
	GEN_LWZ(tmp, (9*4)+R4300OFF_COP0, DYNAREG_R4300);
	// srwi r0, r0, 1                // r0 = (pc - r4300.last_pc)/2
	GEN_SRWI(0, 0, 1);
	// add    r0,  r0, tmp           // r0 += Count
	GEN_ADD(0, 0, tmp);
	if(checkCount){
		// lwz    tmp, 0(&r4300.next_interrupt) // tmp = r4300.next_interrupt
		GEN_LWZ(tmp, 0+R4300OFF_NINTR, DYNAREG_R4300);
	}
	// stw    r0,  9*4(r4300.reg_cop0)    // Count = r0
	GEN_STW(0, (9*4)+R4300OFF_COP0, DYNAREG_R4300);
	if(checkCount){
		// cmpl   cr2,  tmp, r0  // cr2 = r4300.next_interrupt ? Count
		GEN_CMPL(tmp, 0, 2);
	}
	// Free tmp register
	unmapRegisterTemp(tmp);
#else
	// Load the current PC as the argument
	GEN_LIS(3, extractUpper16(get_src_pc()+4));
	GEN_ADDI(3, 3, extractLower16(get_src_pc()+4));
	// Call dyna_update_count
	GEN_B(add_jump((unsigned long)(&dyna_update_count), 1, 1), 0, 1);
	// Load the lr
	GEN_LWZ(0, DYNAOFF_LR, 1);
	GEN_MTLR(0);
	if(checkCount){
		// If r4300.next_interrupt <= Count (cr2)
		GEN_CMPI(3, 0, 2);
	}
#endif
}

// Check whether we need to take a FP unavailable exception
static void genCheckFP(void){
	if(FP_need_check || isDelaySlot){
		flushRegisters();
		if(!isDelaySlot) reset_code_addr(); // Calling this makes no sense in the context of the delay slot
		// lwz r0, 12*4(r4300.reg_cop0)
		GEN_LWZ(0, (12*4)+R4300OFF_COP0, DYNAREG_R4300);
		// andis. r0, r0, 0x2000
		GEN_ANDIS(0, 0, 0x2000);
		// bne cr0, end
		GEN_BNE(0, 8, 0, 0);
		// Load the current PC as arg 1 (upper half)
		GEN_LIS(3, extractUpper16(get_src_pc()));
		// Pass in whether this instruction is in the delay slot as arg 2
		GEN_LI(4, 0, isDelaySlot ? 1 : 0);
		// Current PC (lower half)
		GEN_ADDI(3, 3, extractLower16(get_src_pc()));
		// Call dyna_check_cop1_unusable
		GEN_B(add_jump((unsigned long)(&dyna_check_cop1_unusable), 1, 1), 0, 1);
		// Load the old LR
		GEN_LWZ(0, DYNAOFF_LR, 1);
		// Restore the LR
		GEN_MTLR(0);
		// Return to trampoline
		GEN_BLR(0);
		// Don't check for the rest of this mapping
		// Unless this instruction is in a delay slot
		FP_need_check = isDelaySlot;
	}
}

// Recompiles a Load, includes constant propagation, physical, virtual and runtime determined cases
void genRecompileLoad(memType type, MIPS_instr mips) {
	int isConstant = isRegisterConstant( MIPS_GET_RS(mips) );
	int isPhysical = 1, isVirtual = 1;
	if(isConstant){
		int constant = getRegisterConstant( MIPS_GET_RS(mips) );
		int immediate = MIPS_GET_IMMED(mips);
		immediate |= (immediate&0x8000) ? 0xffff0000 : 0;
		
		if((constant + immediate) > (int)RAM_TOP)
			isPhysical = 0;
		else
			isVirtual = 0;
	}
	
	PowerPC_instr* preCall[2] = {0,0};
	int not_fastmem_id[2] = {0,0};
	int rd, base, rdram_base;
	if(isVirtual) {
		flushRegisters();
		reset_code_addr();
		rdram_base = rd = mapRegisterTemp(); // r3 = rd
		base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr
		invalidateRegisters();
	}
	else {
		base = mapRegister( MIPS_GET_RS(mips) );
		rd = mapRegisterNew( MIPS_GET_RT(mips) );
		rdram_base = mapRegisterTemp();
	}

	if(isPhysical && isVirtual){
		// If base in physical memory
		GEN_CMP(base, DYNAREG_MEM_TOP, 1);
		not_fastmem_id[0] = add_jump_special(0);
		GEN_BGE(1, not_fastmem_id[0], 0, 0);
		preCall[0] = get_curr_dst();
	}
	if(isPhysical){
		// Add rdram pointer
		GEN_ADD(rdram_base, DYNAREG_RDRAM, base);
		// Perform the actual load
		switch(type) {
			case MEM_LW:
				GEN_LWZ(rd, MIPS_GET_IMMED(mips), rdram_base);
			break;
			case MEM_LBU:
				GEN_LBZ(rd, MIPS_GET_IMMED(mips), rdram_base);
			break;
			case MEM_LHU:
				GEN_LHZ(rd, MIPS_GET_IMMED(mips), rdram_base);
			break;
			case MEM_LH:
				GEN_LHA(rd, MIPS_GET_IMMED(mips), rdram_base);
			break;
			case MEM_LB:
				GEN_LBZ(rd, MIPS_GET_IMMED(mips), rdram_base);
				// extsb rt
				GEN_EXTSB(rd, rd);
			break;
			default:
			break;
		}
	}
	if(isVirtual) {
		mapRegisterNew( MIPS_GET_RT(mips) );
		flushRegisters();
	}
	else {
		unmapRegisterTemp(rdram_base);
	}

	if(isPhysical && isVirtual){
		// Skip over else
		not_fastmem_id[1] = add_jump_special(1);
		GEN_B(not_fastmem_id[1], 0, 0);
		preCall[1] = get_curr_dst();
	}

	if(isVirtual){
		int callSize = get_curr_dst() - preCall[0];
		set_jump_special(not_fastmem_id[0], callSize+1);
		// load into rt
		GEN_LI(rd, 0, MIPS_GET_RT(mips));
		genCallDynaMem(type, base, MIPS_GET_IMMED(mips));
	}

	if(isPhysical && isVirtual){
		int callSize = get_curr_dst() - preCall[1];
		set_jump_special(not_fastmem_id[1], callSize+1);
	}
}

void genCallDynaMem(memType type, int base, short immed){
	// PRE: value to store, or register # to load into should be in r3
	// Pass PC as arg 4 (upper half)
	GEN_LIS(6, extractUpper16(get_src_pc()+4));
	// addr = base + immed (arg 2)
	GEN_ADDI(4, base, immed);
	// type passed as arg 3
	GEN_LI(5, 0, type);
	// Lower half of PC
	GEN_ADDI(6, 6, extractLower16(get_src_pc()+4));
	// isDelaySlot as arg 5
	GEN_LI(7, 0, isDelaySlot ? 1 : 0);
	// call dyna_mem
	GEN_B(add_jump((unsigned long)(&dyna_mem), 1, 1), 0, 1);
	// Load old LR
	GEN_LWZ(0, DYNAOFF_LR, 1);
	// Check whether we need to take an interrupt
	GEN_CMPI(3, 0, 6);
	// Restore LR
	GEN_MTLR(0);
	// If so, return to trampoline
	GEN_BNELR(6, 0);
}

void genRecompileStore(memType type, MIPS_instr mips){
	int isConstant = isRegisterConstant( MIPS_GET_RS(mips) );
	int isPhysical = 1, isVirtual = 1;
	if(isConstant){
		int immediate = MIPS_GET_IMMED(mips);
		int constant = getRegisterConstant( MIPS_GET_RS(mips) );
		immediate |= (immediate&0x8000) ? 0xffff0000 : 0;
		
		if((constant + immediate) > (int)RAM_TOP)
			isPhysical = 0;
		else
			isVirtual = 0;
	}

	int rd = 0;
	flushRegisters();
	reset_code_addr();
	if(type == MEM_SWC1) {
		genCheckFP();
		rd = mapRegisterTemp(); // r3 = rd
	}
	else {
		if( MIPS_GET_RT(mips) ){
			rd = mapRegister( MIPS_GET_RT(mips) ); // r3 = value
		} else {
			rd = mapRegisterTemp();
			GEN_LI(rd, 0, 0); // r3 = 0
		}
	}
	int base = mapRegister( MIPS_GET_RS(mips) ); // r4 = addr
	int addr = mapRegisterTemp(); // r5 = fpr_addr
	short immed = MIPS_GET_IMMED(mips);
	invalidateRegisters();
	
	if(type == MEM_SWC1) {	
		// addr = reg_cop1_simple[frt]
		GEN_LWZ(addr, (MIPS_GET_RT(mips)*4)+R4300OFF_FPR_32, DYNAREG_R4300);
		// frs = *addr
		GEN_LWZ(rd, 0, addr);
	}
	
	// DYNAREG_RADDR = address
	GEN_ADDI(4, base, immed);

	if(isPhysical && isVirtual){
		// If base in physical memory
		GEN_CMP(4, DYNAREG_MEM_TOP, 1);
		GEN_BGE(1, 11, 0, 0);
	}
	
	if(isPhysical) {
		/* Fast case - inside RDRAM */

		// Perform the actual store
		if(type == MEM_SB){
			GEN_STBX(3, DYNAREG_RDRAM, 4);
		}
		else if(type == MEM_SH) {
			GEN_STHX(3, DYNAREG_RDRAM, 4);
		}
		else if(type == MEM_SW || type == MEM_SWC1) {
			GEN_STWX(3, DYNAREG_RDRAM, 4);
		}
		
		// r5 = invalid_code_get(address>>12)
		GEN_RLWINM(6, 4, 20, 12, 31);	// address >> 12
		GEN_LBZX(5, 6, DYNAREG_INVCODE);
		GEN_CMPI(5, 0, 0);

		// if (!r5)
		GEN_BNE(0, (isVirtual && isPhysical) ? 14 : 5, 0, 0);
		GEN_ADDI(3, 4, 0);
		// invalidate_func(address);
		GEN_B(add_jump((unsigned long)(&invalidate_func), 1, 1), 0, 1);
		
		// Load old LR
		GEN_LWZ(0, DYNAOFF_LR, 1);
		// Restore LR
		GEN_MTLR(0);
	}
	
	if(isVirtual && isPhysical) {
		// Skip over else
		GEN_B(9, 0, 0);
	}
	
	if(isVirtual) {
		/* Slow case outside of RDRAM */
		
		// r4 = address (already set)
		// adjust delay_slot and pc
		// r5 = pc
		GEN_LIS(5, extractUpper16(get_src_pc()+4));
		// r6 = isDelaySlot
		GEN_LI(6, 0, isDelaySlot ? 1 : 0);
		GEN_ADDI(5, 5, extractLower16(get_src_pc()+4));
		// call dyna_mem_write_<type>
		if(type == MEM_SB){
			GEN_B(add_jump((unsigned long)(&dyna_mem_write_byte), 1, 1), 0, 1);
		}
		else if(type == MEM_SH) {
			GEN_B(add_jump((unsigned long)(&dyna_mem_write_hword), 1, 1), 0, 1);
		}
		else if(type == MEM_SW || type == MEM_SWC1) {
			GEN_B(add_jump((unsigned long)(&dyna_mem_write_word), 1, 1), 0, 1);
		}
		
		// Check whether we need to take an interrupt
		GEN_CMPI(3, 0, 6);	
		// Load old LR
		GEN_LWZ(0, DYNAOFF_LR, 1);
		// Restore LR
		GEN_MTLR(0);
		// If so, return to trampoline
		GEN_BNELR(6, 0);
	}
}

static int mips_is_jump(MIPS_instr instr){
	int opcode = MIPS_GET_OPCODE(instr);
	int format = MIPS_GET_RS    (instr);
	int func   = MIPS_GET_FUNC  (instr);
	return (opcode == MIPS_OPCODE_J     ||
                opcode == MIPS_OPCODE_JAL   ||
                opcode == MIPS_OPCODE_BEQ   ||
                opcode == MIPS_OPCODE_BNE   ||
                opcode == MIPS_OPCODE_BLEZ  ||
                opcode == MIPS_OPCODE_BGTZ  ||
                opcode == MIPS_OPCODE_BEQL  ||
                opcode == MIPS_OPCODE_BNEL  ||
                opcode == MIPS_OPCODE_BLEZL ||
                opcode == MIPS_OPCODE_BGTZL ||
                opcode == MIPS_OPCODE_B     ||
                (opcode == MIPS_OPCODE_R    &&
                 (func  == MIPS_FUNC_JR     ||
                  func  == MIPS_FUNC_JALR)) ||
                (opcode == MIPS_OPCODE_COP1 &&
                 format == MIPS_FRMT_BC)    );
}
