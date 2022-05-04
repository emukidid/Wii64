/**
 * Wii64 - gamehacks.c
 * Copyright (C) 2012 emu_kidid
 * 
 * Game specific hacks to workaround core inaccuracy
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
#include "winlnxdefs.h"
#include "rom.h"
#include "../gc_memory/memory.h"
#include "ROM-Cache.h"
#include "../r4300/r4300.h"

void *game_specific_hack = 0;

void compare_hword_write_hword(u32 compAddr, u32 destAddr, u16 compVal, u16 destVal) {
	address = compAddr;
	read_hword_in_memory();
	if(hword == compVal) {
		address = destAddr;
		hword = destVal;
		write_hword_in_memory();
	}
}

void compare_hword_write_byte(u32 compAddr, u32 destAddr, u16 compVal, u8 destVal) {
	address = compAddr;
	read_hword_in_memory();
	if(hword == compVal) {
		address = destAddr;
		byte = destVal;
		write_byte_in_memory();
	}
}

void compare_byte_write_byte(u32 addr, u8 compVal, u8 destVal) {
	address = addr;
	read_byte_in_memory();
	if(byte == compVal) {
		byte = destVal;
		write_byte_in_memory();
	}
}

// Pokemon Snap (U)
void hack_pkm_snap_u() {
	// Pass 1st Level and Controller Fix
	compare_hword_write_byte(0x80382D1C, 0x80382D0F, 0x802C, 0);

	//Make Picture selectable
	compare_hword_write_hword(0x801E3184, 0x801E3184, 0x2881, 0x2001);
	compare_hword_write_hword(0x801E3186, 0x801E3186, 0x0098, 0x0001);
}


// Pokemon Snap (A)
void hack_pkm_snap_a() {
	// Pass 1st Level and Controller Fix
	compare_hword_write_byte(0x80382D1C, 0x80382D0F, 0x802C, 0);

	//Make Picture selectable
	compare_hword_write_hword(0x801E3C44, 0x801E3C44, 0x2881, 0x2001);
	compare_hword_write_hword(0x801E3C46, 0x801E3C46, 0x0098, 0x0001);
}

// Pokemon Snap (E)
void hack_pkm_snap_e() {
	// Pass 1st Level and Controller Fix
	compare_hword_write_byte(0x80381BFC, 0x80381BEF, 0x802C, 0);

	//Make Picture selectable
	compare_hword_write_hword(0x801E3824, 0x801E3824, 0x2881, 0x2001);
	compare_hword_write_hword(0x801E3826, 0x801E3826, 0x0098, 0x0001);
}

// Pocket Monsters Snap (J)
void hack_pkm_snap_j() {
	// Pass 1st Level and Controller Fix
	compare_hword_write_byte(0x8036D22C, 0x8036D21F, 0x802A, 0);

	//Make Picture selectable
	compare_hword_write_hword(0x801E1EC4, 0x801E1EC4, 0x2881, 0x2001);
	compare_hword_write_hword(0x801E1EC6, 0x801E1EC6, 0x0098, 0x0001);
}

// Top Gear Hyper-Bike (E) 
void hack_topgear_hb_e() {
	//Game Playable Fix (Gent)
	compare_hword_write_byte(0x800021EE, 0x800021EE, 0x0001, 0);
}

// Top Gear Hyper-Bike (J) 
void hack_topgear_hb_j() {
	//Game Playable Fix (Gent)
	compare_hword_write_byte(0x8000225A, 0x8000225A, 0x0001, 0);
}

// Top Gear Hyper-Bike (U) 
void hack_topgear_hb_u() {
	//Game Playable Fix (Gent)
	compare_hword_write_hword(0x800021EA, 0x800021EA, 0x0001, 0);
}

// Top Gear Overdrive (E) 
void hack_topgear_od_e() {
	//Game Playable Fix (Gent)
	compare_hword_write_hword(0x80001AB2, 0x80001AB2, 0x0001, 0);
}

// Top Gear Overdrive (J) 
void hack_topgear_od_j() {
	//Game Playable Fix (Gent)
	hword = 0;
	address = 0x800F7BD4;
	write_hword_in_memory();
	address = 0x800F7BD6;
	write_hword_in_memory();
	address = 0x80001B4E;
	write_hword_in_memory();
}

// Top Gear Overdrive (U) 
void hack_topgear_od_u() {
	//Game Playable Fix (Gent)
	compare_hword_write_hword(0x80001B4E, 0x80001B4E, 0x0001, 0);
}

void hack_winback() {
	//NOP
}

#ifndef RICE_GFX
// Pilot Wings 64 (U)
void hack_pilotwings_u() {
	//Removes annoying shadow that appears below planes
	compare_byte_write_byte(0x80263B41, 0x12, 0xFF);
	compare_byte_write_byte(0x802643C1, 0x12, 0xFF);
	compare_byte_write_byte(0x80264581, 0x12, 0xFF);
	compare_byte_write_byte(0x80263FC1, 0x12, 0xFF);
	compare_byte_write_byte(0x80263E81, 0x12, 0xFF);
	compare_byte_write_byte(0x80263E41, 0x12, 0xFF);
	compare_byte_write_byte(0x80264181, 0x12, 0xFF);
	compare_byte_write_byte(0x80264381, 0x12, 0xFF);
	compare_byte_write_byte(0x80264281, 0x12, 0xFF);
	compare_byte_write_byte(0x802639C1, 0x20, 0xFF);
	compare_byte_write_byte(0x802640C1, 0x20, 0xFF);
	compare_byte_write_byte(0x80264541, 0x20, 0xFF);
	compare_byte_write_byte(0x80264201, 0x20, 0xFF);
	compare_byte_write_byte(0x80263D81, 0x20, 0xFF);
	compare_byte_write_byte(0x80263F81, 0x20, 0xFF);
	compare_byte_write_byte(0x80263A81, 0x29, 0xFF);
	compare_byte_write_byte(0x80264101, 0x29, 0xFF);
	compare_byte_write_byte(0x80263E01, 0x29, 0xFF);
	compare_byte_write_byte(0x80264201, 0x29, 0xFF);
	compare_byte_write_byte(0x80264441, 0x29, 0xFF);
	compare_byte_write_byte(0x80263F81, 0x29, 0xFF);
	compare_byte_write_byte(0x802647C1, 0x29, 0xFF);
	compare_byte_write_byte(0x80264941, 0x29, 0xFF);
	compare_byte_write_byte(0x80264281, 0x29, 0xFF);
	compare_byte_write_byte(0x802639C1, 0x3D, 0xFF);
	compare_byte_write_byte(0x802641C1, 0x3D, 0xFF);
	compare_byte_write_byte(0x80263D41, 0x3D, 0xFF);
	compare_byte_write_byte(0x80263EC1, 0x3D, 0xFF);
	compare_byte_write_byte(0x80263F01, 0x3D, 0xFF);
}

// Pilot Wings 64 (E)
void hack_pilotwings_e() {
	//Removes annoying shadow that appears below planes
	compare_byte_write_byte(0x80264361, 0x12, 0xFF);
	compare_byte_write_byte(0x80264BE1, 0x12, 0xFF);
	compare_byte_write_byte(0x80264DA1, 0x12, 0xFF);
	compare_byte_write_byte(0x802647E1, 0x12, 0xFF);
	compare_byte_write_byte(0x802646A1, 0x12, 0xFF);
	compare_byte_write_byte(0x80264661, 0x12, 0xFF);
	compare_byte_write_byte(0x802649A1, 0x12, 0xFF);
	compare_byte_write_byte(0x80264BA1, 0x12, 0xFF);
	compare_byte_write_byte(0x80264AA1, 0x12, 0xFF);
	compare_byte_write_byte(0x802641E1, 0x20, 0xFF);
	compare_byte_write_byte(0x802648E1, 0x20, 0xFF);
	compare_byte_write_byte(0x80264D61, 0x20, 0xFF);
	compare_byte_write_byte(0x80264A21, 0x20, 0xFF);
	compare_byte_write_byte(0x802645A1, 0x20, 0xFF);
	compare_byte_write_byte(0x802647A1, 0x20, 0xFF);
	compare_byte_write_byte(0x802642A1, 0x29, 0xFF);
	compare_byte_write_byte(0x80264921, 0x29, 0xFF);
	compare_byte_write_byte(0x80264621, 0x29, 0xFF);
	compare_byte_write_byte(0x80264A21, 0x29, 0xFF);
	compare_byte_write_byte(0x80264C61, 0x29, 0xFF);
	compare_byte_write_byte(0x802647A1, 0x29, 0xFF);
	compare_byte_write_byte(0x80264FE1, 0x29, 0xFF);
	compare_byte_write_byte(0x80265161, 0x29, 0xFF);
	compare_byte_write_byte(0x80264AA1, 0x29, 0xFF);
	compare_byte_write_byte(0x802641E1, 0x3D, 0xFF);
	compare_byte_write_byte(0x802649E1, 0x3D, 0xFF);
	compare_byte_write_byte(0x80264561, 0x3D, 0xFF);
	compare_byte_write_byte(0x802646E1, 0x3D, 0xFF);
	compare_byte_write_byte(0x80264721, 0x3D, 0xFF);
}

// Pilot Wings 64 (J)
void hack_pilotwings_j() {
	//Removes annoying shadow that appears below planes
	compare_byte_write_byte(0x802639B1, 0x12, 0xFF);
	compare_byte_write_byte(0x80264231, 0x12, 0xFF);
	compare_byte_write_byte(0x802643F1, 0x12, 0xFF);
	compare_byte_write_byte(0x80263E31, 0x12, 0xFF);
	compare_byte_write_byte(0x80263CF1, 0x12, 0xFF);
	compare_byte_write_byte(0x80263CB1, 0x12, 0xFF);
	compare_byte_write_byte(0x80263FF1, 0x12, 0xFF);
	compare_byte_write_byte(0x802641F1, 0x12, 0xFF);
	compare_byte_write_byte(0x802640F1, 0x12, 0xFF);
	compare_byte_write_byte(0x80263831, 0x20, 0xFF);
	compare_byte_write_byte(0x80263F31, 0x20, 0xFF);
	compare_byte_write_byte(0x802643B1, 0x20, 0xFF);
	compare_byte_write_byte(0x80264071, 0x20, 0xFF);
	compare_byte_write_byte(0x80263BF1, 0x20, 0xFF);
	compare_byte_write_byte(0x80263DF1, 0x20, 0xFF);
	compare_byte_write_byte(0x802638F1, 0x29, 0xFF);
	compare_byte_write_byte(0x80263F71, 0x29, 0xFF);
	compare_byte_write_byte(0x80263C71, 0x29, 0xFF);
	compare_byte_write_byte(0x80264071, 0x29, 0xFF);
	compare_byte_write_byte(0x802642B1, 0x29, 0xFF);
	compare_byte_write_byte(0x80263DF1, 0x29, 0xFF);
	compare_byte_write_byte(0x80264631, 0x29, 0xFF);
	compare_byte_write_byte(0x802647B1, 0x29, 0xFF);
	compare_byte_write_byte(0x802640F1, 0x29, 0xFF);
	compare_byte_write_byte(0x80263831, 0x3D, 0xFF);
	compare_byte_write_byte(0x80264031, 0x3D, 0xFF);
	compare_byte_write_byte(0x80263BB1, 0x3D, 0xFF);
	compare_byte_write_byte(0x80263D31, 0x3D, 0xFF);
	compare_byte_write_byte(0x80263D71, 0x3D, 0xFF);
}
#endif


// Return a pointer to the game specific hack or 0 if there isn't any
void *GetGameSpecificHack() {
	return game_specific_hack;
}

// Game specific hack detection via CRC
void GameSpecificHackSetup() {
	unsigned int curCRC[2];
	ROMCache_read((u8*)&curCRC[0], 0x10, sizeof(unsigned int)*2);
	//print_gecko("CRC values %08X %08X\r\n", curCRC[0], curCRC[1]);
	if(curCRC[0] == 0xCA12B547 && curCRC[1] == 0x71FA4EE4) {
		game_specific_hack = &hack_pkm_snap_u;
	}
	else if(curCRC[0] == 0xEC0F690D && curCRC[1] == 0x32A7438C) {
		game_specific_hack = &hack_pkm_snap_j;
	}
	else if(curCRC[0] == 0x7BB18D40 && curCRC[1] == 0x83138559) {
		game_specific_hack = &hack_pkm_snap_a;
	}
	else if(curCRC[0] == 0x4FF5976F && curCRC[1] == 0xACF559D8) {
		game_specific_hack = &hack_pkm_snap_e;
	}
	else if(curCRC[0] == 0x5F3F49C6 && curCRC[1] == 0x0DC714B0) {
		game_specific_hack = &hack_topgear_hb_e;
	}
	else if(curCRC[0] == 0x845B0269 && curCRC[1] == 0x57DE9502) {
		game_specific_hack = &hack_topgear_hb_j;
	}
	else if(curCRC[0] == 0x8ECC02F0 && curCRC[1] == 0x7F8BDE81) {
		game_specific_hack = &hack_topgear_hb_u;
	}
	else if(curCRC[0] == 0xD09BA538 && curCRC[1] == 0x1C1A5489) {
		game_specific_hack = &hack_topgear_od_e;
	}
	else if(curCRC[0] == 0x0578F24F && curCRC[1] == 0x9175BF17) {
		game_specific_hack = &hack_topgear_od_j;
	}
	else if(curCRC[0] == 0xD741CD80 && curCRC[1] == 0xACA9B912) {
		game_specific_hack = &hack_topgear_od_u;
	}
#ifndef RICE_GFX
	else if((curCRC[0] == 0xED98957E && curCRC[1] == 0x8242DCAC) ||
			(curCRC[0] == 0xD5898CAF && curCRC[1] == 0x6007B65B) ||
			(curCRC[0] == 0x1FA056E0 && curCRC[1] == 0xA4B9946A)) {
		game_specific_hack = &hack_winback;
	}
	else if(curCRC[0] == 0xC851961C && curCRC[1] == 0x78FCAAFA) {
		game_specific_hack = &hack_pilotwings_u;
	}
	else if(curCRC[0] == 0x09CC4801 && curCRC[1] == 0xE42EE491) {
		game_specific_hack = &hack_pilotwings_j;
	}
	else if(curCRC[0] == 0x1AA05AD5 && curCRC[1] == 0x46F52D80) {
		game_specific_hack = &hack_pilotwings_e;
	}
#endif
	else {
		game_specific_hack = 0;
	}
	//print_gecko("Applied a hack? %s\r\n", game_specific_hack ? "yes" : "no");
}
