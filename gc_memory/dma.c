/**
 * Mupen64 - dma.c
 * Copyright (C) 2002 Hacktarux
 * Copyright (C) 2007, 2013 emu_kidid
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *                emukidid@gmail.com
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

#ifdef USE_GUI
#include "../gui/GUI.h"
//#define PRINT GUI_print
#else
//#define PRINT printf
#endif

#include <stdio.h>
#include <malloc.h>
#include "dma.h"
#include "memory.h"
#include "../main/rom.h"
#include "../main/ROM-Cache.h"
#include "../main/guifuncs.h"
#include "../r4300/r4300.h"
#include "../r4300/Recomp-Cache.h"
#include "../r4300/interupt.h"
#include "../r4300/macros.h"
#include "../r4300/Invalid_Code.h"
#include "../fileBrowser/fileBrowser.h"
#include "pif.h"
#include "flashram.h"
#include "Saves.h"

#ifdef USE_EXPANSION
	#define MEMMASK 0x7FFFFF
#else
	#define MEMMASK 0x3FFFFF
#endif

#ifdef HW_RVL
#include "MEM2.h"
#else
#include "ARAM.h"
#endif
static unsigned char *sram = (unsigned char*)(SRAM_LO);

BOOL sramWritten = FALSE;

int loadSram(fileBrowser_file* savepath)
{
	int i, result = 0;
	fileBrowser_file saveFile;
	memcpy(&saveFile, savepath, sizeof(fileBrowser_file));
	memset(&saveFile.name[0],0,FILE_BROWSER_MAX_PATH_LEN);
	sprintf((char*)saveFile.name,"%s/%s%s.sra",savepath->name,ROM_SETTINGS.goodname,saveregionstr());

	if(saveFile_readFile(&saveFile, &i, 4) == 4){ //file exists
		saveFile.offset = 0;
		if(saveFile_readFile(&saveFile, sram, 0x8000)!=0x8000) {  //error reading file
			for (i=0; i<0x8000; i++) sram[i] = 0;
			sramWritten = FALSE;
			return -1;
		}
		result = 1;
		sramWritten = 1;
		return result;  //file read ok
	} else for (i=0; i<0x8000; i++) sram[i] = 0;  //file doesn't exist

	sramWritten = FALSE;
	return result;    //no file
}

int saveSram(fileBrowser_file* savepath)
{
	if(!sramWritten) return 0;
	fileBrowser_file saveFile;
	memcpy(&saveFile, savepath, sizeof(fileBrowser_file));
	memset(&saveFile.name[0],0,FILE_BROWSER_MAX_PATH_LEN);
	sprintf((char*)saveFile.name,"%s/%s%s.sra",savepath->name,ROM_SETTINGS.goodname,saveregionstr());

	if(saveFile_writeFile(&saveFile, sram, 0x8000)!=0x8000)
		return -1;
	return 1;
}

int deleteSram(fileBrowser_file* savepath){
	fileBrowser_file saveFile;
	memcpy(&saveFile, savepath, sizeof(fileBrowser_file));
	memset(&saveFile.name[0],0,FILE_BROWSER_MAX_PATH_LEN);
	sprintf((char*)saveFile.name,"%s/%s%s.sra",savepath->name,ROM_SETTINGS.goodname,saveregionstr());

	return saveFile_deleteFile(&saveFile);
}

void dma_pi_read()
{
	if (pi_register.pi_cart_addr_reg >= 0x08000000 && pi_register.pi_cart_addr_reg < 0x08010000)
	{
		if (flashRAMInfo.use_flashram != 1)
		{
			sramWritten = TRUE;
			memcpy(	&sram[((pi_register.pi_cart_addr_reg-0x08000000))^S8],
					&rdramb[(pi_register.pi_dram_addr_reg)^S8],
					(pi_register.pi_rd_len_reg & 0xFFFFFF)+1);
			flashRAMInfo.use_flashram = -1;
	}
	else
		dma_write_flashram();
	}
 //  else
 //    printf("unknown dma read\n");

	pi_register.read_pi_status_reg |= 1;
	update_count();
	add_interupt_event(PI_INT, 0x1000/*pi_register.pi_rd_len_reg*/);
}

void dma_pi_write()
{
	unsigned long dma_length;
	int i;

	// SRAM or FlashRam Read DMA
	if (pi_register.pi_cart_addr_reg < 0x10000000)
	{
		if (pi_register.pi_cart_addr_reg >= 0x08000000 && pi_register.pi_cart_addr_reg < 0x08010000)
		{
			if (flashRAMInfo.use_flashram != 1)
			{
				memcpy(	&rdramb[(pi_register.pi_dram_addr_reg)^S8],
						&sram[(((pi_register.pi_cart_addr_reg-0x08000000)&0xFFFF))^S8],
						(pi_register.pi_wr_len_reg & 0xFFFFFF)+1);
				flashRAMInfo.use_flashram = -1;
			}
			else
				dma_read_flashram();
		}
		else if (pi_register.pi_cart_addr_reg >= 0x06000000 && pi_register.pi_cart_addr_reg < 0x08000000)
		{	// What lies here?
		}
//	else
//	  printf("unknown dma write:%x\n", (int)pi_register.pi_cart_addr_reg);

		pi_register.read_pi_status_reg |= 1;
		update_count();
		add_interupt_event(PI_INT, /*pi_register.pi_wr_len_reg*/0x1000);

		return;
	}

	if (pi_register.pi_cart_addr_reg >= 0x1fc00000) // for paper mario
	{
		pi_register.read_pi_status_reg |= 1;
		update_count();
		add_interupt_event(PI_INT, 0x1000);
		return;
	}

	// Don't DMA past the end of the ROM nor past the end of MEM
	dma_length = (pi_register.pi_wr_len_reg & 0xFFFFFF)+1;
	i = (pi_register.pi_cart_addr_reg-0x10000000)&0x3FFFFFF;
	dma_length = (i + dma_length) > rom_length ? (rom_length - i) : dma_length;
	dma_length = (pi_register.pi_dram_addr_reg + dma_length) > MEMMASK ?
				 (MEMMASK - pi_register.pi_dram_addr_reg) : dma_length;

	if(i>rom_length || pi_register.pi_dram_addr_reg > MEMMASK)
	{
		pi_register.read_pi_status_reg |= 3;
		update_count();
		add_interupt_event(PI_INT, dma_length/8);
		return;
	}

	/* DMA transfer 
		from cart address: ((pi_register.pi_cart_addr_reg-0x10000000)&0x3FFFFFF)^S8
		to RDRAM address: ((unsigned int)(pi_register.pi_dram_addr_reg)^S8)
		of length:	length
	*/
	ROMCache_read(((unsigned char*)rdram + ((unsigned int)(pi_register.pi_dram_addr_reg)^S8)), 
				  ((pi_register.pi_cart_addr_reg-0x10000000)&0x3FFFFFF)^S8, dma_length);
	if(!interpcore)
	{
		for (i=0; i<dma_length; i++)
		{
			unsigned long rdram_address1 = pi_register.pi_dram_addr_reg+i+0x80000000;
			unsigned long rdram_address2 = pi_register.pi_dram_addr_reg+i+0xa0000000;

			if(blocks[rdram_address1>>12]){
				PowerPC_func* func = find_func(&blocks[rdram_address1>>12]->funcs, rdram_address1);
				if(!invalid_code_get(rdram_address1>>12))
					if(func) RecompCache_Free(func->start_addr);
			}

			if(blocks[rdram_address2>>12]) {
				PowerPC_func* func = find_func(&blocks[rdram_address2>>12]->funcs, rdram_address2);
				if(!invalid_code_get(rdram_address2>>12))
					if(func) RecompCache_Free(func->start_addr);
			}
		}
	}

    // Set the RDRAM memory size when copying main ROM code
    // (This is just a convenient way to run this code once at the beginning)
	if (pi_register.pi_cart_addr_reg == 0x10001000)
	{
		switch(r4300.cic_chip)
		{
			case 1:
			case 2:
			case 3:
			case 6:
#ifdef USE_EXPANSION
				rdram[0x318/4] = 0x800000;
#else
				rdram[0x318/4] = 0x400000;
#endif
			break;
			case 5:
#ifdef USE_EXPANSION
				rdram[0x3F0/4] = 0x800000;
#else
				rdram[0x3F0/4] = 0x400000;
#endif
			break;
		}
	}

	pi_register.read_pi_status_reg |= 3;
	update_count();
	add_interupt_event(PI_INT, dma_length/8);
	return;
}

void dma_sp_write()
{
	unsigned char *spMemType = (unsigned char*)SP_DMEM;
	if ((sp_register.sp_mem_addr_reg & 0x1000) > 0)
	{
		spMemType = (unsigned char*)SP_IMEM;
	}
	memcpy(	&spMemType[((sp_register.sp_mem_addr_reg & 0xFFF))^S8],
			&rdramb[((sp_register.sp_dram_addr_reg & 0xFFFFFF))^S8],
			((sp_register.sp_rd_len_reg & 0xFFF)+1));
}

void dma_sp_read()
{
	unsigned char *spMemType = (unsigned char*)SP_DMEM;
	if ((sp_register.sp_mem_addr_reg & 0x1000) > 0)
	{
		spMemType = (unsigned char*)SP_IMEM;
	}
	memcpy(	&rdramb[((sp_register.sp_dram_addr_reg & 0xFFFFFF))^S8],
			&spMemType[((sp_register.sp_mem_addr_reg & 0xFFF))^S8],
			((sp_register.sp_wr_len_reg & 0xFFF)+1));
}

void dma_si_write()
{
	int i;
	if (si_register.si_pif_addr_wr64b != 0x1FC007C0)
	{
		//	printf("unknown SI use\n");
		r4300.stop=1;
	}
	for (i=0; i<(64/4); i++)
		PIF_RAM[i] = sl(rdram[si_register.si_dram_addr/4+i]);
	update_pif_write();
	update_count();
	add_interupt_event(SI_INT, /*0x100*/0x900);
}

void dma_si_read()
{
	int i;
	if (si_register.si_pif_addr_rd64b != 0x1FC007C0)
	{
		//	printf("unknown SI use\n");
		r4300.stop=1;
	}
	update_pif_read();
	for (i=0; i<(64/4); i++)
		rdram[si_register.si_dram_addr/4+i] = sl(PIF_RAM[i]);
	update_count();
	add_interupt_event(SI_INT, /*0x100*/0x900);
}
