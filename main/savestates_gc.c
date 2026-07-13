/**
 * Mupen64 - savestates.c
 * Copyright (C) 2002 Hacktarux
 * Copyright (C) 2008, 2009, 2013 emu_kidid
 * Copyright (C) 2013 sepp256
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

#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gccore.h>
#include <sys/stat.h>
#include "savestates.h"
#include "guifuncs.h"
#include "rom.h"
#include "../gc_memory/memory.h"
#include "../gc_memory/flashram.h"
#include "../r4300/macros.h"
#include "../r4300/r4300.h"
#include "../r4300/interupt.h"
#include "../gc_memory/TLB-Cache.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../gui/DEBUG.h"
#include "wii64config.h"

#ifdef USE_EXPANSION
#define RDRAM_SIZE (0x800000)
#else
#define RDRAM_SIZE (0x400000)
#endif
#define SAVE_CHUNK_SIZE (32768)
#define LOAD_CHUNK_SIZE (32768)

char* statesmagic = "W64";
char* statespath = "/wii64/saves/";
void LoadingBar_showBar(float percent, const char* string);
#define SAVE_STATE_MSG "Saving State .."
#define LOAD_STATE_MSG "Loading State .."
#define STATE_VERSION 3

static unsigned int savestates_version = STATE_VERSION;
static unsigned int savestates_slot = 0;
static int loadstate_queued = 0;
static int loadstate_slot = 0;

void savestates_select_slot(unsigned int s)
{
	if (s > 9) return;
	savestates_slot = s;
}

char *savestates_get_filename() {
	char *state_filename = calloc(1, 1024);
#ifdef HW_RVL
	sprintf(state_filename, "%s%s%s%s.st%d",(saveStateDevice==SAVESTATEDEVICE_USB)?"usb:":"sd:",
			statespath, ROM_SETTINGS.goodname, saveregionstr(),savestates_slot);
#else
	sprintf(state_filename, "sd:%s%s%s.st%d", statespath, ROM_SETTINGS.goodname, saveregionstr(),savestates_slot);
#endif
	return state_filename;
}

//#define ZLIB_STATES 1
#ifdef ZLIB_STATES
#define stateFile            		gzFile
#define stateOpen(filename, mode)   gzopen((filename), (mode))
#define stateWrite(f, src, size)    gzwrite((f), (src), (size))
#define stateRead(f, dst, size)     gzread((f), (dst), (size))
#define stateSeek(f, size)     		gzseek((f), (size), SEEK_CUR)
#define stateClose(f)               gzclose((f))
#else
#define stateFile            		FILE*
#define stateOpen(filename, mode)   fopen((filename), (mode))
#define stateWrite(f, src, size)    fwrite((src), 1, (size), (f))
#define stateRead(f, dst, size)     fread((dst), 1, (size), (f))
#define stateSeek(f, size)     		fseek((f), (size), SEEK_CUR)
#define stateClose(f)               fclose((f))
#endif


// Checks if the save state contains our magic and is of the right version number
int savestates_check_valid(stateFile f) {
	char statesmagic_read[3];
	unsigned int savestates_version_read;
	//Load Header
	stateRead(f, statesmagic_read, 3); //Read magic "W64"
	stateRead(f, &savestates_version_read, sizeof(unsigned int));
	if (!((strncmp(statesmagic, statesmagic_read, 3)==0)&&(savestates_version==savestates_version_read)))
	{
		stateClose(f);
		return 0;
	}
	return 1;
}

//returns 0 on file not existing
int savestates_exists(unsigned int slot, int mode)
{
	savestates_select_slot(slot);
	char *filename = savestates_get_filename();
	FILE *f = fopen(filename, (mode == SAVESTATE) ? "wb" : "rb");
   	free(filename);
	if(!f) {
		return 0;
	}
	fclose(f);
	return 1;
}

//queue a save state load to happen on next gen_interrupt()
void savestates_queue_load(unsigned int slot) {
	loadstate_queued = 1;
	loadstate_slot = slot;
}

//actually kick off a save state load if we have one set (called from gen_interrupt())
int savestates_queued_load() {
	if(loadstate_queued) {
		savestates_load(loadstate_slot);
		loadstate_queued = 0;
		return 1;
	}
	return 0;
}

// returns 0 on failure.
int savestates_save(unsigned int slot, u8* fb_tex)
{ 
	stateFile f;
	char buf[1024];
	int len, i;
	float progress = 0.0f;
	
	savestates_select_slot(slot);
	char *filename = savestates_get_filename();
	f = stateOpen(filename, "wb");
	
	if(!f) {
		free(filename);
		//print_gecko("Save state failed to save\r\n");
		return 0;
	}
	LoadingBar_showBar(progress, SAVE_STATE_MSG);
	
	//Save Header
	stateWrite(f, statesmagic, 3); //Write magic "W64"
	stateWrite(f, &savestates_version, sizeof(unsigned int));
#if !(defined(GC_BASIC))
	stateWrite(f, fb_tex, FB_THUMB_SIZE);
#endif

	//Save State
	// RDRAM 0->50%
	float slice = 0.50f / (RDRAM_SIZE / SAVE_CHUNK_SIZE);
	unsigned char* ramPtr = (unsigned char*)&rdram[0];
	for(i = 0; i < RDRAM_SIZE; i+= SAVE_CHUNK_SIZE) {
		stateWrite(f, ramPtr, SAVE_CHUNK_SIZE);
		ramPtr+=SAVE_CHUNK_SIZE;
		progress += slice;
		LoadingBar_showBar(progress, SAVE_STATE_MSG);
	}
	//print_gecko("adler of RDRAM %08X\r\n",adler32(0, (Bytef *)&rdram[0], RDRAM_SIZE));

	// TLB 50->90% (cache funcs are just wrapper macros for a non cache build)
	slice = 0.40f / ((0x100000*2) / SAVE_CHUNK_SIZE);
	
	int j = 0;
	for(i = 0; i < 0x100000/SAVE_CHUNK_SIZE; i++) {
		for(j = 0; j < SAVE_CHUNK_SIZE; j++) {
			u32 entry = TLBCache_get_r((i*SAVE_CHUNK_SIZE) + j);
			if(entry) {
				u32 addr = (i*SAVE_CHUNK_SIZE) + j;
				stateWrite(f, &addr, 4);
				stateWrite(f, &entry, 4);
			}
		}
		progress += slice;
		LoadingBar_showBar(progress, SAVE_STATE_MSG);
	}
	u32 endTag = 0x1ABC0FE1;
	stateWrite(f, &endTag, 4);
	stateWrite(f, &endTag, 4);
	
	for(i = 0; i < 0x100000/SAVE_CHUNK_SIZE; i++) {
		for(j = 0; j < SAVE_CHUNK_SIZE; j++) {
			u32 entry = TLBCache_get_w((i*SAVE_CHUNK_SIZE) + j);
			if(entry) {
				u32 addr = (i*SAVE_CHUNK_SIZE) + j;
				stateWrite(f, &addr, 4);
				stateWrite(f, &entry, 4);
			}
		}
		progress += slice;
		LoadingBar_showBar(progress, SAVE_STATE_MSG);
	}
	stateWrite(f, &endTag, 4);
	stateWrite(f, &endTag, 4);

	// Save registers (90->95%)
	stateWrite(f, &rdram_register, sizeof(RDRAM_register));
	stateWrite(f, &MI_register, sizeof(mips_register));
	stateWrite(f, &pi_register, sizeof(PI_register));
	stateWrite(f, &sp_register, sizeof(SP_register));
	stateWrite(f, &rsp_register, sizeof(RSP_register));
	stateWrite(f, &si_register, sizeof(SI_register));
	stateWrite(f, &vi_register, sizeof(VI_register));
	stateWrite(f, &ri_register, sizeof(RI_register));
	stateWrite(f, &ai_register, sizeof(AI_register));
	stateWrite(f, &dpc_register, sizeof(DPC_register));
	stateWrite(f, &dps_register, sizeof(DPS_register));
	progress += 0.05f;
	LoadingBar_showBar(progress, SAVE_STATE_MSG);
	
	// RSP, r4300 struct and interrupt queue (95%->100%)
	stateWrite(f, SP_DMEM, 0x1000);
	stateWrite(f, SP_IMEM, 0x1000);
	stateWrite(f, PIF_RAM, 0x40);
	stateWrite(f, &flashRAMInfo, sizeof(_FlashRAMInfo));
	stateWrite(f, &r4300, sizeof(r4300));
	stateWrite(f, r4300.fpr_data, 32*8);
	
	len = save_eventqueue_infos(buf);
	stateWrite(f, buf, len);
	stateClose(f);
	LoadingBar_showBar(1.0f, SAVE_STATE_MSG);
	free(filename);
	return 1;
}

//load a save state header (texture + datetime)
int savestates_load_header(unsigned int slot, u8* fb_tex, char* date, char* time)
{
	stateFile f = NULL;
	struct stat attrib;

	savestates_select_slot(slot);
	char *filename = savestates_get_filename();

	//get modified time from file attribute
	stat(filename, &attrib);
	if(date)
		strftime(date, 9, "%D", localtime(&(attrib.st_mtime)));//Write date string in MM/DD/YY format
	if(time)
		strftime(time, 9, "%R", localtime(&(attrib.st_mtime)));//Write time string in HH:MM format
	
	f = stateOpen(filename, "rb");
	
	if (!f || !savestates_check_valid(f)) {
		free(filename);
		return -1;
	}
#if !(defined(GC_BASIC))
	stateRead(f, fb_tex, FB_THUMB_SIZE);
#endif
	stateClose(f);
	free(filename);
	return 0;
}

int savestates_load(unsigned int slot)
{
	stateFile f = NULL;
	char buf[1024];
	int len, i;
	float progress = 0.0f;
	
	savestates_select_slot(slot);
	char *filename = savestates_get_filename();
	f = stateOpen(filename, "rb");
	
	if (!f || !savestates_check_valid(f)) {
		free(filename);
		return -1;
	}
	LoadingBar_showBar(progress, LOAD_STATE_MSG);
	
#if !(defined(GC_BASIC))
	//Skip image
	stateSeek(f, FB_THUMB_SIZE);
#endif

	//Load State
	// RDRAM 0->50%
	float slice = 0.50f / (RDRAM_SIZE / LOAD_CHUNK_SIZE);
	unsigned char* ramPtr = (unsigned char*)&rdram[0];
	for(i = 0; i < RDRAM_SIZE; i+= LOAD_CHUNK_SIZE) {
		stateRead(f, ramPtr, LOAD_CHUNK_SIZE);
		ramPtr+=LOAD_CHUNK_SIZE;
		progress += slice;
		LoadingBar_showBar(progress, LOAD_STATE_MSG);
	}
	//print_gecko("adler of RDRAM %08X\r\n",adler32(0, (Bytef *)&rdram[0], RDRAM_SIZE));
	
	// TLB 50->90% (cache funcs are just wrapper macros for a non cache build)
	slice = 0.40f / ((0x100000*2) / LOAD_CHUNK_SIZE);

#ifdef TINY_TLBCACHE
	TLBCache_reset();
#endif
#ifdef HW_RVL
	tlb_mem2_init();
#endif
	
	u32 entry[2] = {0, 0};
	while(entry[0] != 0x1ABC0FE1) {
		stateRead(f, &entry, 8);
		if(entry[0] == 0x1ABC0FE1) { break; }
		TLBCache_set_r(entry[0], entry[1]);
	}
	progress = 0.70f;
	LoadingBar_showBar(progress, LOAD_STATE_MSG);
	entry[0] = 0;
	while(entry[0] != 0x1ABC0FE1) {
		stateRead(f, &entry, 8);
		if(entry[0] == 0x1ABC0FE1) { break; }
		TLBCache_set_w(entry[0], entry[1]);
	}
	progress = 0.90f;
	LoadingBar_showBar(progress, LOAD_STATE_MSG);

	// Load registers (90->95%)
	stateRead(f, &rdram_register, sizeof(RDRAM_register));
	stateRead(f, &MI_register, sizeof(mips_register));
	stateRead(f, &pi_register, sizeof(PI_register));
	stateRead(f, &sp_register, sizeof(SP_register));
	stateRead(f, &rsp_register, sizeof(RSP_register));
	stateRead(f, &si_register, sizeof(SI_register));
	stateRead(f, &vi_register, sizeof(VI_register));
	stateRead(f, &ri_register, sizeof(RI_register));
	stateRead(f, &ai_register, sizeof(AI_register));
	stateRead(f, &dpc_register, sizeof(DPC_register));
	stateRead(f, &dps_register, sizeof(DPS_register));
	progress += 0.05f;
	LoadingBar_showBar(progress, LOAD_STATE_MSG);

	// RSP, r4300 struct and interrupt queue (95%->100%)
	stateRead(f, SP_DMEM, 0x1000);
	stateRead(f, SP_IMEM, 0x1000);
	stateRead(f, PIF_RAM, 0x40);
	stateRead(f, &flashRAMInfo, sizeof(_FlashRAMInfo));
	stateRead(f, &r4300, sizeof(r4300));
	set_fpr_pointers(Status);  // Status is r4300.reg_cop0[12]
	stateRead(f, r4300.fpr_data, 32*8);

	len = 0;
	while(1)
	{
		stateRead(f, buf+len, 4);
		if (*((unsigned long*)&buf[len]) == 0xFFFFFFFF) break;
		stateRead(f, buf+len+4, 4);
		len += 8;
	}
	load_eventqueue_infos(buf);
	stateClose(f);

	r4300.stop = 0;
	LoadingBar_showBar(1.0f, LOAD_STATE_MSG);
	free(filename);
	return 0; //success!
}
