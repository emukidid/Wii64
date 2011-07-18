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

#include <sys/stat.h>
#include "r4300.h"
#include "../gc_memory/memory.h"
#include "../main/winlnxdefs.h"
#include "../main/plugin.h"

static FILE *f;
static int pipe_opened = 0;
static long long int comp_reg[32];
extern unsigned long op;
static unsigned long old_op;

void display_error(char *txt)
{
	int i;
	unsigned long *comp_reg2 = (unsigned long *) comp_reg;
	printf("err: %s\n", txt);
	printf("addr:%x\n", (int) r4300.pc);
	if(!strcmp(txt, "PC"))
		printf("%x - %x\n", (int) r4300.pc, *(int*) &comp_reg[0]);
	printf("%x, %x\n", (unsigned int) r4300.reg_cop0[9],
			(unsigned int) comp_reg2[9]);
	printf("erreur @:%x\n", (int) old_op);
	printf("erreur @:%x\n", (int) op);
   
   if (!strcmp(txt, "gpr"))
       {
	  for (i=0; i<32; i++)
	    {
	       if (r4300.gpr[i] != comp_reg[i])
		 printf("reg[%d]=%llx != reg[%d]=%llx\n",
			i, r4300.gpr[i], i, comp_reg[i]);
	    }
       }
   if (!strcmp(txt, "cop0"))
       {
	  for (i=0; i<32; i++)
	    {
	       if (r4300.reg_cop0[i] != comp_reg2[i])
		 printf("r4300.reg_cop0[%d]=%x != r4300.reg_cop0[%d]=%x\n",
			i, (unsigned int)r4300.reg_cop0[i], i, (unsigned int)comp_reg2[i]);
	    }
       }
   /*for (i=0; i<32; i++)
     {
	if (r4300.reg_cop0[i] != comp_reg[i])
	  printf("r4300.reg_cop0[%d]=%llx != reg[%d]=%llx\n",
		 i, r4300.reg_cop0[i], i, comp_reg[i]);
     }*/
   
   stop_it();
}

void check_input_sync(unsigned char *value)
{
   if(dynacore) {
		fread(value, 4, 1, f);
	} else {
		fwrite(value, 4, 1, f);
	}
}

void compare_core()
{   
   if(dynacore) {
		if(!pipe_opened) {
			mkfifo("compare_pipe", 0600);
			f = fopen("compare_pipe", "r");
			pipe_opened = 1;
		}
		/*if(wait == 1 && r4300.reg_cop0[9] > 0x35000000) wait=0;
		 if(wait) return;*/

		fread(comp_reg, 4, sizeof(long), f);
		if(memcmp(&r4300.pc, comp_reg, 4))
			display_error("PC");
		fread(comp_reg, 32, sizeof(long long int), f);
		if(memcmp(r4300.gpr, comp_reg, 32 * sizeof(long long int)))
			display_error("gpr");
		fread(comp_reg, 32, sizeof(long), f);
		if(memcmp(r4300.reg_cop0, comp_reg, 32 * sizeof(long)))
			display_error("cop0");
		fread(comp_reg, 32, sizeof(long long int), f);
		if(memcmp(r4300.fpr_data, comp_reg, 32 * sizeof(long long int)))
			display_error("cop1");
		/*fread(comp_reg, 1, sizeof(long), f);
		 if (memcmp(&rdram[0x31280/4], comp_reg, sizeof(long)))
		 display_error("mem");*/
		/*fread (comp_reg, 4, 1, f);
		 if (memcmp(&r4300.fcr31, comp_reg, 4))
		 display_error();*/
		old_op = op;
	} else {
		//if (r4300.reg_cop0[9] > 0x6800000) printf("PC=%x\n", (int)r4300.pc);
		if(!pipe_opened) {
			f = fopen("compare_pipe", "w");
			pipe_opened = 1;
		}
		/*if(wait == 1 && r4300.reg_cop0[9] > 0x35000000) wait=0;
		 if(wait) return;*/

		fwrite(&r4300.pc, 4, sizeof(long), f);
		fwrite(r4300.gpr, 32, sizeof(long long int), f);
		fwrite(r4300.reg_cop0, 32, sizeof(long), f);
		fwrite(r4300.fpr_data, 32, sizeof(long long int), f);
		//fwrite(&rdram[0x31280/4], 1, sizeof(long), f);
		/*fwrite(&r4300.fcr31, 4, 1, f);*/
	}
}
