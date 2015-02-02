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
/*
// Pokemon Snap (U)
void hack_pkm_snap_u() {
	// Pass 1st Level and Controller Fix
	if(read_hword_in_memory(0x80382D1C, 0) == 0x802C) {
		write_byte_in_memory(0x80382D0F, 0);
	}
	//Make Picture selectable
	if(read_hword_in_memory(0x801E3184, 0) == 0x2881) {
		write_hword_in_memory(0x801E3184, 0x2001);
	}
	if(read_hword_in_memory(0x801E3186, 0) == 0x0098) {
		write_hword_in_memory(0x801E3186, 0x0001);
	}
}


// Pokemon Snap (A)
void hack_pkm_snap_a() {
	// Pass 1st Level and Controller Fix
	if(read_hword_in_memory(0x80382D1C, 0) == 0x802C) {
		write_byte_in_memory(0x80382D0F, 0);
	}
	//Make Picture selectable
	if(read_hword_in_memory(0x801E3C44, 0) == 0x2881) {
		write_hword_in_memory(0x801E3C44, 0x2001);
	}
	if(read_hword_in_memory(0x801E3C46, 0) == 0x0098) {
		write_hword_in_memory(0x801E3C46, 0x0001);
	}
}

// Pokemon Snap (E)
void hack_pkm_snap_e() {
	// Pass 1st Level and Controller Fix
	if(read_hword_in_memory(0x80381BFC, 0) == 0x802C) {
		write_byte_in_memory(0x80381BEF, 0);
	}
	//Make Picture selectable
	if(read_hword_in_memory(0x801E3824, 0) == 0x2881) {
		write_hword_in_memory(0x801E3824, 0x2001);
	}
	if(read_hword_in_memory(0x801E3826, 0) == 0x0098) {
		write_hword_in_memory(0x801E3826, 0x0001);
	}
}

// Pocket Monsters Snap (J)
void hack_pkm_snap_j() {
	// Pass 1st Level and Controller Fix
	if(read_hword_in_memory(0x8036D22C, 0) == 0x802A) {
		write_byte_in_memory(0x8036D21F, 0);
	}
	//Make Picture selectable
	if(read_hword_in_memory(0x801E1EC4, 0) == 0x2881) {
		write_hword_in_memory(0x801E1EC4, 0x2001);
	}
	if(read_hword_in_memory(0x801E1EC6, 0) == 0x0098) {
		write_hword_in_memory(0x801E1EC6, 0x0001);
	}
}

// Top Gear Hyper-Bike (E) 
void hack_topgear_hb_e() {
	//Game Playable Fix (Gent)
	if(read_hword_in_memory(0x800021EE, 0) == 0x0001) {
		write_byte_in_memory(0x800021EE, 0);
	}
}

// Top Gear Hyper-Bike (J) 
void hack_topgear_hb_j() {
	//Game Playable Fix (Gent)
	if(read_hword_in_memory(0x8000225A, 0) == 0x0001) {
		write_byte_in_memory(0x8000225A, 0);
	}
}

// Top Gear Hyper-Bike (U) 
void hack_topgear_hb_u() {
	//Game Playable Fix (Gent)
	if(read_hword_in_memory(0x800021EA, 0) == 0x0001) {
		write_byte_in_memory(0x800021EA, 0);
	}
}

// Top Gear Overdrive (E) 
void hack_topgear_od_e() {
	//Game Playable Fix (Gent)
	if(read_hword_in_memory(0x80001AB2, 0) == 0x0001) {
		write_byte_in_memory(0x80001AB2, 0);
	}
}

// Top Gear Overdrive (J) 
void hack_topgear_od_j() {
	//Game Playable Fix (Gent)
	if(read_hword_in_memory(0x80001B4E, 0) == 0x0001) {
		write_byte_in_memory(0x80001B4E, 0);
	}
}

// Top Gear Overdrive (U) 
void hack_topgear_od_u() {
	//Game Playable Fix (Gent)
	if(read_hword_in_memory(0x80001B4E, 0) == 0x0001) {
		write_byte_in_memory(0x80001B4E, 0);
	}
}
*/
// Return a pointer to the game specific hack or 0 if there isn't any
void *GetGameSpecificHack() {
	return game_specific_hack;
}

// Game specific hack detection via CRC
void GameSpecificHackSetup() {
	/*unsigned int curCRC[2];
	ROMCache_read((u8*)&curCRC[0], 0x10, sizeof(unsigned int)*2);
	
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
	else {*/
		game_specific_hack = 0;
	/*}*/
}
