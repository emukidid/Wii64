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
#include "../r4300/ppc/Wrappers.h"
#include "../fileBrowser/fileBrowser.h"
#include "pif.h"
#include "flashram.h"
#include "Saves.h"

#ifdef USE_EXPANSION
	#define MEM_SIZE (0x800000)
#else
	#define MEM_SIZE (0x400000)
#endif
#define MEMMASK		(MEM_SIZE-1)

#ifdef HW_RVL
#include "MEM2.h"
#else
#include "ARAM.h"
#endif
static unsigned char *const sram = (unsigned char*)(SRAM_LO);

bool sramWritten = false;

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
			sramWritten = false;
			return -1;
		}
		result = 1;
		sramWritten = 1;
		return result;  //file read ok
	} else for (i=0; i<0x8000; i++) sram[i] = 0;  //file doesn't exist

	sramWritten = false;
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

#define PI_ADVANCE_ADDR(len) do { \
	pi_register.pi_dram_addr_reg = (pi_register.pi_dram_addr_reg + (len) + 7) & ~7UL; \
	pi_register.pi_cart_addr_reg = (pi_register.pi_cart_addr_reg + (len) + 1) & ~1UL; \
} while (0)

void dma_pi_read()
{
	unsigned long dma_length;
	unsigned long req_length;
	int i;
	
	if (pi_register.pi_cart_addr_reg < 0x10000000)
	{
		if (pi_register.pi_cart_addr_reg >= 0x08000000 && pi_register.pi_cart_addr_reg < 0x08010000)
		{
			if (flashRAMInfo.use_flashram != 1)
			{
				sramWritten = true;
				memcpy(	&sram[((pi_register.pi_cart_addr_reg-0x08000000)&0xFFFE)^S8],
						&rdramb[(pi_register.pi_dram_addr_reg)^S8],
						(pi_register.pi_rd_len_reg & 0xFFFFFE)+2);
				flashRAMInfo.use_flashram = -1;
			}
			else
				dma_write_flashram();
		}
		
		pi_register.read_pi_status_reg |= 1;
		update_count();
		add_interupt_event(PI_INT, 0x1000/*pi_register.pi_rd_len_reg*/);
		
		return;
	}
	
	if (pi_register.pi_cart_addr_reg >= 0x1fc00000) // for paper mario
	{
		pi_register.read_pi_status_reg |= 1;
		update_count();
		add_interupt_event(PI_INT, 0x1000);
		return;
	}
	
	// Cart DMA: Don't DMA past the end of the ROM nor past the end of MEM
	// Not that it matters, but actual N64 hardware will repeat pi_dram_addr_reg>>16 
	// over the unmapped ROM region past the end of ROM, which we don't do.
	dma_length = (pi_register.pi_rd_len_reg & UINT32_C(0x00ffffff)) + 1;
	// The PI treats the first 128 bytes of a transfer differently
	if (dma_length >= 0x7f && (dma_length & 1)) dma_length += 1;
	i = (pi_register.pi_cart_addr_reg-0x10000000)&0x3FFFFFE;
	req_length = dma_length;
	dma_length = (i + dma_length) > rom_length ? (rom_length - i) : dma_length;
	dma_length = (pi_register.pi_dram_addr_reg + dma_length) > MEM_SIZE ?
				 (MEM_SIZE - pi_register.pi_dram_addr_reg) : dma_length;

	if(i>rom_length || pi_register.pi_dram_addr_reg > MEMMASK)
	{
		pi_register.read_pi_status_reg |= 3;
		PI_ADVANCE_ADDR(req_length);
		update_count();
		add_interupt_event(PI_INT, (dma_length/8) + add_random_interupt_time());
		return;
	}
	
	if(!interpcore)
	{
		for (i=0; i<dma_length; i+=4)
		{
			unsigned long rom_address1 = pi_register.pi_cart_addr_reg+i+0x80000000;
			unsigned long rom_address2 = pi_register.pi_cart_addr_reg+i+0xa0000000;

			invalidate_func(rom_address1);
			invalidate_func(rom_address2);
		}
	}

	pi_register.read_pi_status_reg |= 3;
	update_count();
	add_interupt_event(PI_INT, (dma_length/8) + add_random_interupt_time());
}

void dma_pi_write()
{
	unsigned long dma_length;
	unsigned long req_length;
	int i;

	// Non Cart region DMA
	if (pi_register.pi_cart_addr_reg < 0x10000000 || pi_register.pi_cart_addr_reg >= 0x1fc00000)
	{
		// SRAM or FlashRam Read DMA
		if (pi_register.pi_cart_addr_reg >= 0x08000000 && pi_register.pi_cart_addr_reg < 0x08010000)
		{
			if (flashRAMInfo.use_flashram != 1)
			{
				memcpy(	&rdramb[(pi_register.pi_dram_addr_reg)^S8],
						&sram[(((pi_register.pi_cart_addr_reg-0x08000000)&0xFFFE))^S8],
						(pi_register.pi_wr_len_reg & 0xFFFFFE)+2);
				flashRAMInfo.use_flashram = -1;
			}
			else
				dma_read_flashram();
		}
		// 64DD IPL region
		else if (pi_register.pi_cart_addr_reg >= 0x06000000 && pi_register.pi_cart_addr_reg < 0x08000000) 
		{
		}
		// Stops Paper Mario from reading beyond cart region
		else if (pi_register.pi_cart_addr_reg >= 0x1fc00000) 
		{
		}

		pi_register.read_pi_status_reg |= 1;
		update_count();
		add_interupt_event(PI_INT, /*pi_register.pi_wr_len_reg*/0x1000);
		return;
	}

	// Cart DMA: Don't DMA past the end of the ROM nor past the end of MEM
	// Not that it matters, but actual N64 hardware will repeat pi_dram_addr_reg>>16 
	// over the unmapped ROM region past the end of ROM, which we don't do.
	dma_length = (pi_register.pi_wr_len_reg & UINT32_C(0x00ffffff)) + 1;
	// Another "first 128 bytes" PI quirk
	if (dma_length >= 0x7f && (dma_length & 1)) dma_length += 1;
	if (dma_length <= 0x80)
		dma_length = dma_length >= (pi_register.pi_dram_addr_reg & 0x7) ?
					 dma_length - (pi_register.pi_dram_addr_reg & 0x7) : 0;
	i = (pi_register.pi_cart_addr_reg-0x10000000)&0x3FFFFFE;
	req_length = dma_length;
	dma_length = (i + dma_length) > rom_length ? (rom_length - i) : dma_length;
	dma_length = (pi_register.pi_dram_addr_reg + dma_length) > MEM_SIZE ?
				 (MEM_SIZE - pi_register.pi_dram_addr_reg) : dma_length;

	if(i>rom_length || pi_register.pi_dram_addr_reg > MEMMASK)
	{
		pi_register.read_pi_status_reg |= 3;
		PI_ADVANCE_ADDR(req_length);
		update_count();
		add_interupt_event(PI_INT, (dma_length/8) + add_random_interupt_time());
		return;
	}

	/* DMA transfer
		from cart address: ((pi_register.pi_cart_addr_reg-0x10000000)&0x3FFFFFF)^S8
		to RDRAM address: ((unsigned int)(pi_register.pi_dram_addr_reg)^S8)
		of length:	length
	*/
	ROMCache_read(((unsigned char*)rdram + ((unsigned int)(pi_register.pi_dram_addr_reg)^S8)), 
				  i, dma_length);
	
	// Dynarec, invalidate any code we wrote over.
	if(!interpcore)
	{
		for (i=0; i<dma_length; i+=4)
		{
			unsigned long rdram_address1 = pi_register.pi_dram_addr_reg+i+0x80000000;
			unsigned long rdram_address2 = pi_register.pi_dram_addr_reg+i+0xa0000000;
			invalidate_func(rdram_address1);
			invalidate_func(rdram_address2);
		}
	}

    // Set the RDRAM memory size when copying main ROM code
    // (This is just a convenient way to run this code once at the beginning)
	if (pi_register.pi_cart_addr_reg == 0x10001000)
	{
		if(r4300.cic_chip == 5) {
			rdram[0x3F0/4] = MEM_SIZE;
		}
		else {
			rdram[0x318/4] = MEM_SIZE;
		}
	}

	pi_register.read_pi_status_reg |= 3;
	PI_ADVANCE_ADDR(req_length);
	update_count();
	add_interupt_event(PI_INT, (dma_length/8) + add_random_interupt_time());
	return;
}

void dma_sp_write()
{
	unsigned char *spMemType = (unsigned char*)SP_DMEM;
	unsigned long length, count, skip, memaddr, dramaddr, i, memoff, chunk;

	if ((sp_register.sp_mem_addr_reg & 0x1000) > 0)
	{
		spMemType = (unsigned char*)SP_IMEM;
	}

	length   = ((sp_register.sp_rd_len_reg & 0xFFF) | 7) + 1;
	count    = ((sp_register.sp_rd_len_reg >> 12) & 0xFF) + 1;
	skip     = (sp_register.sp_rd_len_reg >> 20) & 0xFFF;
	memaddr  = sp_register.sp_mem_addr_reg & 0xFF8;
	dramaddr = sp_register.sp_dram_addr_reg & 0xFFFFF8;

	for (i = 0; i < count; i++)
	{
		memoff = memaddr & 0xFFF;
		chunk = (0x1000 - memoff < length) ? (0x1000 - memoff) : length;
		memcpy(&spMemType[memoff^S8], &rdramb[dramaddr^S8], chunk);
		if (chunk < length)
			memcpy(&spMemType[0^S8], &rdramb[(dramaddr+chunk)^S8], length-chunk);
		memaddr  += length;
		dramaddr += length + skip;
	}

	sp_register.sp_mem_addr_reg  = memaddr & 0xFFF;
	sp_register.sp_dram_addr_reg = dramaddr & 0xFFFFFF;
	sp_register.sp_rd_len_reg    = 0xFF8;
}

void dma_sp_read()
{
	unsigned char *spMemType = (unsigned char*)SP_DMEM;
	unsigned long length, count, skip, memaddr, dramaddr, i, memoff, chunk;

	if ((sp_register.sp_mem_addr_reg & 0x1000) > 0)
	{
		spMemType = (unsigned char*)SP_IMEM;
	}

	length   = ((sp_register.sp_wr_len_reg & 0xFFF) | 7) + 1;
	count    = ((sp_register.sp_wr_len_reg >> 12) & 0xFF) + 1;
	skip     = (sp_register.sp_wr_len_reg >> 20) & 0xFFF;
	memaddr  = sp_register.sp_mem_addr_reg & 0xFF8;
	dramaddr = sp_register.sp_dram_addr_reg & 0xFFFFF8;

	for (i = 0; i < count; i++)
	{
		memoff = memaddr & 0xFFF;
		chunk = (0x1000 - memoff < length) ? (0x1000 - memoff) : length;
		memcpy(&rdramb[dramaddr^S8], &spMemType[memoff^S8], chunk);
		if (chunk < length)
			memcpy(&rdramb[(dramaddr+chunk)^S8], &spMemType[0^S8], length-chunk);
		memaddr  += length;
		dramaddr += length + skip;
	}

	sp_register.sp_mem_addr_reg  = memaddr & 0xFFF;
	sp_register.sp_dram_addr_reg = dramaddr & 0xFFFFFF;
	sp_register.sp_wr_len_reg    = 0xFF8;
}

#define SI_NO_DMA    0
#define SI_DMA_READ  1
#define SI_DMA_WRITE 2
static int si_dma_dir = SI_NO_DMA;

static unsigned long si_dram_word_index(void)
{
	unsigned long a = si_register.si_dram_addr & MEMMASK & ~3UL;
	if (a > (unsigned long)(MEM_SIZE - 64))
		a = (unsigned long)(MEM_SIZE - 64);
	return a >> 2;
}

void si_reset_dma_dir(void)
{
	si_dma_dir = SI_NO_DMA;
}

void si_set_dma_dir_write(void)
{
	si_dma_dir = SI_DMA_WRITE;
}

void si_end_of_dma(void)
{
	int i;
	if (si_dma_dir == SI_DMA_WRITE)
	{
		update_pif_write();
	}
	else if (si_dma_dir == SI_DMA_READ)
	{
		for (i=0; i<(64/4); i++)
			rdram[si_dram_word_index()+i] = sl(PIF_RAM[i]);
	}
	si_dma_dir = SI_NO_DMA;
}

void dma_si_write()
{
	if (si_register.si_status & 0x1)
	{
		si_register.si_status |= 0x8;
		return;
	}
	int i;
	if (si_register.si_pif_addr_wr64b != 0x1FC007C0)
	{
		//	printf("unknown SI use\n");
		r4300.stop=1;
	}
	for (i=0; i<(64/4); i++)
		PIF_RAM[i] = sl(rdram[si_dram_word_index()+i]);
	si_dma_dir = SI_DMA_WRITE;
	si_register.si_status |= 0x1;
	update_count();
	add_interupt_event(SI_INT, 0x900 + add_random_interupt_time());
}

void dma_si_read()
{
	if (si_register.si_status & 0x1)
	{
		si_register.si_status |= 0x8;
		return;
	}
	if (si_register.si_pif_addr_rd64b != 0x1FC007C0)
	{
		//	printf("unknown SI use\n");
		r4300.stop=1;
	}
	update_pif_read();
	si_dma_dir = SI_DMA_READ;
	si_register.si_status |= 0x1;
	update_count();
	add_interupt_event(SI_INT, 0x900 + add_random_interupt_time());
}
