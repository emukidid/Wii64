/**
 * Mupen64 - compare_core.c
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

#ifdef COMPARE_CORE
#include <sys/stat.h>
#include <stdarg.h>
#include "r4300.h"
#include "../gc_memory/memory.h"
#include "../main/winlnxdefs.h"
#include "../main/plugin.h"

static FILE *f;
static int pipe_opened = 0;
static long long int comp_reg[32];
extern unsigned long op;
static unsigned long old_op;


void print_gecko(const char* fmt, ...)
{
	if(usb_isgeckoalive(1)) {
		char tempstr[2048];
		va_list arglist;
		va_start(arglist, fmt);
		vsprintf(tempstr, fmt, arglist);
		va_end(arglist);
		// write out over usb gecko ;)
		usb_sendbuffer_safe(1,tempstr,strlen(tempstr));
	}
}

void display_error(char *txt)
{
	int i;
	unsigned long *comp_reg2 = (unsigned long *) comp_reg;
	print_gecko("err: %s\n", txt);
	print_gecko("addr:%x\n", (int) r4300.pc);
	if(!strcmp(txt, "PC"))
		print_gecko("%x - %x\n", (int) r4300.pc, *(int*) &comp_reg[0]);
	print_gecko("%x, %x\n", (unsigned int) r4300.reg_cop0[9],
			(unsigned int) comp_reg2[9]);
	print_gecko("erreur @:%x\n", (int) old_op);
	print_gecko("erreur @:%x\n", (int) op);
   
   if (!strcmp(txt, "gpr"))
       {
	  for (i=0; i<32; i++)
	    {
	       if (r4300.gpr[i] != comp_reg[i])
		 print_gecko("reg[%d]=%llx != reg[%d]=%llx\n",
			i, r4300.gpr[i], i, comp_reg[i]);
	    }
       }
   if (!strcmp(txt, "cop0"))
       {
	  for (i=0; i<32; i++)
	    {
	       if (r4300.reg_cop0[i] != comp_reg2[i])
		 print_gecko("r4300.reg_cop0[%d]=%x != r4300.reg_cop0[%d]=%x\n",
			i, (unsigned int)r4300.reg_cop0[i], i, (unsigned int)comp_reg2[i]);
	    }
       }
   
   stop_it();
}

void compare_core()
{   
   if(dynacore) {
		if(!pipe_opened) {
			f = fopen("sd:/compare_core.bin", "r");
			pipe_opened = 1;
		}

		memset(comp_reg, 0x13, 4*sizeof(long));
		fread(comp_reg, 4, sizeof(long), f);
		if(memcmp(&r4300.pc, comp_reg, 4))
			display_error("PC");
		memset(comp_reg, 0x13, 32*sizeof(long long int));
		fread(comp_reg, 32, sizeof(long long int), f);
		if(memcmp(r4300.gpr, comp_reg, 32 * sizeof(long long int)))
			display_error("gpr");
		memset(comp_reg, 0x13, 32*sizeof(long));
		fread(comp_reg, 32, sizeof(long), f);
		if(memcmp(r4300.reg_cop0, comp_reg, 32 * sizeof(long)))
			display_error("cop0");
		old_op = op;
	} else {
		if(!pipe_opened) {
			remove("sd:/compare_core.bin");
			f = fopen("sd:/compare_core.bin", "wb");
			pipe_opened = 1;
		}

		fwrite(&r4300.pc, 4, sizeof(long), f);
		fwrite(r4300.gpr, 32, sizeof(long long int), f);
		fwrite(r4300.reg_cop0, 32, sizeof(long), f);
	}
}
#endif
