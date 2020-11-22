/**
 * Wii64 - fileBrowser-DVD.c
 * Copyright (C) 2007, 2008, 2009, 2013 emu_kidid
 * 
 * fileBrowser module for ISO9660 DVD Discs
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address:  emukidid@gmail.com
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


#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <ogc/machine/processor.h>
#include "fileBrowser.h"
#include "gc_dvd.h"

/* DVD Globals */
int dvd_init = 0;

#define ZELDA_OOT_NAME  "Zelda - Ocarina of Time.z64"
#define ZELDA_MQ_NAME   "Zelda - Ocarina of Time Master Quest.z64"
#define ZELDA_MM_NAME   "Zelda - Majoras Mask.z64"

fileBrowser_file topLevel_DVD =
	{ "\\", // file name
	  0ULL,      // discoffset (u64)
	  0,         // offset
	  0,         // size
	  FILE_BROWSER_ATTR_DIR
	};

static int num_entries = 0;
int fileBrowser_DVD_readDir(fileBrowser_file* ffile, fileBrowser_file** dir, int recursive, int n64only){	
	
	if(dvd_get_error() || !dvd_init) { //if some error
		int ret = init_dvd();
		if(ret) {    //try init
			return ret; //fail
		}
		dvd_init = 1;
	}

	if (!memcmp((void*)0x80000000, "D43", 3)) { //OoT+MQ bonus disc support.
		num_entries = 2;
		*dir = malloc( num_entries * sizeof(fileBrowser_file) );
		strcpy( (*dir)[0].name, ZELDA_OOT_NAME);
		(*dir)[0].offset = 0;
		(*dir)[0].size   = 0x2000000;
		(*dir)[0].attr	 = 0;
		strcpy( (*dir)[1].name, ZELDA_MQ_NAME);
		(*dir)[1].offset = 0;
		(*dir)[1].size   = 0x2000000;
		(*dir)[1].attr	 = 0;
		
		switch(((char*)0x80000000)[3]) {
			case 'U':
				(*dir)[0].discoffset = 0x54FBEEF4ULL;
				(*dir)[1].discoffset = 0x52CCC5FCULL;
			break;
			case 'P':
				(*dir)[0].discoffset = 0x52CCA240ULL;
				(*dir)[1].discoffset = 0x54FBCB38ULL;
			break;
			case 'E':
				(*dir)[0].discoffset = 0x550569D8ULL;
				(*dir)[1].discoffset = 0x52FBC1E0ULL;
			break;
		}
		return num_entries;
	}
	else if (!memcmp((void*)0x80000000, "PZLP01", 6)) { //Zelda Collectors disc support.
		num_entries = 2;
		*dir = malloc( num_entries * sizeof(fileBrowser_file) );
		strcpy( (*dir)[0].name, ZELDA_OOT_NAME);
		(*dir)[0].discoffset = 0x3B9D1FC0ULL;
		(*dir)[0].offset = 0;
		(*dir)[0].size   = 0x2000000;
		(*dir)[0].attr	 = 0;
		strcpy( (*dir)[1].name, ZELDA_MM_NAME);
		(*dir)[1].discoffset = 0x0C4E1FC0ULL;
		(*dir)[1].offset = 0;
		(*dir)[1].size   = 0x2000000;
		(*dir)[1].attr	 = 0;
		return num_entries;
	}

	file_entry *DVDToc = NULL;

	// Call the corresponding DVD function
	int dvd_entries = dvd_read_directoryentries(ffile->discoffset,ffile->size, &DVDToc);

	// If it was not successful, just return the error
	if(dvd_entries <= 0) return FILE_BROWSER_ERROR;

	// Temp entry
	fileBrowser_file *direntry = malloc(sizeof(fileBrowser_file));
	// Convert the DVD "file" data to fileBrowser_files
	int i;
	for(i = 0; i < dvd_entries; ++i) {
		// Create a temporary entry for this directory entry.
		memset(direntry, 0, sizeof(fileBrowser_file));
		strcpy(direntry->name, DVDToc[i].name );
		direntry->discoffset = (uint64_t)(((uint64_t)DVDToc[i].sector)*2048);
		direntry->offset = 0;
		direntry->size   = DVDToc[i].size;
		direntry->attr	 = (DVDToc[i].flags == 2) ? FILE_BROWSER_ATTR_DIR : 0;
		if(direntry->name[strlen(direntry->name)-1] == '/' )
			direntry->name[strlen(direntry->name)-1] = 0;	//get rid of trailing '/'
		
		// If recursive, search all directories
		if(recursive && (direntry->attr == FILE_BROWSER_ATTR_DIR)) {
			if((strlen(&direntry->name[0]) == 0) || direntry->name[0] == '.') continue;	// hide/do not browse these directories.
			//print_gecko("Entering directory: %s\r\n", direntry->name);
			fileBrowser_DVD_readDir(direntry, dir, recursive, n64only);
		}
		else {
			if(*dir == NULL) {
				*dir = malloc(sizeof(fileBrowser_file));
				num_entries = 0;
			}
			else {
				//print_gecko("Size of *dir = %i\r\n", num_entries);
				*dir = realloc( *dir, ((num_entries)+1) * sizeof(fileBrowser_file) ); 
			}
			if(n64only) {
				if(!(strcasestr(direntry->name,".v64") || strcasestr(direntry->name,".z64") ||
					 strcasestr(direntry->name,".n64") || strcasestr(direntry->name,".bin")))
					continue;
			}
			
			memcpy(&(*dir)[num_entries], direntry, sizeof(fileBrowser_file));
			//print_gecko("Adding file: %s\r\n", (*dir)[num_entries].name);
			num_entries++;
		}
	}
	if(direntry) 
		free(direntry);
	if(DVDToc)
		free(DVDToc);

	if(strlen((*dir)[0].name) == 0)
		strcpy( (*dir)[0].name, ".." );

	return num_entries;
}

int fileBrowser_DVD_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	return 0;
}

int fileBrowser_DVD_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	int bytesread = DVD_Read(buffer,file->discoffset+file->offset,length);
	if(bytesread > 0)
		file->offset += bytesread;
	return bytesread;
}

int fileBrowser_DVD_init(fileBrowser_file* file){
	return 0;
}

int fileBrowser_DVD_deinit(fileBrowser_file* file) {
	return 0;
}

