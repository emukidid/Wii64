/**
 * Wii64 - fileBrowser-libfat.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009, 2013 emu_kidid
 * 
 * fileBrowser for any devices using libfat
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                emukidid@gmail.com
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


#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <dirent.h>
#include "fileBrowser.h"
#include <sdcard/gcsd.h>
#include "../r4300/r4300.h"
#include "../main/ROM-Cache.h"

extern BOOL hasLoadedROM;
extern int stop;

#ifdef HW_RVL
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>
DISC_INTERFACE* frontsd = &__io_wiisd;
DISC_INTERFACE* usb = &__io_usbstorage;
DISC_INTERFACE* carda = &__io_gcsda;
DISC_INTERFACE* cardb = &__io_gcsdb;

#else
DISC_INTERFACE* carda = &__io_gcsda;
DISC_INTERFACE* cardb = &__io_gcsdb;
DISC_INTERFACE* sd2sp2 = &__io_gcsd2;
#endif

#define FRONTSD 1
#define CARD_A  2
#define CARD_B  3
#ifdef HW_RVL

static char sdMounted  = 0;
static char sdNeedsUnmount  = 0;
static char usbMounted = 0;
static char usbNeedsUnmount = 0;
#endif

fileBrowser_file topLevel_libfat_Default =
	{ "sd:/wii64/roms", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };
	 
fileBrowser_file topLevel_libfat_USB =
	{ "usb:/wii64/roms", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file saveDir_libfat_Default =
	{ "sd:/wii64/saves",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };
	 
fileBrowser_file saveDir_libfat_USB =
	{ "usb:/wii64/saves",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };

static int num_entries = 0;
int fileBrowser_libfat_readDir(fileBrowser_file* file, fileBrowser_file** dir, int recursive, int n64only){
	
	DIR* dp = opendir( file->name );
	if(!dp) return FILE_BROWSER_ERROR;
	struct stat fstat;
	fileBrowser_file *direntry = malloc(sizeof(fileBrowser_file));
	
	// Read each entry of the directory
	while(1) {
		struct dirent *entry = readdir(dp);
		if(!entry)
			break;
	
		// Create a temporary entry for this directory entry.
		memset(direntry, 0, sizeof(fileBrowser_file));
		sprintf(direntry->name, "%s/%s", file->name, entry->d_name);
		stat(direntry->name,&fstat);
		direntry->offset = 0;
		direntry->size   = fstat.st_size;
		direntry->attr   = (fstat.st_mode & _IFDIR) ?
							FILE_BROWSER_ATTR_DIR : 0;
		
		// If recursive, search all directories
		if(recursive && (direntry->attr == FILE_BROWSER_ATTR_DIR)) {
			if(entry->d_name[0] == '.') continue;	// hide/do not browse these directories.
			//print_gecko("Entering directory: %s\r\n", direntry->name);
			fileBrowser_libfat_readDir(direntry, dir, recursive, n64only);
		}
		else {
			if(*dir == NULL) {
				*dir = malloc( sizeof(fileBrowser_file) );
				num_entries = 0;
			}
			else {
				//print_gecko("Size of *dir = %i\r\n", num_entries);
				*dir = realloc( *dir, ((num_entries)+1) * sizeof(fileBrowser_file) ); 
			}
			if(n64only) {
				const char *ext = strrchr(direntry->name, '.');
				if(!(strcasecmp(ext, ".v64") || strcasecmp(ext, ".z64") ||
					 strcasecmp(ext, ".n64") || strcasecmp(ext, ".bin")))
					continue;
			}
			
			memcpy(&(*dir)[num_entries], direntry, sizeof(fileBrowser_file));
			//print_gecko("Adding file: %s\r\n", (*dir)[num_entries].name);
			num_entries++;
		}
	}
	if(direntry)
		free(direntry);
	
	closedir(dp);

	return num_entries;
}

int fileBrowser_libfat_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	
	return 0;
}

int fileBrowser_libfat_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	FILE* f = fopen( file->name, "rb" );
	if(!f) return FILE_BROWSER_ERROR;
	
	fseek(f, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, f);
	if(bytes_read > 0) file->offset += bytes_read;
	
	fclose(f);
	return bytes_read;
}

int fileBrowser_libfat_writeFile(fileBrowser_file* file, void* buffer, unsigned int length){
	FILE* f = fopen( file->name, "wb" );
	if(!f) return FILE_BROWSER_ERROR;
	
	fseek(f, file->offset, SEEK_SET);
	int bytes_read = fwrite(buffer, 1, length, f);
	if(bytes_read > 0) file->offset += bytes_read;
	
	fclose(f);
	return bytes_read;
}

/* call fileBrowser_libfat_init as much as you like for all devices
    - returns 0 on device not present/error
    - returns 1 on ok
*/
static int mounted[5];

int fileBrowser_libfat_init(fileBrowser_file* f){

	int res = 0;
#ifdef HW_RVL
	if(f->name[0] == 's') {     //SD
		if(mounted[0]) return 1;	// already.
		if(fatMountSimple ("sd", frontsd)) {
			mounted[0] = 1;
			return res;
		}
	}
	else {
		if(mounted[1]) return 1;	// already.
		if(fatMountSimple ("usb", usb)) {
			mounted[1] = 1;
			return res;
		}
	}
#else
	// GC has only SD
	if(mounted[2]) return 1;
	res = fatMountSimple ("sd", sd2sp2);
#endif
	if(res) {
		mounted[2] = 1;
		return res;
	}
	res = fatMountSimple ("sd", carda);
	if(res) {
		mounted[2] = 1;
		return res;
	}
	res = fatMountSimple ("sd", cardb);
	if(res) {
		mounted[2] = 1;
		return res;
	}
	return res;
}

int fileBrowser_libfat_deleteFile(fileBrowser_file* file){
	return (remove(file->name) == -1) ? 0 : 1;
}

static FILE* fd;
int fileBrowser_libfat_deinit(fileBrowser_file* f){
	if(f->name[0] == 's') {      //SD
		if(fd) {
			fclose(fd);
			fd = NULL;
		}
		//fatUnmount("sd");
 	}
	return 0;
}

/* Special for ROM loading only */
int fileBrowser_libfatROM_deinit(fileBrowser_file* f){
	if(fd)
		fclose(fd);
	fd = NULL;
	return 0;
}
int fileBrowser_libfatROM_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	if(!fd) fd = fopen( file->name, "rb");
	fseek(fd, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, fd);
	if(bytes_read > 0) file->offset += bytes_read;

	return bytes_read;
}

