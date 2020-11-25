/**
 * Wii64 - ARAM.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * This is the ARAM manager
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
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


#ifndef ARAM_H
#define ARAM_H

#ifdef MEM2_H
#error MEM2 and ARAM included in the same code, FAIL!
#endif

#define MB (1024*1024)
#define KB (1024)

#ifdef USE_EXPANSION
	#define MRAM_BACKING	(1*MB)			// Use 1MB to page our 16MB if expansion pak
#else
	#define MRAM_BACKING	(2*MB)			// Use 2MB to page our 16MB
#endif

#define ARAM_RESERVED	(64*KB)			// Reserved for DSP/AESND/etc

#define ARAM_VM_BASE	(0x7F000000)	// Map ARAM to here
#define ARAM_START		(ARAM_RESERVED + ARAM_VM_BASE) 
#define ARAM_SIZE		((16*MB) - ARAM_RESERVED)	// ARAM is ~16MB

// 4MB Blocks
#define BLOCKS_LO 		(ARAM_START)
#define BLOCKS_HI 		(BLOCKS_LO + 4*MB)

// 1MB Invalid Code
#define INVCODE_LO		(BLOCKS_HI)
#define INVCODE_HI		(INVCODE_LO + 1*MB)

// 128KB FlashROM
#define FLASHRAM_LO		(INVCODE_HI)
#define FLASHRAM_HI		(FLASHRAM_LO + 128*KB)

// 32KB SRAM
#define SRAM_LO			(FLASHRAM_HI)
#define SRAM_HI			(SRAM_LO + 32*KB)

// 32*4 MEMPAK
#define MEMPACK_LO		(SRAM_HI)
#define MEMPACK_HI		(MEMPACK_LO + 128*KB)

// 2MB RecompCache Meta
#define RECOMPMETA_LO   (MEMPACK_HI)
#define RECOMPMETA_SIZE	((2*MB) + (512*KB))
#define RECOMPMETA_HI   (RECOMPMETA_LO + RECOMPMETA_SIZE)

// 8MB ROM Cache (fill up the rest of ARAM)
#define ROMCACHE_LO		(RECOMPMETA_HI)
#define ROMCACHE_SIZE	(8*MB)
#define ROMCACHE_HI		(ROMCACHE_LO + ROMCACHE_SIZE)

#endif

