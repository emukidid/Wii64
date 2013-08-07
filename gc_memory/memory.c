/*
 * Mupen64 - memory.c
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
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#include "../config.h"

#include <stdio.h>
#include <stdlib.h>
#ifndef __WIN32__
#include "../main/winlnxdefs.h"
#else
#include <windows.h>
#endif

#include "memory.h"
#include "dma.h"
#include "../main/ROM-Cache.h"
#include "TLB-Cache.h"
#include "Saves.h"
#include "../r4300/r4300.h"
#include "../r4300/macros.h"
#include "../r4300/interupt.h"
#include "../r4300/Invalid_Code.h"
#include "pif.h"
#include "flashram.h"
#include "../main/plugin.h"
#include "../main/guifuncs.h"

#include "../gui/DEBUG.h"
#include <assert.h>

/* definitions of the rcp's structures and memory area */
RDRAM_register rdram_register;
mips_register MI_register;
PI_register pi_register;
SP_register sp_register;
RSP_register rsp_register;
SI_register si_register;
VI_register vi_register;
RI_register ri_register;
AI_register ai_register;
DPC_register dpc_register;
DPS_register dps_register;

#ifdef USE_EXPANSION
	#define MEM_SIZE 0x800000
	#define MEM_TBL_SIZE 0x80
	#define MEMMASK 0x7FFFFF
#else
	#define MEM_SIZE 0x400000
	#define MEM_TBL_SIZE 0x40
	#define MEMMASK 0x3FFFFF
#endif

#ifdef _BIG_ENDIAN
#define GET_LOW_ADDR(address) ((unsigned short)((address) & 0xFFFF))
#else
#define GET_LOW_ADDR(address) ((unsigned short)(((address) >>16) & 0xFFFF))
#endif

unsigned long rdram[MEM_SIZE/4]  __attribute__((aligned(32)));
unsigned char *rdramb = (unsigned char *)(rdram);
unsigned long SP_DMEM[0x1000/4*2];
unsigned long *SP_IMEM = SP_DMEM+0x1000/4;
unsigned char *SP_DMEMb = (unsigned char *)(SP_DMEM);
unsigned char *SP_IMEMb = (unsigned char*)(SP_DMEM+0x1000/4);
unsigned long PIF_RAM[0x40/4];
unsigned char *PIF_RAMb = (unsigned char *)(PIF_RAM);

// trash : when we write to unmaped memory it is written here
static unsigned long trash;

// hash tables of memory functions
unsigned long long int (**rwmem[0x10000])(unsigned long, unsigned long long int);

unsigned long long int (*rw_nothing[8])(unsigned long, unsigned long long int) =
	{ read_nothing,  read_nothingb,  read_nothingh,  read_nothingd,
	  write_nothing, write_nothingb, write_nothingh, write_nothingd };

unsigned long long int (*rw_nomem[8])(unsigned long, unsigned long long int) =
	{ read_nomem,  read_nomemb,  read_nomemh,  read_nomemd,
	  write_nomem, write_nomemb, write_nomemh, write_nomemd };

unsigned long long int (*rw_rdram[8])(unsigned long, unsigned long long int) =
	{ read_rdram,  read_rdramb,  read_rdramh,  read_rdramd,
	  write_rdram, write_rdramb, write_rdramh, write_rdramd };

unsigned long long int (*rw_rdramFB[8])(unsigned long, unsigned long long int) =
	{ read_rdramFB,  read_rdramFBb,  read_rdramFBh,  read_rdramFBd,
	  write_rdramFB, write_rdramFBb, write_rdramFBh, write_rdramFBd };

unsigned long long int (*rw_rdramreg[8])(unsigned long, unsigned long long int) =
	{ read_rdramreg,  read_rdramregb,  read_rdramregh,  read_rdramregd,
	  write_rdramreg, write_rdramregb, write_rdramregh, write_rdramregd };

unsigned long long int (*rw_rsp_mem[8])(unsigned long, unsigned long long int) =
	{ read_rsp_mem,  read_rsp_memb,  read_rsp_memh,  read_rsp_memd,
	  write_rsp_mem, write_rsp_memb, write_rsp_memh, write_rsp_memd };

unsigned long long int (*rw_rsp_reg[8])(unsigned long, unsigned long long int) =
	{ read_rsp_reg,  read_rsp_regb,  read_rsp_regh,  read_rsp_regd,
	  write_rsp_reg, write_rsp_regb, write_rsp_regh, write_rsp_regd };

unsigned long long int (*rw_rsp[8])(unsigned long, unsigned long long int) =
	{ read_rsp,  read_rspb,  read_rsph,  read_rspd,
	  write_rsp, write_rspb, write_rsph, write_rspd };

unsigned long long int (*rw_dp[8])(unsigned long, unsigned long long int) =
	{ read_dp,  read_dpb,  read_dph,  read_dpd,
	  write_dp, write_dpb, write_dph, write_dpd };

unsigned long long int (*rw_dps[8])(unsigned long, unsigned long long int) =
	{ read_dps,  read_dpsb,  read_dpsh,  read_dpsd,
	  write_dps, write_dpsb, write_dpsh, write_dpsd };

unsigned long long int (*rw_mi[8])(unsigned long, unsigned long long int) =
	{ read_mi,  read_mib,  read_mih,  read_mid,
	  write_mi, write_mib, write_mih, write_mid };

unsigned long long int (*rw_vi[8])(unsigned long, unsigned long long int) =
	{ read_vi,  read_vib,  read_vih,  read_vid,
	  write_vi, write_vib, write_vih, write_vid };

unsigned long long int (*rw_ai[8])(unsigned long, unsigned long long int) =
	{ read_ai,  read_aib,  read_aih,  read_aid,
	  write_ai, write_aib, write_aih, write_aid };

unsigned long long int (*rw_pi[8])(unsigned long, unsigned long long int) =
	{ read_pi,  read_pib,  read_pih,  read_pid,
	  write_pi, write_pib, write_pih, write_pid };

unsigned long long int (*rw_ri[8])(unsigned long, unsigned long long int) =
	{ read_ri,  read_rib,  read_rih,  read_rid,
	  write_ri, write_rib, write_rih, write_rid };

unsigned long long int (*rw_si[8])(unsigned long, unsigned long long int) =
	{ read_si,  read_sib,  read_sih,  read_sid,
	  write_si, write_sib, write_sih, write_sid };

unsigned long long int (*rw_flashram0[8])(unsigned long, unsigned long long int) =
	{ read_flashram_status,  read_flashram_statusb,
	  read_flashram_statush, read_flashram_statusd,
	  write_flashram_dummy,  write_flashram_dummyb,
	  write_flashram_dummyh, write_flashram_dummyd };

unsigned long long int (*rw_flashram1[8])(unsigned long, unsigned long long int) =
	{ read_nothing,            read_nothingb,
	  read_nothingh,           read_nothingd,
	  write_flashram_command,  write_flashram_commandb,
	  write_flashram_commandh, write_flashram_commandd };

unsigned long long int (*rw_rom0[8])(unsigned long, unsigned long long int) =
	{ read_rom,      read_romb,      read_romh,      read_romd,
	  write_nothing, write_nothingb, write_nothingh, write_nothingd };

unsigned long long int (*rw_rom1[8])(unsigned long, unsigned long long int) =
	{ read_rom,      read_romb,      read_romh,      read_romd,
	  write_rom,     write_nothingb, write_nothingh, write_nothingd };

unsigned long long int (*rw_pif[8])(unsigned long, unsigned long long int) =
	{ read_pif,  read_pifb,  read_pifh,  read_pifd,
	  write_pif, write_pifb, write_pifh, write_pifd };


// memory sections
static unsigned long *readrdramreg[0x28];
static unsigned long *readrspreg[0x30];
static unsigned long *readrsp[0x10];
static unsigned long *readmi[0x20];
static unsigned long *readvi[0x40];
static unsigned long *readai[0x20];
static unsigned long *readpi[0x34];
static unsigned long *readri[0x30];
static unsigned long *readsi[0x20];
static unsigned long *readdp[0x30];
static unsigned long *readdps[0x20];

// the frameBufferInfos
static FrameBufferInfo frameBufferInfos[6];
static char framebufferRead[0x800];
static int firstFrameBufferSetting;

int init_memory()
{
	int i;
      
	//init hash tables
	for (i=0; i<(0x10000); i++) rwmem[i] = rw_nomem;
   
	//init RDRAM
	for (i=0; i<(MEM_SIZE/4); i++) rdram[i]=0;

	for (i=0; i<MEM_TBL_SIZE; i++)
	{
		rwmem[(0x8000+i)] = rw_rdram;
		rwmem[(0xa000+i)] = rw_rdram;
	}
	for (i=MEM_TBL_SIZE; i<0x3F0; i++)
    {
		rwmem[0x8000+i] = rw_nothing;
		rwmem[0xa000+i] = rw_nothing;
	}
   
	//init RDRAM registers
	rwmem[0x83f0] = rw_rdramreg;
	rwmem[0xa3f0] = rw_rdramreg;
	memset(&rdram_register, 0, sizeof(RDRAM_register));
	readrdramreg[0x0] = &rdram_register.rdram_config;
	readrdramreg[0x4] = &rdram_register.rdram_device_id;
	readrdramreg[0x8] = &rdram_register.rdram_delay;
	readrdramreg[0xc] = &rdram_register.rdram_mode;
	readrdramreg[0x10] = &rdram_register.rdram_ref_interval;
	readrdramreg[0x14] = &rdram_register.rdram_ref_row;
	readrdramreg[0x18] = &rdram_register.rdram_ras_interval;
	readrdramreg[0x1c] = &rdram_register.rdram_min_interval;
	readrdramreg[0x20] = &rdram_register.rdram_addr_select;
	readrdramreg[0x24] = &rdram_register.rdram_device_manuf;
   
	for (i=1; i<0x10; i++)
	{
		rwmem[0x83f0+i] = rw_nothing;
		rwmem[0xa3f0+i] = rw_nothing;
	}
   
	//init RSP memory
	rwmem[0x8400] = rw_rsp_mem;
	rwmem[0xa400] = rw_rsp_mem;
	for (i=0; i<(0x1000/4); i++) SP_DMEM[i]=0;
	for (i=0; i<(0x1000/4); i++) SP_IMEM[i]=0;
   
	for (i=1; i<0x4; i++)
	{
		rwmem[0x8400+i] = rw_nothing;
		rwmem[0xa400+i] = rw_nothing;
	}
   
	//init RSP registers
	rwmem[0x8404] = rw_rsp_reg;
	rwmem[0xa404] = rw_rsp_reg;
	memset(&sp_register, 0, sizeof(SP_register));
	sp_register.sp_status_reg=1;
	sp_register.halt=1;
	readrspreg[0x0] = &sp_register.sp_mem_addr_reg;
	readrspreg[0x4] = &sp_register.sp_dram_addr_reg;
	readrspreg[0x8] = &sp_register.sp_rd_len_reg;
	readrspreg[0xc] = &sp_register.sp_wr_len_reg;
	readrspreg[0x10] = &sp_register.sp_status_reg;
	readrspreg[0x14] = &sp_register.sp_dma_full_reg;
	readrspreg[0x18] = &sp_register.sp_dma_busy_reg;
	readrspreg[0x1c] = &sp_register.sp_semaphore_reg;

	for (i=0x20; i<0x30; i++) readrspreg[i] = &trash;
	for (i=5; i<8; i++)
	{
		rwmem[0x8400+i] = rw_nothing;
		rwmem[0xa400+i] = rw_nothing;
	}
   
	rwmem[0x8408] = rw_rsp;
	rwmem[0xa408] = rw_rsp;
	memset(&rsp_register, 0, sizeof(RSP_register));
	readrsp[0x0] = &rsp_register.rsp_pc;
	readrsp[0x4] = &rsp_register.rsp_ibist;

	for (i=0x8; i<0x10; i++) readrsp[i] = &trash;
	for (i=9; i<0x10; i++)
	{
		rwmem[0x8400+i] = rw_nothing;
		rwmem[0xa400+i] = rw_nothing;
	}
   
	//init rdp command registers
	rwmem[0x8410] = rw_dp;
	rwmem[0xa410] = rw_dp;
	memset(&dpc_register, 0, sizeof(DPC_register));
	readdp[0x0] = &dpc_register.dpc_start;
	readdp[0x4] = &dpc_register.dpc_end;
	readdp[0x8] = &dpc_register.dpc_current;
	readdp[0xc] = &dpc_register.dpc_status;
	readdp[0x10] = &dpc_register.dpc_clock;
	readdp[0x14] = &dpc_register.dpc_bufbusy;
	readdp[0x18] = &dpc_register.dpc_pipebusy;
	readdp[0x1c] = &dpc_register.dpc_tmem;

	for (i=0x20; i<0x30; i++) readdp[i] = &trash;
	for (i=1; i<0x10; i++)
	{
		rwmem[0x8410+i] = rw_nothing;
		rwmem[0xa410+i] = rw_nothing;
	}
   
	//init rsp span registers
	rwmem[0x8420] = rw_dps;
	rwmem[0xa420] = rw_dps;
	memset(&dps_register, 0, sizeof(DPS_register));
	readdps[0x0] = &dps_register.dps_tbist;
	readdps[0x4] = &dps_register.dps_test_mode;
	readdps[0x8] = &dps_register.dps_buftest_addr;
	readdps[0xc] = &dps_register.dps_buftest_data;
   
	for (i=0x10; i<0x20; i++) readdps[i] = &trash;
	for (i=1; i<0x10; i++)
	{
		rwmem[0x8420+i] = rw_nothing;
		rwmem[0xa420+i] = rw_nothing;
	}
   
	//init mips registers
	rwmem[0xa830] = rw_mi;
	rwmem[0xa430] = rw_mi;
	memset(&MI_register, 0, sizeof(mips_register));
	MI_register.mi_version_reg = 0x02020102;
	readmi[0x0] = &MI_register.mi_init_mode_reg;
	readmi[0x4] = &MI_register.mi_version_reg;
	readmi[0x8] = &MI_register.mi_intr_reg;
	readmi[0xc] = &MI_register.mi_intr_mask_reg;

	for (i=0x10; i<0x20; i++) readmi[i] = &trash;
	for (i=1; i<0x10; i++)
	{
		rwmem[0x8430+i] = rw_nothing;
		rwmem[0xa430+i] = rw_nothing;
	}
   
	//init VI registers
	rwmem[0x8440] = rw_vi;
	rwmem[0xa440] = rw_vi;
	memset(&vi_register, 0, sizeof(VI_register));
	readvi[0x0] = &vi_register.vi_status;
	readvi[0x4] = &vi_register.vi_origin;
	readvi[0x8] = &vi_register.vi_width;
	readvi[0xc] = &vi_register.vi_v_intr;
	readvi[0x10] = &vi_register.vi_current;
	readvi[0x14] = &vi_register.vi_burst;
	readvi[0x18] = &vi_register.vi_v_sync;
	readvi[0x1c] = &vi_register.vi_h_sync;
	readvi[0x20] = &vi_register.vi_leap;
	readvi[0x24] = &vi_register.vi_h_start;
	readvi[0x28] = &vi_register.vi_v_start;
	readvi[0x2c] = &vi_register.vi_v_burst;
	readvi[0x30] = &vi_register.vi_x_scale;
	readvi[0x34] = &vi_register.vi_y_scale;

	for (i=0x38; i<0x40; i++) readvi[i] = &trash;
	for (i=1; i<0x10; i++)
	{
		rwmem[0x8440+i] = rw_nothing;
		rwmem[0xa440+i] = rw_nothing;
	}
   
	//init AI registers
	rwmem[0x8450] = rw_ai;
	rwmem[0xa450] = rw_ai;
	memset(&ai_register, 0, sizeof(AI_register));
	readai[0x0] = &ai_register.ai_dram_addr;
	readai[0x4] = &ai_register.ai_len;
	readai[0x8] = &ai_register.ai_control;
	readai[0xc] = &ai_register.ai_status;
	readai[0x10] = &ai_register.ai_dacrate;
	readai[0x14] = &ai_register.ai_bitrate;

	for (i=0x18; i<0x20; i++) readai[i] = &trash;
	for (i=1; i<0x10; i++)
	{
		rwmem[0x8450+i] = rw_nothing;
		rwmem[0xa450+i] = rw_nothing;
	}
   
	//init PI registers
	rwmem[0x8460] = rw_pi;
	rwmem[0xa460] = rw_pi;
	memset(&pi_register, 0, sizeof(PI_register));
	readpi[0x0] = &pi_register.pi_dram_addr_reg;
	readpi[0x4] = &pi_register.pi_cart_addr_reg;
	readpi[0x8] = &pi_register.pi_rd_len_reg;
	readpi[0xc] = &pi_register.pi_wr_len_reg;
	readpi[0x10] = &pi_register.read_pi_status_reg;
	readpi[0x14] = &pi_register.pi_bsd_dom1_lat_reg;
	readpi[0x18] = &pi_register.pi_bsd_dom1_pwd_reg;
	readpi[0x1c] = &pi_register.pi_bsd_dom1_pgs_reg;
	readpi[0x20] = &pi_register.pi_bsd_dom1_rls_reg;
	readpi[0x24] = &pi_register.pi_bsd_dom2_lat_reg;
	readpi[0x28] = &pi_register.pi_bsd_dom2_pwd_reg;
	readpi[0x2c] = &pi_register.pi_bsd_dom2_pgs_reg;
	readpi[0x30] = &pi_register.pi_bsd_dom2_rls_reg;

	for (i=1; i<0x10; i++)
	{
		rwmem[0x8460+i] = rw_nothing;
		rwmem[0xa460+i] = rw_nothing;
	}
   
	//init RI registers
	rwmem[0x8470] = rw_ri;
	rwmem[0xa470] = rw_ri;
	memset(&ri_register, 0, sizeof(RI_register));
	readri[0x0] = &ri_register.ri_mode;
	readri[0x4] = &ri_register.ri_config;
	readri[0x8] = &ri_register.ri_current_load;
	readri[0xc] = &ri_register.ri_select;
	readri[0x10] = &ri_register.ri_refresh;
	readri[0x14] = &ri_register.ri_latency;
	readri[0x18] = &ri_register.ri_error;
	readri[0x1c] = &ri_register.ri_werror;
   
	for (i=0x20; i<0x30; i++) readri[i] = &trash;
	for (i=1; i<0x10; i++)
	{
		rwmem[0x8470+i] = rw_nothing;
		rwmem[0xa470+i] = rw_nothing;
	}
   
	//init SI registers
	rwmem[0x8480] = rw_si;
	rwmem[0xa480] = rw_si;
	memset(&si_register, 0, sizeof(SI_register));
	readsi[0x0] = &si_register.si_dram_addr;
	readsi[0x4] = &si_register.si_pif_addr_rd64b;
	readsi[0x8] = &trash;
	readsi[0x10] = &si_register.si_pif_addr_wr64b;
	readsi[0x14] = &trash;
	readsi[0x18] = &si_register.si_status;

	for (i=0x1c; i<0x20; i++) readsi[i] = &trash;
	for (i=0x481; i<0x800; i++)
	{
		rwmem[0x8000+i] = rw_nothing;
		rwmem[0xa000+i] = rw_nothing;
	}
   
	//init flashram / sram
	rwmem[0x8800] = rw_flashram0;
	rwmem[0xa800] = rw_flashram0;
	rwmem[0x8801] = rw_flashram1;
	rwmem[0xa801] = rw_flashram1;

	for (i=0x802; i<0x1000; i++)
	{
		rwmem[0x8000+i] = rw_nothing;
		rwmem[0xa000+i] = rw_nothing;
	}
   
	//init rom area
	for (i=0; i<(rom_length >> 16); i++) 
	{
		rwmem[0x9000+i] = rw_rom0;
		rwmem[0xb000+i] = rw_rom1;
	}
	for (i=(rom_length >> 16); i<0xfc0; i++) 
	{
		rwmem[0x9000+i] = rw_nothing;
		rwmem[0xb000+i] = rw_nothing;
	}

	//init PIF_RAM
	rwmem[0x9fc0] = rw_pif;
	rwmem[0xbfc0] = rw_pif;
	for (i=0; i<(0x40/4); i++) PIF_RAM[i]=0;

	for (i=0xfc1; i<0x1000; i++) 
	{
		rwmem[0x9000+i] = rw_nothing;
		rwmem[0xb000+i] = rw_nothing;
	}

	flashRAMInfo.use_flashram = 0;
	init_flashram();

	frameBufferInfos[0].addr = 0;
	firstFrameBufferSetting = 1;

	//printf("memory initialized\n");
	return 0;
}

void free_memory()
{
#ifdef USE_TLB_CACHE
	TLBCache_deinit();
#endif
}

static void update_MI_init_mode_reg()
{
	MI_register.init_length = MI_register.w_mi_init_mode_reg & 0x7F;
	if (MI_register.w_mi_init_mode_reg & 0x80) 
		MI_register.init_mode = 0;
	if (MI_register.w_mi_init_mode_reg & 0x100)
		MI_register.init_mode = 1;
	if (MI_register.w_mi_init_mode_reg & 0x200)
		MI_register.ebus_test_mode = 0;
	if (MI_register.w_mi_init_mode_reg & 0x400)
		MI_register.ebus_test_mode = 1;
	if (MI_register.w_mi_init_mode_reg & 0x800)
	{
		MI_register.mi_intr_reg &= 0xFFFFFFDF;
		check_interupt();
	}
	if (MI_register.w_mi_init_mode_reg & 0x1000)
		MI_register.RDRAM_reg_mode=0;
	if (MI_register.w_mi_init_mode_reg & 0x2000)
		MI_register.RDRAM_reg_mode=1;
	MI_register.mi_init_mode_reg = ((MI_register.init_length) |
		(MI_register.init_mode << 7) |
		(MI_register.ebus_test_mode << 8) |
		(MI_register.RDRAM_reg_mode << 9)
		);
}

static void update_MI_intr_mask_reg()
{
	if (MI_register.w_mi_intr_mask_reg & 0x1)   MI_register.SP_intr_mask = 0;
	if (MI_register.w_mi_intr_mask_reg & 0x2)   MI_register.SP_intr_mask = 1;
	if (MI_register.w_mi_intr_mask_reg & 0x4)   MI_register.SI_intr_mask = 0;
	if (MI_register.w_mi_intr_mask_reg & 0x8)   MI_register.SI_intr_mask = 1;
	if (MI_register.w_mi_intr_mask_reg & 0x10)  MI_register.AI_intr_mask = 0;
	if (MI_register.w_mi_intr_mask_reg & 0x20)  MI_register.AI_intr_mask = 1;
	if (MI_register.w_mi_intr_mask_reg & 0x40)  MI_register.VI_intr_mask = 0;
	if (MI_register.w_mi_intr_mask_reg & 0x80)  MI_register.VI_intr_mask = 1;
	if (MI_register.w_mi_intr_mask_reg & 0x100) MI_register.PI_intr_mask = 0;
	if (MI_register.w_mi_intr_mask_reg & 0x200) MI_register.PI_intr_mask = 1;
	if (MI_register.w_mi_intr_mask_reg & 0x400) MI_register.DP_intr_mask = 0;
	if (MI_register.w_mi_intr_mask_reg & 0x800) MI_register.DP_intr_mask = 1;
	MI_register.mi_intr_mask_reg = ((MI_register.SP_intr_mask) |
		(MI_register.SI_intr_mask << 1) |
		(MI_register.AI_intr_mask << 2) |
		(MI_register.VI_intr_mask << 3) |
		(MI_register.PI_intr_mask << 4) |
		(MI_register.DP_intr_mask << 5)
		);
}

// It seems the SP task shouldn't be done right after the freeze bit is cleared
// else Perfect Dark intro freezes, I don't have a N64 to test the proper timing
// but that value seems fine for DK64/BC/PD...
// GliGli

#define FROZEN_TASK_DELAY 500
#define INT_MIN -1
static int SP_Task_pending_cycles=INT_MIN;

void do_SP_Task(int delayedDP, int cycles)
{
	if(delayedDP)
	{
		if(SP_Task_pending_cycles==INT_MIN) // not pending
			return;

		SP_Task_pending_cycles-=cycles;

		if(SP_Task_pending_cycles>0) // still pending
			return;
	}

	SP_Task_pending_cycles=INT_MIN;

	int save_pc = rsp_register.rsp_pc & ~0xFFF;
    if (SP_DMEM[0xFC0/4] == 1)
    {
		if (dpc_register.dpc_status & 0x2) // DP frozen (DK64, BC)
        {
            // don't do the task now
            // the task will be done when DP is unfreezed (see update_DPC)
            return;
        }
		// unprotecting old frame buffers
		if(fBGetFrameBufferInfo && fBRead && fBWrite && 
		frameBufferInfos[0].addr)
		{
			int i;
			for(i=0; i<6; i++)
			{
				if(frameBufferInfos[i].addr)
				{
					int j;
					int start = frameBufferInfos[i].addr & MEMMASK;
					int end = start + frameBufferInfos[i].width*
					frameBufferInfos[i].height*
					frameBufferInfos[i].size - 1;
					start = start >> 16;
					end = end >> 16;
					for(j=start; j<=end; j++)
					{
						rwmem[0x8000+j] = rw_rdram;
						rwmem[0xa000+j] = rw_rdram;
					}
				}
			}
		}

		//processDList();
		rsp_register.rsp_pc &= 0xFFF;
		start_section(GFX_SECTION);
		doRspCycles(100);
		end_section(GFX_SECTION);
		rsp_register.rsp_pc |= save_pc;
		new_frame();

		update_count();
		if (MI_register.mi_intr_reg & 0x1)
			add_interupt_event(SP_INT, 500);
		if (MI_register.mi_intr_reg & 0x20)
			add_interupt_event(DP_INT, 1000);
		MI_register.mi_intr_reg &= ~0x21;
		sp_register.sp_status_reg &= ~0x303;

		// protecting new frame buffers
		if(fBGetFrameBufferInfo && fBRead && fBWrite) fBGetFrameBufferInfo(frameBufferInfos);
		if(fBGetFrameBufferInfo && fBRead && fBWrite &&
		frameBufferInfos[0].addr)
		{
			int i;
			for(i=0; i<6; i++)
			{
				if(frameBufferInfos[i].addr)
				{
					int j;
					int start = frameBufferInfos[i].addr & MEMMASK;
					int end = start + frameBufferInfos[i].width*
					frameBufferInfos[i].height*
					frameBufferInfos[i].size - 1;
					int start1 = start;
					int end1 = end;
					start >>= 16;
					end >>= 16;
					for(j=start; j<=end; j++)
					{
						rwmem[0x8000+j] = rw_rdramFB;
						rwmem[0xa000+j] = rw_rdramFB;
					}
					start <<= 4;
					end <<= 4;
					for(j=start; j<=end; j++)
					{
						if(j>=start1 && j<=end1) framebufferRead[j]=1;
						else framebufferRead[j] = 0;
					}
					if(firstFrameBufferSetting)
					{
						firstFrameBufferSetting = 0;
						// TODO: fast_memory = 0?
						for(j=0; j<0x100000; j++)
							invalid_code_set(j, 1);
					}
				}
			}
		}
		return;
	}
	
	if(delayedDP) // only DPs in delayed mode
		return;

	if (SP_DMEM[0xFC0/4] == 2)
    {
        //audio.processAList();
        rsp_register.rsp_pc &= 0xFFF;
        start_section(AUDIO_SECTION);
        doRspCycles(100);
        end_section(AUDIO_SECTION);
        rsp_register.rsp_pc |= save_pc;

        update_count();
        if (MI_register.mi_intr_reg & 0x1)
            add_interupt_event(SP_INT, 100);
        MI_register.mi_intr_reg &= ~0x1;
        sp_register.sp_status_reg &= ~0x303;
        
    }
    else
    {
        rsp_register.rsp_pc &= 0xFFF;
        doRspCycles(100);
        rsp_register.rsp_pc |= save_pc;

        update_count();
        if (MI_register.mi_intr_reg & 0x1)
            add_interupt_event(SP_INT, 0);
        MI_register.mi_intr_reg &= ~0x1;
        sp_register.sp_status_reg &= ~0x203;
    }
}

void update_SP()
{
	if (sp_register.w_sp_status_reg & 0x1)
		sp_register.halt = 0;
	if (sp_register.w_sp_status_reg & 0x2)
		sp_register.halt = 1;
	if (sp_register.w_sp_status_reg & 0x4)
		sp_register.broke = 0;
	if (sp_register.w_sp_status_reg & 0x8)
	{
		MI_register.mi_intr_reg &= 0xFFFFFFFE;
		check_interupt();
	}
	if (sp_register.w_sp_status_reg & 0x10)
	{
		MI_register.mi_intr_reg |= 1;
		check_interupt();
	}
	if (sp_register.w_sp_status_reg & 0x20)
		sp_register.single_step = 0;
	if (sp_register.w_sp_status_reg & 0x40)
		sp_register.single_step = 1;
	if (sp_register.w_sp_status_reg & 0x80)
		sp_register.intr_break = 0;
	if (sp_register.w_sp_status_reg & 0x100)
		sp_register.intr_break = 1;
	if (sp_register.w_sp_status_reg & 0x200)
		sp_register.signal0 = 0;
	if (sp_register.w_sp_status_reg & 0x400)
		sp_register.signal0 = 1;
	if (sp_register.w_sp_status_reg & 0x800)
		sp_register.signal1 = 0;
	if (sp_register.w_sp_status_reg & 0x1000)
		sp_register.signal1 = 1;
	if (sp_register.w_sp_status_reg & 0x2000)
		sp_register.signal2 = 0;
	if (sp_register.w_sp_status_reg & 0x4000)
		sp_register.signal2 = 1;
	if (sp_register.w_sp_status_reg & 0x8000)
		sp_register.signal3 = 0;
	if (sp_register.w_sp_status_reg & 0x10000)
		sp_register.signal3 = 1;
	if (sp_register.w_sp_status_reg & 0x20000)
		sp_register.signal4 = 0;
	if (sp_register.w_sp_status_reg & 0x40000)
		sp_register.signal4 = 1;
	if (sp_register.w_sp_status_reg & 0x80000)
		sp_register.signal5 = 0;
	if (sp_register.w_sp_status_reg & 0x100000)
		sp_register.signal5 = 1;
	if (sp_register.w_sp_status_reg & 0x200000)
		sp_register.signal6 = 0;
	if (sp_register.w_sp_status_reg & 0x400000)
		sp_register.signal6 = 1;
	if (sp_register.w_sp_status_reg & 0x800000)
		sp_register.signal7 = 0;
	if (sp_register.w_sp_status_reg & 0x1000000)
		sp_register.signal7 = 1;
		sp_register.sp_status_reg = ((sp_register.halt) |
			(sp_register.broke << 1) |
			(sp_register.dma_busy << 2) |
			(sp_register.dma_full << 3) |
			(sp_register.io_full << 4) |
			(sp_register.single_step << 5) |
			(sp_register.intr_break << 6) |
			(sp_register.signal0 << 7) |
			(sp_register.signal1 << 8) |
			(sp_register.signal2 << 9) |
			(sp_register.signal3 << 10) |
			(sp_register.signal4 << 11) |
			(sp_register.signal5 << 12) |
			(sp_register.signal6 << 13) |
			(sp_register.signal7 << 14)
		);
	//if (get_event(SP_INT)) return;
	if (!(sp_register.w_sp_status_reg & 0x1) && !(sp_register.w_sp_status_reg & 0x4)) return;
	if (!sp_register.halt && !sp_register.broke)
		do_SP_Task(0,0); // immediate run
}

void update_DPC(void)
{
    if (dpc_register.w_dpc_status & 0x1) // clear xbus_dmem_dma
        dpc_register.dpc_status &= ~0x1;
    if (dpc_register.w_dpc_status & 0x2) // set xbus_dmem_dma
        dpc_register.dpc_status |= 0x1;

    if (dpc_register.w_dpc_status & 0x4) // clear freeze
    {
        dpc_register.dpc_status &= ~0x2;

        // see do_SP_task for more info
        if (!(sp_register.sp_status_reg & 0x3)) // !halt && !broke
            SP_Task_pending_cycles=FROZEN_TASK_DELAY;
    }
    if (dpc_register.w_dpc_status & 0x8) // set freeze
        dpc_register.dpc_status |= 0x2;

    if (dpc_register.w_dpc_status & 0x10) // clear flush
        dpc_register.dpc_status &= ~0x4;
    if (dpc_register.w_dpc_status & 0x20) // set flush
        dpc_register.dpc_status |= 0x4;
}

unsigned long long int read_nothing(unsigned long address, unsigned long long int data)
{
	if (address == 0xa5000508) 
		return 0xFFFFFFFF;
	else
		return 0;
}

unsigned long long int read_nothingb(unsigned long address, unsigned long long int data)
{
	return 0;
}

unsigned long long int read_nothingh(unsigned long address, unsigned long long int data)
{
	return 0;
}

unsigned long long int read_nothingd(unsigned long address, unsigned long long int data)
{
	return 0;
}

unsigned long long int write_nothing(unsigned long address, unsigned long long int data)
{
	return 0;
}

unsigned long long int write_nothingb(unsigned long address, unsigned long long int data)
{
	return 0;
}

unsigned long long int write_nothingh(unsigned long address, unsigned long long int data)
{
	return 0;
}

unsigned long long int write_nothingd(unsigned long address, unsigned long long int data)
{
	return 0;
}

unsigned long long int read_nomem(unsigned long address, unsigned long long int data)
{
	address = virtual_to_physical_address(address,0);
	if (address == 0x00000000) return 0;
	return read_word_in_memory(address, 0);
}

unsigned long long int read_nomemb(unsigned long address, unsigned long long int data)
{
	address = virtual_to_physical_address(address,0);
	if (address == 0x00000000) return 0;
	return read_byte_in_memory(address, 0);
}

unsigned long long int read_nomemh(unsigned long address, unsigned long long int data)
{
   address = virtual_to_physical_address(address,0);
   if (address == 0x00000000) return 0;
   return read_hword_in_memory(address, 0);
}

unsigned long long int read_nomemd(unsigned long address, unsigned long long int data)
{
	address = virtual_to_physical_address(address,0);
	if (address == 0x00000000) return 0;
	return read_dword_in_memory(address, 0);
}

#define check_memory(address) \
	if(!interpcore && !invalid_code_get(address>>12)) \
		invalidate_func(address);
unsigned long long int write_nomem(unsigned long address, unsigned long long int data)
{
	check_memory(address);
	address = virtual_to_physical_address(address,1);
	if (address == 0x00000000) return 0;
	return write_word_in_memory(address, data);
}

unsigned long long int write_nomemb(unsigned long address, unsigned long long int data)
{
	check_memory(address);
	address = virtual_to_physical_address(address,1);
	if (address == 0x00000000) return 0;
	return write_byte_in_memory(address, data);
}

unsigned long long int write_nomemh(unsigned long address, unsigned long long int data)
{
	check_memory(address);
	address = virtual_to_physical_address(address,1);
	if (address == 0x00000000) return 0;
	return write_hword_in_memory(address, data);
}

unsigned long long int write_nomemd(unsigned long address, unsigned long long int data)
{
	check_memory(address);
	address = virtual_to_physical_address(address,1);
	if (address == 0x00000000) return 0;
	return write_dword_in_memory(address, data);
}

unsigned long long int read_rdram(unsigned long address, unsigned long long int data)
{
	return *((unsigned long *)(rdramb + (address & MEMMASK)));
}

unsigned long long int read_rdramb(unsigned long address, unsigned long long int data)
{
	return *(rdramb + ((address & MEMMASK)^S8));
}

unsigned long long int read_rdramh(unsigned long address, unsigned long long int data)
{
	return *((unsigned short *)(rdramb + ((address & MEMMASK)^S16)));
}

unsigned long long int read_rdramd(unsigned long address, unsigned long long int data)
{
	return ((unsigned long long int)(*(unsigned long *)(rdramb + (address & MEMMASK))) << 32) | ((*(unsigned long *)(rdramb + (address & MEMMASK) + 4)));
}

unsigned long long int read_rdramFB(unsigned long address, unsigned long long int data)
{
	int i;
	for(i=0; i<6; i++)
	{
		if(frameBufferInfos[i].addr)
		{
			int start = frameBufferInfos[i].addr & MEMMASK;
			int end = start + frameBufferInfos[i].width*
			frameBufferInfos[i].height*
			frameBufferInfos[i].size - 1;
			if((address & MEMMASK) >= start && (address & MEMMASK) <= end && framebufferRead[(address & MEMMASK)>>12])
			{
				fBRead(address);
				framebufferRead[(address & MEMMASK)>>12] = 0;
			}
		}
	}
	return read_rdram(address, 0);
}

unsigned long long int read_rdramFBb(unsigned long address, unsigned long long int data)
{
	int i;
	for(i=0; i<6; i++)
	{
		if(frameBufferInfos[i].addr)
		{
			int start = frameBufferInfos[i].addr & MEMMASK;
			int end = start + frameBufferInfos[i].width*
			frameBufferInfos[i].height*
			frameBufferInfos[i].size - 1;
			if((address & MEMMASK) >= start && (address & MEMMASK) <= end && framebufferRead[(address & MEMMASK)>>12])
			{
				fBRead(address);
				framebufferRead[(address & MEMMASK)>>12] = 0;
			}
		}
	}
	return read_rdramb(address, 0);
}

unsigned long long int read_rdramFBh(unsigned long address, unsigned long long int data)
{
	int i;
	for(i=0; i<6; i++)
	{
		if(frameBufferInfos[i].addr)
		{
			int start = frameBufferInfos[i].addr & MEMMASK;
			int end = start + frameBufferInfos[i].width*
			frameBufferInfos[i].height*
			frameBufferInfos[i].size - 1;
			if((address & MEMMASK) >= start && (address & MEMMASK) <= end && framebufferRead[(address & MEMMASK)>>12])
			{
				fBRead(address);
				framebufferRead[(address & MEMMASK)>>12] = 0;
			}
		}
	}
	return read_rdramh(address, 0);
}

unsigned long long int read_rdramFBd(unsigned long address, unsigned long long int data)
{
	int i;
	for(i=0; i<6; i++)
	{
		if(frameBufferInfos[i].addr)
		{
			int start = frameBufferInfos[i].addr & MEMMASK;
			int end = start + frameBufferInfos[i].width*
			frameBufferInfos[i].height*
			frameBufferInfos[i].size - 1;
			if((address & MEMMASK) >= start && (address & MEMMASK) <= end && framebufferRead[(address & MEMMASK)>>12])
			{
				fBRead(address);
				framebufferRead[(address & MEMMASK)>>12] = 0;
			}
		}
	}
	return read_rdramd(address, 0);
}

unsigned long long int write_rdram(unsigned long address, unsigned long long int data)
{
	*((unsigned long *)(rdramb + (address & MEMMASK))) = (unsigned long)data;
	return 0;
}

unsigned long long int write_rdramb(unsigned long address, unsigned long long int data)
{
	*((rdramb + ((address & MEMMASK)^S8))) = (unsigned char)data;
	return 0;
}

unsigned long long int write_rdramh(unsigned long address, unsigned long long int data)
{
	*(unsigned short *)((rdramb + ((address & MEMMASK)^S16))) = (unsigned short)data;
	return 0;
}

unsigned long long int write_rdramd(unsigned long address, unsigned long long int data)
{
	*((unsigned long *)(rdramb + (address & MEMMASK))) = data >> 32;
	*((unsigned long *)(rdramb + (address & MEMMASK) + 4 )) = data & 0xFFFFFFFF;
	return 0;
}

unsigned long long int write_rdramFB(unsigned long address, unsigned long long int data)
{
	int i;
	for(i=0; i<6; i++)
	{
		if(frameBufferInfos[i].addr)
		{
			int start = frameBufferInfos[i].addr & MEMMASK;
			int end = start + frameBufferInfos[i].width*
			frameBufferInfos[i].height*
			frameBufferInfos[i].size - 1;
			if((address & MEMMASK) >= start && (address & MEMMASK) <= end)
				fBWrite(address, 4);
		}
	}
	return write_rdram(address, data);
}

unsigned long long int write_rdramFBb(unsigned long address, unsigned long long int data)
{
	int i;
	for(i=0; i<6; i++)
	{
		if(frameBufferInfos[i].addr)
		{
			int start = frameBufferInfos[i].addr & MEMMASK;
			int end = start + frameBufferInfos[i].width*
			frameBufferInfos[i].height*
			frameBufferInfos[i].size - 1;
			if((address & MEMMASK) >= start && (address & MEMMASK) <= end)
				fBWrite(address^S8, 1);
		}
	}
	return write_rdramb(address, data);
}

unsigned long long int write_rdramFBh(unsigned long address, unsigned long long int data)
{
	int i;
	for(i=0; i<6; i++)
	{
		if(frameBufferInfos[i].addr)
		{
			int start = frameBufferInfos[i].addr & MEMMASK;
			int end = start + frameBufferInfos[i].width*
			frameBufferInfos[i].height*
			frameBufferInfos[i].size - 1;
			if((address & MEMMASK) >= start && (address & MEMMASK) <= end)
				fBWrite(address^S16, 2);
		}
	}
	return write_rdramh(address, data);
}

unsigned long long int write_rdramFBd(unsigned long address, unsigned long long int data)
{
	int i;
	for(i=0; i<6; i++)
	{
		if(frameBufferInfos[i].addr)
		{
			int start = frameBufferInfos[i].addr & MEMMASK;
			int end = start + frameBufferInfos[i].width*
			frameBufferInfos[i].height*
			frameBufferInfos[i].size - 1;
			if((address & MEMMASK) >= start && (address & MEMMASK) <= end)
				fBWrite(address, 8);
		}
	}
	return write_rdramd(address, data);
}

unsigned long long int read_rdramreg(unsigned long address, unsigned long long int data)
{
	if(unlikely(GET_LOW_ADDR(address) >= sizeof(readrdramreg))) {
		return trash;
	}
	else {
		return *(readrdramreg[GET_LOW_ADDR(address)]);
	}   
}

unsigned long long int read_rdramregb(unsigned long address, unsigned long long int data)
{
	if(unlikely((GET_LOW_ADDR(address) & 0xfffc) >= sizeof(readrdramreg))) {
		return trash & 0xFF;
	}
	else {
		return *((unsigned char*)readrdramreg[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) );
	}
}

unsigned long long int read_rdramregh(unsigned long address, unsigned long long int data)
{
	if(unlikely((GET_LOW_ADDR(address) & 0xfffc) >= sizeof(readrdramreg))) {
		return trash & 0xFFFF;
	}
	else {
		return *((unsigned short*)((unsigned char*)readrdramreg[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) ));
	}
}

unsigned long long int read_rdramregd(unsigned long address, unsigned long long int data)
{
	if(unlikely(GET_LOW_ADDR(address) > sizeof(readrdramreg) - sizeof(long long int))) {
		return trash;
	}
	else {
		return ((unsigned long long int)(*readrdramreg[GET_LOW_ADDR(address)])<<32) | *readrdramreg[GET_LOW_ADDR(address)+4];
	}
}

unsigned long long int write_rdramreg(unsigned long address, unsigned long long int data)
{
	if(unlikely(GET_LOW_ADDR(address) >= sizeof(readrdramreg))) {
		trash = (unsigned long)data;
	}
	else {
		*readrdramreg[GET_LOW_ADDR(address)] = (unsigned long)data;
	}
	return 0;
}

unsigned long long int write_rdramregb(unsigned long address, unsigned long long int data)
{
	if(unlikely((GET_LOW_ADDR(address) & 0xfffc) >= sizeof(readrdramreg))) {
		trash = (unsigned char)data;
	}
	else {
		*((unsigned char*)readrdramreg[GET_LOW_ADDR(address) & 0xfffc]
			+ ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
	}
	return 0;
}

unsigned long long int write_rdramregh(unsigned long address, unsigned long long int data)
{
	if(unlikely((GET_LOW_ADDR(address) & 0xfffc) >= sizeof(readrdramreg))) {
		trash = (unsigned short)data;
	}
	else {
		*((unsigned short*)((unsigned char*)readrdramreg[GET_LOW_ADDR(address) & 0xfffc]
			+ ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
	}
	return 0;
}

unsigned long long int write_rdramregd(unsigned long address, unsigned long long int data)
{
	if(unlikely(GET_LOW_ADDR(address) > sizeof(readrdramreg) - sizeof(long long int))) {
		trash = (unsigned long)data;
	}
	else {
		*readrdramreg[GET_LOW_ADDR(address)] = data >> 32;
		*readrdramreg[GET_LOW_ADDR(address)+4] = data & 0xFFFFFFFF;
	}
	return 0;
}

unsigned long long int read_rsp_mem(unsigned long address, unsigned long long int data)
{
	if (GET_LOW_ADDR(address) < 0x1000)
		return *((unsigned long *)(SP_DMEMb + (GET_LOW_ADDR(address))));
	else if (GET_LOW_ADDR(address) < 0x2000)
		return *((unsigned long *)(SP_IMEMb + (GET_LOW_ADDR(address)&0xFFF)));
	else
		return read_nomem(address, 0);
}

unsigned long long int read_rsp_memb(unsigned long address, unsigned long long int data)
{
	if (GET_LOW_ADDR(address) < 0x1000)
		return *(SP_DMEMb + (GET_LOW_ADDR(address)^S8));
	else if (GET_LOW_ADDR(address) < 0x2000)
		return *(SP_IMEMb + ((GET_LOW_ADDR(address)&0xFFF)^S8));
	else
		return read_nomemb(address, 0);
}

unsigned long long int read_rsp_memh(unsigned long address, unsigned long long int data)
{
	if (GET_LOW_ADDR(address) < 0x1000)
		return *((unsigned short *)(SP_DMEMb + (GET_LOW_ADDR(address)^S16)));
	else if (GET_LOW_ADDR(address) < 0x2000)
		return *((unsigned short *)(SP_IMEMb + ((GET_LOW_ADDR(address)&0xFFF)^S16)));
	else
		return read_nomemh(address, 0);
}

unsigned long long int read_rsp_memd(unsigned long address, unsigned long long int data)
{
	if (GET_LOW_ADDR(address) < 0x1000)
	{
		return ((unsigned long long int)(*(unsigned long *)(SP_DMEMb + (GET_LOW_ADDR(address)))) << 32) | ((*(unsigned long *)(SP_DMEMb + (GET_LOW_ADDR(address)) + 4)));
	}
	else if (GET_LOW_ADDR(address) < 0x2000)
	{
		return ((unsigned long long int)(*(unsigned long *)(SP_IMEMb + (GET_LOW_ADDR(address)&0xFFF))) << 32) | ((*(unsigned long *)(SP_IMEMb + (GET_LOW_ADDR(address)&0xFFF) + 4)));
	}
	else
		return read_nomemd(address, 0);
}

unsigned long long int write_rsp_mem(unsigned long address, unsigned long long int data)
{
	if (GET_LOW_ADDR(address) < 0x1000)
		*((unsigned long *)(SP_DMEMb + (GET_LOW_ADDR(address)))) = (unsigned long)data;
	else if (GET_LOW_ADDR(address) < 0x2000)
		*((unsigned long *)(SP_IMEMb + (GET_LOW_ADDR(address)&0xFFF))) = (unsigned long)data;
	else
		write_nomem(address, data);
	return 0;
}

unsigned long long int write_rsp_memb(unsigned long address, unsigned long long int data)
{
	if (GET_LOW_ADDR(address) < 0x1000)
		*(SP_DMEMb + (GET_LOW_ADDR(address)^S8)) = (unsigned char)data;
	else if (GET_LOW_ADDR(address) < 0x2000)
		*(SP_IMEMb + ((GET_LOW_ADDR(address)&0xFFF)^S8)) = (unsigned char)data;
	else
		write_nomemb(address, data);
	return 0;
}

unsigned long long int write_rsp_memh(unsigned long address, unsigned long long int data)
{
	if (GET_LOW_ADDR(address) < 0x1000)
		*((unsigned short *)(SP_DMEMb + (GET_LOW_ADDR(address)^S16))) = (unsigned short)data;
	else if (GET_LOW_ADDR(address) < 0x2000)
		*((unsigned short *)(SP_IMEMb + ((GET_LOW_ADDR(address)&0xFFF)^S16))) = (unsigned short)data;
	else
		write_nomemh(address, data);
	return 0;
}

unsigned long long int write_rsp_memd(unsigned long address, unsigned long long int data)
{
	if (GET_LOW_ADDR(address) < 0x1000)
	{
		*((unsigned long *)(SP_DMEMb + GET_LOW_ADDR(address))) = data >> 32;
		*((unsigned long *)(SP_DMEMb + GET_LOW_ADDR(address) + 4 )) = data & 0xFFFFFFFF;
	}
	else if (GET_LOW_ADDR(address) < 0x2000)
	{
		*((unsigned long *)(SP_IMEMb + (GET_LOW_ADDR(address)&0xFFF))) = data >> 32;
		*((unsigned long *)(SP_IMEMb + (GET_LOW_ADDR(address)&0xFFF) + 4 )) = data & 0xFFFFFFFF;
	}
	else
		write_nomemd(address, data);
	return 0;
}

unsigned long long int read_rsp_reg(unsigned long address, unsigned long long int data)
{
	if(GET_LOW_ADDR(address) == 0x1C)
		sp_register.sp_semaphore_reg = 1;
	return *(readrspreg[GET_LOW_ADDR(address)]);
}

unsigned long long int read_rsp_regb(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			sp_register.sp_semaphore_reg = 1;
		break;
	}
	return *((unsigned char*)readrspreg[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) );
}

unsigned long long int read_rsp_regh(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x1c:
		case 0x1e:
			sp_register.sp_semaphore_reg = 1;
		break;
	}
	return *((unsigned short*)((unsigned char*)readrspreg[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) ));
}

unsigned long long int read_rsp_regd(unsigned long address, unsigned long long int data)
{
	if(GET_LOW_ADDR(address) == 0x18)
		sp_register.sp_semaphore_reg = 1;
	return ((unsigned long long int)(*readrspreg[GET_LOW_ADDR(address)])<<32) | *readrspreg[GET_LOW_ADDR(address)+4];
}

unsigned long long int write_rsp_reg(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x10:
			sp_register.w_sp_status_reg = (unsigned long)data;
			update_SP();
			return 0;
		break;
		case 0x14:
		case 0x18:
			return 0;
		break;
	}
	*readrspreg[GET_LOW_ADDR(address)] = (unsigned long)data;
	switch(GET_LOW_ADDR(address))
	{
		case 0x8:
			dma_sp_write();
		break;
		case 0xc:
			dma_sp_read();
		break;
		case 0x1c:
			sp_register.sp_semaphore_reg = 0;
		break;
	}
	return 0;
}

unsigned long long int write_rsp_regb(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			*((unsigned char*)&sp_register.w_sp_status_reg + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
			return 0;
		break;
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
			return 0;
		break;
	}
	*((unsigned char*)readrspreg[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
	switch(GET_LOW_ADDR(address))
	{
		case 0x8:
		case 0x9:
		case 0xa:
		case 0xb:
			dma_sp_write();
		break;
		case 0xc:
		case 0xd:
		case 0xe:
		case 0xf:
			dma_sp_read();
		break;
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			sp_register.sp_semaphore_reg = 0;
		break;
	}
	return 0;
}

unsigned long long int write_rsp_regh(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x10:
		case 0x12:
			*((unsigned short*)((unsigned char*)&sp_register.w_sp_status_reg + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
			return 0;
		break;
		case 0x14:
		case 0x16:
		case 0x18:
		case 0x1a:
			return 0;
		break;
	}
	*((unsigned short*)((unsigned char*)readrspreg[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
	switch(GET_LOW_ADDR(address))
	{
		case 0x8:
		case 0xa:
			dma_sp_write();
		break;
		case 0xc:
		case 0xe:
			dma_sp_read();
		break;
		case 0x1c:
		case 0x1e:
			sp_register.sp_semaphore_reg = 0;
		break;
	}
	return 0;
}

unsigned long long int write_rsp_regd(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x10:
			sp_register.w_sp_status_reg = data >> 32;
			update_SP();
			return 0;
		break;
		case 0x18:
			sp_register.sp_semaphore_reg = 0;
			return 0;
		break;
	}
	*readrspreg[GET_LOW_ADDR(address)] = data >> 32;
	*readrspreg[GET_LOW_ADDR(address)+4] = data & 0xFFFFFFFF;
	switch(GET_LOW_ADDR(address))
	{
		case 0x8:
			dma_sp_write();
			dma_sp_read();
		break;
	}
	return 0;
}

unsigned long long int read_rsp(unsigned long address, unsigned long long int data)
{
	return *(readrsp[GET_LOW_ADDR(address)]);
}

unsigned long long int read_rspb(unsigned long address, unsigned long long int data)
{
	return *((unsigned char*)readrsp[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) );
}

unsigned long long int read_rsph(unsigned long address, unsigned long long int data)
{
	return *((unsigned short*)((unsigned char*)readrsp[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) ));
}

unsigned long long int read_rspd(unsigned long address, unsigned long long int data)
{
	return ((unsigned long long int)(*readrsp[GET_LOW_ADDR(address)])<<32) | *readrsp[GET_LOW_ADDR(address)+4];
}

unsigned long long int write_rsp(unsigned long address, unsigned long long int data)
{
	*readrsp[GET_LOW_ADDR(address)] = (unsigned long)data;
	return 0;
}

unsigned long long int write_rspb(unsigned long address, unsigned long long int data)
{
	*((unsigned char*)readrsp[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
	return 0;
}

unsigned long long int write_rsph(unsigned long address, unsigned long long int data)
{
	*((unsigned short*)((unsigned char*)readrsp[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
	return 0;
}

unsigned long long int write_rspd(unsigned long address, unsigned long long int data)
{
	*readrsp[GET_LOW_ADDR(address)] = data >> 32;
	*readrsp[GET_LOW_ADDR(address)+4] = data & 0xFFFFFFFF;
	return 0;
}

unsigned long long int read_dp(unsigned long address, unsigned long long int data)
{
	return *(readdp[GET_LOW_ADDR(address)]);
}

unsigned long long int read_dpb(unsigned long address, unsigned long long int data)
{
	return *((unsigned char*)readdp[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) );
}

unsigned long long int read_dph(unsigned long address, unsigned long long int data)
{
	return *((unsigned short*)((unsigned char*)readdp[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) ));
}

unsigned long long int read_dpd(unsigned long address, unsigned long long int data)
{
	return ((unsigned long long int)(*readdp[GET_LOW_ADDR(address)])<<32) | *readdp[GET_LOW_ADDR(address)+4];
}

unsigned long long int write_dp(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0xc:
			dpc_register.w_dpc_status = (unsigned long)data;
			update_DPC();
			return 0;
		break;
		case 0x8:
		case 0x10:
		case 0x14:
		case 0x18:
		case 0x1c:
			return 0;
		break;
	}
	*readdp[GET_LOW_ADDR(address)] = (unsigned long)data;
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
			dpc_register.dpc_current = dpc_register.dpc_start;
		break;
		case 0x4:
			processRDPList();
			MI_register.mi_intr_reg |= 0x20;
			check_interupt();
		break;
	}
	return 0;
}

unsigned long long int write_dpb(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0xc:
		case 0xd:
		case 0xe:
		case 0xf:
			*((unsigned char*)&dpc_register.w_dpc_status + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
			update_DPC();
			return 0;
		break;
		case 0x8:
		case 0x9:
		case 0xa:
		case 0xb:
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			return 0;
		break;
	}
	*((unsigned char*)readdp[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
			dpc_register.dpc_current = dpc_register.dpc_start;
		break;
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
			processRDPList();
			MI_register.mi_intr_reg |= 0x20;
			check_interupt();
		break;
	}
	return 0;
}

unsigned long long int write_dph(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0xc:
		case 0xe:
			*((unsigned short*)((unsigned char*)&dpc_register.w_dpc_status + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
			update_DPC();
			return 0;
		break;
		case 0x8:
		case 0xa:
		case 0x10:
		case 0x12:
		case 0x14:
		case 0x16:
		case 0x18:
		case 0x1a:
		case 0x1c:
		case 0x1e:
			return 0;
		break;
	}
	*((unsigned short*)((unsigned char*)readdp[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
		case 0x2:
			dpc_register.dpc_current = dpc_register.dpc_start;
		break;
		case 0x4:
		case 0x6:
			processRDPList();
			MI_register.mi_intr_reg |= 0x20;
			check_interupt();
		break;
	}
	return 0;
}

unsigned long long int write_dpd(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x8:
			dpc_register.w_dpc_status = data & 0xFFFFFFFF;
			update_DPC();
			return 0;
		break;
		case 0x10:
		case 0x18:
			return 0;
		break;
	}
	*readdp[GET_LOW_ADDR(address)] = data >> 32;
	*readdp[GET_LOW_ADDR(address)+4] = data & 0xFFFFFFFF;
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
			dpc_register.dpc_current = dpc_register.dpc_start;
			processRDPList();
			MI_register.mi_intr_reg |= 0x20;
			check_interupt();
		break;
	}
	return 0;
}

unsigned long long int read_dps(unsigned long address, unsigned long long int data)
{
	return *(readdps[GET_LOW_ADDR(address)]);
}

unsigned long long int read_dpsb(unsigned long address, unsigned long long int data)
{
	return *((unsigned char*)readdps[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) );
}

unsigned long long int read_dpsh(unsigned long address, unsigned long long int data)
{
	return *((unsigned short*)((unsigned char*)readdps[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) ));
}

unsigned long long int read_dpsd(unsigned long address, unsigned long long int data)
{
	return ((unsigned long long int)(*readdps[GET_LOW_ADDR(address)])<<32) | *readdps[GET_LOW_ADDR(address)+4];
}

unsigned long long int write_dps(unsigned long address, unsigned long long int data)
{
	*readdps[GET_LOW_ADDR(address)] = (unsigned long)data;
	return 0;
}

unsigned long long int write_dpsb(unsigned long address, unsigned long long int data)
{
	*((unsigned char*)readdps[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
	return 0;
}

unsigned long long int write_dpsh(unsigned long address, unsigned long long int data)
{
	*((unsigned short*)((unsigned char*)readdps[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
	return 0;
}

unsigned long long int write_dpsd(unsigned long address, unsigned long long int data)
{
	*readdps[GET_LOW_ADDR(address)] = data >> 32;
	*readdps[GET_LOW_ADDR(address)+4] = data & 0xFFFFFFFF;
	return 0;
}

unsigned long long int read_mi(unsigned long address, unsigned long long int data)
{
	return *(readmi[GET_LOW_ADDR(address)]);
}

unsigned long long int read_mib(unsigned long address, unsigned long long int data)
{
	return *((unsigned char*)readmi[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) );
}

unsigned long long int read_mih(unsigned long address, unsigned long long int data)
{
	return *((unsigned short*)((unsigned char*)readmi[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) ));
}

unsigned long long int read_mid(unsigned long address, unsigned long long int data)
{
	return ((unsigned long long int)(*readmi[GET_LOW_ADDR(address)])<<32) | *readmi[GET_LOW_ADDR(address)+4];
}

unsigned long long int write_mi(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
			MI_register.w_mi_init_mode_reg = (unsigned long)data;
			update_MI_init_mode_reg();
		break;
		case 0xc:
			MI_register.w_mi_intr_mask_reg = (unsigned long)data;
			update_MI_intr_mask_reg();
			check_interupt();
			update_count();
			if (r4300.next_interrupt <= Count) gen_interupt();
		break;
	}
	return 0;
}

unsigned long long int write_mib(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
			*((unsigned char*)&MI_register.w_mi_init_mode_reg + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
			update_MI_init_mode_reg();
		break;
		case 0xc:
		case 0xd:
		case 0xe:
		case 0xf:
			*((unsigned char*)&MI_register.w_mi_intr_mask_reg + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
			update_MI_intr_mask_reg();
			check_interupt();
			update_count();
			if (r4300.next_interrupt <= Count) gen_interupt();
		break;
	}
	return 0;
}

unsigned long long int write_mih(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
		case 0x2:
			*((unsigned short*)((unsigned char*)&MI_register.w_mi_init_mode_reg + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
			update_MI_init_mode_reg();
		break;
		case 0xc:
		case 0xe:
			*((unsigned short*)((unsigned char*)&MI_register.w_mi_intr_mask_reg + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
			update_MI_intr_mask_reg();
			check_interupt();
			update_count();
			if (r4300.next_interrupt <= Count) gen_interupt();
		break;
	}
	return 0;
}

unsigned long long int write_mid(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
			MI_register.w_mi_init_mode_reg = data >> 32;
			update_MI_init_mode_reg();
		break;
		case 0x8:
			MI_register.w_mi_intr_mask_reg = data & 0xFFFFFFFF;
			update_MI_intr_mask_reg();
			check_interupt();
			update_count();
			if (r4300.next_interrupt <= Count) gen_interupt();
		break;
	}
	return 0;
}

unsigned long long int read_vi(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x10:
			update_count();
			vi_register.vi_current = (vi_register.vi_delay-(r4300.next_vi-Count))/1500;
			vi_register.vi_current = (vi_register.vi_current&(~1))|r4300.vi_field;
		break;
	}
	return *(readvi[GET_LOW_ADDR(address)]);
}

unsigned long long int read_vib(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			update_count();
			vi_register.vi_current = (vi_register.vi_delay-(r4300.next_vi-Count))/1500;
			vi_register.vi_current = (vi_register.vi_current&(~1))|r4300.vi_field;
		break;
	}
	return *((unsigned char*)readvi[GET_LOW_ADDR(address) & 0xfffc]+ ((GET_LOW_ADDR(address)&3)^S8) );
}

unsigned long long int read_vih(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x10:
		case 0x12:
			update_count();
			vi_register.vi_current = (vi_register.vi_delay-(r4300.next_vi-Count))/1500;
			vi_register.vi_current = (vi_register.vi_current&(~1))|r4300.vi_field;
		break;
	}
	return *((unsigned short*)((unsigned char*)readvi[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) ));
}

unsigned long long int read_vid(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x10:
			update_count();
			vi_register.vi_current = (vi_register.vi_delay-(r4300.next_vi-Count))/1500;
			vi_register.vi_current = (vi_register.vi_current&(~1))|r4300.vi_field;
		break;
	}
	return ((unsigned long long int)(*readvi[GET_LOW_ADDR(address)])<<32) | *readvi[GET_LOW_ADDR(address)+4];
}

unsigned long long int write_vi(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
			if (vi_register.vi_status != (unsigned long)data)
			{
				vi_register.vi_status = (unsigned long)data;
				viStatusChanged();
			}
			return 0;
		break;
		case 0x8:
			if (vi_register.vi_width != (unsigned long)data)
			{
				vi_register.vi_width = (unsigned long)data;
				viWidthChanged();
			}
			return 0;
		break;
		case 0x10:
			MI_register.mi_intr_reg &= 0xFFFFFFF7;
			check_interupt();
			return 0;
		break;
	}
	*readvi[GET_LOW_ADDR(address)] = (unsigned long)data;
	return 0;
}

unsigned long long int write_vib(unsigned long address, unsigned long long int data)
{
	int temp;
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
			temp = vi_register.vi_status;
			*((unsigned char*)&temp + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
			if (vi_register.vi_status != temp)
			{
				vi_register.vi_status = temp;
				viStatusChanged();
			}
			return 0;
		break;
		case 0x8:
		case 0x9:
		case 0xa:
		case 0xb:
			temp = vi_register.vi_status;
			*((unsigned char*)&temp + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
			if (vi_register.vi_width != temp)
			{
				vi_register.vi_width = temp;
				viWidthChanged();
			}
			return 0;
		break;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			MI_register.mi_intr_reg &= 0xFFFFFFF7;
			check_interupt();
			return 0;
		break;
	}
	*((unsigned char*)readvi[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
	return 0;
}

unsigned long long int write_vih(unsigned long address, unsigned long long int data)
{
	int temp;
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
		case 0x2:
			temp = vi_register.vi_status;
			*((unsigned short*)((unsigned char*)&temp + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
			if (vi_register.vi_status != temp)
			{
				vi_register.vi_status = temp;
				viStatusChanged();
			}
			return 0;
		break;
		case 0x8:
		case 0xa:
			temp = vi_register.vi_status;
			*((unsigned short*)((unsigned char*)&temp + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
			if (vi_register.vi_width != temp)
			{
				vi_register.vi_width = temp;
				viWidthChanged();
			}
			return 0;
		break;
		case 0x10:
		case 0x12:
			MI_register.mi_intr_reg &= 0xFFFFFFF7;
			check_interupt();
			return 0;
		break;
	}
	*((unsigned short*)((unsigned char*)readvi[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
	return 0;
}

unsigned long long int write_vid(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
			if (vi_register.vi_status != data >> 32)
			{
				vi_register.vi_status = data >> 32;
				viStatusChanged();
			}
			vi_register.vi_origin = data & 0xFFFFFFFF;
			return 0;
		break;
		case 0x8:
			if (vi_register.vi_width != data >> 32)
			{
				vi_register.vi_width = data >> 32;
				viWidthChanged();
			}
			vi_register.vi_v_intr = data & 0xFFFFFFFF;
			return 0;
		break;
		case 0x10:
			MI_register.mi_intr_reg &= 0xFFFFFFF7;
			check_interupt();
			vi_register.vi_burst = data & 0xFFFFFFFF;
			return 0;
		break;
	}
	*readvi[GET_LOW_ADDR(address)] = data >> 32;
	*readvi[GET_LOW_ADDR(address)+4] = data & 0xFFFFFFFF;
	return 0;
}

unsigned long long int read_ai(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x4:
			update_count();
			if (ai_register.current_delay != 0 && get_event(AI_INT) != 0 && (get_event(AI_INT)-Count) < 0x80000000)
				return ((get_event(AI_INT)-Count)*(long long)ai_register.current_len)/ai_register.current_delay;
			else
				return 0;
		break;
	}
	return *(readai[GET_LOW_ADDR(address)]);
}

unsigned long long int read_aib(unsigned long address, unsigned long long int data)
{
	unsigned long len;
	switch(GET_LOW_ADDR(address))
	{
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
			update_count();
			if (ai_register.current_delay != 0 && get_event(AI_INT) != 0)
				len = ((get_event(AI_INT)-Count)*(long long)ai_register.current_len)/ai_register.current_delay;
			else
				len = 0;
			return *((unsigned char*)&len + ((GET_LOW_ADDR(address)&3)^S8) );
		break;
	}
	return *((unsigned char*)readai[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) );
}

unsigned long long int read_aih(unsigned long address, unsigned long long int data)
{
	unsigned long len;
	switch(GET_LOW_ADDR(address))
	{
		case 0x4:
		case 0x6:
			update_count();
			if (ai_register.current_delay != 0 && get_event(AI_INT) != 0)
				len = ((get_event(AI_INT)-Count)*(long long)ai_register.current_len)/ai_register.current_delay;
			else
				len = 0;
			return *((unsigned short*)((unsigned char*)&len + ((GET_LOW_ADDR(address)&3)^S16) ));
		break;
	}
	return *((unsigned short*)((unsigned char*)readai[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) ));
}

unsigned long long int read_aid(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
			update_count();
			if (ai_register.current_delay != 0 && get_event(AI_INT) != 0)
				return ((get_event(AI_INT)-Count)*(long long)ai_register.current_len)/ai_register.current_delay;
			else
				return (unsigned long long)ai_register.ai_dram_addr << 32 & 0xFFFFFFFF00000000LL;
		break;
	}
	return ((unsigned long long int)(*readai[GET_LOW_ADDR(address)])<<32) | *readai[GET_LOW_ADDR(address)+4];
}

unsigned long long int write_ai(unsigned long address, unsigned long long int data)
{
	unsigned long delay=0;
	switch(GET_LOW_ADDR(address))
	{
		case 0x4:
			ai_register.ai_len = (unsigned long)data;
			aiLenChanged();
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
				{
					unsigned long f = 49656530/(ai_register.ai_dacrate+1);
					if (f)
					delay = ((unsigned long long)ai_register.ai_len*vi_register.vi_delay*50)/(f*4);
				}
				break;
				case 0x37:
				case 0x41:
				case 0x45:
				case 0x4a:
				{
					unsigned long f = 48681812/(ai_register.ai_dacrate+1);
					if (f)
					delay = ((unsigned long long)ai_register.ai_len*vi_register.vi_delay*60)/(f*4);
				}
				break;
			}
			if (no_audio_delay) delay = 0;
			if (ai_register.ai_status & 0x40000000) // busy
			{
				ai_register.next_delay = delay;
				ai_register.next_len = ai_register.ai_len;
				ai_register.ai_status |= 0x80000000;
			}
			else
			{
				ai_register.current_delay = delay;
				ai_register.current_len = ai_register.ai_len;
				update_count();
				add_interupt_event(AI_INT, delay);
				ai_register.ai_status |= 0x40000000;
			}
			return 0;
		break;
		case 0xc:
			MI_register.mi_intr_reg &= 0xFFFFFFFB;
			check_interupt();
			return 0;
		break;
		case 0x10:
			if (ai_register.ai_dacrate != (unsigned long)data)
			{
				ai_register.ai_dacrate = (unsigned long)data;
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
					aiDacrateChanged(SYSTEM_PAL);
				break;
				case 0x37:
				case 0x41:
				case 0x45:
				case 0x4a:
					aiDacrateChanged(SYSTEM_NTSC);
				break;
				}
			}
			return 0;
		break;
	}
	*readai[GET_LOW_ADDR(address)] = (unsigned long)data;
	return 0;
}

unsigned long long int write_aib(unsigned long address, unsigned long long int data)
{
	int temp;
	unsigned long delay=0;
	switch(GET_LOW_ADDR(address))
	{
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
			temp = ai_register.ai_len;
			*((unsigned char*)&temp + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
			ai_register.ai_len = temp;
			aiLenChanged();
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
					delay = ((unsigned long long)ai_register.ai_len*(ai_register.ai_dacrate+1)*vi_register.vi_delay*50)/49656530;
				break;
				case 0x37:
				case 0x41:
				case 0x45:
				case 0x4a:
					delay = ((unsigned long long)ai_register.ai_len*(ai_register.ai_dacrate+1)*vi_register.vi_delay*60)/48681812;
				break;
			}
			//delay = 0;
			if (ai_register.ai_status & 0x40000000) // busy
			{
				ai_register.next_delay = delay;
				ai_register.next_len = ai_register.ai_len;
				ai_register.ai_status |= 0x80000000;
			}
			else
			{
				ai_register.current_delay = delay;
				ai_register.current_len = ai_register.ai_len;
				update_count();
				add_interupt_event(AI_INT, delay/2);
				ai_register.ai_status |= 0x40000000;
			}
			return 0;
		break;
		case 0xc:
		case 0xd:
		case 0xe:
		case 0xf:
			MI_register.mi_intr_reg &= 0xFFFFFFFB;
			check_interupt();
			return 0;
		break;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			temp = ai_register.ai_dacrate;
			*((unsigned char*)&temp + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
			if (ai_register.ai_dacrate != temp)
			{
				ai_register.ai_dacrate = temp;
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
						aiDacrateChanged(SYSTEM_PAL);
					break;
					case 0x37:
					case 0x41:
					case 0x45:
					case 0x4a:
						aiDacrateChanged(SYSTEM_NTSC);
					break;
				}
			}
			return 0;
		break;
	}
	*((unsigned char*)readai[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
	return 0;
}

unsigned long long int write_aih(unsigned long address, unsigned long long int data)
{
	int temp;
	unsigned long delay=0;
	switch(GET_LOW_ADDR(address))
	{
		case 0x4:
		case 0x6:
			temp = ai_register.ai_len;
			*((unsigned short*)((unsigned char*)&temp + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
			ai_register.ai_len = temp;
			aiLenChanged();
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
					delay = ((unsigned long long)ai_register.ai_len*(ai_register.ai_dacrate+1)*vi_register.vi_delay*50)/49656530;
				break;
				case 0x37:
				case 0x41:
				case 0x45:
				case 0x4a:
					delay = ((unsigned long long)ai_register.ai_len*(ai_register.ai_dacrate+1)*vi_register.vi_delay*60)/48681812;
				break;
			}
			if (no_audio_delay) delay = 0;
			if (ai_register.ai_status & 0x40000000) // busy
			{
				ai_register.next_delay = delay;
				ai_register.next_len = ai_register.ai_len;
				ai_register.ai_status |= 0x80000000;
			}
			else
			{
				ai_register.current_delay = delay;
				ai_register.current_len = ai_register.ai_len;
				update_count();
				add_interupt_event(AI_INT, delay/2);
				ai_register.ai_status |= 0x40000000;
			}
			return 0;
		break;
		case 0xc:
		case 0xe:
			MI_register.mi_intr_reg &= 0xFFFFFFFB;
			check_interupt();
			return 0;
		break;
		case 0x10:
		case 0x12:
			temp = ai_register.ai_dacrate;
			*((unsigned short*)((unsigned char*)&temp + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
			if (ai_register.ai_dacrate != temp)
			{
				ai_register.ai_dacrate = temp;
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
						aiDacrateChanged(SYSTEM_PAL);
					break;
					case 0x37:
					case 0x41:
					case 0x45:
					case 0x4a:
						aiDacrateChanged(SYSTEM_NTSC);
					break;
				}
			}
			return 0;
		break;
	}
	*((unsigned short*)((unsigned char*)readai[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
	return 0;
}

unsigned long long int write_aid(unsigned long address, unsigned long long int data)
{
	unsigned long delay=0;
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
			ai_register.ai_dram_addr = data >> 32;
			ai_register.ai_len = data & 0xFFFFFFFF;
			aiLenChanged();
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
					delay = ((unsigned long long)ai_register.ai_len*(ai_register.ai_dacrate+1)*
					vi_register.vi_delay*50)/49656530;
				break;
				case 0x37:
				case 0x41:
				case 0x45:
				case 0x4a:
					delay = ((unsigned long long)ai_register.ai_len*(ai_register.ai_dacrate+1)*
					vi_register.vi_delay*60)/48681812;
				break;
			}
			if (no_audio_delay) delay = 0;
			if (ai_register.ai_status & 0x40000000) // busy
			{
				ai_register.next_delay = delay;
				ai_register.next_len = ai_register.ai_len;
				ai_register.ai_status |= 0x80000000;
			}
			else
			{
				ai_register.current_delay = delay;
				ai_register.current_len = ai_register.ai_len;
				update_count();
				add_interupt_event(AI_INT, delay/2);
				ai_register.ai_status |= 0x40000000;
			}
			return 0;
		break;
		case 0x8:
			ai_register.ai_control = data >> 32;
			MI_register.mi_intr_reg &= 0xFFFFFFFB;
			check_interupt();
			return 0;
		break;
		case 0x10:
			if (ai_register.ai_dacrate != data >> 32)
			{
				ai_register.ai_dacrate = data >> 32;
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
					aiDacrateChanged(SYSTEM_PAL);
					break;
					case 0x37:
					case 0x41:
					case 0x45:
					case 0x4a:
					aiDacrateChanged(SYSTEM_NTSC);
					break;
				}
			}
			ai_register.ai_bitrate = data & 0xFFFFFFFF;
			return 0;
		break;
	}
	*readai[GET_LOW_ADDR(address)] = data >> 32;
	*readai[GET_LOW_ADDR(address)+4] = data & 0xFFFFFFFF;
	return 0;
}

unsigned long long int read_pi(unsigned long address, unsigned long long int data)
{
	if(unlikely(GET_LOW_ADDR(address) >= sizeof(readpi))) {
		return trash;
	}
	else {
		return *(readpi[GET_LOW_ADDR(address)]);
	}
}

unsigned long long int read_pib(unsigned long address, unsigned long long int data)
{
	if(unlikely((GET_LOW_ADDR(address) & 0xfffc) >= sizeof(readpi))) {
		return trash & 0xFF;
	}
	else {
		return *((unsigned char*)readpi[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) );
	}
}

unsigned long long int read_pih(unsigned long address, unsigned long long int data)
{
	if(unlikely((GET_LOW_ADDR(address) & 0xfffc) >= sizeof(readpi))) {
		return trash & 0xFFFF;
	}
	else {
		return *((unsigned short*)((unsigned char*)readpi[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) ));
	}
}

unsigned long long int read_pid(unsigned long address, unsigned long long int data)
{
	if(unlikely(GET_LOW_ADDR(address) > sizeof(readpi) - sizeof(long long int))) {
		return trash;
	}
	else {
		return ((unsigned long long int)(*readpi[GET_LOW_ADDR(address)])<<32) | *readpi[GET_LOW_ADDR(address)+4];
	}
}

unsigned long long int write_pi(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x8:
			pi_register.pi_rd_len_reg = (unsigned long)data;
			dma_pi_read();
			return 0;
		break;
		case 0xc:
			pi_register.pi_wr_len_reg = (unsigned long)data;
			dma_pi_write();
			return 0;
		break;
		case 0x10:
			if ((unsigned long)data & 2) MI_register.mi_intr_reg &= 0xFFFFFFEF;
			check_interupt();
			return 0;
		break;
		case 0x14:
		case 0x18:
		case 0x1c:
		case 0x20:
		case 0x24:
		case 0x28:
		case 0x2c:
		case 0x30:
			*readpi[GET_LOW_ADDR(address)] = (unsigned long)data & 0xFF;
			return 0;
		break;
	}
	if(unlikely(GET_LOW_ADDR(address) >= sizeof(readpi))) {
		trash = (unsigned long)data;
	}
	else {
		*readpi[GET_LOW_ADDR(address)] = (unsigned long)data;
	}
	return 0;
}

unsigned long long int write_pib(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x8:
		case 0x9:
		case 0xa:
		case 0xb:
			*((unsigned char*)&pi_register.pi_rd_len_reg + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
			dma_pi_read();
			return 0;
		break;
		case 0xc:
		case 0xd:
		case 0xe:
		case 0xf:
			*((unsigned char*)&pi_register.pi_wr_len_reg + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
			dma_pi_write();
			return 0;
		break;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			if ((unsigned char)data) MI_register.mi_intr_reg &= 0xFFFFFFEF;
			check_interupt();
			return 0;
		break;
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x28:
		case 0x29:
		case 0x2a:
		case 0x2c:
		case 0x2d:
		case 0x2e:
		case 0x30:
		case 0x31:
		case 0x32:
			return 0;
		break;
	}
	if(unlikely(GET_LOW_ADDR(address) >= sizeof(readpi))) {
		trash = (unsigned char)data;
	}
	else {
		*((unsigned char*)readpi[GET_LOW_ADDR(address) & 0xfffc]	+ ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
	}
	return 0;
}

unsigned long long int write_pih(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x8:
		case 0xa:
			*((unsigned short*)((unsigned char*)&pi_register.pi_rd_len_reg + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
			dma_pi_read();
			return 0;
		break;
		case 0xc:
		case 0xe:
			*((unsigned short*)((unsigned char*)&pi_register.pi_wr_len_reg + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
			dma_pi_write();
			return 0;
		break;
		case 0x10:
		case 0x12:
			if ((unsigned short)data) MI_register.mi_intr_reg &= 0xFFFFFFEF;
			check_interupt();
			return 0;
		break;
		case 0x16:
		case 0x1a:
		case 0x1e:
		case 0x22:
		case 0x26:
		case 0x2a:
		case 0x2e:
		case 0x32:
			*((unsigned short*)((unsigned char*)readpi[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data & 0xFF;
			return 0;
		break;
		case 0x14:
		case 0x18:
		case 0x1c:
		case 0x20:
		case 0x24:
		case 0x28:
		case 0x2c:
		case 0x30:
			return 0;
		break;
	}
	if(unlikely(GET_LOW_ADDR(address) >= sizeof(readpi))) {
		trash = (unsigned short)data;
	}
	else {
		*((unsigned short*)((unsigned char*)readpi[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
	}
	return 0;
}

unsigned long long int write_pid(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x8:
			pi_register.pi_rd_len_reg = data >> 32;
			dma_pi_read();
			pi_register.pi_wr_len_reg = data & 0xFFFFFFFF;
			dma_pi_write();
			return 0;
		break;
		case 0x10:
			if (data) MI_register.mi_intr_reg &= 0xFFFFFFEF;
			check_interupt();
			*readpi[GET_LOW_ADDR(address)+4] = data & 0xFF;
			return 0;
		break;
		case 0x18:
		case 0x20:
		case 0x28:
		case 0x30:
			*readpi[GET_LOW_ADDR(address)] = (data >> 32) & 0xFF;
			*readpi[GET_LOW_ADDR(address)+4] = data & 0xFF;
			return 0;
		break;
	}
	if(unlikely(GET_LOW_ADDR(address) > sizeof(readpi) - sizeof(long long int))) {
		trash = data & 0xFFFFFFFF;
	}
	else {
		*readpi[GET_LOW_ADDR(address)] = data >> 32;
		*readpi[GET_LOW_ADDR(address)+4] = data & 0xFFFFFFFF;
	}
	return 0;
}

unsigned long long int read_ri(unsigned long address, unsigned long long int data)
{
	return *(readri[GET_LOW_ADDR(address)]);
}

unsigned long long int read_rib(unsigned long address, unsigned long long int data)
{
	return *((unsigned char*)readri[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) );
}

unsigned long long int read_rih(unsigned long address, unsigned long long int data)
{
	return *((unsigned short*)((unsigned char*)readri[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) ));
}

unsigned long long int read_rid(unsigned long address, unsigned long long int data)
{
	return ((unsigned long long int)(*readri[GET_LOW_ADDR(address)])<<32) | *readri[GET_LOW_ADDR(address)+4];
}

unsigned long long int write_ri(unsigned long address, unsigned long long int data)
{
	*readri[GET_LOW_ADDR(address)] = (unsigned long)data;
	return 0;
}

unsigned long long int write_rib(unsigned long address, unsigned long long int data)
{
	*((unsigned char*)readri[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
	return 0;
}

unsigned long long int write_rih(unsigned long address, unsigned long long int data)
{
	*((unsigned short*)((unsigned char*)readri[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
	return 0;
}

unsigned long long int write_rid(unsigned long address, unsigned long long int data)
{
	*readri[GET_LOW_ADDR(address)] = data >> 32;
	*readri[GET_LOW_ADDR(address)+4] = data & 0xFFFFFFFF;
	return 0;
}

unsigned long long int read_si(unsigned long address, unsigned long long int data)
{
	return *(readsi[GET_LOW_ADDR(address)]);
}

unsigned long long int read_sib(unsigned long address, unsigned long long int data)
{
	return *((unsigned char*)readsi[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S8) );
}

unsigned long long int read_sih(unsigned long address, unsigned long long int data)
{
	return *((unsigned short*)((unsigned char*)readsi[GET_LOW_ADDR(address) & 0xfffc] + ((GET_LOW_ADDR(address)&3)^S16) ));
}

unsigned long long int read_sid(unsigned long address, unsigned long long int data)
{
	return ((unsigned long long int)(*readsi[GET_LOW_ADDR(address)])<<32) | *readsi[GET_LOW_ADDR(address)+4];
}

unsigned long long int write_si(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
			si_register.si_dram_addr = (unsigned long)data;
		break;
		case 0x4:
			si_register.si_pif_addr_rd64b = (unsigned long)data;
			dma_si_read();
		break;
		case 0x10:
			si_register.si_pif_addr_wr64b = (unsigned long)data;
			dma_si_write();
		break;
		case 0x18:
			MI_register.mi_intr_reg &= 0xFFFFFFFD;
			si_register.si_status &= ~0x1000;
			check_interupt();
		break;
	}
	return 0;
}

unsigned long long int write_sib(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
			*((unsigned char*)&si_register.si_dram_addr + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
		break;
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
			*((unsigned char*)&si_register.si_pif_addr_rd64b + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
			dma_si_read();
		break;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			*((unsigned char*)&si_register.si_pif_addr_wr64b + ((GET_LOW_ADDR(address)&3)^S8) ) = (unsigned char)data;
			dma_si_write();
		break;
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
			MI_register.mi_intr_reg &= 0xFFFFFFFD;
			si_register.si_status &= ~0x1000;
			check_interupt();
		break;
	}
	return 0;
}

unsigned long long int write_sih(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
		case 0x2:
			*((unsigned short*)((unsigned char*)&si_register.si_dram_addr + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
		break;
		case 0x4:
		case 0x6:
			*((unsigned short*)((unsigned char*)&si_register.si_pif_addr_rd64b + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
			dma_si_read();
		break;
		case 0x10:
		case 0x12:
			*((unsigned short*)((unsigned char*)&si_register.si_pif_addr_wr64b + ((GET_LOW_ADDR(address)&3)^S16) )) = (unsigned short)data;
			dma_si_write();
		break;
		case 0x18:
		case 0x1a:
			MI_register.mi_intr_reg &= 0xFFFFFFFD;
			si_register.si_status &= ~0x1000;
			check_interupt();
		break;
	}
	return 0;
}

unsigned long long int write_sid(unsigned long address, unsigned long long int data)
{
	switch(GET_LOW_ADDR(address))
	{
		case 0x0:
			si_register.si_dram_addr = data >> 32;
			si_register.si_pif_addr_rd64b = data & 0xFFFFFFFF;
			dma_si_read();
		break;
		case 0x10:
			si_register.si_pif_addr_wr64b = data >> 32;
			dma_si_write();
		break;
		case 0x18:
			MI_register.mi_intr_reg &= 0xFFFFFFFD;
			si_register.si_status &= ~0x1000;
			check_interupt();
		break;
	}
	return 0;
}

unsigned long long int read_flashram_status(unsigned long address, unsigned long long int data)
{
	if (likely(flashRAMInfo.use_flashram != -1 && GET_LOW_ADDR(address) == 0))
	{
		flashRAMInfo.use_flashram = 1;
		return flashram_status();
	}
	else
		printf("unknown read in read_flashram_status\n");
	return 0;
}

unsigned long long int read_flashram_statusb(unsigned long address, unsigned long long int data)
{
   printf("read_flashram_statusb\n");
   return 0;
}

unsigned long long int read_flashram_statush(unsigned long address, unsigned long long int data)
{
   printf("read_flashram_statush\n");
   return 0;
}

unsigned long long int read_flashram_statusd(unsigned long address, unsigned long long int data)
{
   printf("read_flashram_statusd\n");
   return 0;
}

unsigned long long int write_flashram_dummy(unsigned long address, unsigned long long int data)
{
	return 0;
}

unsigned long long int write_flashram_dummyb(unsigned long address, unsigned long long int data)
{
	return 0;
}

unsigned long long int write_flashram_dummyh(unsigned long address, unsigned long long int data)
{
	return 0;
}

unsigned long long int write_flashram_dummyd(unsigned long address, unsigned long long int data)
{
	return 0;
}

unsigned long long int write_flashram_command(unsigned long address, unsigned long long int data)
{
	if (likely(flashRAMInfo.use_flashram != -1 && GET_LOW_ADDR(address) == 0))
	{
		flashram_command((unsigned long)data);
		flashRAMInfo.use_flashram = 1;
	}
	else
		printf("unknown write in write_flashram_command\n");
	return 0;
}

unsigned long long int write_flashram_commandb(unsigned long address, unsigned long long int data)
{
	printf("write_flashram_commandb\n");
	return 0;
}

unsigned long long int write_flashram_commandh(unsigned long address, unsigned long long int data)
{
	printf("write_flashram_commandh\n");
	return 0;
}

unsigned long long int write_flashram_commandd(unsigned long address, unsigned long long int data)
{
	printf("write_flashram_commandd\n");
	return 0;
}

static unsigned long lastwrite = 0;

unsigned long long int read_rom(unsigned long address, unsigned long long int data)
{
	if (unlikely(lastwrite))
	{
		unsigned long tmp = lastwrite;
		lastwrite = 0;
		return tmp;
	}
	else {
		unsigned long long int ret;
		ROMCache_read((u8*)&ret+4, (address & 0x03FFFFFF), 4);
		sign_extended(ret);
		return ret;
	}
}

unsigned long long int read_romb(unsigned long address, unsigned long long int data)
{
	unsigned long long int ret;
	ROMCache_read((u8*)&ret+7, (address & 0x03FFFFFF), 1);
	sign_extendedb(ret);
	return ret;
}

unsigned long long int read_romh(unsigned long address, unsigned long long int data)
{
	unsigned long long int ret;
	ROMCache_read((u8*)&ret+6, (address & 0x03FFFFFF), 2);
	sign_extendedh(ret);
	return ret;
}

unsigned long long int read_romd(unsigned long address, unsigned long long int data)
{
	unsigned long long int ret;
	ROMCache_read((u8*)&ret, (address & 0x03FFFFFF), 8);
	return ret;
}

unsigned long long int write_rom(unsigned long address, unsigned long long int data)
{
	lastwrite = (unsigned long)data;
	return 0;
}

unsigned long long int read_pif(unsigned long address, unsigned long long int data)
{
	return sl(*((unsigned long *)(PIF_RAMb + (address & 0x7FF) - 0x7C0)));
}

unsigned long long int read_pifb(unsigned long address, unsigned long long int data)
{
	return *(PIF_RAMb + ((address & 0x7FF) - 0x7C0));
}

unsigned long long int read_pifh(unsigned long address, unsigned long long int data)
{
	return (*(PIF_RAMb + ((address & 0x7FF) - 0x7C0)) << 8) | *(PIF_RAMb + (((address+1) & 0x7FF) - 0x7C0));
}

unsigned long long int read_pifd(unsigned long address, unsigned long long int data)
{
	return ((unsigned long long)sl(*((unsigned long *)(PIF_RAMb + (address & 0x7FF) - 0x7C0))) << 32)|
		sl(*((unsigned long *)(PIF_RAMb + ((address+4) & 0x7FF) - 0x7C0)));
}

unsigned long long int write_pif(unsigned long address, unsigned long long int data)
{
	*((unsigned long *)(PIF_RAMb + (address & 0x7FF) - 0x7C0)) = sl(((unsigned long)data));
	if (likely((address & 0x7FF) == 0x7FC))
	{
		if (PIF_RAMb[0x3F] == 0x08)
		{
			PIF_RAMb[0x3F] = 0;
			update_count();
			add_interupt_event(SI_INT, /*0x100*/0x900);
		}
		else
			update_pif_write();
	}
	return 0;
}

unsigned long long int write_pifb(unsigned long address, unsigned long long int data)
{
	*(PIF_RAMb + (address & 0x7FF) - 0x7C0) = (unsigned char)data;
	if (likely((address & 0x7FF) == 0x7FF))
	{
		if (PIF_RAMb[0x3F] == 0x08)
		{
			PIF_RAMb[0x3F] = 0;
			update_count();
			add_interupt_event(SI_INT, /*0x100*/0x900);
		}
		else
			update_pif_write();
	}
	return 0;
}

unsigned long long int write_pifh(unsigned long address, unsigned long long int data)
{
	*(PIF_RAMb + (address & 0x7FF) - 0x7C0) = ((unsigned short)data) >> 8;
	*(PIF_RAMb + ((address+1) & 0x7FF) - 0x7C0) = ((unsigned char)data) & 0xFF;
	if (likely((address & 0x7FF) == 0x7FE))
	{
		if (PIF_RAMb[0x3F] == 0x08)
		{
			PIF_RAMb[0x3F] = 0;
			update_count();
			add_interupt_event(SI_INT, /*0x100*/0x900);
		}
		else
			update_pif_write();
	}
	return 0;
}

unsigned long long int write_pifd(unsigned long address, unsigned long long int data)
{
	*((unsigned long *)(PIF_RAMb + (address & 0x7FF) - 0x7C0)) = sl((unsigned long)(data >> 32));
	*((unsigned long *)(PIF_RAMb + (address & 0x7FF) - 0x7C0)) = sl((unsigned long)(data & 0xFFFFFFFF));
	if (likely((address & 0x7FF) == 0x7F8))
	{
		if (PIF_RAMb[0x3F] == 0x08)
		{
			PIF_RAMb[0x3F] = 0;
			update_count();
			add_interupt_event(SI_INT, /*0x100*/0x900);
		}
		else
			update_pif_write();
	}
	return 0;
}
