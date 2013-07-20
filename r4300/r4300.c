/**
 * Mupen64 - r4300.c
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

#include "../config.h"
#include "../main/ROM-Cache.h"
#include "r4300.h"
#include "../gc_memory/memory.h"
#include "exception.h"
#include "interupt.h"
#include "macros.h"
#include "recomp.h"
#include "Invalid_Code.h"
#include "Recomp-Cache.h"
#include "ARAM-blocks.h"
#include "ppc/Wrappers.h"
#include <malloc.h>

#ifdef DBG
extern int debugger_mode;
extern void update_debugger();
#endif

R4300 r4300;

unsigned long i, dynacore = 0, interpcore = 0;
int no_audio_delay = 0;
int no_compiled_jump = 0;
tlb tlb_e[32];
unsigned long dyna_interp = 0;
unsigned long long int debug_count = 0;
precomp_instr *PC = NULL;
//char invalid_code[0x100000];

#ifdef PPC_DYNAREC
#include "ppc/Recompile.h"
#ifdef HW_RVL
#include "../gc_memory/MEM2.h"
PowerPC_block **blocks = (PowerPC_block**)(BLOCKS_LO);
#else
#include "../gc_memory/ARAM.h"
PowerPC_block **blocks = (PowerPC_block**)(BLOCKS_LO);
#endif
PowerPC_block *actual;
#else
precomp_block *blocks[0x100000], *actual;
#endif
int rounding_mode = 0x33F, trunc_mode = 0xF3F, round_mode = 0x33F,
    ceil_mode = 0xB3F, floor_mode = 0x73F;

inline unsigned long update_invalid_addr(unsigned long addr)
{
   if(rom_base_in_tlb && (addr >= 0x7f000000 && addr < 0x80000000)) {
	return rom_base_in_tlb + (addr & 0x7FFFFF);	// GoldenEye Hack
   }
   if (addr >= 0x80000000 && addr < 0xa0000000)
     {
	if (invalid_code_get(addr>>12)) invalid_code_set((addr+0x20000000)>>12, 1);
	if (invalid_code_get((addr+0x20000000)>>12)) invalid_code_set(addr>>12, 1);
	return addr;
     }
   else if (addr >= 0xa0000000 && addr < 0xc0000000)
     {
	if (invalid_code_get(addr>>12)) invalid_code_set((addr-0x20000000)>>12, 1);
	if (invalid_code_get((addr-0x20000000)>>12)) invalid_code_set(addr>>12, 1);
	return addr;
     }
   else
     {
	unsigned long paddr = virtual_to_physical_address(addr, 2);
	if (paddr)
	  {
	     unsigned long beg_paddr = paddr - (addr - (addr&~0xFFF));
	     update_invalid_addr(paddr);
	     if (invalid_code_get((beg_paddr+0x000)>>12)) invalid_code_set(addr>>12, 1);
	     if (invalid_code_get((beg_paddr+0xFFC)>>12)) invalid_code_set(addr>>12, 1);
	     if (invalid_code_get(addr>>12)) invalid_code_set((beg_paddr+0x000)>>12, 1);
	     if (invalid_code_get(addr>>12)) invalid_code_set((beg_paddr+0xFFC)>>12, 1);
	  }
	return paddr;
     }
}

/* Refer to Figure 6-2 on page 155 and explanation on page B-11
   of MIPS R4000 Microprocessor User's Manual (Second Edition)
   by Joe Heinrich.
*/
void shuffle_fpr_data(int oldStatus, int newStatus)
{
#if defined(_BIG_ENDIAN)
    const int isBigEndian = 1;
#else
    const int isBigEndian = 0;
#endif

    if ((newStatus & 0x04000000) != (oldStatus & 0x04000000))
    {
        int i;
        int temp_fgr_32[32];

        // pack or unpack the FGR register data
        if (newStatus & 0x04000000)
        {   // switching into 64-bit mode
            // retrieve 32 FPR values from packed 32-bit FGR registers
            for (i = 0; i < 32; i++)
            {
                temp_fgr_32[i] = *((int *) &r4300.fpr_data[i>>1] + ((i & 1) ^ isBigEndian));
            }
            // unpack them into 32 64-bit registers, taking the high 32-bits from their temporary place in the upper 16 FGRs
            for (i = 0; i < 32; i++)
            {
                int high32 = *((int *) &r4300.fpr_data[(i>>1)+16] + (i & 1));
                *((int *) &r4300.fpr_data[i] + isBigEndian)     = temp_fgr_32[i];
                *((int *) &r4300.fpr_data[i] + (isBigEndian^1)) = high32;
            }
        }
        else
        {   // switching into 32-bit mode
            // retrieve the high 32 bits from each 64-bit FGR register and store in temp array
            for (i = 0; i < 32; i++)
            {
                temp_fgr_32[i] = *((int *) &r4300.fpr_data[i] + (isBigEndian^1));
            }
            // take the low 32 bits from each register and pack them together into 64-bit pairs
            for (i = 0; i < 16; i++)
            {
                unsigned int least32 = *((unsigned int *) &r4300.fpr_data[i*2] + isBigEndian);
                unsigned int most32 = *((unsigned int *) &r4300.fpr_data[i*2+1] + isBigEndian);
                r4300.fpr_data[i] = ((unsigned long long) most32 << 32) | (unsigned long long) least32;
            }
            // store the high bits in the upper 16 FGRs, which wont be accessible in 32-bit mode
            for (i = 0; i < 32; i++)
            {
                *((int *) &r4300.fpr_data[(i>>1)+16] + (i & 1)) = temp_fgr_32[i];
            }
        }
    }
}

void set_fpr_pointers(int newStatus)
{
    int i;
#if defined(_BIG_ENDIAN)
    const int isBigEndian = 1;
#else
    const int isBigEndian = 0;
#endif

    // update the FPR register pointers
    if (newStatus & 0x04000000)
    {
        for (i = 0; i < 32; i++)
        {
            r4300.fpr_double[i] = (double*) &r4300.fpr_data[i];
            r4300.fpr_single[i] = ((float*) &r4300.fpr_data[i]) + isBigEndian;
        }
    }
    else
    {
        for (i = 0; i < 32; i++)
        {
            r4300.fpr_double[i] = (double*) &r4300.fpr_data[i>>1];
            r4300.fpr_single[i] = ((float*) &r4300.fpr_data[i>>1]) + ((i & 1) ^ isBigEndian);
        }
    }
}

int check_cop1_unusable()
{
   if (!(Status & 0x20000000))
     {
	Cause = (11 << 2) | 0x10000000;
	exception_general();
	return 1;
     }
   return 0;
}

#include "../gui/DEBUG.h"

void update_count()
{
	//sprintf(txtbuffer, "trace: addr = 0x%08x\n", r4300.pc);
#ifdef SHOW_DEBUG
	if(r4300.pc < r4300.last_pc) {
		sprintf(txtbuffer, "r4300.pc (%08x) < r4300.last_pc (%08x)\n");
		DEBUG_print(txtbuffer, DBG_USBGECKO);
	}
#endif
	do_SP_Task(1,(r4300.pc - r4300.last_pc) / 2);
	Count = Count + (r4300.pc - r4300.last_pc) / 2;
	r4300.last_pc = r4300.pc;
	
#ifdef COMPARE_CORE
	if (r4300.delay_slot)
		compare_core();
#endif
#ifdef DBG
	if (debugger_mode) update_debugger();
#endif
}

void init_blocks()
{
   int i;
   for (i=0; i<0x100000; i++)
     {
	invalid_code_set(i, 1);
	blocks_set(i, NULL);
     }
#ifndef PPC_DYNAREC
   blocks[0xa4000000>>12] = malloc(sizeof(precomp_block));
   blocks[0xa4000000>>12]->code = NULL;
   blocks[0xa4000000>>12]->block = NULL;
   blocks[0xa4000000>>12]->jumps_table = NULL;
   blocks[0xa4000000>>12]->start = 0xa4000000;
   blocks[0xa4000000>>12]->end = 0xa4001000;
#else
   PowerPC_block* temp_block = malloc(sizeof(PowerPC_block));
   blocks_set(0xa4000000>>12, temp_block);
   //blocks[0xa4000000>>12]->code_addr = NULL;
   temp_block->funcs = NULL;
   temp_block->start_address = 0xa4000000;
   temp_block->end_address = 0xa4001000;
#endif
   invalid_code_set(0xa4000000>>12, 1);
   actual=temp_block;
   init_block(temp_block, 0xa4000000);

#ifdef DBG
   if (debugger_mode) // debugger shows initial state (before 1st instruction).
     update_debugger();
#endif
}

static int cpu_inited;
void go()
{
	r4300.stop = 0;

   if(dynacore == 2) {
		dynacore = 0;
		interpcore = 1;
		pure_interpreter();
		dynacore = 2;
	} else {
		interpcore = 0;
		dynacore = 1;
		//printf("dynamic recompiler\n");
		if(cpu_inited) {
			RecompCache_Init();
			init_blocks();
			cpu_inited = 0;
		}
		dynarec(r4300.pc);
	}
	debug_count += Count;
}

void cpu_init(void){
   long long CRC = 0;
   debug_count = 0;
   ROMCache_read((u8*)SP_DMEM+0x40, 0x40, 0xFBC);
   r4300.delay_slot=0;
   r4300.skip_jump=0;
   r4300.stop = 0;
   for (i=0;i<32;i++)
     {
	r4300.gpr[i]=0;
	r4300.reg_cop0[i]=0;
	r4300.fpr_data[i]=0;

	// --------------tlb------------------------
	tlb_e[i].mask=0;
	tlb_e[i].vpn2=0;
	tlb_e[i].g=0;
	tlb_e[i].asid=0;
	tlb_e[i].pfn_even=0;
	tlb_e[i].c_even=0;
	tlb_e[i].d_even=0;
	tlb_e[i].v_even=0;
	tlb_e[i].pfn_odd=0;
	tlb_e[i].c_odd=0;
	tlb_e[i].d_odd=0;
	tlb_e[i].v_odd=0;
	tlb_e[i].r=0;
	//tlb_e[i].check_parity_mask=0x1000;

	tlb_e[i].start_even=0;
	tlb_e[i].end_even=0;
	tlb_e[i].phys_even=0;
	tlb_e[i].start_odd=0;
	tlb_e[i].end_odd=0;
	tlb_e[i].phys_odd=0;
     }
   r4300.tlb_e=tlb_e;
   
#ifndef USE_TLB_CACHE
   for (i=0; i<0x100000; i++)
     {
	tlb_LUT_r[i] = 0;
	tlb_LUT_w[i] = 0;
     }
#endif
   r4300.llbit=0;
   r4300.hi=0;
   r4300.lo=0;
   r4300.fcr0=0x511;
   r4300.fcr31=0;

   //--------
   /*reg[20]=1;
   reg[22]=0x3F;
   reg[29]=0xFFFFFFFFA0400000LL;
   Random=31;
   Status=0x70400004;
   Config=0x66463;
   PRevID=0xb00;*/
   //--------

   // the following values are extracted from the pj64 source code
   // thanks to Zilmar and Jabo

   r4300.gpr[6] = 0xFFFFFFFFA4001F0CLL;
   r4300.gpr[7] = 0xFFFFFFFFA4001F08LL;
   r4300.gpr[8] = 0x00000000000000C0LL;
   r4300.gpr[10]= 0x0000000000000040LL;
   r4300.gpr[11]= 0xFFFFFFFFA4000040LL;
   r4300.gpr[29]= 0xFFFFFFFFA4001FF0LL;

   Random = 31;
   Status= 0x34000000;
   set_fpr_pointers(Status);
   Config= 0x6e463;
   PRevID = 0xb00;
   Count = 0x5000;
   Cause = 0x5C;
   Context = 0x7FFFF0;
   EPC = 0xFFFFFFFF;
   BadVAddr = 0xFFFFFFFF;
   ErrorEPC = 0xFFFFFFFF;

   for (i = 0x40/4; i < (0x1000/4); i++)
     CRC += SP_DMEM[i];
   switch(CRC) {
    case 0x000000D0027FDF31LL:
    case 0x000000CFFB631223LL:
      r4300.cic_chip = 1;
      break;
    case 0x000000D057C85244LL:
      r4300.cic_chip = 2;
      break;
    case 0x000000D6497E414BLL:
      r4300.cic_chip = 3;
      break;
    case 0x0000011A49F60E96LL:
      r4300.cic_chip = 5;
      break;
    case 0x000000D6D5BE5580LL:
      r4300.cic_chip = 6;
      break;
    default:
      r4300.cic_chip = 2;
   }

   switch(ROM_HEADER->Country_code&0xFF)
     {
      case 0x44:
      case 0x46:
      case 0x49:
      case 0x50:
      case 0x53:
      case 0x55:
      case 0x58:
      case 0x59:
	switch (r4300.cic_chip) {
	 case 2:
	   r4300.gpr[5] = 0xFFFFFFFFC0F1D859LL;
	   r4300.gpr[14]= 0x000000002DE108EALL;
	   break;
	 case 3:
	   r4300.gpr[5] = 0xFFFFFFFFD4646273LL;
	   r4300.gpr[14]= 0x000000001AF99984LL;
	   break;
	 case 5:
	   SP_IMEM[1] = 0xBDA807FC;
	   r4300.gpr[5] = 0xFFFFFFFFDECAAAD1LL;
	   r4300.gpr[14]= 0x000000000CF85C13LL;
	   r4300.gpr[24]= 0x0000000000000002LL;
	   break;
	 case 6:
	   r4300.gpr[5] = 0xFFFFFFFFB04DC903LL;
	   r4300.gpr[14]= 0x000000001AF99984LL;
	   r4300.gpr[24]= 0x0000000000000002LL;
	   break;
	}
	r4300.gpr[23]= 0x0000000000000006LL;
	r4300.gpr[31]= 0xFFFFFFFFA4001554LL;
	break;
      case 0x37:
      case 0x41:
      case 0x45:
      case 0x4A:
      default:
	switch (r4300.cic_chip) {
	 case 2:
	   r4300.gpr[5] = 0xFFFFFFFFC95973D5LL;
	   r4300.gpr[14]= 0x000000002449A366LL;
	   break;
	 case 3:
	   r4300.gpr[5] = 0xFFFFFFFF95315A28LL;
	   r4300.gpr[14]= 0x000000005BACA1DFLL;
	   break;
	 case 5:
	   SP_IMEM[1] = 0x8DA807FC;
	   r4300.gpr[5] = 0x000000005493FB9ALL;
	   r4300.gpr[14]= 0xFFFFFFFFC2C20384LL;
	   break;
	 case 6:
	   r4300.gpr[5] = 0xFFFFFFFFE067221FLL;
	   r4300.gpr[14]= 0x000000005CD2B70FLL;
	   break;
	}
	r4300.gpr[20]= 0x0000000000000001LL;
	r4300.gpr[24]= 0x0000000000000003LL;
	r4300.gpr[31]= 0xFFFFFFFFA4001550LL;
     }
   switch (r4300.cic_chip) {
    case 1:
      r4300.gpr[22]= 0x000000000000003FLL;
      break;
    case 2:
      r4300.gpr[1] = 0x0000000000000001LL;
      r4300.gpr[2] = 0x000000000EBDA536LL;
      r4300.gpr[3] = 0x000000000EBDA536LL;
      r4300.gpr[4] = 0x000000000000A536LL;
      r4300.gpr[12]= 0xFFFFFFFFED10D0B3LL;
      r4300.gpr[13]= 0x000000001402A4CCLL;
      r4300.gpr[15]= 0x000000003103E121LL;
      r4300.gpr[22]= 0x000000000000003FLL;
      r4300.gpr[25]= 0xFFFFFFFF9DEBB54FLL;
      break;
    case 3:
      r4300.gpr[1] = 0x0000000000000001LL;
      r4300.gpr[2] = 0x0000000049A5EE96LL;
      r4300.gpr[3] = 0x0000000049A5EE96LL;
      r4300.gpr[4] = 0x000000000000EE96LL;
      r4300.gpr[12]= 0xFFFFFFFFCE9DFBF7LL;
      r4300.gpr[13]= 0xFFFFFFFFCE9DFBF7LL;
      r4300.gpr[15]= 0x0000000018B63D28LL;
      r4300.gpr[22]= 0x0000000000000078LL;
      r4300.gpr[25]= 0xFFFFFFFF825B21C9LL;
      break;
    case 5:
      SP_IMEM[0] = 0x3C0DBFC0;
      SP_IMEM[2] = 0x25AD07C0;
      SP_IMEM[3] = 0x31080080;
      SP_IMEM[4] = 0x5500FFFC;
      SP_IMEM[5] = 0x3C0DBFC0;
      SP_IMEM[6] = 0x8DA80024;
      SP_IMEM[7] = 0x3C0BB000;
      r4300.gpr[2] = 0xFFFFFFFFF58B0FBFLL;
      r4300.gpr[3] = 0xFFFFFFFFF58B0FBFLL;
      r4300.gpr[4] = 0x0000000000000FBFLL;
      r4300.gpr[12]= 0xFFFFFFFF9651F81ELL;
      r4300.gpr[13]= 0x000000002D42AAC5LL;
      r4300.gpr[15]= 0x0000000056584D60LL;
      r4300.gpr[22]= 0x0000000000000091LL;
      r4300.gpr[25]= 0xFFFFFFFFCDCE565FLL;
      break;
    case 6:
      r4300.gpr[2] = 0xFFFFFFFFA95930A4LL;
      r4300.gpr[3] = 0xFFFFFFFFA95930A4LL;
      r4300.gpr[4] = 0x00000000000030A4LL;
      r4300.gpr[12]= 0xFFFFFFFFBCB59510LL;
      r4300.gpr[13]= 0xFFFFFFFFBCB59510LL;
      r4300.gpr[15]= 0x000000007A3C07F4LL;
      r4300.gpr[22]= 0x0000000000000085LL;
      r4300.gpr[25]= 0x00000000465E3F72LL;
      break;
   }

   rounding_mode = 0x33F;

   r4300.last_pc = 0xa4000040;
   r4300.next_interrupt = 624999;
   init_interupt();
   interpcore = 0;

   // I'm adding this from pure_interpreter()
   r4300.pc = 0xa4000040;
   PC = malloc(sizeof(precomp_instr));
   
   // Hack for the interpreter
   cpu_inited = 1;
}

void cpu_deinit(void){
	// No need to check these if we were in the pure interp
	if(dynacore != 2 && !cpu_inited){
		for (i=0; i<0x100000; i++) {
  		PowerPC_block* temp_block = blocks_get(i);
		if (temp_block) {
#ifdef PPC_DYNAREC
			deinit_block(temp_block);
#else
			if (temp_block->block) {
#ifdef USE_RECOMP_CACHE
				invalidate_block(temp_block);
#else
				free(temp_block->block);
#endif
				temp_block->block = NULL;
			}
			if (temp_block->code) {
				free(temp_block->code);
				temp_block->code = NULL;
			}
			if (temp_block->jumps_table) {
				free(temp_block->jumps_table);
				temp_block->jumps_table = NULL;
			}
#endif
			free(temp_block);
			blocks_set(i, NULL);
		}
		}
	}
	if(PC) free(PC);
	PC = NULL;
}
