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
#include "wii64config.h"

static void *game_specific_hack = 0;
static u32 zelda_subscreen_address = 0;

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

// World Driver Championship (U) //jal 8006ED70 osRecvMesg #14
void hack_worlddriver_u() {
	word = 0;
	address = 0x80023FE4;
	write_word_in_memory();
}

// World Driver Championship (E) //jal 800706D0 osRecvMesg #14
void hack_worlddriver_e() {
	word = 0;
	address = 0x800241D4;
	write_word_in_memory();
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

void hack_zelda_oot() {
	if(zelda_subscreen_address) {
		rdramb[zelda_subscreen_address] = 2;
	}
}


// Return a pointer to the game specific hack or 0 if there isn't any
void *GetGameSpecificHack() {
	return game_specific_hack;
}

static int old_count_per_op = -1;

void restore_count_per_op() {
	if(old_count_per_op != -1) {
		count_per_op = old_count_per_op;
	}	
}

// Game specific hack detection via CRC
void GameSpecificHackSetup() {
	unsigned int curCRC[2];
	ROMCache_read((u8*)&curCRC[0], 0x10, sizeof(unsigned int)*2);
	//print_gecko("CRC values %08X %08X\r\n", curCRC[0], curCRC[1]);
	if(curCRC[0] == 0xCA12B547 && curCRC[1] == 0x71FA4EE4) {
		game_specific_hack = &hack_pkm_snap_u;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0xEC0F690D && curCRC[1] == 0x32A7438C) {
		game_specific_hack = &hack_pkm_snap_j;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0x7BB18D40 && curCRC[1] == 0x83138559) {
		game_specific_hack = &hack_pkm_snap_a;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0x4FF5976F && curCRC[1] == 0xACF559D8) {
		game_specific_hack = &hack_pkm_snap_e;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0x5F3F49C6 && curCRC[1] == 0x0DC714B0) {
		game_specific_hack = &hack_topgear_hb_e;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0x845B0269 && curCRC[1] == 0x57DE9502) {
		game_specific_hack = &hack_topgear_hb_j;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0x8ECC02F0 && curCRC[1] == 0x7F8BDE81) {
		game_specific_hack = &hack_topgear_hb_u;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0xD09BA538 && curCRC[1] == 0x1C1A5489) {
		game_specific_hack = &hack_topgear_od_e;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0x0578F24F && curCRC[1] == 0x9175BF17) {
		game_specific_hack = &hack_topgear_od_j;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0xD741CD80 && curCRC[1] == 0xACA9B912) {
		game_specific_hack = &hack_topgear_od_u;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0x308DFEC8 && curCRC[1] == 0xCE2EB5F6) {
		game_specific_hack = &hack_worlddriver_u;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0xAC062778 && curCRC[1] == 0xDFADFCB8) {
		game_specific_hack = &hack_worlddriver_e;
		restore_count_per_op();
	}
	else if((curCRC[0] == 0x0CAD17E6 && curCRC[1] == 0x71A5B797) ||
			(curCRC[0] == 0x75A4E247 && curCRC[1] == 0x6008963D) ||
			(curCRC[0] == 0x6AA4DDE7 && curCRC[1] == 0xE3E2F4E7)) {
				old_count_per_op = count_per_op;
				count_per_op = COUNT_PER_OP_3;
	}
#ifndef RICE_GFX
	else if((curCRC[0] == 0xED98957E && curCRC[1] == 0x8242DCAC) ||
			(curCRC[0] == 0xD5898CAF && curCRC[1] == 0x6007B65B) ||
			(curCRC[0] == 0x1FA056E0 && curCRC[1] == 0xA4B9946A)) {
		game_specific_hack = &hack_winback;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0xC851961C && curCRC[1] == 0x78FCAAFA) {
		game_specific_hack = &hack_pilotwings_u;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0x09CC4801 && curCRC[1] == 0xE42EE491) {
		game_specific_hack = &hack_pilotwings_j;
		restore_count_per_op();
	}
	else if(curCRC[0] == 0x1AA05AD5 && curCRC[1] == 0x46F52D80) {
		game_specific_hack = &hack_pilotwings_e;
		restore_count_per_op();
	}
#endif
	else if(strncmp((char *)ROM_HEADER.Name, "THE LEGEND OF ZELDA", 19) == 0) {
		zelda_subscreen_address = 0;
        if (curCRC[0] == 0xEC7011B7 && curCRC[1] == 0x7616D72B) {
            // Legend of Zelda, The - Ocarina of Time (U) + (J) (V1.0)
            zelda_subscreen_address = 0x1DA5CB;
        } else if (curCRC[0] == 0xD43DA81F && curCRC[1] == 0x021E1E19) {
            // Legend of Zelda, The - Ocarina of Time (U) + (J) (V1.1)
            zelda_subscreen_address = 0x1DA78B;
        } else if (curCRC[0] == 0x693BA2AE && curCRC[1] == 0xB7F14E9F) {
            // Legend of Zelda, The - Ocarina of Time (U) + (J) (V1.2)
            zelda_subscreen_address = 0x1DAE8B;
        } else if (curCRC[0] == 0xB044B569 && curCRC[1] == 0x373C1985) {
            // Legend of Zelda, The - Ocarina of Time (E) (V1.0)
            zelda_subscreen_address = 0x1D860B;
         } else if (curCRC[0] == 0xB2055FBD && curCRC[1] == 0x0BAB4E0C) {
            // Legend of Zelda, The - Ocarina of Time (E) (V1.1)
            zelda_subscreen_address = 0x1D864B;
        // GC Versions
        } else if (curCRC[0] == 0x1D4136F3 && curCRC[1] == 0xAF63EEA9) {
            // Legend of Zelda, The - Ocarina of Time - Master Quest (E) (GC Version)
            zelda_subscreen_address = 0x1D8F4B;
        } else if (curCRC[0] == 0x09465AC3 && curCRC[1] == 0xF8CB501B) {
            // Legend of Zelda, The - Ocarina of Time (E) (GC Version)
            zelda_subscreen_address = 0x1D8F8B;
        } else if (curCRC[0] == 0xF3DD35BA && curCRC[1] == 0x4152E075) {
            // Legend of Zelda, The - Ocarina of Time (U) (GC Version) 
            zelda_subscreen_address = 0x1DB78B;
        } else if (curCRC[0] == 0xF034001A && curCRC[1] == 0xAE47ED06) {
            // Legend of Zelda, The - Ocarina of Time - Master Quest (U) (GC Version)
            zelda_subscreen_address = 0x1DB74B;
        } else if (curCRC[0] == 0xF7F52DB8 && curCRC[1] == 0x2195E636) {
            // Zelda no Densetsu - Toki no Ocarina - Zelda Collection Version (J) (GC Version) 
            zelda_subscreen_address = 0x1DB78B;
        } else if (curCRC[0] == 0xF611F4BA && curCRC[1] == 0xC584135C) {
            // Zelda no Densetsu - Toki no Ocarina GC (J) (GC Version) 
            zelda_subscreen_address = 0x1DB78B;
        } else if (curCRC[0] == 0xF43B45BA && curCRC[1] == 0x2F0E9B6F) {
            // Zelda no Densetsu - Toki no Ocarina GC Ura (J) (GC Version)
            zelda_subscreen_address = 0x1DB78B;
		}
		if(zelda_subscreen_address != 0) {
			game_specific_hack = &hack_zelda_oot;
		}
	}
	else {
		game_specific_hack = 0;
	}
	
	//print_gecko("Applied a hack? %s\r\n", game_specific_hack ? "yes" : "no");
}
