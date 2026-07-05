/**
 * Wii64 - LoadRomFrame.cpp
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

#include <unistd.h>
#include "MenuContext.h"
#include "LoadRomFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"

extern "C" {
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-DVD.h"
extern void LoadingBar_showBar(float percent, const char* string);
extern void control_info_init(void);
}
extern int loadROM(fileBrowser_file* rom);

void Func_LoadFromSD();
void Func_LoadFromDVD();
void Func_LoadFromUSB();
void Func_ReturnFromLoadRomFrame();

extern MenuContext *pMenuContext;
extern void Func_PlayGame();
extern void Func_SetPlayGame();

#ifdef HW_RVL
#include "../main/Autoboot.h"
#define NUM_FRAME_BUTTONS 3
#else
#define NUM_FRAME_BUTTONS 2
#endif
#define FRAME_BUTTONS loadRomFrameButtons
#define FRAME_STRINGS loadRomFrameStrings

static char FRAME_STRINGS[3][25] =
	{ "Load from SD",
	  "Load from DVD",
	  "Load from USB"};

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
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	150.0,	100.0,	340.0,	56.0,	 NUM_FRAME_BUTTONS-1,	 1,	-1,	-1,	Func_LoadFromSD,	Func_ReturnFromLoadRomFrame }, // Load From SD
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[1],	150.0,	200.0,	340.0,	56.0,	 0,	 NUM_FRAME_BUTTONS == 2 ? 0 : 2,	-1,	-1,	Func_LoadFromDVD,	Func_ReturnFromLoadRomFrame }, // Load From DVD
#ifdef HW_RVL
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[2],	150.0,	300.0,	340.0,	56.0,	 1,	 0,	-1,	-1,	Func_LoadFromUSB,	Func_ReturnFromLoadRomFrame }, // Load From USB
#endif
};

void setupFatDevice(fileBrowser_file* topLevel) 
{
	// TODO once you've done this you can no longer play a game that requires to be streamed in.
	// Deinit any existing romFile state
	if(romFile_deinit) romFile_deinit( romFile_topLevel );
	// Change all the romFile pointers
	romFile_topLevel = topLevel;
	romFile_readDir  = fileBrowser_libfat_readDir;
	romFile_readFile = fileBrowser_libfatROM_readFile;
	romFile_seekFile = fileBrowser_libfat_seekFile;
	romFile_init     = fileBrowser_libfat_init;
	romFile_deinit   = fileBrowser_libfatROM_deinit;
	// Make sure the romFile system is ready before we browse the filesystem
	romFile_deinit( romFile_topLevel );
	romFile_init( romFile_topLevel );
}

LoadRomFrame::LoadRomFrame()
{
#ifdef HW_RVL	
	// argv is only setup if we detect argv[0] sd:/ or usb:/
	// Yes this happens early, before we've actually run the menu for any frames.
	if (Autoboot::hasPath())
    {
        const char* path = Autoboot::getPath();
        //print_gecko("Autoboot path: %s\n", path);

        fileBrowser_file* autoBoot = (fileBrowser_file*)calloc(1, sizeof(fileBrowser_file));
        strncpy(autoBoot->name, path, 191);
		autoBoot->name[191] = '\0';
		
		setupFatDevice(strncmp(path, "sd:/", 4) == 0 ? &topLevel_libfat_Default : &topLevel_libfat_USB);
        int ret = loadROM(autoBoot);
		menu::Gui::getInstance().draw();	

        if (ret)
        {
            menu::MessageBox::getInstance().setMessage("Autoboot failed");
        }
        else
        {
            Func_SetPlayGame();
            pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
			LoadingBar_showBar(1.0f, "Loading ...");
			sleep(3);
			control_info_init(); // by now a Wii remote should've re-sync'd.
            Func_PlayGame();
        }
		Autoboot::setPath(NULL);
    }
#endif
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
	setBackFunc(Func_ReturnFromLoadRomFrame);
	setEnabled(true);

}

LoadRomFrame::~LoadRomFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}

}

extern MenuContext *pMenuContext;
extern void fileBrowserFrame_OpenDirectory(fileBrowser_file* dir);

void Func_LoadFromSD()
{
	setupFatDevice(&topLevel_libfat_Default);
	pMenuContext->setActiveFrame(MenuContext::FRAME_FILEBROWSER);
	fileBrowserFrame_OpenDirectory(romFile_topLevel);
}

void Func_LoadFromDVD()
{
	// Deinit any existing romFile state
	if(romFile_deinit) romFile_deinit( romFile_topLevel );
	// Change all the romFile pointers
	romFile_topLevel = &topLevel_DVD;
	romFile_readDir  = fileBrowser_DVD_readDir;
	romFile_readFile = fileBrowser_DVD_readFile;
	romFile_seekFile = fileBrowser_DVD_seekFile;
	romFile_init     = fileBrowser_DVD_init;
	romFile_deinit   = fileBrowser_DVD_deinit;
	// Make sure the romFile system is ready before we browse the filesystem
	romFile_init( romFile_topLevel );

	pMenuContext->setActiveFrame(MenuContext::FRAME_FILEBROWSER);
	fileBrowserFrame_OpenDirectory(romFile_topLevel);
}

void Func_LoadFromUSB()
{
#ifdef WII
	setupFatDevice(&topLevel_libfat_USB);	
	pMenuContext->setActiveFrame(MenuContext::FRAME_FILEBROWSER);
	fileBrowserFrame_OpenDirectory(romFile_topLevel);
#endif
}

void Func_ReturnFromLoadRomFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}
