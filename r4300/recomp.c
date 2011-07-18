/**
 * Mupen64 - recomp.c
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

#include <malloc.h>

#include "recomp.h"
#include "macros.h"
#include "r4300.h"
#include "../gc_memory/memory.h"
#include "Invalid_Code.h"
#include "Recomp-Cache.h"

#include "../gui/DEBUG.h"

long src; // the current recompiled instruction


#define RSV recompile_nothing

static void recompile_standard_i_type()
{
   PC->f.i.rs = r4300.gpr + ((src >> 21) & 0x1F);
   PC->f.i.rt = r4300.gpr + ((src >> 16) & 0x1F);
   PC->f.i.immediate = src & 0xFFFF;
}

static void recompile_standard_j_type()
{
   PC->f.j.inst_index = src & 0x3FFFFFF;
}

static void recompile_standard_r_type()
{
   PC->f.r.rs = r4300.gpr + ((src >> 21) & 0x1F);
   PC->f.r.rt = r4300.gpr + ((src >> 16) & 0x1F);
   PC->f.r.rd = r4300.gpr + ((src >> 11) & 0x1F);
   PC->f.r.sa = (src >>  6) & 0x1F;
}

static void recompile_standard_lf_type()
{
   PC->f.lf.base = (src >> 21) & 0x1F;
   PC->f.lf.ft = (src >> 16) & 0x1F;
   PC->f.lf.offset = src & 0xFFFF;
}

static void recompile_standard_cf_type()
{
   PC->f.cf.ft = (src >> 16) & 0x1F;
   PC->f.cf.fs = (src >> 11) & 0x1F;
   PC->f.cf.fd = (src >>  6) & 0x1F;
}

static void recompile_nothing() { }

//-------------------------------------------------------------------------
//                                  SPECIAL                                
//-------------------------------------------------------------------------

#define RSLL recompile_standard_r_type

#define RSRL recompile_standard_r_type

#define RSRA recompile_standard_r_type

#define RSLLV recompile_standard_r_type

#define RSRLV recompile_standard_r_type

#define RSRAV recompile_standard_r_type

#define RJR recompile_standard_i_type

#define RJALR recompile_standard_r_type

#define RSYSCALL recompile_nothing

#define RBREAK recompile_nothing

#define RSYNC recompile_nothing

#define RMFHI recompile_standard_r_type

#define RMTHI recompile_standard_r_type

#define RMFLO recompile_standard_r_type

#define RMTLO recompile_standard_r_type

#define RDSLLV recompile_standard_r_type

#define RDSRLV recompile_standard_r_type

#define RDSRAV recompile_standard_r_type

#define RMULT recompile_standard_r_type

#define RMULTU recompile_standard_r_type

#define RDIV recompile_standard_r_type

#define RDIVU recompile_standard_r_type

#define RDMULT recompile_standard_r_type

#define RDMULTU recompile_standard_r_type

#define RDDIV recompile_standard_r_type

#define RDDIVU recompile_standard_r_type

#define RADD recompile_standard_r_type

#define RADDU recompile_standard_r_type

#define RSUB recompile_standard_r_type

#define RSUBU recompile_standard_r_type

#define RAND recompile_standard_r_type

#define ROR recompile_standard_r_type

#define RXOR recompile_standard_r_type

#define RNOR recompile_standard_r_type

#define RSLT recompile_standard_r_type

#define RSLTU recompile_standard_r_type

#define RDADD recompile_standard_r_type

#define RDADDU recompile_standard_r_type

#define RDSUB recompile_standard_r_type

#define RDSUBU recompile_standard_r_type

#define RTGE recompile_nothing

#define RTGEU recompile_nothing

#define RTLT recompile_nothing

#define RTLTU recompile_nothing

#define RTEQ recompile_standard_r_type

#define RTNE recompile_nothing

#define RDSLL recompile_standard_r_type

#define RDSRL recompile_standard_r_type

#define RDSRA recompile_standard_r_type

#define RDSLL32 recompile_standard_r_type

#define RDSRL32 recompile_standard_r_type

#define RDSRA32 recompile_standard_r_type

static void (*recomp_special[64])(void) =
{
   RSLL , RSV   , RSRL , RSRA , RSLLV   , RSV    , RSRLV  , RSRAV  ,
   RJR  , RJALR , RSV  , RSV  , RSYSCALL, RBREAK , RSV    , RSYNC  ,
   RMFHI, RMTHI , RMFLO, RMTLO, RDSLLV  , RSV    , RDSRLV , RDSRAV ,
   RMULT, RMULTU, RDIV , RDIVU, RDMULT  , RDMULTU, RDDIV  , RDDIVU ,
   RADD , RADDU , RSUB , RSUBU, RAND    , ROR    , RXOR   , RNOR   ,
   RSV  , RSV   , RSLT , RSLTU, RDADD   , RDADDU , RDSUB  , RDSUBU ,
   RTGE , RTGEU , RTLT , RTLTU, RTEQ    , RSV    , RTNE   , RSV    ,
   RDSLL, RSV   , RDSRL, RDSRA, RDSLL32 , RSV    , RDSRL32, RDSRA32
};

//-------------------------------------------------------------------------
//                                   REGIMM                                
//-------------------------------------------------------------------------

#define RBLTZ recompile_standard_i_type

#define RBGEZ recompile_standard_i_type

#define RBLTZL recompile_standard_i_type

#define RBGEZL recompile_standard_i_type

#define RTGEI recompile_nothing

#define RTGEIU recompile_nothing

#define RTLTI recompile_nothing

#define RTLTIU recompile_nothing

#define RTEQI recompile_nothing

#define RTNEI recompile_nothing

#define RBLTZAL recompile_standard_i_type

#define RBGEZAL recompile_standard_i_type

#define RBLTZALL recompile_standard_i_type

#define RBGEZALL recompile_standard_i_type

static void (*recomp_regimm[32])(void) =
{
   RBLTZ  , RBGEZ  , RBLTZL  , RBGEZL  , RSV  , RSV, RSV  , RSV,
   RTGEI  , RTGEIU , RTLTI   , RTLTIU  , RTEQI, RSV, RTNEI, RSV,
   RBLTZAL, RBGEZAL, RBLTZALL, RBGEZALL, RSV  , RSV, RSV  , RSV,
   RSV    , RSV    , RSV     , RSV     , RSV  , RSV, RSV  , RSV
};

//-------------------------------------------------------------------------
//                                     TLB                                 
//-------------------------------------------------------------------------

#define RTLBR recompile_nothing

#define RTLBWI recompile_nothing

#define RTLBWR recompile_nothing

#define RTLBP recompile_nothing

#define RERET recompile_nothing

static void (*recomp_tlb[64])(void) =
{
   RSV  , RTLBR, RTLBWI, RSV, RSV, RSV, RTLBWR, RSV, 
   RTLBP, RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RERET, RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV
};

//-------------------------------------------------------------------------
//                                    COP0                                 
//-------------------------------------------------------------------------

static void RMFC0()
{
      recompile_standard_r_type();
   PC->f.r.rd = (long long*)(r4300.reg_cop0 + ((src >> 11) & 0x1F));
   PC->f.r.nrd = (src >> 11) & 0x1F;
}

static void RMTC0()
{
      recompile_standard_r_type();
   PC->f.r.nrd = (src >> 11) & 0x1F;

}

static void RTLB()
{
   recomp_tlb[(src & 0x3F)]();
}

static void (*recomp_cop0[32])(void) =
{
   RMFC0, RSV, RSV, RSV, RMTC0, RSV, RSV, RSV,
   RSV  , RSV, RSV, RSV, RSV  , RSV, RSV, RSV,
   RTLB , RSV, RSV, RSV, RSV  , RSV, RSV, RSV,
   RSV  , RSV, RSV, RSV, RSV  , RSV, RSV, RSV
};

//-------------------------------------------------------------------------
//                                     BC                                  
//-------------------------------------------------------------------------

#define RBC1F recompile_standard_i_type

#define RBC1T recompile_standard_i_type

#define RBC1FL recompile_standard_i_type

#define RBC1TL recompile_standard_i_type

static void (*recomp_bc[4])(void) =
{
   RBC1F , RBC1T ,
   RBC1FL, RBC1TL
};

//-------------------------------------------------------------------------
//                                     S                                   
//-------------------------------------------------------------------------

#define RADD_S recompile_standard_cf_type

#define RSUB_S recompile_standard_cf_type

#define RMUL_S recompile_standard_cf_type

#define RDIV_S recompile_standard_cf_type

#define RSQRT_S recompile_standard_cf_type

#define RABS_S recompile_standard_cf_type

#define RMOV_S recompile_standard_cf_type

#define RNEG_S recompile_standard_cf_type

#define RROUND_L_S recompile_standard_cf_type

#define RTRUNC_L_S recompile_standard_cf_type

#define RCEIL_L_S recompile_standard_cf_type

#define RFLOOR_L_S recompile_standard_cf_type

#define RROUND_W_S recompile_standard_cf_type

#define RTRUNC_W_S recompile_standard_cf_type

#define RCEIL_W_S recompile_standard_cf_type

#define RFLOOR_W_S recompile_standard_cf_type

#define RCVT_D_S recompile_standard_cf_type

#define RCVT_W_S recompile_standard_cf_type

#define RCVT_L_S recompile_standard_cf_type

#define RC_F_S recompile_standard_cf_type

#define RC_UN_S recompile_standard_cf_type

#define RC_EQ_S recompile_standard_cf_type

#define RC_UEQ_S recompile_standard_cf_type

#define RC_OLT_S recompile_standard_cf_type

#define RC_ULT_S recompile_standard_cf_type

#define RC_OLE_S recompile_standard_cf_type

#define RC_ULE_S recompile_standard_cf_type

#define RC_SF_S recompile_standard_cf_type

#define RC_NGLE_S recompile_standard_cf_type

#define RC_SEQ_S recompile_standard_cf_type

#define RC_NGL_S recompile_standard_cf_type

#define RC_LT_S recompile_standard_cf_type

#define RC_NGE_S recompile_standard_cf_type

#define RC_LE_S recompile_standard_cf_type

#define RC_NGT_S recompile_standard_cf_type

static void (*recomp_s[64])(void) =
{
   RADD_S    , RSUB_S    , RMUL_S   , RDIV_S    , RSQRT_S   , RABS_S    , RMOV_S   , RNEG_S    , 
   RROUND_L_S, RTRUNC_L_S, RCEIL_L_S, RFLOOR_L_S, RROUND_W_S, RTRUNC_W_S, RCEIL_W_S, RFLOOR_W_S, 
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       , 
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       , 
   RSV       , RCVT_D_S  , RSV      , RSV       , RCVT_W_S  , RCVT_L_S  , RSV      , RSV       , 
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       , 
   RC_F_S    , RC_UN_S   , RC_EQ_S  , RC_UEQ_S  , RC_OLT_S  , RC_ULT_S  , RC_OLE_S , RC_ULE_S  , 
   RC_SF_S   , RC_NGLE_S , RC_SEQ_S , RC_NGL_S  , RC_LT_S   , RC_NGE_S  , RC_LE_S  , RC_NGT_S
};

//-------------------------------------------------------------------------
//                                     D                                   
//-------------------------------------------------------------------------

#define RADD_D recompile_standard_cf_type

#define RSUB_D recompile_standard_cf_type

#define RMUL_D recompile_standard_cf_type

#define RDIV_D recompile_standard_cf_type

#define RSQRT_D recompile_standard_cf_type

#define RABS_D recompile_standard_cf_type

#define RMOV_D recompile_standard_cf_type

#define RNEG_D recompile_standard_cf_type

#define RROUND_L_D recompile_standard_cf_type

#define RTRUNC_L_D recompile_standard_cf_type

#define RCEIL_L_D recompile_standard_cf_type

#define RFLOOR_L_D recompile_standard_cf_type

#define RROUND_W_D recompile_standard_cf_type

#define RTRUNC_W_D recompile_standard_cf_type

#define RCEIL_W_D recompile_standard_cf_type

#define RFLOOR_W_D recompile_standard_cf_type

#define RCVT_S_D recompile_standard_cf_type

#define RCVT_W_D recompile_standard_cf_type

#define RCVT_L_D recompile_standard_cf_type

#define RC_F_D recompile_standard_cf_type

#define RC_UN_D recompile_standard_cf_type

#define RC_EQ_D recompile_standard_cf_type

#define RC_UEQ_D recompile_standard_cf_type

#define RC_OLT_D recompile_standard_cf_type

#define RC_ULT_D recompile_standard_cf_type

#define RC_OLE_D recompile_standard_cf_type

#define RC_ULE_D recompile_standard_cf_type

#define RC_SF_D recompile_standard_cf_type

#define RC_NGLE_D recompile_standard_cf_type

#define RC_SEQ_D recompile_standard_cf_type

#define RC_NGL_D recompile_standard_cf_type

#define RC_LT_D recompile_standard_cf_type

#define RC_NGE_D recompile_standard_cf_type

#define RC_LE_D recompile_standard_cf_type

#define RC_NGT_D recompile_standard_cf_type

static void (*recomp_d[64])(void) =
{
   RADD_D    , RSUB_D    , RMUL_D   , RDIV_D    , RSQRT_D   , RABS_D    , RMOV_D   , RNEG_D    ,
   RROUND_L_D, RTRUNC_L_D, RCEIL_L_D, RFLOOR_L_D, RROUND_W_D, RTRUNC_W_D, RCEIL_W_D, RFLOOR_W_D,
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       ,
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       ,
   RCVT_S_D  , RSV       , RSV      , RSV       , RCVT_W_D  , RCVT_L_D  , RSV      , RSV       ,
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       ,
   RC_F_D    , RC_UN_D   , RC_EQ_D  , RC_UEQ_D  , RC_OLT_D  , RC_ULT_D  , RC_OLE_D , RC_ULE_D  ,
   RC_SF_D   , RC_NGLE_D , RC_SEQ_D , RC_NGL_D  , RC_LT_D   , RC_NGE_D  , RC_LE_D  , RC_NGT_D
};

//-------------------------------------------------------------------------
//                                     W                                   
//-------------------------------------------------------------------------

#define RCVT_S_W recompile_standard_cf_type

#define RCVT_D_W recompile_standard_cf_type

static void (*recomp_w[64])(void) =
{
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RCVT_S_W, RCVT_D_W, RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV
};

//-------------------------------------------------------------------------
//                                     L                                   
//-------------------------------------------------------------------------

#define RCVT_S_L recompile_standard_cf_type

#define RCVT_D_L recompile_standard_cf_type

static void (*recomp_l[64])(void) =
{
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV,
   RCVT_S_L, RCVT_D_L, RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
};

//-------------------------------------------------------------------------
//                                    COP1                                 
//-------------------------------------------------------------------------

static void RMFC1()
{
      recompile_standard_r_type();
   PC->f.r.nrd = (src >> 11) & 0x1F;
}

static void RDMFC1()
{
      recompile_standard_r_type();
   PC->f.r.nrd = (src >> 11) & 0x1F;
}

static void RCFC1()
{
      recompile_standard_r_type();
   PC->f.r.nrd = (src >> 11) & 0x1F;
}

static void RMTC1()
{
      recompile_standard_r_type();
   PC->f.r.nrd = (src >> 11) & 0x1F;

}

static void RDMTC1()
{
      recompile_standard_r_type();
   PC->f.r.nrd = (src >> 11) & 0x1F;

}

static void RCTC1()
{
      recompile_standard_r_type();
   PC->f.r.nrd = (src >> 11) & 0x1F;

}

static void RBC()
{
   recomp_bc[((src >> 16) & 3)]();
}

static void RS()
{
   recomp_s[(src & 0x3F)]();
}

static void RD()
{
   recomp_d[(src & 0x3F)]();
}

static void RW()
{
   recomp_w[(src & 0x3F)]();
}

static void RL()
{
   recomp_l[(src & 0x3F)]();
}

static void (*recomp_cop1[32])(void) =
{
   RMFC1, RDMFC1, RCFC1, RSV, RMTC1, RDMTC1, RCTC1, RSV,
   RBC  , RSV   , RSV  , RSV, RSV  , RSV   , RSV  , RSV,
   RS   , RD    , RSV  , RSV, RW   , RL    , RSV  , RSV,
   RSV  , RSV   , RSV  , RSV, RSV  , RSV   , RSV  , RSV
};

//-------------------------------------------------------------------------
//                                   R4300                                 
//-------------------------------------------------------------------------

static void RSPECIAL()
{
   recomp_special[(src & 0x3F)]();
}

static void RREGIMM()
{
   recomp_regimm[((src >> 16) & 0x1F)]();
}

#define RJ recompile_standard_j_type

#define RJAL recompile_standard_j_type

#define RBEQ recompile_standard_i_type

#define RBNE recompile_standard_i_type

#define RBLEZ recompile_standard_i_type

#define RBGTZ recompile_standard_i_type

#define RADDI recompile_standard_i_type

#define RADDIU recompile_standard_i_type

#define RSLTI recompile_standard_i_type

#define RSLTIU recompile_standard_i_type

#define RANDI recompile_standard_i_type

#define RORI recompile_standard_i_type

#define RXORI recompile_standard_i_type

#define RLUI recompile_standard_i_type

static void RCOP0()
{
   recomp_cop0[((src >> 21) & 0x1F)]();
}

static void RCOP1()
{
   recomp_cop1[((src >> 21) & 0x1F)]();
}

#define RBEQL recompile_standard_i_type

#define RBNEL recompile_standard_i_type

#define RBLEZL recompile_standard_i_type

#define RBGTZL recompile_standard_i_type

#define RDADDI recompile_standard_i_type

#define RDADDIU recompile_standard_i_type

#define RLDL recompile_standard_i_type

#define RLDR recompile_standard_i_type

#define RLB recompile_standard_i_type

#define RLH recompile_standard_i_type

#define RLWL recompile_standard_i_type

#define RLW recompile_standard_i_type

#define RLBU recompile_standard_i_type

#define RLHU recompile_standard_i_type

#define RLWR recompile_standard_i_type

#define RLWU recompile_standard_i_type

#define RSB recompile_standard_i_type

#define RSH recompile_standard_i_type

#define RSWL recompile_standard_i_type

#define RSW recompile_standard_i_type

#define RSDL recompile_standard_i_type

#define RSDR recompile_standard_i_type

#define RSWR recompile_standard_i_type

#define RCACHE recompile_nothing

#define RLL recompile_standard_i_type

#define RLWC1 recompile_standard_lf_type

#define RLLD recompile_standard_i_type

#define RLDC1 recompile_standard_lf_type

#define RLD recompile_standard_i_type

#define RSC recompile_standard_i_type

#define RSWC1 recompile_standard_lf_type

#define RSCD recompile_standard_i_type

#define RSDC1 recompile_standard_lf_type

#define RSD recompile_standard_i_type

static void (*recomp_ops[64])(void) =
{
   RSPECIAL, RREGIMM, RJ   , RJAL  , RBEQ , RBNE , RBLEZ , RBGTZ ,
   RADDI   , RADDIU , RSLTI, RSLTIU, RANDI, RORI , RXORI , RLUI  ,
   RCOP0   , RCOP1  , RSV  , RSV   , RBEQL, RBNEL, RBLEZL, RBGTZL,
   RDADDI  , RDADDIU, RLDL , RLDR  , RSV  , RSV  , RSV   , RSV   ,
   RLB     , RLH    , RLWL , RLW   , RLBU , RLHU , RLWR  , RLWU  ,
   RSB     , RSH    , RSWL , RSW   , RSDL , RSDR , RSWR  , RCACHE,
   RLL     , RLWC1  , RSV  , RSV   , RLLD , RLDC1, RSV   , RLD   ,
   RSC     , RSWC1  , RSV  , RSV   , RSCD , RSDC1, RSV   , RSD
};


/**********************************************************************
 ************** decode one opcode (for the interpreter) ***************
 **********************************************************************/
extern unsigned long op;
void prefetch_opcode(unsigned long instr)
{
   src = instr;
   op = instr;
   //printf("Interpreting %08x\n",op);
   recomp_ops[((src >> 26) & 0x3F)]();
}
