/**
 * Wii64 - MIPS-to-PPC.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * Functions for converting MIPS code to PPC
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

#ifndef MIPS_TO_PPC_H
#define MIPS_TO_PPC_H

#include "MIPS.h"
#include "PowerPC.h"

#define CONVERT_ERROR   -1
#define CONVERT_SUCCESS  0
#define CONVERT_WARNING  1
#define INTERPRETED      2

/* These functions must be implemented
   by code using this module           */
extern MIPS_instr get_next_src(void);
extern MIPS_instr peek_next_src(void);
extern void       set_next_dst(PowerPC_instr);
// These are unfortunate hacks necessary for jumping to delay slots
extern PowerPC_instr* get_curr_dst(void);
extern void unget_last_src(void);
extern void nop_ignored(void);
extern int is_j_dst(int i);
extern unsigned int get_src_pc(void);
// Adjust code_addr to not include flushing of previous mappings
void reset_code_addr(void);
/* Adds src and dst address, and src jump address to tables
    it returns a unique address identifier.
   This data should be used to fill in addresses in pass two. */
extern int  add_jump(int old_address, int is_li, int is_aa);
extern int  is_j_out(int branch, int is_aa);
// Use these for jumps that won't be known until later in compile time
extern int  add_jump_special(int is_j);
extern void set_jump_special(int which, int new_jump);
// Set up appropriate register mappings
void start_new_block(void);
void start_new_mapping(void);

/* Convert one conceptual instruction
    this may use and/or generate more
    than one actual instruction       */
int convert(void);

#define GEN_B(dst,aa,lk) \
{ PowerPC_instr ppc; __B(ppc,dst,aa,lk); set_next_dst(ppc); }
#define GEN_MTCTR(rs) \
{ PowerPC_instr ppc; _MTCTR(ppc,rs); set_next_dst(ppc); }
#define GEN_MFCTR(rd) \
{ PowerPC_instr ppc; _MFCTR(ppc,rd); set_next_dst(ppc); }
#define GEN_ADDIS(rd,ra,immed) \
{ PowerPC_instr ppc; _ADDIS(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_LIS(rd,immed) \
{ PowerPC_instr ppc; _LIS(ppc,rd,immed); set_next_dst(ppc); }
#define GEN_LI(rd,immed) \
{ PowerPC_instr ppc; _LI(ppc,rd,immed); set_next_dst(ppc); }
#define GEN_LWZ(rd,immed,ra) \
{ PowerPC_instr ppc; _LWZ(ppc,rd,immed,ra); set_next_dst(ppc); }
#define GEN_LWZU(rd,immed,ra) \
{ PowerPC_instr ppc; _LWZU(ppc,rd,immed,ra); set_next_dst(ppc); }
#define GEN_LHZ(rd,immed,ra) \
{ PowerPC_instr ppc; _LHZ(ppc,rd,immed,ra); set_next_dst(ppc); }
#define GEN_LHZU(rd,immed,ra) \
{ PowerPC_instr ppc; _LHZU(ppc,rd,immed,ra); set_next_dst(ppc); }
#define GEN_LHZX(rd,ra,rb) \
{ PowerPC_instr ppc; _LHZX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_LHZUX(rd,ra,rb) \
{ PowerPC_instr ppc; _LHZUX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_LHA(rd,immed,ra) \
{ PowerPC_instr ppc; _LHA(ppc,rd,immed,ra); set_next_dst(ppc); }
#define GEN_LHAU(rd,immed,ra) \
{ PowerPC_instr ppc; _LHAU(ppc,rd,immed,ra); set_next_dst(ppc); }
#define GEN_LHAX(rd,ra,rb) \
{ PowerPC_instr ppc; _LHAX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_LHAUX(rd,ra,rb) \
{ PowerPC_instr ppc; _LHAUX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_LBZ(rd,immed,ra) \
{ PowerPC_instr ppc; _LBZ(ppc,rd,immed,ra); set_next_dst(ppc); }
#define GEN_LBZU(rd,immed,ra) \
{ PowerPC_instr ppc; _LBZU(ppc,rd,immed,ra); set_next_dst(ppc); }
#define GEN_LBZX(rd,ra,rb) \
{ PowerPC_instr ppc; _LBZX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_LBZUX(rd,ra,rb) \
{ PowerPC_instr ppc; _LBZUX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_EXTSB(rd,rs) \
{ PowerPC_instr ppc; _EXTSB(ppc,rd,rs); set_next_dst(ppc); }
#define GEN_EXTSH(rd,rs) \
{ PowerPC_instr ppc; _EXTSH(ppc,rd,rs); set_next_dst(ppc); }
#define GEN_EXTSW(rd,rs) \
{ PowerPC_instr ppc; _EXTSW(ppc,rd,rs); set_next_dst(ppc); }
#define GEN_STB(rs,immed,ra) \
{ PowerPC_instr ppc; _STB(ppc,rs,immed,ra); set_next_dst(ppc); }
#define GEN_STBU(rs,immed,ra) \
{ PowerPC_instr ppc; _STBU(ppc,rs,immed,ra); set_next_dst(ppc); }
#define GEN_STBX(rd,ra,rb) \
{ PowerPC_instr ppc; _STBX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_STBUX(rd,ra,rb) \
{ PowerPC_instr ppc; _STBUX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_STH(rs,immed,ra) \
{ PowerPC_instr ppc; _STH(ppc,rs,immed,ra); set_next_dst(ppc); }
#define GEN_STHU(rs,immed,ra) \
{ PowerPC_instr ppc; _STHU(ppc,rs,immed,ra); set_next_dst(ppc); }
#define GEN_STHX(rd,ra,rb) \
{ PowerPC_instr ppc; _STHX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_STHUX(rd,ra,rb) \
{ PowerPC_instr ppc; _STHUX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_STW(rs,immed,ra) \
{ PowerPC_instr ppc; _STW(ppc,rs,immed,ra); set_next_dst(ppc); }
#define GEN_STWU(rs,immed,ra) \
{ PowerPC_instr ppc; _STWU(ppc,rs,immed,ra); set_next_dst(ppc); }
#define GEN_STWX(rd,ra,rb) \
{ PowerPC_instr ppc; _STWX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_STWUX(rd,ra,rb) \
{ PowerPC_instr ppc; _STWUX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_BCTR(ppce) \
{ PowerPC_instr ppc; _BCTR(ppc); set_next_dst(ppc); }
#define GEN_BCTRL(ppce) \
{ PowerPC_instr ppc; _BCTRL(ppc); set_next_dst(ppc); }
#define GEN_BCCTR(bo,bi,lk) \
{ PowerPC_instr ppc; _BCCTR(ppc,bo,bi,lk); set_next_dst(ppc); }
#define GEN_CMP(cr,ra,rb) \
{ PowerPC_instr ppc; _CMP(ppc,cr,ra,rb); set_next_dst(ppc); }
#define GEN_CMPL(cr,ra,rb) \
{ PowerPC_instr ppc; _CMPL(ppc,cr,ra,rb); set_next_dst(ppc); }
#define GEN_CMPI(cr,ra,immed) \
{ PowerPC_instr ppc; _CMPI(ppc,cr,ra,immed); set_next_dst(ppc); }
#define GEN_CMPLI(cr,ra,immed) \
{ PowerPC_instr ppc; _CMPLI(ppc,cr,ra,immed); set_next_dst(ppc); }
#define GEN_BC(dst,aa,lk,bo,bi) \
{ PowerPC_instr ppc; _BC(ppc,dst,aa,lk,bo,bi); set_next_dst(ppc); }
#define GEN_BNE(cr,dst,aa,lk) \
{ PowerPC_instr ppc; _BNE(ppc,cr,dst,aa,lk); set_next_dst(ppc); }
#define GEN_BEQ(cr,dst,aa,lk) \
{ PowerPC_instr ppc; _BEQ(ppc,cr,dst,aa,lk); set_next_dst(ppc); }
#define GEN_BGT(cr,dst,aa,lk) \
{ PowerPC_instr ppc; _BGT(ppc,cr,dst,aa,lk); set_next_dst(ppc); }
#define GEN_BLE(cr,dst,aa,lk) \
{ PowerPC_instr ppc; _BLE(ppc,cr,dst,aa,lk); set_next_dst(ppc); }
#define GEN_BGE(cr,dst,aa,lk) \
{ PowerPC_instr ppc; _BGE(ppc,cr,dst,aa,lk); set_next_dst(ppc); }
#define GEN_BLT(cr,dst,aa,lk) \
{ PowerPC_instr ppc; _BLT(ppc,cr,dst,aa,lk); set_next_dst(ppc); }
#define GEN_ADDI(rd,ra,immed) \
{ PowerPC_instr ppc; _ADDI(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_SUBI(rd,ra,immed) \
{ PowerPC_instr ppc; _SUBI(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_RLWIMI(rd,ra,sh,mb,me) \
{ PowerPC_instr ppc; _RLWIMI(ppc,rd,ra,sh,mb,me); set_next_dst(ppc); }
#define GEN_RLWINM(rd,ra,sh,mb,me) \
{ PowerPC_instr ppc; _RLWINM(ppc,rd,ra,sh,mb,me); set_next_dst(ppc); }
#define GEN_RLWINM_(rd,ra,sh,mb,me) \
{ PowerPC_instr ppc; _RLWINM_(ppc,rd,ra,sh,mb,me); set_next_dst(ppc); }
#define GEN_RLWIMI(rd,ra,sh,mb,me) \
{ PowerPC_instr ppc; _RLWIMI(ppc,rd,ra,sh,mb,me); set_next_dst(ppc); }
#define GEN_CLRLWI(rd,ra,n) \
{ PowerPC_instr ppc; _CLRLWI(ppc,rd,ra,n); set_next_dst(ppc); }
#define GEN_CLRRWI(rd,ra,n) \
{ PowerPC_instr ppc; _CLRRWI(ppc,rd,ra,n); set_next_dst(ppc); }
#define GEN_CLRLSLWI(rd,ra,b,n) \
{ PowerPC_instr ppc; _CLRLSLWI(ppc,rd,ra,b,n); set_next_dst(ppc); }
#define GEN_SRWI(rd,ra,sh) \
{ PowerPC_instr ppc; _SRWI(ppc,rd,ra,sh); set_next_dst(ppc); }
#define GEN_SLWI(rd,ra,sh) \
{ PowerPC_instr ppc; _SLWI(ppc,rd,ra,sh); set_next_dst(ppc); }
#define GEN_SRAWI(rd,ra,sh) \
{ PowerPC_instr ppc; _SRAWI(ppc,rd,ra,sh); set_next_dst(ppc); }
#define GEN_SLW(rd,ra,rb) \
{ PowerPC_instr ppc; _SLW(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_SRW(rd,ra,rb) \
{ PowerPC_instr ppc; _SRW(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_SRAW(rd,ra,rb) \
{ PowerPC_instr ppc; _SRAW(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_ANDI(rd,ra,immed) \
{ PowerPC_instr ppc; _ANDI(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_ORI(rd,ra,immed) \
{ PowerPC_instr ppc; _ORI(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_XORI(rd,ra,immed) \
{ PowerPC_instr ppc; _XORI(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_XORIS(rd,ra,immed) \
{ PowerPC_instr ppc; _XORIS(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_MULLW(rd,ra,rb) \
{ PowerPC_instr ppc; _MULLW(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_MULHW(rd,ra,rb) \
{ PowerPC_instr ppc; _MULHW(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_MULHWU(rd,ra,rb) \
{ PowerPC_instr ppc; _MULHWU(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_DIVW(rd,ra,rb) \
{ PowerPC_instr ppc; _DIVW(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_DIVWU(rd,ra,rb) \
{ PowerPC_instr ppc; _DIVWU(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_ADD(rd,ra,rb) \
{ PowerPC_instr ppc; _ADD(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_SUBF(rd,ra,rb) \
{ PowerPC_instr ppc; _SUBF(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_SUBFC(rd,ra,rb) \
{ PowerPC_instr ppc; _SUBFC(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_SUBFE(rd,ra,rb) \
{ PowerPC_instr ppc; _SUBFE(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_ADDIC(rd,ra,immed) \
{ PowerPC_instr ppc; _ADDIC(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_ADDIC_(rd,ra,immed) \
{ PowerPC_instr ppc; _ADDIC_(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_SUB(rd,ra,rb) \
{ PowerPC_instr ppc; _SUB(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_SUBIC(rd,ra,immed) \
{ PowerPC_instr ppc; _SUBIC(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_SUBIC_(rd,ra,immed) \
{ PowerPC_instr ppc; _SUBIC_(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_AND(rd,ra,rb) \
{ PowerPC_instr ppc; __AND(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_NAND(rd,ra,rb) \
{ PowerPC_instr ppc; _NAND(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_ANDC(rd,ra,rb) \
{ PowerPC_instr ppc; _ANDC(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_NOR(rd,ra,rb) \
{ PowerPC_instr ppc; _NOR(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_NOT(rd,rs) \
{ PowerPC_instr ppc; _NOT(ppc,rd,rs); set_next_dst(ppc); }
#define GEN_OR(rd,ra,rb) \
{ PowerPC_instr ppc; _OR(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_MR(rd,rs) \
{ PowerPC_instr ppc; _MR(ppc,rd,rs); set_next_dst(ppc); }
#define GEN_XOR(rd,ra,rb) \
{ PowerPC_instr ppc; _XOR(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_BLR(lk) \
{ PowerPC_instr ppc; _BLR(ppc,lk); set_next_dst(ppc); }
#define GEN_MTLR(rs) \
{ PowerPC_instr ppc; _MTLR(ppc,rs); set_next_dst(ppc); }
#define GEN_MFLR(rd) \
{ PowerPC_instr ppc; _MFLR(ppc,rd); set_next_dst(ppc); }
#define GEN_MTCR(rs) \
{ PowerPC_instr ppc; _MTCR(ppc,rs); set_next_dst(ppc); }
#define GEN_NEG(rd,rs) \
{ PowerPC_instr ppc; _NEG(ppc,rd,rs); set_next_dst(ppc); }
#define GEN_EQV(rd,ra,rb) \
{ PowerPC_instr ppc; _EQV(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_ADDZE(rd,rs) \
{ PowerPC_instr ppc; _ADDZE(ppc,rd,rs); set_next_dst(ppc); }
#define GEN_ADDC(rd,ra,rb) \
{ PowerPC_instr ppc; _ADDC(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_ADDE(rd,ra,rb) \
{ PowerPC_instr ppc; _ADDE(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_SUBFIC(rd,ra,immed) \
{ PowerPC_instr ppc; _SUBFIC(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_SUBFME(rd,rs) \
{ PowerPC_instr ppc; _SUBFME(ppc,rd,rs); set_next_dst(ppc); }
#define GEN_SUBFZE(rd,rs) \
{ PowerPC_instr ppc; _SUBFZE(ppc,rd,rs); set_next_dst(ppc); }
#define GEN_STFD(fs,immed,rb) \
{ PowerPC_instr ppc; _STFD(ppc,fs,immed,rb); set_next_dst(ppc); }
#define GEN_STFDU(fs,immed,rb) \
{ PowerPC_instr ppc; _STFDU(ppc,fs,immed,rb); set_next_dst(ppc); }
#define GEN_STFDX(rd,ra,rb) \
{ PowerPC_instr ppc; _STFDX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_STFDUX(rd,ra,rb) \
{ PowerPC_instr ppc; _STFDUX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_STFS(fs,immed,rb) \
{ PowerPC_instr ppc; _STFS(ppc,fs,immed,rb); set_next_dst(ppc); }
#define GEN_STFSU(fs,immed,rb) \
{ PowerPC_instr ppc; _STFSU(ppc,fs,immed,rb); set_next_dst(ppc); }
#define GEN_STFSX(rd,ra,rb) \
{ PowerPC_instr ppc; _STFSX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_STFSUX(rd,ra,rb) \
{ PowerPC_instr ppc; _STFSUX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_LFD(fd,immed,rb) \
{ PowerPC_instr ppc; _LFD(ppc,fd,immed,rb); set_next_dst(ppc); }
#define GEN_LFDU(fd,immed,rb) \
{ PowerPC_instr ppc; _LFDU(ppc,fd,immed,rb); set_next_dst(ppc); }
#define GEN_LFDX(rd,ra,rb) \
{ PowerPC_instr ppc; _LFDX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_LFDUX(rd,ra,rb) \
{ PowerPC_instr ppc; _LFDUX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_LFS(fd,immed,rb) \
{ PowerPC_instr ppc; _LFS(ppc,fd,immed,rb); set_next_dst(ppc); }
#define GEN_LFSU(fd,immed,rb) \
{ PowerPC_instr ppc; _LFSU(ppc,fd,immed,rb); set_next_dst(ppc); }
#define GEN_LFSX(rd,ra,rb) \
{ PowerPC_instr ppc; _LFSX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_LFSUX(rd,ra,rb) \
{ PowerPC_instr ppc; _LFSUX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_FADD(fd,fa,fb,dbl) \
{ PowerPC_instr ppc; _FADD(ppc,fd,fa,fb,dbl); set_next_dst(ppc); }
#define GEN_FSUB(fd,fa,fb,dbl) \
{ PowerPC_instr ppc; _FSUB(ppc,fd,fa,fb,dbl); set_next_dst(ppc); }
#define GEN_FMUL(fd,fa,fb,dbl) \
{ PowerPC_instr ppc; _FMUL(ppc,fd,fa,fb,dbl); set_next_dst(ppc); }
#define GEN_FDIV(fd,fa,fb,dbl) \
{ PowerPC_instr ppc; _FDIV(ppc,fd,fa,fb,dbl); set_next_dst(ppc); }
#define GEN_FABS(fd,fs) \
{ PowerPC_instr ppc; _FABS(ppc,fd,fs); set_next_dst(ppc); }
#define GEN_FCFID(fd,fs) \
{ PowerPC_instr ppc; _FCFID(ppc,fd,fs); set_next_dst(ppc); }
#define GEN_FRSP(fd,fs) \
{ PowerPC_instr ppc; _FRSP(ppc,fd,fs); set_next_dst(ppc); }
#define GEN_FMR(fd,fs) \
{ PowerPC_instr ppc; _FMR(ppc,fd,fs); set_next_dst(ppc); }
#define GEN_FNEG(fd,fs) \
{ PowerPC_instr ppc; _FNEG(ppc,fd,fs); set_next_dst(ppc); }
#define GEN_FCTIW(fd,fs) \
{ PowerPC_instr ppc; _FCTIW(ppc,fd,fs); set_next_dst(ppc); }
#define GEN_FCTIWZ(fd,fs) \
{ PowerPC_instr ppc; _FCTIWZ(ppc,fd,fs); set_next_dst(ppc); }
#define GEN_STFIWX(fs,ra,rb) \
{ PowerPC_instr ppc; _STFIWX(ppc,fs,ra,rb); set_next_dst(ppc); }
#define GEN_MTFSFI(field,immed) \
{ PowerPC_instr ppc; _MTFSFI(ppc,field,immed); set_next_dst(ppc); }
#define GEN_MTFSF(fields,fs) \
{ PowerPC_instr ppc; _MTFSF(ppc,fields,fs); set_next_dst(ppc); }
#define GEN_FCMPU(cr,fa,fb) \
{ PowerPC_instr ppc; _FCMPU(ppc,cr,fa,fb); set_next_dst(ppc); }
#define GEN_FRSQRTE(fd,fs) \
{ PowerPC_instr ppc; _FRSQRTE(ppc,fd,fs); set_next_dst(ppc); }
#define GEN_FSQRT(fd,fs) \
{ PowerPC_instr ppc; _FSQRT(ppc,fd,fs); set_next_dst(ppc); }
#define GEN_FSQRTS(fd,fs) \
{ PowerPC_instr ppc; _FSQRTS(ppc,fd,fs); set_next_dst(ppc); }
#define GEN_FSEL(fd,fa,fb,fc) \
{ PowerPC_instr ppc; _FSEL(ppc,fd,fa,fb,fc); set_next_dst(ppc); }
#define GEN_FRES(fd,fs) \
{ PowerPC_instr ppc; _FRES(ppc,fd,fs); set_next_dst(ppc); }
#define GEN_FNMADD(fd,fa,fb,fc,dbl) \
{ PowerPC_instr ppc; _FNMADD(ppc,fd,fa,fb,fc,dbl); set_next_dst(ppc); }
#define GEN_FNMSUB(fd,fa,fb,fc,dbl) \
{ PowerPC_instr ppc; _FNMSUB(ppc,fd,fa,fb,fc,dbl); set_next_dst(ppc); }
#define GEN_FNMSUBS(fd,fa,fc,fb) \
{ PowerPC_instr ppc; _FNMSUBS(ppc,fd,fa,fc,fb); set_next_dst(ppc); }
#define GEN_FMADD(fd,fa,fc,fb, dbl) \
{ PowerPC_instr ppc; _FMADD(ppc,fd,fa,fc,fb,dbl); set_next_dst(ppc); }
#define GEN_FMSUB(fd,fa,fc,fb, dbl) \
{ PowerPC_instr ppc; _FMSUB(ppc,fd,fa,fc,fb,dbl); set_next_dst(ppc); }
#define GEN_FMADDS(fd,fa,fc,fb) \
{ PowerPC_instr ppc; _FMADDS(ppc,fd,fa,fc,fb); set_next_dst(ppc); }
#define GEN_BCLR(lk,bo,bi) \
{ PowerPC_instr ppc; _BCLR(ppc,lk,bo,bi); set_next_dst(ppc); }
#define GEN_BNELR(cr,lk) \
{ PowerPC_instr ppc; _BNELR(ppc,cr,lk); set_next_dst(ppc); }
#define GEN_BLELR(cr,lk) \
{ PowerPC_instr ppc; _BLELR(ppc,cr,lk); set_next_dst(ppc); }
#define GEN_ANDIS(rd,ra,immed) \
{ PowerPC_instr ppc; _ANDIS(ppc,rd,ra,immed); set_next_dst(ppc); }
#define GEN_ORIS(rd,rs,immed) \
{ PowerPC_instr ppc; _ORIS(ppc,rd,rs,immed); set_next_dst(ppc); }
#define GEN_CROR(cd,ca,cb) \
{ PowerPC_instr ppc; _CROR(ppc,cd,ca,cb); set_next_dst(ppc); }
#define GEN_CRORC(cd,ca,cb) \
{ PowerPC_instr ppc; _CRORC(ppc,cd,ca,cb); set_next_dst(ppc); }
#define GEN_CRXOR(cd,ca,cb) \
{ PowerPC_instr ppc; _CRXOR(ppc,cd,ca,cb); set_next_dst(ppc); }
#define GEN_CRNOR(cd,ca,cb) \
{ PowerPC_instr ppc; _CRNOR(ppc,cd,ca,cb); set_next_dst(ppc); }
#define GEN_MFCR(rt) \
{ PowerPC_instr ppc; _MFCR(ppc,rt); set_next_dst(ppc); }
#define GEN_MCRXR(bf) \
{ PowerPC_instr ppc; _MCRXR(ppc,bf); set_next_dst(ppc); }
#define GEN_ADDME(rd,ra) \
{ PowerPC_instr ppc; _ADDME(ppc,rd,ra); set_next_dst(ppc); }
#define GEN_LWZX(rd,ra,rb) \
{ PowerPC_instr ppc; _LWZX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_LWZUX(rd,ra,rb) \
{ PowerPC_instr ppc; _LWZUX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_STBX(rd,ra,rb) \
{ PowerPC_instr ppc; _STBX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_STHX(rd,ra,rb) \
{ PowerPC_instr ppc; _STHX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_STWX(rd,ra,rb) \
{ PowerPC_instr ppc; _STWX(ppc,rd,ra,rb); set_next_dst(ppc); }
#define GEN_PSQ_L(fd,imm12,ra,qri) \
{ PowerPC_instr ppc; _PSQ_L(ppc,fd,imm12,ra,qri); set_next_dst(ppc); }


#endif
