/**
 * Wii64/Cube64 - gc_dvd.c
 * Copyright (C) 2007, 2008, 2009, 2010 emu_kidid
 * 
 * DVD Reading support for GC/Wii
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: emukidid@gmail.com
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ogc/dvd.h>
#include <malloc.h>
#include <string.h>
#include <gccore.h>
#include <unistd.h>
#include "gc_dvd.h"
#include <ogc/machine/processor.h>

/* DVD Stuff */
u32 dvd_hard_init = 0;
static u32 read_cmd = NORMAL;
static int last_current_dir = -1;
int is_unicode;
u32 inquiryBuf[32] __attribute__((aligned(32)));
u8 dvdbuffer[2048] __attribute__((aligned(32)));
 
#ifdef HW_RVL
volatile unsigned long* dvd = (volatile unsigned long*)0xCD806000;
#else
#include "drivecodes.h"
#define is_gamecube() (((mfpvr() == GC_CPU_VERSION01)||((mfpvr() == GC_CPU_VERSION02))))
volatile unsigned long* dvd = (volatile unsigned long*)0xCC006000;
#endif

int have_hw_access() {
  if((*(volatile unsigned int*)HW_ARMIRQMASK)&&(*(volatile unsigned int*)HW_ARMIRQFLAG)) {
	// disable DVD irq for starlet
	mask32(HW_ARMIRQMASK, 1<<18, 0);
    return 1;
  }
  return 0;
}

int init_dvd() {
// Gamecube Mode
#ifdef HW_DOL
  if(!is_gamecube()) //GC mode on Wii, modchip required
  {
    dvd_reset();
    dvd_read_id();
    if(!dvd_get_error()) {
      return 0; //we're ok
    }
  }
  else      //GC, no modchip even required :)
  {
    dvd_reset();
	dvd_read_id();
	// Avoid lid open scenario
	if((dvd_get_error()>>24) && (dvd_get_error()>>24 != 1)) {
		dvd_enable_patches();	// Could be a dvd backup
		if(!dvd_get_error())
			return 0;
	}
	else if((dvd_get_error()>>24) == 1) {  // Lid is open, tell the user!
		return NO_DISC;
	}
  }
  if(dvd_get_error()>>24) {
    return NO_DISC;
  }
  return -1;

#endif
// Wii (Wii mode)
#ifdef HW_RVL
  if(!have_hw_access()) {
    return NO_HW_ACCESS;
  }
  
  STACK_ALIGN(u8,id,32,32);
  // enable GPIO for spin-up on drive reset (active low)
  mask32(0x0D8000E0, 0x10, 0);
  // assert DI reset (active low)
  mask32(0x0D800194, 0x400, 0);
  usleep(1000);
  // deassert DI reset
  mask32(0x0D800194, 0, 0x400);

  if ((dvd_get_error() >> 24) == 1) {
  	return NO_DISC;
  }
  
  if((!dvd_hard_init) || (dvd_get_error())) {
    // read id
	dvd[0] = 0x54;
	dvd[2] = 0xA8000040;
	dvd[3] = 0;
	dvd[4] = 0x20;
	dvd[5] = (u32)id & 0x1FFFFFFF;
	dvd[6] = 0x20;
	dvd[7] = 3;
	while (dvd[7] & 1)
	usleep(20000);
	dvd_hard_init = 1;
  }

  if((dvd_get_error()&0xFFFFFF)==0x053000) {
    read_cmd = DVDR;
  }
  else {
    read_cmd = NORMAL;
  }
  dvd_read_id();
  return 0;
#endif
}

/* Standard DVD functions */

void dvd_reset()
{
	dvd[1] = 2;
	volatile unsigned long v = *(volatile unsigned long*)0xcc003024;
	*(volatile unsigned long*)0xcc003024 = (v & ~4) | 1;
	sleep(1);
	*(volatile unsigned long*)0xcc003024 = v | 5;
}

int dvd_read_id()
{
#ifdef HW_RVL
  DVD_LowRead64((void*)0x80000000, 32, 0ULL);  //for easter egg disc support
  return 0;
#endif
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xA8000040;
	dvd[3] = 0;
	dvd[4] = 0x20;
	dvd[5] = 0;
	dvd[6] = 0x20;
	dvd[7] = 3; // enable reading!
	while (dvd[7] & 1);
	if (dvd[0] & 0x4)
		return 1;
	return 0;
}

unsigned int dvd_get_error(void)
{
	dvd[2] = 0xE0000000;
	dvd[8] = 0;
	dvd[7] = 1; // IMM
	int timeout = 1000000;
	while ((dvd[7] & 1) && timeout) {timeout--;}
	return timeout ? dvd[8] : 0xFFFFFFFF;
}

void dvd_motor_off()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xe3000000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1; // IMM
	while (dvd[7] & 1);
}

/* 
DVD_LowRead64(void* dst, unsigned int len, uint64_t offset)
  Read Raw, needs to be on sector boundaries
  Has 8,796,093,020,160 byte limit (8 TeraBytes)
  Synchronous function.
    return -1 if offset is out of range
*/
int DVD_LowRead64(void* dst, unsigned int len, uint64_t offset)
{
  if(offset>>2 > 0xFFFFFFFF)
    return -1;
    
  if ((((int)dst) & 0xC0000000) == 0x80000000) // cached?
		dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = read_cmd;
	dvd[3] = read_cmd == DVDR ? offset>>11 : offset >> 2;
	dvd[4] = read_cmd == DVDR ? len>>11 : len;
	dvd[5] = (unsigned long)dst;
	dvd[6] = len;
	dvd[7] = 3; // enable reading!
	DCInvalidateRange(dst, len);
	while (dvd[7] & 1);

	if (dvd[0] & 0x4)
		return 1;
	return 0;
}

static char error_str[256];
char *dvd_error_str()
{
  unsigned int err = dvd_get_error();
  if(!err) return "OK";
  
  memset(&error_str[0],0,256);
  switch(err>>24) {
    case 0:
      break;
    case 1:
      strcpy(&error_str[0],"Lid open");
      break;
    case 2:
      strcpy(&error_str[0],"No disk/Disk changed");
      break;
    case 3:
      strcpy(&error_str[0],"No disk");
      break;  
    case 4:
      strcpy(&error_str[0],"Motor off");
      break;  
    case 5:
      strcpy(&error_str[0],"Disk not initialized");
      break;
  }
  switch(err&0xFFFFFF) {
    case 0:
      break;
    case 0x020400:
      strcat(&error_str[0]," Motor Stopped");
      break;
    case 0x020401:
      strcat(&error_str[0]," Disk ID not read");
      break;
    case 0x023A00:
      strcat(&error_str[0]," Medium not present / Cover opened");
      break;  
    case 0x030200:
      strcat(&error_str[0]," No Seek complete");
      break;  
    case 0x031100:
      strcat(&error_str[0]," UnRecoverd read error");
      break;
    case 0x040800:
      strcat(&error_str[0]," Transfer protocol error");
      break;
    case 0x052000:
      strcat(&error_str[0]," Invalid command operation code");
      break;
    case 0x052001:
      strcat(&error_str[0]," Audio Buffer not set");
      break;  
    case 0x052100:
      strcat(&error_str[0]," Logical block address out of range");
      break;  
    case 0x052400:
      strcat(&error_str[0]," Invalid Field in command packet");
      break;
    case 0x052401:
      strcat(&error_str[0]," Invalid audio command");
      break;
    case 0x052402:
      strcat(&error_str[0]," Configuration out of permitted period");
      break;
    case 0x053000:
      strcat(&error_str[0]," DVD-R"); //?
      break;
    case 0x053100:
      strcat(&error_str[0]," Wrong Read Type"); //?
      break;
    case 0x056300:
      strcat(&error_str[0]," End of user area encountered on this track");
      break;  
    case 0x062800:
      strcat(&error_str[0]," Medium may have changed");
      break;  
    case 0x0B5A01:
      strcat(&error_str[0]," Operator medium removal request");
      break;
  }
  if(!error_str[0])
    sprintf(&error_str[0],"Unknown %08X",err);
  return &error_str[0];
  
}

#ifdef HW_DOL
void drive_version(u8 *buf)
{
	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = 0x12000000;
	dvd[3] = 0;
	dvd[4] = 0x20;
	dvd[5] = (long)(&inquiryBuf[0]) & 0x1FFFFFFF;
	dvd[6] = 0x20;
	dvd[7] = 3;
	
	int retries = 1000000;
	while(( dvd[7] & 1) && retries) {
		retries --;
	}
	DCFlushRange((void *)inquiryBuf, 0x20);
	
	if(!retries) {
		buf[0] = 0;
	}
	else {
		memcpy(buf,&inquiryBuf[1],8);
	}
}

/* Debug mode enabling functions - GameCube only */

void dvd_setstatus()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;

	dvd[2] = 0xee060300;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1; // enable reading!
	while (dvd[7] & 1);
}

void dvd_setextension()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;

	dvd[2] = 0x55010000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1; // enable reading!
	while (dvd[7] & 1);
}

void dvd_unlock()
{
	dvd[0] |= 0x00000014;
	dvd[1] = 0;
	dvd[2] = 0xFF014D41;
	dvd[3] = 0x54534849;	//MATS
	dvd[4] = 0x54410200;	//HITA
	dvd[7] = 1;
	while (dvd[7] & 1);
	
	dvd[0] |= 0x00000014;
	dvd[1] = 0;
	dvd[2] = 0xFF004456;
	dvd[3] = 0x442D4741;	//DVD-
	dvd[4] = 0x4D450300;	//GAME
	dvd[7] = 1;
	while (dvd[7] & 1);
}

int dvd_writemem_32(u32 addr, u32 dat)
{
	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = 0xFE010100;	
	dvd[3] = addr;
	dvd[4] = 0x00040000;	
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 3;
	while (dvd[7] & 1);

	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = dat;
	dvd[7] = 1;
	while (dvd[7] & 1);

	return 0;
}

int dvd_writemem_array(u32 addr, void* buf, u32 size)
{
	u32* ptr = (u32*)buf;
	int rem = size;

	while (rem > 0)
	{
		dvd_writemem_32(addr, *ptr++);
		addr += 4;
		rem -= 4;
	}
	return 0;
}

#define DEBUG_STOP_DRIVE 0
#define DEBUG_START_DRIVE 0x100
#define DEBUG_ACCEPT_COPY 0x4000
#define DEBUG_DISC_CHECK 0x8000

void dvd_motor_on_extra()
{
	dvd[0] = 0x2e;
	dvd[1] = 1;
	dvd[2] = (0xfe110000 | DEBUG_ACCEPT_COPY | DEBUG_START_DRIVE);
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1;
	while (dvd[7] & 1);
}

void* drive_patch_ptr(u32 driveVersion)
{
	if(driveVersion == 0x20020402)
		return &Drive04;
	if(driveVersion == 0x20010608)
		return &Drive06;
	if(driveVersion == 0x20010831)
		return &DriveQ;
	if(driveVersion == 0x20020823)
		return &Drive08;
	return 0;
}

void dvd_enable_patches() 
{
	// Get the drive date
	u8* driveDate = (u8*)memalign(32,32);
	memset(driveDate,0,32);
	drive_version(driveDate);
	
	u32 driveVersion = *(u32*)&driveDate[0];
	free(driveDate);
	
	if(!driveVersion) return;	// Unsupported drive

	void* patchCode = drive_patch_ptr(driveVersion);
	
	//print_gecko("Drive date %08X\r\nUnlocking DVD\r\n",driveVersion);
	dvd_unlock();
	//print_gecko("Unlocking DVD - done\r\nWrite patch\r\n");
	dvd_writemem_array(0xff40d000, patchCode, 0x1F0);
	dvd_writemem_32(0x804c, 0x00d04000);
	//print_gecko("Write patch - done\r\nSet extension %08X\r\n",dvd_get_error());
	dvd_setextension();
	//print_gecko("Set extension - done\r\nUnlock again %08X\r\n",dvd_get_error());
	dvd_unlock();
	//print_gecko("Unlock again - done\r\nDebug Motor On %08X\r\n",dvd_get_error());
	dvd_motor_on_extra();
	//print_gecko("Debug Motor On - done\r\nSet Status %08X\r\n",dvd_get_error());
	dvd_setstatus();
	//print_gecko("Set Status - done %08X\r\n",dvd_get_error());
	dvd_read_id();
	//print_gecko("Read ID %08X\r\n",dvd_get_error());
}
#endif

/* Wrapper functions */

int read_sector(void* buffer, uint32_t sector)
{
	return DVD_LowRead64(buffer, 2048, sector * 2048);
}

/* 
DVD_Read(void* dst, uint64_t offset, int len)
  Reads from any offset and handles alignment from device
  Synchronous function.
    return -1 if offset is out of range
*/
int DVD_Read(void* dst, uint64_t offset, int len)
{
	int ol = len;
	int ret = 0;	
  char *sector_buffer = (char*)memalign(32,2048);
	while (len)
	{
		uint32_t sector = offset / 2048;
		ret |= DVD_LowRead64(sector_buffer, 2048, sector * 2048);
		uint32_t off = offset & 2047;

		int rl = 2048 - off;
		if (rl > len)
			rl = len;
		memcpy(dst, sector_buffer + off, rl);	

		offset += rl;
		len -= rl;
		dst += rl;
	}
	free(sector_buffer);
	if(ret)
		return -1;

	return ol;
}

int read_direntry(unsigned char* direntry, file_entry *DVDEntry, int *added)
{
       int nrb = *direntry++;
       ++direntry;

       int sector;

       direntry += 4;
       sector = (*direntry++) << 24;
       sector |= (*direntry++) << 16;
       sector |= (*direntry++) << 8;
       sector |= (*direntry++);        

       int size;

       direntry += 4;

       size = (*direntry++) << 24;
       size |= (*direntry++) << 16;
       size |= (*direntry++) << 8;
       size |= (*direntry++);

       direntry += 7; // skip date

       int flags = *direntry++;
       ++direntry; ++direntry; direntry += 4;

       int nl = *direntry++;

       char* name = DVDEntry->name;

       DVDEntry->sector = sector;
       DVDEntry->size = size;
       DVDEntry->flags = flags;

       if ((nl == 1) && (direntry[0] == 1)) // ".."
       {
               DVDEntry->name[0] = 0;
               if (last_current_dir != sector)
                       *added=1;
       }
       else if ((nl == 1) && (direntry[0] == 0))
       {
               last_current_dir = sector;
       }
       else
       {
		   if (is_unicode)
		   {
				   int i;
				   for (i = 0; i < (nl / 2); ++i)
						   name[i] = direntry[i * 2 + 1];
				   name[i] = 0;
				   nl = i;
		   }
		   else
		   {
				   memcpy(name, direntry, nl);
				   name[nl] = 0;
		   }

		   if (!(flags & 2))
		   {
				   if (name[nl - 2] == ';')
						   name[nl - 2] = 0;

				   int i = nl;
				   while (i >= 0)
						   if (name[i] == '.')
								   break;
						   else
								   --i;

				   ++i;

		   }
		   else
		   {
				   name[nl++] = '/';
				   name[nl] = 0;
		   }

		   *added = 1;
       }

       return nrb;
}



int read_directory(int sector, int len, file_entry **DVDToc)
{
  int ptr = 0, files = 0;
  unsigned char *sector_buffer = (unsigned char*)memalign(32,2048);
  read_sector(sector_buffer, sector);
  
  while (len > 0)
  {
	int added = 0;
	*DVDToc = realloc( *DVDToc, ((files)+1) * sizeof(file_entry) ); 
    ptr += read_direntry(sector_buffer + ptr, &(*DVDToc)[files], &added);
    if (ptr >= 2048 || !sector_buffer[ptr])
    {
      len -= 2048;
      sector++;
      read_sector(sector_buffer, sector);
      ptr = 0;
    }
	files += added;
  }
  free(sector_buffer);
  return files;
}

int dvd_read_directoryentries(uint64_t offset, int size, file_entry **DVDToc) {
	if(size + offset == 0) {
		int sector = 16;
		struct pvd_s* pvd = 0;
		struct pvd_s* svd = 0;

		while (sector < 32)
		{
			if (read_sector(dvdbuffer, sector))
			{
				return FATAL_ERROR;
			}
			if (!memcmp(((struct pvd_s *)dvdbuffer)->id, "\2CD001\1", 8))
			{
				  svd = (void*)dvdbuffer;
				  break;
			}
			++sector;
		}


		if (!svd)
		{
			sector = 16;
			while (sector < 32)
			{
				if (read_sector(dvdbuffer, sector))
				{
					return FATAL_ERROR;
				}

				if (!memcmp(((struct pvd_s *)dvdbuffer)->id, "\1CD001\1", 8))
				{
					pvd = (void*)dvdbuffer;
					break;
				}
				++sector;
			}
		}

		if ((!pvd) && (!svd))
		{
			return NO_ISO9660_DISC;
		}

		is_unicode = svd ? 1 : 0;
		int added = 0;
		*DVDToc = malloc(sizeof(file_entry));
		read_direntry(svd ? svd->root_direntry : pvd->root_direntry, &(*DVDToc)[0], &added);
		return read_directory((*DVDToc)[0].sector, (*DVDToc)[0].size, DVDToc) + added;
	}
  
	*DVDToc = malloc(sizeof(file_entry));
	int files = read_directory(offset>>11, size, DVDToc);

	return (files>0) ? files : NO_FILES;
}




