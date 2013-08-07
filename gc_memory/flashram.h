/**
 * Mupen64 - flashram.c
 * Copyright (C) 2002 Hacktarux
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

typedef enum flashram_mode
{
   NOPES_MODE = 0,
   ERASE_MODE,
   WRITE_MODE,
   READ_MODE,
   STATUS_MODE
} Flashram_mode;

typedef struct {
	int use_flashram;
	int mode;
	unsigned long long status;
	unsigned long erase_offset;
	unsigned long write_pointer;
} _FlashRAMInfo;
extern _FlashRAMInfo flashRAMInfo;


void init_flashram();
void reset_flashram();
void flashram_command(unsigned long command);
unsigned long flashram_status();
void dma_read_flashram();
void dma_write_flashram();

