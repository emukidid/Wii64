/**
 * Mupen64 hle rsp - main.c
 * Code changes  2010 wii64Team
 * Copyright (C) 2002 Hacktarux
 *
 * Wii64 homepage: http
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
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

#ifdef __WIN32__
#include <windows.h>
#include "./winproject/resource.h"
#include "./win/win.h"
#else
#ifdef USE_GTK
#include <gtk/gtk.h>
#endif
#include "wintypes.h"
#include <string.h>
#endif
#include <stdio.h>

#include "../gui/DEBUG.h"

#ifdef __PPC__
#include "RSPPlugin.h"
#endif

#include "Rsp_#1.1.h"
#include "hle.h"

#include "Audio_#1.1.h"

//#define DEBUG_RSP 1

RSP_INFO rsp;

BOOL AudioHle = FALSE, GraphicsHle = TRUE, SpecificHle = FALSE;

#ifdef __WIN32__
extern void (*processAList)();
static BOOL firstTime = TRUE;
void loadPlugin();
#endif

__declspec(dllexport) void CloseDLL (void) {}

__declspec(dllexport) void DllAbout ( HWND hParent ) 
{
#ifdef __WIN32__
	MessageBox(NULL, "Mupen64 HLE RSP plugin v0.2 with Azimers code by Hacktarux", "RSP HLE", MB_OK);
#else
#ifdef USE_GTK
	char tMsg[256];
	GtkWidget *dialog, *label, *okay_button;

	dialog = gtk_dialog_new();
	sprintf(tMsg,"Mupen64 HLE RSP plugin v0.2 with Azimers code by Hacktarux");
	label = gtk_label_new(tMsg);
	okay_button = gtk_button_new_with_label("OK");

	gtk_signal_connect_object(GTK_OBJECT(okay_button), "clicked",
				 GTK_SIGNAL_FUNC(gtk_widget_destroy),
				 GTK_OBJECT(dialog));
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area),
			 okay_button);
	
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),
			 label);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_widget_show_all(dialog);
#else
	char tMsg[256];
	sprintf(tMsg,"Mupen64 HLE RSP plugin v0.2 with Azimers code by Hacktarux");
	//fprintf(stderr, "About\n%s\n", tMsg);
#endif
#endif
}

__declspec(dllexport) void DllConfig ( HWND hParent )
{
#ifdef __WIN32__
	if (firstTime)
	DialogBox(dll_hInstance,
							MAKEINTRESOURCE(IDD_RSPCONFIG), hParent, ConfigDlgProc);
	//MessageBox(NULL, "no config", "noconfig", MB_OK);
#endif
}

__declspec(dllexport) void DllTest ( HWND hParent )
{
#ifdef __WIN32__
	MessageBox(NULL, "no test", "no test", MB_OK);
#endif
}

static int audio_ucode_detect(OSTask_t *task)
{
	if (*(unsigned long*)(rsp.RDRAM + task->ucode_data + 0) != 0x1) {
		if (*(rsp.RDRAM + task->ucode_data + (0 ^ (3-S8))) == 0xF) {
	 		return 4;
 		}
		else {
	 		return 3;
 		}
	}
	else {
		if (*(unsigned long*)(rsp.RDRAM + task->ucode_data + 0x30) == 0xF0000F00) {
	 		return 1;
 		}
		else {
	 		return 2;
 		}
	}
}

extern void (*ABI1[0x20])();
extern void (*ABI2[0x20])();
extern void (*ABI3[0x20])();

void (*ABI[0x20])();

u32 inst1, inst2;

static int audio_ucode(OSTask_t *task)
{
	unsigned long *p_alist = (unsigned long*)(rsp.RDRAM + task->data_ptr);
	unsigned int i;

	switch(audio_ucode_detect(task)) {
	case 1: // mario ucode
		memcpy( ABI, ABI1, sizeof(ABI[0])*0x20 );
		break;
	case 2: // banjo kazooie ucode
		memcpy( ABI, ABI2, sizeof(ABI[0])*0x20 );
		break;
	case 3: // zelda ucode
		memcpy( ABI, ABI3, sizeof(ABI[0])*0x20 );
		break;
	default: // unknown ucode
		return -1;
	}

	// Execute the task using the correct HLE funcs
	for (i = 0; i < (task->data_size/4); i += 2) {
		inst1 = p_alist[i];
		inst2 = p_alist[i+1];
		ABI[inst1 >> 24]();
	}

	return 0;
}

__declspec(dllexport) DWORD DoRspCycles ( DWORD Cycles )
{
	OSTask_t *task = (OSTask_t*)(rsp.DMEM + 0xFC0);
	unsigned int i, sum=0;
#ifdef __WIN32__
	if(firstTime) {
		firstTime=FALSE;
		if (SpecificHle)
				loadPlugin();
	}
#endif
	// Process DList
	if( task->type == 1 && task->data_ptr != 0 && GraphicsHle) {
		if (rsp.ProcessDlistList != NULL) {
			rsp.ProcessDlistList();
		}
		*rsp.SP_STATUS_REG |= 0x0203;
		if ((*rsp.SP_STATUS_REG & 0x40) != 0 ) {
			*rsp.MI_INTR_REG |= 0x1;
			rsp.CheckInterrupts();
		}

		*rsp.DPC_STATUS_REG &= ~0x0002;
		return Cycles;
	} 
	// Process AList
	else if (task->type == 2 && AudioHle) {
#ifdef __WIN32__
		if (SpecificHle)
				processAList();
		else
#endif
		if (rsp.ProcessAlistList != NULL) {
			rsp.ProcessAlistList();
		}
		*rsp.SP_STATUS_REG |= 0x0203;
		if ((*rsp.SP_STATUS_REG & 0x40) != 0 ) {
			*rsp.MI_INTR_REG |= 0x1;
			rsp.CheckInterrupts();
		}
		return Cycles;
	} 
	// Process CPU Framebuffer
	else if (task->type == 7) {
		rsp.ShowCFB();
	}

	// Check for Interrupts if required
	*rsp.SP_STATUS_REG |= 0x203;
	if ((*rsp.SP_STATUS_REG & 0x40) != 0 ) {
		*rsp.MI_INTR_REG |= 0x1;
		rsp.CheckInterrupts();
	 } 

	// Boot code task
	if (task->ucode_size > 0x1000) {
		switch(rsp.IMEM[4]) {
			case 0x16: // banjo tooie (U) boot code
			{
#ifdef DEBUG_RSP
				sprintf(txtbuffer, "RSP: Tooie (U) Boot sum: %08X\r\n",sum);
				DEBUG_print(txtbuffer, DBG_USBGECKO);
#endif
				int i,j;
			 	memcpy(rsp.IMEM + 0x120, rsp.RDRAM + 0x1e8, 0x1e8);
			 	for (j=0; j<0xfc; j++) {
					for (i=0; i<8; i++) {
						*(rsp.RDRAM+((0x2fb1f0+j*0xff0+i)^S8))=*(rsp.IMEM+((0x120+j*8+i)^S8));
					}
				}
			 	return Cycles;
			}
			case 0x26: // banjo tooie (E) + zelda oot (E) boot code
			{
#ifdef DEBUG_RSP
				sprintf(txtbuffer, "RSP: Tooie,OOT (E) Boot sum: %08X\r\n",sum);
				DEBUG_print(txtbuffer, DBG_USBGECKO);
#endif
				int i,j;
			 	memcpy(rsp.IMEM + 0x120, rsp.RDRAM + 0x1e8, 0x1e8);
			 	for (j=0; j<0xfc; j++) {
					for (i=0; i<8; i++) {
						*(rsp.RDRAM+((0x2fb1f0+j*0xff0+i)^S8))=*(rsp.IMEM+((0x120+j*8+i)^S8));
					}
				}
			 	return Cycles;
			}
			default:
#ifdef DEBUG_RSP
				// Calculate the sum of the task
				for (i=0; i<(0x1000/2); i++) {
					sum += *(rsp.IMEM + i);
				}
				sprintf(txtbuffer, "RSP: Unknown Boot code task! size: %08X sum: %08X\r\n",task->ucode_size,sum);
				DEBUG_print(txtbuffer, DBG_USBGECKO);
#endif
				break;
		}
	}
	// Audio / JPEG task
	else {
		switch(task->type) {
			case 2: // audio
#ifdef DEBUG_RSP
					DEBUG_print("RSP: Audio ucode\r\n", DBG_USBGECKO);
#endif
				if (!audio_ucode(task)) {
					return Cycles;
				} 
#ifdef DEBUG_RSP
				else {
					for (i=0; i<(task->ucode_size/2); i++) {
						sum += *(rsp.RDRAM + task->ucode + i);
					}
					sprintf(txtbuffer, "RSP: Unknown Audio task sum: %08X\r\n",sum);
					DEBUG_print(txtbuffer, DBG_USBGECKO);
				}
				break;
#endif
			case 4: // jpeg
		 	{
			 	for (i=0; i<(task->ucode_size/2); i++) {
					sum += *(rsp.RDRAM + task->ucode + i);
				}
			 	switch(sum) {
					case 0x278: // used by zelda during boot
#ifdef DEBUG_RSP
		 				DEBUG_print("RSP: Zelda Boot\r\n", DBG_USBGECKO);
#endif
		 				*rsp.SP_STATUS_REG |= 0x200;
		 				return Cycles;
					case 0x2e4fc: // uncompress
#ifdef DEBUG_RSP
		 				DEBUG_print("RSP: JPEG Uncompress\r\n", DBG_USBGECKO);
#endif
		 				jpg_uncompress(task);
		 				return Cycles;
					default:	// unknown
#ifdef DEBUG_RSP
						sprintf(txtbuffer, "RSP: Unknown JPEG task sum: %08X\r\n",sum);
						DEBUG_print(txtbuffer, DBG_USBGECKO);
#endif
						break;
				}
			}
			default:
#ifdef DEBUG_RSP
				for (i=0; i<(task->ucode_size/2); i++) {
					sum += *(rsp.RDRAM + task->ucode + i);
				}
				sprintf(txtbuffer, "Unknown task! type: %i sum: %08X\r\n",task->type, sum);
				DEBUG_print(txtbuffer, DBG_USBGECKO);
#endif
				break;
		}
		memcpy(rsp.DMEM, rsp.RDRAM+task->ucode_data, task->ucode_data_size);
	    memcpy(rsp.IMEM+0x80, rsp.RDRAM+task->ucode, 0xF7F);
	}


	return Cycles;
}

__declspec(dllexport) void GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
	PluginInfo->Version = 0x0101;
	PluginInfo->Type = PLUGIN_TYPE_RSP;
	strcpy(PluginInfo->Name, "Hacktarux/Azimer hle rsp plugin");
	PluginInfo->NormalMemory = TRUE;
	PluginInfo->MemoryBswaped = TRUE;
}

__declspec(dllexport) void InitiateRSP ( RSP_INFO Rsp_Info, DWORD * CycleCount)
{
	rsp = Rsp_Info;
}

__declspec(dllexport) void RomClosed (void)
{
	int i;
	for (i=0; i<0x1000; i++) {
		rsp.DMEM[i] = rsp.IMEM[i] = 0;
	}
	init_ucode2();
#ifdef __WIN32__
	firstTime = TRUE;
#endif
}
