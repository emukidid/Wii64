/**
 * Mupen64 - rom_gc.c
 * Copyright (C) 2002 Hacktarux,
 * Wii/GC Additional code by tehpola, emu_kidid
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
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
#include <string.h>
#include <ctype.h>
#include <ogc/machine/processor.h>
#include "../gui/GUI.h"
#include "rom.h"
#include "ROM-Cache.h"
#include "wii64config.h"
#include "../gc_memory/memory.h"
#include "../fileBrowser/fileBrowser.h"
#include "gamehacks.h"

#define PRINT GUI_print

static fileBrowser_file* rom_file;
int rom_length;
rom_header ROM_HEADER;
rom_settings ROM_SETTINGS;

void stripInvalidChars(char *str) {
	int i = 0;
	for(i = 0; i < strlen(str); i++) {
		if(str[i] == '\\' || str[i] == '/' || str[i] == ':'|| str[i] == '*'
		|| str[i] == '?'|| str[i] == '"'|| str[i] == '<'|| str[i] == '>'|| str[i] == '|') {
			str[i] = '_';
		}
	}
}

int init_byte_swap(unsigned int magicWord){

	switch(magicWord){
		case 0x37804012:					//37804012 aka byteswapped
			return BYTE_SWAP_BYTE;
		case 0x40123780:					//40123780 aka little endian, aka halfswapped
			return BYTE_SWAP_HALF;
		case 0x80371240:
			return BYTE_SWAP_NONE;
	}
	return BYTE_SWAP_BAD;
}

void byte_swap(char* buffer, unsigned int length, int byte_swap_type) {
	if(byte_swap_type == BYTE_SWAP_NONE || byte_swap_type == BYTE_SWAP_BAD)
		return;

	int i = 0;
	
	if(byte_swap_type == BYTE_SWAP_HALF){	//aka little endian (40123780) vs (80371240)
		for (i = 0; i < (length & ~3); i += 4)
			*(u32 *)(buffer + i) = __lwbrx(buffer, i);
	} else if(byte_swap_type == BYTE_SWAP_BYTE){	// (37804012) vs (80371240)
		for (i = 0; i < (length & ~3); i += 2)
			*(u16 *)(buffer + i) = __lhbrx(buffer, i);
	}
}

static struct {
	int Cartridge_ID;
	char* Country_codes;
} ROM_TABLE[] = {
	{ 'D3', "J",   }, // Akumajou Dracula Mokushiroku - Real Action Adventure
	{ 'D4', "J",   }, // Akumajou Dracula Mokushiroku Gaiden - Legend of Cornell
	{ 'B7', "EJPU" }, // Banjo-Tooie
	{ 'GT', "J",   }, // City Tour Grandprix - Zennihon GT Senshuken
	{ 'FU', "EP",  }, // Conker's Bad Fur Day
	{ 'CW', "EP"   }, // Cruis'n World
	{ 'CZ', "J",   }, // Custom Robo V2
	{ 'D6', "J",   }, // Densha de GO! 64
	{ 'DO', "EJP"  }, // Donkey Kong 64
	{ 'D2', "J",   }, // Doraemon 2 - Nobita to Hikari no Shinden
	{ '3D', "J",   }, // Doraemon 3 - Nobita no Machi SOS!
	{ 'MX', "EJP"  }, // Excitebike 64
	{ 'X7', "E"    }, // GoldenEye X 5d
	{ 'GC', "EP"   }, // GT 64 - Championship Edition
	{ 'IM', "J",   }, // Ide Yousuke no Mahjong Juku
	{ 'K4', "EJP"  }, // Kirby 64 - The Crystal Shards
	{ 'NB', "EP"   }, // Kobe Bryant in NBA Courtside
	{ 'MV', "EJP"  }, // Mario Party 3
	{ 'M8', "EJP"  }, // Mario Tennis
	{ 'EV', "J"    }, // Neon Genesis Evangelion
	{ 'PP', "J"    }, // Parlor! Pro 64 - Pachinko Jikki Simulation Game
	{ 'UB', "J"    }, // PD Ultraman Battle Collection 64
	{ 'PD', "EJP"  }, // Perfect Dark
	{ 'R7', "J"    }, // Robot Ponkottsu 64 - 7tsu no Umi no Caramel
	{ 'RZ', "EP"   }, // RR64 - Ridge Racer 64
	{ 'EP', "EJP"  }, // Star Wars Episode I - Racer
	{ 'YS', "EJP"  }, // Yoshi's Story
};

// Checks if the current game is in the ID list for 16kbit eeprom save type
// cause it's cheaper to have a ID list than an entire .ini file :)
bool isEEPROM16k()
{
	int i;

	for (i = 0; i < sizeof(ROM_TABLE) / sizeof(ROM_TABLE[0]); i++)
	{
		if (ROM_TABLE[i].Cartridge_ID == ROM_HEADER.Cartridge_ID &&
			strchr(ROM_TABLE[i].Country_codes, ROM_HEADER.Country_code))
			return true;
	}

	return false;
}

/* Loads the ROM into the ROM cache */
int rom_read(fileBrowser_file* file){

   char buffer[1024];
   int i;

   rom_file = file;
   rom_length = file->size;

   ROMCache_init(rom_file);
   int ret = ROMCache_load(rom_file);
   if(ret) {
     ROMCache_deinit(rom_file);
     return ret;
   }
   ROMCache_read(&ROM_HEADER, 0, sizeof(rom_header));

  //Copy header name as Goodname (in the .ini we can use CRC to identify ROMS)
  memset((char*)buffer,0,1024);
  strncpy(buffer, (char*)ROM_HEADER.Name,32);
  //Maximum ROM name is 32 bytes. Lets make sure we cut off trailing spaces
  for(i = strlen(buffer); i>0; i--)
  {
    if(buffer[i-1] !=  ' ') {
  		strncpy(&ROM_SETTINGS.goodname[0],&buffer[0],i);
  		ROM_SETTINGS.goodname[i] = 0; //terminate it too
  		break;
    }
  }
  // Replace any non file system complaint chars with underscores
  stripInvalidChars((char*)&ROM_SETTINGS.goodname[0]);
  // Fix save type for certain special sized (16kbit) eeprom games
  ROM_SETTINGS.eeprom_16kb = isEEPROM16k();
  // Apply game specific hacks
  GameSpecificHackSetup();
  //Set VI limit based on ROM header
  InitTimer();
	
  return ret;
}

#define tr 
void countrycodestring(unsigned short countrycode, char *string)
{
    switch (countrycode)
    {
    case 0:    /* Demo */
        strcpy(string, ("Demo"));
        break;

    case '7':  /* Beta */
        strcpy(string, ("Beta"));
        break;

    case 0x41: /* Japan / USA */
        strcpy(string, ("USA/Japan"));
        break;

    case 0x44: /* Germany */
        strcpy(string, ("Germany"));
        break;

    case 0x45: /* USA */
        strcpy(string, ("USA"));
        break;

    case 0x46: /* France */
        strcpy(string, ("France"));
        break;

    case 'I':  /* Italy */
        strcpy(string, ("Italy"));
        break;

    case 0x4A: /* Japan */
        strcpy(string, ("Japan"));
        break;

    case 'S':  /* Spain */
        strcpy(string, ("Spain"));
        break;

    case 0x55: case 0x59:  /* Australia */
        sprintf(string, ("Australia (0x%2.2X)"), countrycode);
        break;

    case 0x50: case 0x58: case 0x20:
    case 0x21: case 0x38: case 0x70:
        sprintf(string, ("Europe (0x%02X)"), countrycode);
        break;

    default:
        sprintf(string, ("Unknown (0x%02X)"), countrycode);
        break;
    }
}

char *saveregionstr()
{
    switch (ROM_HEADER.Country_code&0xFF)
    {
    case 0:    /* Demo */
        return "(Demo)";
        break;
    case '7':  /* Beta */
        return "(Beta)";
        break;
    case 0x41: /* Japan / USA */
        return "(JU)";
        break;
    case 0x44: /* Germany */
        return "(G)";
        break;
    case 0x45: /* USA */
        return "(U)";
        break;
    case 0x46: /* France */
        return "(F)";
        break;
    case 'I':  /* Italy */
        return "(I)";
        break;
    case 0x4A: /* Japan */
        return "(J)";
        break;
    case 'S':  /* Spain */
        return "(S)";
        break;
    case 0x55: case 0x59:  /* Australia */
        return "(A)";
        break;
    case 0x50: case 0x58: case 0x20:
    case 0x21: case 0x38: case 0x70:
        return "(E)";
        break;
    default:
        return "(Unk)";
        break;
    }
}

