/**
 * Mupen64 - savestates.h
 * Copyright (C) 2002 Hacktarux
 * Copyright (C) 2008, 2009 emu_kidid
 * Copyright (C) 2013 sepp256
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *                emukidid@gmail.com
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

#define SAVESTATE 1
#define LOADSTATE 2

# define FB_THUMB_WD 160
# define FB_THUMB_HT 120
# define FB_THUMB_BPP 2 //4 or 2
# define FB_THUMB_FMT GX_TF_RGB565 //GX_TF_RGBA8 or GX_TF_RGB565
# define FB_THUMB_SIZE (FB_THUMB_WD*FB_THUMB_HT*FB_THUMB_BPP)

void savestates_save(unsigned int slot, u8* fb_tex);
int savestates_load_header(unsigned int slot, u8* fb_tex, char* date, char* time);
int savestates_load(unsigned int slot);
void savestates_queue_load(unsigned int slot);
int  savestates_exists(unsigned int slot, int mode);

void savestates_select_slot(unsigned int s);
void savestates_select_filename();
int savestates_queued_load();
