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
#include "wii64config.h"

#ifdef USE_EXPANSION
#define RDRAM_SIZE (0x800000)
#else
#define RDRAM_SIZE (0x400000)
#endif
#define CHUNK_SIZE (0x10000)

char* statesmagic = "W64";
char* statespath = "/wii64/saves/";
void LoadingBar_showBar(float percent, const char* string);
#define SAVE_STATE_MSG "Saving State .."
#define LOAD_STATE_MSG "Loading State .."
#define STATE_VERSION 2

static unsigned int savestates_version = STATE_VERSION;
static unsigned int savestates_slot = 0;
static int loadstate_queued = 0;
static int loadstate_slot = 0;
static char state_filename[1024];

void savestates_select_slot(unsigned int s)
{
	if (s > 9) return;
	savestates_slot = s;
}

char *savestates_get_filename() {
	memset(&state_filename[0], 0, 1024);
#ifdef HW_RVL
	sprintf(state_filename, "%s%s%s%s.st%d",(saveStateDevice==SAVESTATEDEVICE_USB)?"usb:":"sd:",
			statespath, ROM_SETTINGS.goodname, saveregionstr(),savestates_slot);
#else
	sprintf(state_filename, "sd:%s%s%s.st%d", statespath, ROM_SETTINGS.goodname, saveregionstr(),savestates_slot);
#endif
	return &state_filename[0];
}

// Checks if the save state contains our magic and is of the right version number
int savestates_check_valid(gzFile f) {
	char statesmagic_read[3];
	unsigned int savestates_version_read;
	//Load Header
	gzread(f, statesmagic_read, 3); //Read magic "W64"
	gzread(f, &savestates_version_read, sizeof(unsigned int));
	if (!((strncmp(statesmagic, statesmagic_read, 3)==0)&&(savestates_version==savestates_version_read)))
	{
		gzclose(f);
		return 0;
	}
	return 1;
}

//returns 0 on file not existing
int savestates_exists(unsigned int slot, int mode)
{
	savestates_select_slot(slot);
	gzFile f = gzopen(savestates_get_filename(), (mode == SAVESTATE) ? "wb" : "rb");
   	
	if(!f) {
		return 0;
	}
	gzclose(f);
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

void savestates_save(unsigned int slot, u8* fb_tex)
{ 
	gzFile f;
	char buf[1024];
	int len, i;
	float progress = 0.0f;
	
	savestates_select_slot(slot);
	f = gzopen(savestates_get_filename(), "wb");
	
	if(!f) {
		return;
	}
	LoadingBar_showBar(progress, SAVE_STATE_MSG);
	
	//Save Header
	gzwrite(f, statesmagic, 3); //Write magic "W64"
	gzwrite(f, &savestates_version, sizeof(unsigned int));
#if !(defined(HW_DOL) && defined(USE_EXPANSION))
	gzwrite(f, fb_tex, FB_THUMB_SIZE);
#endif

	//Save State
	// RDRAM 0->50%
	float slice = 0.50f / (RDRAM_SIZE / CHUNK_SIZE);
	unsigned char* ramPtr = (unsigned char*)&rdram[0];
	for(i = 0; i < RDRAM_SIZE; i+= CHUNK_SIZE) {
		gzwrite(f, ramPtr, CHUNK_SIZE);
		ramPtr+=CHUNK_SIZE;
		progress += slice;
		LoadingBar_showBar(progress, SAVE_STATE_MSG);
	}
	//print_gecko("adler of RDRAM %08X\r\n",adler32(0, (Bytef *)&rdram[0], RDRAM_SIZE));

	// TLB 50->90% (cache funcs are just wrapper macros for a non cache build)
	slice = 0.40f / ((0x100000*2) / CHUNK_SIZE);
	int j = 0;
	for(i = 0; i < 0x100000/CHUNK_SIZE; i++) {
		for(j = 0; j < CHUNK_SIZE; j++) {
			u32 entry = TLBCache_get_r((i*CHUNK_SIZE) + j);
			gzwrite(f, &entry, 4);
		}
		progress += slice;
		LoadingBar_showBar(progress, SAVE_STATE_MSG);
	}
	for(i = 0; i < 0x100000/CHUNK_SIZE; i++) {
		for(j = 0; j < CHUNK_SIZE; j++) {
			u32 entry = TLBCache_get_w((i*CHUNK_SIZE) + j);
			gzwrite(f, &entry, 4);
		}
		progress += slice;
		LoadingBar_showBar(progress, SAVE_STATE_MSG);
	}

	// Save registers (90->95%)
	gzwrite(f, &rdram_register, sizeof(RDRAM_register));
	gzwrite(f, &MI_register, sizeof(mips_register));
	gzwrite(f, &pi_register, sizeof(PI_register));
	gzwrite(f, &sp_register, sizeof(SP_register));
	gzwrite(f, &rsp_register, sizeof(RSP_register));
	gzwrite(f, &si_register, sizeof(SI_register));
	gzwrite(f, &vi_register, sizeof(VI_register));
	gzwrite(f, &ri_register, sizeof(RI_register));
	gzwrite(f, &ai_register, sizeof(AI_register));
	gzwrite(f, &dpc_register, sizeof(DPC_register));
	gzwrite(f, &dps_register, sizeof(DPS_register));
	progress += 0.05f;
	LoadingBar_showBar(progress, SAVE_STATE_MSG);
	
	// RSP, r4300 struct and interrupt queue (95%->100%)
	gzwrite(f, SP_DMEM, 0x1000);
	gzwrite(f, SP_IMEM, 0x1000);
	gzwrite(f, PIF_RAM, 0x40);
	gzwrite(f, &flashRAMInfo, sizeof(_FlashRAMInfo));
	gzwrite(f, &r4300, sizeof(r4300));
	gzwrite(f, r4300.fpr_data, 32*8);
	
	len = save_eventqueue_infos(buf);
	gzwrite(f, buf, len);
	gzclose(f);
	LoadingBar_showBar(1.0f, SAVE_STATE_MSG);
}

//load a save state header (texture + datetime)
int savestates_load_header(unsigned int slot, u8* fb_tex, char* date, char* time)
{
	gzFile f = NULL;
	struct stat attrib;

	savestates_select_slot(slot);

	//get modified time from file attribute
	stat(savestates_get_filename(), &attrib);
	if(date)
		strftime(date, 9, "%D", localtime(&(attrib.st_mtime)));//Write date string in MM/DD/YY format
	if(time)
		strftime(time, 9, "%R", localtime(&(attrib.st_mtime)));//Write time string in HH:MM format
	
	f = gzopen(savestates_get_filename(), "rb");
	
	if (!f || !savestates_check_valid(f)) {
		return -1;
	}
#if !(defined(HW_DOL) && defined(USE_EXPANSION))
	gzread(f, fb_tex, FB_THUMB_SIZE);
#endif
	gzclose(f);
	return 0;
}

int savestates_load(unsigned int slot)
{
	gzFile f = NULL;
	char buf[1024];
	int len, i;
	float progress = 0.0f;
	
	savestates_select_slot(slot);
	f = gzopen(savestates_get_filename(), "rb");
	
	if (!f || !savestates_check_valid(f)) {
		return -1;
	}
	LoadingBar_showBar(progress, LOAD_STATE_MSG);
	
#if !(defined(HW_DOL) && defined(USE_EXPANSION))
	//Skip image
	gzseek(f, FB_THUMB_SIZE, SEEK_CUR);
#endif

	//Load State
	// RDRAM 0->50%
	float slice = 0.50f / (RDRAM_SIZE / CHUNK_SIZE);
	unsigned char* ramPtr = (unsigned char*)&rdram[0];
	for(i = 0; i < RDRAM_SIZE; i+= CHUNK_SIZE) {
		gzread(f, ramPtr, CHUNK_SIZE);
		ramPtr+=CHUNK_SIZE;
		progress += slice;
		LoadingBar_showBar(progress, LOAD_STATE_MSG);
	}
	//print_gecko("adler of RDRAM %08X\r\n",adler32(0, (Bytef *)&rdram[0], RDRAM_SIZE));
	
	// TLB 50->90% (cache funcs are just wrapper macros for a non cache build)
	slice = 0.40f / ((0x100000*2) / CHUNK_SIZE);
	u32 j, entry;

	TLBCache_init();
	for(i = 0; i < 0x100000/CHUNK_SIZE; i++) {
		for(j = 0; j < CHUNK_SIZE; j++) {
			gzread(f, &entry, 4);
			if(entry) {
				TLBCache_set_r((i*CHUNK_SIZE) + j, entry);
			}
		}
		progress += slice;
		LoadingBar_showBar(progress, LOAD_STATE_MSG);
	}
	for(i = 0; i < 0x100000/CHUNK_SIZE; i++) {
		for(j = 0; j < CHUNK_SIZE; j++) {
			gzread(f, &entry, 4);
			if(entry) {
				TLBCache_set_w((i*CHUNK_SIZE) + j, entry);
			}
		}
		progress += slice;
		LoadingBar_showBar(progress, LOAD_STATE_MSG);
	}

	// Load registers (90->95%)
	gzread(f, &rdram_register, sizeof(RDRAM_register));
	gzread(f, &MI_register, sizeof(mips_register));
	gzread(f, &pi_register, sizeof(PI_register));
	gzread(f, &sp_register, sizeof(SP_register));
	gzread(f, &rsp_register, sizeof(RSP_register));
	gzread(f, &si_register, sizeof(SI_register));
	gzread(f, &vi_register, sizeof(VI_register));
	gzread(f, &ri_register, sizeof(RI_register));
	gzread(f, &ai_register, sizeof(AI_register));
	gzread(f, &dpc_register, sizeof(DPC_register));
	gzread(f, &dps_register, sizeof(DPS_register));
	progress += 0.05f;
	LoadingBar_showBar(progress, LOAD_STATE_MSG);

	// RSP, r4300 struct and interrupt queue (95%->100%)
	gzread(f, SP_DMEM, 0x1000);
	gzread(f, SP_IMEM, 0x1000);
	gzread(f, PIF_RAM, 0x40);
	gzread(f, &flashRAMInfo, sizeof(_FlashRAMInfo));
	gzread(f, &r4300, sizeof(r4300));
	set_fpr_pointers(Status);  // Status is r4300.reg_cop0[12]
	gzread(f, r4300.fpr_data, 32*8);

	len = 0;
	while(1)
	{
		gzread(f, buf+len, 4);
		if (*((unsigned long*)&buf[len]) == 0xFFFFFFFF) break;
		gzread(f, buf+len+4, 4);
		len += 8;
	}
	load_eventqueue_infos(buf);
	gzclose(f);

	r4300.stop = 0;
	LoadingBar_showBar(1.0f, LOAD_STATE_MSG);
	return 0; //success!
}
