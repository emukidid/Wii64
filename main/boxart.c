/* boxart.c 
	- Boxart texture loading from a binary compilation
	by emu_kidid
 */
#if !(defined(GC_BASIC))
#include <string.h>
#include <stdio.h>
#include <fat.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/dir.h>
#include "boxart.h"
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"


#define MAX_HEADER_SIZE 0x2000
FILE* boxartFile = NULL;
u32 *boxartHeader = NULL;
extern u32 missBoxArt_length;
extern u8 missBoxArt[];	//default "no boxart" texture

/* Call this in the menu to set things up */
void BOXART_Init()
{
	// open boxart file
	if(!boxartFile) {
#ifdef WII
		if(romFile_topLevel == &topLevel_libfat_Default) {
			boxartFile = fopen( "sd:/wii64/boxart.bin", "rb" );
		}
		else if(romFile_topLevel == &topLevel_libfat_USB) {
			boxartFile = fopen( "usb:/wii64/boxart.bin", "rb" );
		}
#else
		if(romFile_topLevel == &topLevel_libfat_Default) {
			boxartFile = fopen( "sd:/wii64/boxart.bin", "rb" );
		}
#endif
	}
	
	// file wasn't found	
	if(!boxartFile)
		return;
	
	// allocate header space and read header
	if(!boxartHeader)
	{
		boxartHeader = (u32*)memalign(32,MAX_HEADER_SIZE);
		fread(boxartHeader, 1, MAX_HEADER_SIZE, boxartFile);
	}
	
}

/* Call this when we launch a game to free up resources */
void BOXART_DeInit()
{
	//close boxart file
	if(boxartFile)
	{
		fclose(boxartFile);
		boxartFile = NULL;
	}
	//free header data
	if(boxartHeader)
	{
		free(boxartHeader);
		boxartHeader = NULL;
	}
}

/* Pass In a CRC and a aligned buffer of size BOXART_TEX_SIZE
	- a RGB565 image will be returned */
void BOXART_LoadTexture(u32 CRC, char *buffer)
{
	memset(buffer, 0x5A, BOXART_TEX_SIZE);
	if(!boxartFile)
	{
		memcpy(buffer,&missBoxArt,missBoxArt_length);
		return;
	}
		
	int i = 0;
	for(i = 0; i < (MAX_HEADER_SIZE/4); i+=2)
	{
		if(boxartHeader[i] == CRC)
		{
			fseek(boxartFile,boxartHeader[i+1],SEEK_SET);
			fread(buffer, 1, BOXART_TEX_SIZE, boxartFile);
			return;
		}
	}
	// if we didn't find it, then return the default image
	memcpy(buffer,&missBoxArt,missBoxArt_length);
}
#endif
