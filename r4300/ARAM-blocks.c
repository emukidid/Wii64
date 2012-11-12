/**
 * Wii64 - ARAM-blocks.c
 * Copyright (C) 2007, 2008, 2009, 2010 emu_kidid
 * 
 * DELETE ME
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
#include <gctypes.h>
#include "ppc/Recompile.h"

inline PowerPC_block* blocks_get(u32 addr){
  return blocks[addr];
}


inline void blocks_set(u32 addr, PowerPC_block* ptr){
  blocks[addr] = ptr;
}


