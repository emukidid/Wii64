/**
 * Mupen64 - flashRAMInfo.c
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
#include <stdlib.h>

#ifdef USE_GUI
#include "../gui/GUI.h"
#define PRINT GUI_print
#else
#define PRINT printf
#endif

#include "memory.h"
#include "../r4300/r4300.h"
#include "../main/guifuncs.h"
#include "../fileBrowser/fileBrowser.h"
#include "flashram.h"
#include "Saves.h"

#ifdef HW_RVL
#include "MEM2.h"
#else
#include "ARAM.h"
#endif
static unsigned char *const flashram = (unsigned char*)(FLASHRAM_LO);

BOOL flashramWritten = FALSE;
_FlashRAMInfo flashRAMInfo;

int loadFlashram(fileBrowser_file* savepath){
	int i, result = 0;
	fileBrowser_file saveFile;
	memcpy(&saveFile, savepath, sizeof(fileBrowser_file));
	memset(&saveFile.name[0],0,FILE_BROWSER_MAX_PATH_LEN);
	sprintf((char*)saveFile.name,"%s/%s%s.fla",savepath->name,ROM_SETTINGS.goodname,saveregionstr());

	if(saveFile_readFile(&saveFile, &i, 4) == 4) {  //file exists
		saveFile.offset = 0;
		if(saveFile_readFile(&saveFile, flashram, 0x20000)!=0x20000) {  //error reading file
  		for (i=0; i<0x20000; i++) flashram[i] = 0xff;
  		flashramWritten = FALSE;
  		return -1;
		}
		result = 1;
		flashramWritten = 1;
		return result;  //file read ok
	} else for (i=0; i<0x20000; i++) flashram[i] = 0xff;  //file doesn't exist

	flashramWritten = FALSE;

	return result;    //no file
}

int saveFlashram(fileBrowser_file* savepath){
	if(!flashramWritten) return 0;
	fileBrowser_file saveFile;
	memcpy(&saveFile, savepath, sizeof(fileBrowser_file));
	memset(&saveFile.name[0],0,FILE_BROWSER_MAX_PATH_LEN);
	sprintf((char*)saveFile.name,"%s/%s%s.fla",savepath->name,ROM_SETTINGS.goodname,saveregionstr());

	if(saveFile_writeFile(&saveFile, flashram, 0x20000)!=0x20000)
		return -1;

	return 1;
}

int deleteFlashram(fileBrowser_file* savepath){
	fileBrowser_file saveFile;
	memcpy(&saveFile, savepath, sizeof(fileBrowser_file));
	memset(&saveFile.name[0],0,FILE_BROWSER_MAX_PATH_LEN);
	sprintf((char*)saveFile.name,"%s/%s%s.fla",savepath->name,ROM_SETTINGS.goodname,saveregionstr());

	return saveFile_deleteFile(&saveFile);
}

void init_flashram()
{
	flashRAMInfo.mode = NOPES_MODE;
	flashRAMInfo.status = 0;
}

void reset_flashram() {
	memset(&flashram[0],0xFF,0x20000);
}

unsigned long flashram_status()
{
	return (flashRAMInfo.status >> 32);
}

void flashram_command(unsigned long command)
{
	switch(command & 0xff000000)
	{
		case 0x4b000000:
			flashRAMInfo.erase_offset = (command & 0xffff) * 128;
		break;
		case 0x78000000:
			flashRAMInfo.mode = ERASE_MODE;
			flashRAMInfo.status = 0x1111800800c20000LL;
		break;
		case 0xa5000000:
			flashRAMInfo.erase_offset = (command & 0xffff) * 128;
			flashRAMInfo.status = 0x1111800400c20000LL;
		break;
		case 0xb4000000:
			flashRAMInfo.mode = WRITE_MODE;
		break;
		case 0xd2000000:  // execute
			switch (flashRAMInfo.mode)
			{
				case NOPES_MODE:
				break;
				case ERASE_MODE:	// Erase a 128 byte page with 0xFF
				{
					int i;
					flashramWritten = TRUE;
					for (i=flashRAMInfo.erase_offset; i<(flashRAMInfo.erase_offset+128); i++)
						flashram[i^S8] = 0xff;
				}
				break;
				case WRITE_MODE:	// Write 128 byte page from RDRAM to the freshly erased page
				{
					int i;
					flashramWritten = TRUE;
					for (i=0; i<128; i++)
						flashram[(flashRAMInfo.erase_offset+i)^S8] = ((unsigned char*)rdram)[(flashRAMInfo.write_pointer+i)^S8];
				}
				break;
				case READ_MODE:
				case STATUS_MODE:
				break;
				default:
					//printf("unknown flashram command with flashRAMInfo.mode:%x\n", (int)flashRAMInfo.mode);
					r4300.stop=1;
				break;
			}
			flashRAMInfo.mode = NOPES_MODE;
		break;
		case 0xe1000000:
			flashRAMInfo.mode = STATUS_MODE;
			flashRAMInfo.status = 0x1111800100c20000LL;
		break;
		case 0xf0000000:
			flashRAMInfo.mode = READ_MODE;
			flashRAMInfo.status = 0x11118004f0000000LL;
		break;
		default:
			//printf("unknown flashram command:%x\n", (int)command);
			r4300.stop=1;
		break;
	}
}

void dma_read_flashram()
{
	int i;

	switch(flashRAMInfo.mode)
	{
		case STATUS_MODE:
			rdram[pi_register.pi_dram_addr_reg/4] = (unsigned long)(flashRAMInfo.status >> 32);
			rdram[pi_register.pi_dram_addr_reg/4+1] = (unsigned long)(flashRAMInfo.status);
		break;
		case READ_MODE:
			for (i=0; i<(pi_register.pi_wr_len_reg & 0x0FFFFFE)+2; i++)
				((unsigned char*)rdram)[(pi_register.pi_dram_addr_reg+i)^S8] =
				flashram[(((pi_register.pi_cart_addr_reg-0x08000000)&0xFFFE)*2+i)^S8];
		break;
		default:
			//printf("unknown dma_read_flashram:%x\n", flashRAMInfo.mode);
			r4300.stop=1;
		break;
	}
}

void dma_write_flashram()
{
	switch(flashRAMInfo.mode)
	{
		case WRITE_MODE:
			flashRAMInfo.write_pointer = pi_register.pi_dram_addr_reg;
		break;
		default:
			//printf("unknown dma_read_flashram:%x\n", flashRAMInfo.mode);
			r4300.stop=1;
		break;
	}
}
