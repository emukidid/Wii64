/**
 * Mupen64 - savestates.c
 * Copyright (C) 2002 Hacktarux
 * Copyright (C) 2008, 2009 emu_kidid
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

void savestates_select_slot(unsigned int s)
{
	if (s > 9) return;
	savestates_slot = s;
}
	
//returns 0 on file not existing
int savestates_exists(int mode)
{
	gzFile f;
	char *filename;
	filename = malloc(1024);
#ifdef HW_RVL
	sprintf(filename, "%s%s%s%s.st%d",(saveStateDevice==SAVESTATEDEVICE_USB)?"usb:":"sd:",
			statespath, ROM_SETTINGS.goodname, saveregionstr(),savestates_slot);
#else
	sprintf(filename, "sd:%s%s%s.st%d", statespath, ROM_SETTINGS.goodname, saveregionstr(),savestates_slot);
#endif

	f = gzopen(filename, (mode == SAVESTATE) ? "wb" : "rb");
	free(filename);
   	
	if(!f) {
		return 0;
	}
	gzclose(f);
	return 1;
}

void savestates_save(unsigned int slot, u8* fb_tex)
{ 
	gzFile f;
	char *filename, buf[1024];
	int len, i;
	
	savestates_select_slot(slot);
	
	/* fix the filename to %s.st%d format */
	filename = malloc(1024);
#ifdef HW_RVL
	sprintf(filename, "%s%s%s%s.st%d",(saveStateDevice==SAVESTATEDEVICE_USB)?"usb:":"sd:",
			statespath, ROM_SETTINGS.goodname, saveregionstr(),savestates_slot);
#else
	sprintf(filename, "sd:%s%s%s.st%d", statespath, ROM_SETTINGS.goodname, saveregionstr(),savestates_slot);
#endif

	f = gzopen(filename, "wb");
	free(filename);
   	
	if(!f) {
		return;
	}
	LoadingBar_showBar(0.0f, SAVE_STATE_MSG);
	//Save Header
	gzwrite(f, statesmagic, 3); //Write magic "W64"
	gzwrite(f, &savestates_version, sizeof(unsigned int));
	gzwrite(f, fb_tex, FB_THUMB_SIZE);
	//Save State
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
	LoadingBar_showBar(0.10f, SAVE_STATE_MSG);
#ifdef USE_EXPANSION
	gzwrite(f, rdram, 0x800000);
#else
	gzwrite(f, rdram, 0x400000);
#endif
	LoadingBar_showBar(0.50f, SAVE_STATE_MSG);
	gzwrite(f, SP_DMEM, 0x1000);
	gzwrite(f, SP_IMEM, 0x1000);
	gzwrite(f, PIF_RAM, 0x40);
	
	save_flashram_infos(buf);
	gzwrite(f, buf, 24);
#ifndef USE_TLB_CACHE
	gzwrite(f, tlb_LUT_r, 0x100000);		
	gzwrite(f, tlb_LUT_w, 0x100000);
#else
	//Traverse the TLB cache hash and dump it
	for(i = 0; i < 0x100000; i++) {
		u32 entry = TLBCache_get_r(i);
		gzwrite(f, &entry, 4);
	}
	for(i = 0; i < 0x100000; i++) {
		u32 entry = TLBCache_get_w(i);
		gzwrite(f, &entry, 4);
	}
#endif
	LoadingBar_showBar(0.85f, SAVE_STATE_MSG);

	gzwrite(f, &r4300, sizeof(r4300));
	if ((Status & 0x04000000) == 0)
	{   // FR bit == 0 means 32-bit (MIPS I) FGR mode
		shuffle_fpr_data(0, 0x04000000);  // shuffle data into 64-bit register format for storage
		gzwrite(f, r4300.fpr_data, 32*8);
		shuffle_fpr_data(0x04000000, 0);  // put it back in 32-bit mode
	}
	else
	{
		gzwrite(f, r4300.fpr_data, 32*8);
	}
	
	gzwrite(f, tlb_e, 32*sizeof(tlb));
	gzwrite(f, &next_vi, 4);
	gzwrite(f, &vi_field, 4);
	
	len = save_eventqueue_infos(buf);
	gzwrite(f, buf, len);
	gzclose(f);
	LoadingBar_showBar(1.0f, SAVE_STATE_MSG);
}

int savestates_load_header(unsigned int slot, u8* fb_tex, char* date, char* time)
{
	gzFile f = NULL;
	char *filename, statesmagic_read[3];
//	int len, i;
	unsigned int savestates_version_read=0;
	struct stat attrib;

	savestates_select_slot(slot);

	/* fix the filename to %s.st%d format */
	filename = malloc(1024);
#ifdef HW_RVL
	sprintf(filename, "%s%s%s%s.st%d",(saveStateDevice==SAVESTATEDEVICE_USB)?"usb:":"sd:",
			statespath, ROM_SETTINGS.goodname, saveregionstr(),savestates_slot);
#else
	sprintf(filename, "sd:%s%s%s.st%d", statespath, ROM_SETTINGS.goodname, saveregionstr(),savestates_slot);
#endif

	//get modified time from file attribute
	stat(filename, &attrib);
	if(date)
		strftime(date, 9, "%D", localtime(&(attrib.st_mtime)));//Write date string in MM/DD/YY format
	if(time)
		strftime(time, 9, "%R", localtime(&(attrib.st_mtime)));//Write time string in HH:MM format
	
	f = gzopen(filename, "rb");
	free(filename);
	
	if (!f) {
		return -1;
	}
	//Load Header
	gzread(f, statesmagic_read, 3); //Read magic "W64"
	gzread(f, &savestates_version_read, sizeof(unsigned int));
	if (!((strncmp(statesmagic, statesmagic_read, 3)==0)&&(savestates_version==savestates_version_read)))
	{
		gzclose(f);
		return -1;
	}
	gzread(f, fb_tex, FB_THUMB_SIZE);
	gzclose(f);
	return 0;
}

void savestates_queue_load(unsigned int slot) {
	loadstate_queued = 1;
	loadstate_slot = slot;
}

int savestates_queued_load() {
	if(loadstate_queued) {
		savestates_load(loadstate_slot);
		loadstate_queued = 0;
		return 1;
	}
	return 0;
}

int savestates_load(unsigned int slot)
{
	gzFile f = NULL;
	char *filename, buf[1024], statesmagic_read[3];
	int len, i;
	unsigned int savestates_version_read;
		
	savestates_select_slot(slot);

	/* fix the filename to %s.st%d format */
	filename = malloc(1024);
#ifdef HW_RVL
	sprintf(filename, "%s%s%s%s.st%d",(saveStateDevice==SAVESTATEDEVICE_USB)?"usb:":"sd:",
			statespath, ROM_SETTINGS.goodname, saveregionstr(),savestates_slot);
#else
	sprintf(filename, "sd:%s%s%s.st%d", statespath, ROM_SETTINGS.goodname, saveregionstr(),savestates_slot);
#endif
	
	f = gzopen(filename, "rb");
	free(filename);
	
	if (!f) {
		return -1;
	}
	LoadingBar_showBar(0.0f, LOAD_STATE_MSG);
	//Load Header
	gzread(f, statesmagic_read, 3); //Read magic "W64"
	gzread(f, &savestates_version_read, sizeof(unsigned int));
	if (!((strncmp(statesmagic, statesmagic_read, 3)==0)&&(savestates_version==savestates_version_read)))
	{
		gzclose(f);
		return -1;
	}

	//Skip image
	gzseek(f, FB_THUMB_SIZE, SEEK_CUR);
	//Load State
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
	LoadingBar_showBar(0.10f, LOAD_STATE_MSG);
#ifdef USE_EXPANSION
	gzread(f, rdram, 0x800000);
#else
	gzread(f, rdram, 0x400000);
#endif
	gzread(f, SP_DMEM, 0x1000);
	gzread(f, SP_IMEM, 0x1000);
	gzread(f, PIF_RAM, 0x40);
	gzread(f, buf, 24);
	load_flashram_infos(buf);
	
#ifndef USE_TLB_CACHE
	gzread(f, tlb_LUT_r, 0x100000);
	gzread(f, tlb_LUT_w, 0x100000);
#else
	LoadingBar_showBar(0.5f, LOAD_STATE_MSG);
	u32 entry = 0;
	for(i = 0; i < 0x100000; i++) {
		gzread(f, &entry, 4);
		if(entry)
			TLBCache_set_r(i, entry);
	}
	for(i = 0; i < 0x100000; i++) {
		gzread(f, &entry, 4);
		if(entry)
			TLBCache_set_w(i, entry);
	}
#endif
	LoadingBar_showBar(0.85f, LOAD_STATE_MSG);
	gzread(f, &r4300, sizeof(r4300));
	set_fpr_pointers(Status);  // Status is r4300.reg_cop0[12]
	gzread(f, r4300.fpr_data, 32*8);
	if ((Status & 0x04000000) == 0)  // 32-bit FPR mode requires data shuffling because 64-bit layout is always stored in savestate file
		shuffle_fpr_data(0x04000000, 0);
	gzread(f, tlb_e, 32*sizeof(tlb));
	gzread(f, &next_vi, 4);
	gzread(f, &vi_field, 4);
	
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
	LoadingBar_showBar(1.0f, LOAD_STATE_MSG);
	r4300.stop = 0;
	return 0; //success!
}
