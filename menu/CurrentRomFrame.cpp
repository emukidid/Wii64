/**
 * Wii64 - CurrentRomFrame.cpp
 * Copyright (C) 2009, 2013 sepp256
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
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

#include "MenuContext.h"
#include "CurrentRomFrame.h"
#include "../libgui/GuiResources.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"

extern "C" {
#include "../gc_memory/memory.h"
#include "../gc_memory/Saves.h"
#include "../r4300/interupt.h"
#include "../main/wii64config.h"
#include "../main/rom.h"
#include "../main/plugin.h"
#include "../main/savestates.h"
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-CARD.h"
}

void Func_ShowRomInfo();
void Func_ResetROM();
void Func_LoadSave();
void Func_SaveGame();
void Func_DeleteSave();
void Func_LoadState();
void Func_SaveState();
void Func_StateCycle();
void Func_ReturnFromCurrentRomFrame();

#define NUM_FRAME_BUTTONS 8
#define FRAME_BUTTONS currentRomFrameButtons
#define FRAME_STRINGS currentRomFrameStrings

static char FRAME_STRINGS[8][25] =
	{ "Show ROM Info",
	  "Restart Game",
	  "Load Save File",
	  "Save Game",
	  "Delete Save Game",
	  "Load State",
	  "Save State",
	  "Slot 0"};

struct ButtonInfo
{
	menu::Button	*button;
	int				buttonStyle;
	char*			buttonString;
	float			x;
	float			y;
	float			width;
	float			height;
	int				focusUp;
	int				focusDown;
	int				focusLeft;
	int				focusRight;
	ButtonFunc		clickedFunc;
	ButtonFunc		returnFunc;
} FRAME_BUTTONS[NUM_FRAME_BUTTONS] =
{ //	button	buttonStyle	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc			returnFunc
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	150.0,	 50.0,	340.0,	50.0,	 7,	 1,	-1,	-1,	Func_ShowRomInfo,	Func_ReturnFromCurrentRomFrame }, // Show ROM Info
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[1],	150.0,	110.0,	340.0,	50.0,	 0,	 2,	-1,	-1,	Func_ResetROM,		Func_ReturnFromCurrentRomFrame }, // Reset ROM
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[2],	150.0,	170.0,	340.0,	50.0,	 1,	 3,	-1,	-1,	Func_LoadSave,		Func_ReturnFromCurrentRomFrame }, // Load Native Save
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[3],	150.0,	230.0,	340.0,	50.0,	 2,	 4,	-1,	-1,	Func_SaveGame,		Func_ReturnFromCurrentRomFrame }, // Save Native Save
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[4],	150.0,	290.0,	340.0,	50.0,	 3,	 5,	-1,	-1,	Func_DeleteSave,	Func_ReturnFromCurrentRomFrame }, // Delete Native Save
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[5],	150.0,	350.0,	220.0,	50.0,	 4,	 6,	 7,	 7,	Func_LoadState,		Func_ReturnFromCurrentRomFrame }, // Load State 
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[6],	150.0,	410.0,	220.0,	50.0,	 5,	 7,	 7,	 7,	Func_SaveState,		Func_ReturnFromCurrentRomFrame }, // Save State 
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[7],	390.0,	375.0,	100.0,	50.0,	 6,	 0,	 5,	 5,	Func_StateCycle,	Func_ReturnFromCurrentRomFrame }, // Cycle State 
};

CurrentRomFrame::CurrentRomFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
		FRAME_BUTTONS[i].button = new menu::Button(FRAME_BUTTONS[i].buttonStyle, &FRAME_BUTTONS[i].buttonString, 
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y, 
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height);

	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		if (FRAME_BUTTONS[i].focusUp != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[FRAME_BUTTONS[i].focusUp].button);
		if (FRAME_BUTTONS[i].focusDown != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[FRAME_BUTTONS[i].focusDown].button);
		if (FRAME_BUTTONS[i].focusLeft != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_LEFT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusLeft].button);
		if (FRAME_BUTTONS[i].focusRight != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_RIGHT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusRight].button);
		FRAME_BUTTONS[i].button->setActive(true);
		if (FRAME_BUTTONS[i].clickedFunc) FRAME_BUTTONS[i].button->setClicked(FRAME_BUTTONS[i].clickedFunc);
		if (FRAME_BUTTONS[i].returnFunc) FRAME_BUTTONS[i].button->setReturn(FRAME_BUTTONS[i].returnFunc);
		add(FRAME_BUTTONS[i].button);
		menu::Cursor::getInstance().addComponent(this, FRAME_BUTTONS[i].button, FRAME_BUTTONS[i].x, 
												FRAME_BUTTONS[i].x+FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].y, 
												FRAME_BUTTONS[i].y+FRAME_BUTTONS[i].height);
	}
	setDefaultFocus(FRAME_BUTTONS[0].button);
	setBackFunc(Func_ReturnFromCurrentRomFrame);
	setEnabled(true);

}

CurrentRomFrame::~CurrentRomFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}

}

extern MenuContext *pMenuContext;
extern int rom_length;

void Func_ShowRomInfo()
{
	char RomInfo[256] = "";
	char buffer [50];
	char buffer2 [50];
	sprintf(buffer,"Rom name: %s\n",ROM_SETTINGS.goodname);
	strcat(RomInfo,buffer);
	sprintf(buffer,"Rom size: %d Mb\n",rom_length/1024/1024);
	strcat(RomInfo,buffer);
	if(ROM_HEADER->Manufacturer_ID == 'N') sprintf(buffer,"Manufacturer: Nintendo\n");
	else sprintf(buffer,"Manufacturer: %x\n", (unsigned int)(ROM_HEADER->Manufacturer_ID));
	strcat(RomInfo,buffer);
    countrycodestring(ROM_HEADER->Country_code&0xFF, buffer2);
	sprintf(buffer,"Country: %s\n",buffer2);
	strcat(RomInfo,buffer);

	menu::MessageBox::getInstance().setMessage(RomInfo);
}

extern BOOL hasLoadedROM;

extern "C" {
void cpu_init();
void cpu_deinit();
}

void Func_SetPlayGame();

void Func_ResetROM()
{
	if(hasLoadedROM)
	{
		cpu_deinit();
		romClosed_RSP();
		romClosed_input();
		romClosed_audio();
		romClosed_gfx();
		free_memory();
		
		init_memory();
		romOpen_gfx();
		romOpen_audio();
		romOpen_input();
		cpu_init();
		//Clear FB image
		memset(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_CURRENT_FB)->getTexture(), 0x00, FB_THUMB_SIZE);
		DCFlushRange(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_CURRENT_FB)->getTexture(), FB_THUMB_SIZE);
		GX_InvalidateTexAll();
		menu::MessageBox::getInstance().setMessage("Game restarted");
		Func_SetPlayGame();
	}
	else	
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
	}
}

extern BOOL sramWritten;
extern BOOL eepromWritten;
extern BOOL mempakWritten;
extern BOOL flashramWritten;

void Func_LoadSave()
{
	if(!hasLoadedROM)
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
		return;
	}

	switch (nativeSaveDevice)
  {
  	case NATIVESAVEDEVICE_SD:
  	case NATIVESAVEDEVICE_USB:
  		// Adjust saveFile pointers
  		saveFile_dir = (nativeSaveDevice==NATIVESAVEDEVICE_SD) ? &saveDir_libfat_Default:&saveDir_libfat_USB;
  		saveFile_readFile  = fileBrowser_libfat_readFile;
  		saveFile_writeFile = fileBrowser_libfat_writeFile;
  		saveFile_init      = fileBrowser_libfat_init;
  		saveFile_deinit    = fileBrowser_libfat_deinit;
  		break;
  	case NATIVESAVEDEVICE_CARDA:
  	case NATIVESAVEDEVICE_CARDB:
  		// Adjust saveFile pointers
  		saveFile_dir       = (nativeSaveDevice==NATIVESAVEDEVICE_CARDA) ? &saveDir_CARD_SlotA:&saveDir_CARD_SlotB;
  		saveFile_readFile  = fileBrowser_CARD_readFile;
  		saveFile_writeFile = fileBrowser_CARD_writeFile;
  		saveFile_init      = fileBrowser_CARD_init;
  		saveFile_deinit    = fileBrowser_CARD_deinit;
  		break;
  }

	// Try loading everything
	int result = 0;
	saveFile_init(saveFile_dir);
	result += loadEeprom(saveFile_dir);
	result += loadSram(saveFile_dir);
	result += loadMempak(saveFile_dir);
	result += loadFlashram(saveFile_dir);
	saveFile_deinit(saveFile_dir);

	switch (nativeSaveDevice)
	{
		case NATIVESAVEDEVICE_SD:
			if (result) menu::MessageBox::getInstance().setMessage("Loaded save from SD card");
			else		menu::MessageBox::getInstance().setMessage("No saves found on SD card");
			break;
		case NATIVESAVEDEVICE_USB:
			if (result) menu::MessageBox::getInstance().setMessage("Loaded save from USB device");
			else		menu::MessageBox::getInstance().setMessage("No saves found on USB device");
			break;
		case NATIVESAVEDEVICE_CARDA:
			if (result) menu::MessageBox::getInstance().setMessage("Loaded save from memcard in slot A");
			else		menu::MessageBox::getInstance().setMessage("No saves found on memcard A");
			break;
		case NATIVESAVEDEVICE_CARDB:
			if (result) menu::MessageBox::getInstance().setMessage("Loaded save from memcard in slot A");
			else		menu::MessageBox::getInstance().setMessage("No saves found on memcard B");
			break;
	}
	sramWritten = eepromWritten = mempakWritten = flashramWritten = false;
}

void Func_SaveGame()
{
  if(!flashramWritten && !sramWritten && !eepromWritten && !mempakWritten) {
    menu::MessageBox::getInstance().setMessage("Nothing to save");
    return;
  }
	switch (nativeSaveDevice)
  {
  	case NATIVESAVEDEVICE_SD:
  	case NATIVESAVEDEVICE_USB:
  		// Adjust saveFile pointers
  		saveFile_dir = (nativeSaveDevice==NATIVESAVEDEVICE_SD) ? &saveDir_libfat_Default:&saveDir_libfat_USB;
  		saveFile_readFile  = fileBrowser_libfat_readFile;
  		saveFile_writeFile = fileBrowser_libfat_writeFile;
  		saveFile_init      = fileBrowser_libfat_init;
  		saveFile_deinit    = fileBrowser_libfat_deinit;
  		break;
  	case NATIVESAVEDEVICE_CARDA:
  	case NATIVESAVEDEVICE_CARDB:
  		// Adjust saveFile pointers
  		saveFile_dir       = (nativeSaveDevice==NATIVESAVEDEVICE_CARDA) ? &saveDir_CARD_SlotA:&saveDir_CARD_SlotB;
  		saveFile_readFile  = fileBrowser_CARD_readFile;
  		saveFile_writeFile = fileBrowser_CARD_writeFile;
  		saveFile_init      = fileBrowser_CARD_init;
  		saveFile_deinit    = fileBrowser_CARD_deinit;
  		break;
  }

	// Try saving everything
	int amountSaves = flashramWritten + sramWritten + eepromWritten + mempakWritten;
	int result = 0;
	saveFile_init(saveFile_dir);
	result += saveEeprom(saveFile_dir);
	result += saveSram(saveFile_dir);
	result += saveMempak(saveFile_dir);
	result += saveFlashram(saveFile_dir);
	saveFile_deinit(saveFile_dir);

	if (result==amountSaves) {	
		switch (nativeSaveDevice)
		{
			case NATIVESAVEDEVICE_SD:
				menu::MessageBox::getInstance().setMessage("Saved game to SD card");
				break;
			case NATIVESAVEDEVICE_USB:
				menu::MessageBox::getInstance().setMessage("Saved game to USB device");
				break;
			case NATIVESAVEDEVICE_CARDA:
				menu::MessageBox::getInstance().setMessage("Saved game to memcard in Slot A");
				break;
			case NATIVESAVEDEVICE_CARDB:
				menu::MessageBox::getInstance().setMessage("Saved game to memcard in Slot B");
				break;
		}
		sramWritten = eepromWritten = mempakWritten = flashramWritten = false;
	}
	else		menu::MessageBox::getInstance().setMessage("Failed to Save");
}

void Func_DeleteSave()
{
	if(!menu::MessageBox::getInstance().askMessage("Are you sure you want to delete the save?"))
		return;
	switch (nativeSaveDevice)
	{
		case NATIVESAVEDEVICE_SD:
		case NATIVESAVEDEVICE_USB:
			// Adjust saveFile pointers
			saveFile_dir = (nativeSaveDevice==NATIVESAVEDEVICE_SD) ? &saveDir_libfat_Default:&saveDir_libfat_USB;
			saveFile_readFile  = fileBrowser_libfat_readFile;
			saveFile_writeFile = fileBrowser_libfat_writeFile;
			saveFile_init      = fileBrowser_libfat_init;
			saveFile_deinit    = fileBrowser_libfat_deinit;
			saveFile_deleteFile= fileBrowser_libfat_deleteFile;
		break;
		case NATIVESAVEDEVICE_CARDA:
		case NATIVESAVEDEVICE_CARDB:
			// Adjust saveFile pointers
			saveFile_dir       = (nativeSaveDevice==NATIVESAVEDEVICE_CARDA) ? &saveDir_CARD_SlotA:&saveDir_CARD_SlotB;
			saveFile_readFile  = fileBrowser_CARD_readFile;
			saveFile_writeFile = fileBrowser_CARD_writeFile;
			saveFile_init      = fileBrowser_CARD_init;
			saveFile_deinit    = fileBrowser_CARD_deinit;
			saveFile_deleteFile= fileBrowser_CARD_deleteFile;
		break;
	}

	// Try deleting everything
	saveFile_init(saveFile_dir);
	u32 eepromDeleted = deleteEeprom(saveFile_dir);
	u32 sramDeleted = deleteSram(saveFile_dir);
	u32 mempakDeleted = deleteMempak(saveFile_dir);
	u32 flashramDeleted = deleteFlashram(saveFile_dir);
	saveFile_deinit(saveFile_dir);

	if(!flashramDeleted && !sramDeleted && !eepromDeleted && !mempakDeleted) {
		menu::MessageBox::getInstance().setMessage("Nothing to delete");
	}
	else {
		menu::MessageBox::getInstance().setMessage("Successfully deleted");
	}
}

static unsigned int which_slot = 0;

void Func_LoadState()
{
  if(!savestates_exists(which_slot, LOADSTATE)) {
    menu::MessageBox::getInstance().setMessage("Save doesn't exist");
  }
  else {
//    savestates_load();
	savestates_queue_load(which_slot);
	menu::MessageBox::getInstance().setMessage("Game will resume from save state");
  }
}

void Func_SaveState()
{
  if(!savestates_exists(which_slot, SAVESTATE)) {
    menu::MessageBox::getInstance().setMessage("Failed to create save state");
  }
  else {
//    savestates_save();
	savestates_save(which_slot, menu::Resources::getInstance().getImage(menu::Resources::IMAGE_CURRENT_FB)->getTexture());
	menu::MessageBox::getInstance().setMessage("State Saved Successfully");
  }
}

void Func_StateCycle()
{
	which_slot = (which_slot+1) %10;
	savestates_select_slot(which_slot);
	FRAME_STRINGS[7][5] = which_slot + '0';
}

void Func_ReturnFromCurrentRomFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}
