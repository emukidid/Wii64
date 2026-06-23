/**
 * Wii64 - LoadSaveFrame.cpp
 * Copyright (C) 2009 sepp256
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
#include "LoadSaveFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"


extern "C" {
#include "../gc_memory/Saves.h"
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
}

void Func_LoadSaveSD();
void Func_LoadSaveUSB();
void Func_ReturnFromLoadSaveFrame();

#ifdef HW_RVL
#define NUM_FRAME_BUTTONS 2
#else
#define NUM_FRAME_BUTTONS 1
#endif
#define FRAME_BUTTONS loadSaveFrameButtons
#define FRAME_STRINGS loadSaveFrameStrings

static char FRAME_STRINGS[2][22] =
	{ "SD Card", "USB"};

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
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	150.0,	 50.0,	340.0,	56.0,	 NUM_FRAME_BUTTONS == 1 ? -1 : 1,	 NUM_FRAME_BUTTONS == 1 ? -1 : 1,	-1,	-1,	Func_LoadSaveSD,	Func_ReturnFromLoadSaveFrame }, // Load From SD
#ifdef HW_RVL
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[1],	150.0,	150.0,	340.0,	56.0,	 0,	 0,	-1,	-1,	Func_LoadSaveUSB,	Func_ReturnFromLoadSaveFrame }, // Load From USB
#endif
};

LoadSaveFrame::LoadSaveFrame()
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
	setBackFunc(Func_ReturnFromLoadSaveFrame);
	setEnabled(true);

}

LoadSaveFrame::~LoadSaveFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}

}

extern BOOL hasLoadedROM;

void Func_LoadSaveSD()
{
	if(!hasLoadedROM)
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
		return;
	}
	// Adjust saveFile pointers
	saveFile_dir = &saveDir_libfat_Default;
	saveFile_readFile  = fileBrowser_libfat_readFile;
	saveFile_writeFile = fileBrowser_libfat_writeFile;
	saveFile_init      = fileBrowser_libfat_init;
	saveFile_deinit    = fileBrowser_libfat_deinit;
		
	// Try loading everything
	int result = 0;
	saveFile_deinit(saveFile_dir);
	saveFile_init(saveFile_dir);
	result += loadEeprom(saveFile_dir);
	result += loadSram(saveFile_dir);
	result += loadMempak(saveFile_dir);
	result += loadFlashram(saveFile_dir);
		
	if (result)	menu::MessageBox::getInstance().setMessage("Loaded save from SD card");
	else		menu::MessageBox::getInstance().setMessage("No saves found on SD card");
}

void Func_LoadSaveUSB()
{
	if(!hasLoadedROM)
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
		return;
	}
	// Adjust saveFile pointers
	saveFile_dir = &saveDir_libfat_USB;
	saveFile_readFile  = fileBrowser_libfat_readFile;
	saveFile_writeFile = fileBrowser_libfat_writeFile;
	saveFile_init      = fileBrowser_libfat_init;
	saveFile_deinit    = fileBrowser_libfat_deinit;
		
	// Try loading everything
	int result = 0;
	saveFile_deinit(saveFile_dir);
	saveFile_init(saveFile_dir);
	result += loadEeprom(saveFile_dir);
	result += loadSram(saveFile_dir);
	result += loadMempak(saveFile_dir);
	result += loadFlashram(saveFile_dir);
		
	if (result)	menu::MessageBox::getInstance().setMessage("Loaded save from SD card");
	else		menu::MessageBox::getInstance().setMessage("No saves found on SD card");
}

extern MenuContext *pMenuContext;

void Func_ReturnFromLoadSaveFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}
