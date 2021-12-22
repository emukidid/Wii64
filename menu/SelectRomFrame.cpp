/**
 * Wii64 - SelectRomFrame.cpp
 * Copyright (C) 2009, 2010, 2013 sepp256
 * Copyright (C) 2013 emu_kidid
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *                emukidid@gmail.com
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
#if !(defined(GC_BASIC))
#include <math.h>
#include <cstdlib>
#ifdef HW_RVL
#include "../gc_memory/MEM2.h"
#include <ogc/lwp_heap.h>
#endif
#include "MenuContext.h"
#include "SelectRomFrame.h"
#include "../libgui/Button.h"
#include "../libgui/TextBox.h"
#include "../libgui/GuiResources.h"
#include "../libgui/resources.h"
#include "../libgui/MessageBox.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/Gui.h"

extern "C" {
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-DVD.h"
#include "../fileBrowser/fileBrowser-CARD.h"
#include "../fileBrowser/gc_dvd.h"
#include "../main/rom.h"
#include "../main/ROM-Cache.h"
#include "../main/wii64config.h"
#include "../gui/DEBUG.h"
}
#ifdef HW_RVL
heap_cntrl* boxartTexCache = NULL;
#endif

void Func_ReturnFromSelectRomFrame();
void Func_SR_SD();
void Func_SR_USB();
void Func_SR_DVD();
void Func_SR_PrevPage();
void Func_SR_NextPage();
void Func_SR_Select1();
void Func_SR_Select2();
void Func_SR_Select3();
void Func_SR_Select4();
void Func_SR_Select5();
void Func_SR_Select6();
void Func_SR_Select7();
void Func_SR_Select8();
void Func_SR_Select9();
void Func_SR_Select10();
void Func_SR_Select11();
void Func_SR_Select12();
void Func_SR_Select13();
void Func_SR_Select14();
void Func_SR_Select15();
void Func_SR_Select16();

#define NUM_FRAME_BUTTONS 21
#define NUM_FILE_SLOTS 16
#define FRAME_BUTTONS selectRomFrameButtons
#define FRAME_STRINGS selectRomFrameStrings
#define NUM_FRAME_TEXTBOXES 2
#define FRAME_TEXTBOXES selectRomFrameTextBoxes

static char FRAME_STRINGS[7][14] =
	{ "SD",
	  "USB",
	  "DVD",
	  "Prev",
	  "Next",
	  "",
	  "Page XX of XX"};

static u8* fileTextures[NUM_FILE_SLOTS];
static fileBrowser_file* dir_entries;
static rom_header* 		rom_headers;
static int				num_entries;
static int				current_page;
static int				max_page;
extern rom_header 		ROM_HEADER;

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
{ //	button	buttonStyle		buttonString		x		y	width	height	Up	Dwn	Lft	Rt	clickFunc			returnFunc
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[0],	120.0,	 40.0,	 70.0,	40.0,	-1,	 5,	-1,	 1,	Func_SR_SD,			Func_ReturnFromSelectRomFrame }, // SD
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[1],	220.0,	 40.0,	 70.0,	40.0,	-1,	 6,	 0,	 2,	Func_SR_USB,		Func_ReturnFromSelectRomFrame }, // USB
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[2],	320.0,	 40.0,	 70.0,	40.0,	-1,	 7,	 1,	-1,	Func_SR_DVD,		Func_ReturnFromSelectRomFrame }, // DVD
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[3],	 35.0,	215.0,	 70.0,	40.0,	-1,	-1,	-1,	 5,	Func_SR_PrevPage,	Func_ReturnFromSelectRomFrame }, // Prev
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[4],	535.0,	215.0,	 70.0,	40.0,	-1,	-1,	 8,	-1,	Func_SR_NextPage,	Func_ReturnFromSelectRomFrame }, // Next
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	120.0,	 90.0,	100.0,	80.0,	 0,	 9,	 3,	 6,	Func_SR_Select1,	Func_ReturnFromSelectRomFrame }, // File Button 1
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	220.0,	 90.0,	100.0,	80.0,	 1,	10,	 5,	 7,	Func_SR_Select2,	Func_ReturnFromSelectRomFrame }, // File Button 2
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	320.0,	 90.0,	100.0,	80.0,	 2,	11,	 6,	 8,	Func_SR_Select3,	Func_ReturnFromSelectRomFrame }, // File Button 3
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	420.0,	 90.0,	100.0,	80.0,	 2,	12,	 7,	 4,	Func_SR_Select4,	Func_ReturnFromSelectRomFrame }, // File Button 4
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	120.0,	165.0,	100.0,	80.0,	 5,	13,	 3,	10,	Func_SR_Select5,	Func_ReturnFromSelectRomFrame }, // File Button 5
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	220.0,	165.0,	100.0,	80.0,	 6,	14,	 9,	11,	Func_SR_Select6,	Func_ReturnFromSelectRomFrame }, // File Button 6
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	320.0,	165.0,	100.0,	80.0,	 7,	15,	10,	12,	Func_SR_Select7,	Func_ReturnFromSelectRomFrame }, // File Button 7
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	420.0,	165.0,	100.0,	80.0,	 8,	16,	11,	 4,	Func_SR_Select8,	Func_ReturnFromSelectRomFrame }, // File Button 8
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	120.0,	240.0,	100.0,	80.0,	 9,	17,	 3,	14,	Func_SR_Select9,	Func_ReturnFromSelectRomFrame }, // File Button 9
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	220.0,	240.0,	100.0,	80.0,	10,	18,	13,	15,	Func_SR_Select10,	Func_ReturnFromSelectRomFrame }, // File Button 10
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	320.0,	240.0,	100.0,	80.0,	11,	19,	14,	16,	Func_SR_Select11,	Func_ReturnFromSelectRomFrame }, // File Button 11
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	420.0,	240.0,	100.0,	80.0,	12,	20,	15,	 4,	Func_SR_Select12,	Func_ReturnFromSelectRomFrame }, // File Button 12
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	120.0,	315.0,	100.0,	80.0,	13,	-1,	 3,	18,	Func_SR_Select13,	Func_ReturnFromSelectRomFrame }, // File Button 13
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	220.0,	315.0,	100.0,	80.0,	14,	-1,	17,	19,	Func_SR_Select14,	Func_ReturnFromSelectRomFrame }, // File Button 14
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	320.0,	315.0,	100.0,	80.0,	15,	-1,	18,	20,	Func_SR_Select15,	Func_ReturnFromSelectRomFrame }, // File Button 15
	{	NULL,	BTN_BOX3D,	FRAME_STRINGS[5],	420.0,	315.0,	100.0,	80.0,	16,	-1,	19,	 4,	Func_SR_Select16,	Func_ReturnFromSelectRomFrame }, // File Button 16
};

struct TextBoxInfo
{
	menu::TextBox	*textBox;
	char*			textBoxString;
	float			x;
	float			y;
	float			scale;
	bool			centered;
} FRAME_TEXTBOXES[NUM_FRAME_TEXTBOXES] =
{ //	textBox	textBoxString		x		y		scale	centered
	{	NULL,	FRAME_STRINGS[6],	500.0,	440.0,	 1.0,	true }, // Page XX of XX
	{	NULL,	FRAME_STRINGS[5],	320.0,	410.0,	 1.0,	true }, // Rom file name
};

extern "C" {
	#include <stdarg.h>
	void print_gecko(const char* fmt, ...);
}

SelectRomFrame::SelectRomFrame()
		: activeSubmenu(SUBMENU_DEFAULT)
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

/*	for (int i = 2; i < NUM_FRAME_BUTTONS; i++)
	{
		FRAME_BUTTONS[i].button->setLabelMode(menu::Button::LABEL_SCROLLONFOCUS);
		FRAME_BUTTONS[i].button->setLabelScissor(6);
	}*/

	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
	{
		FRAME_TEXTBOXES[i].textBox = new menu::TextBox(&FRAME_TEXTBOXES[i].textBoxString, 
										FRAME_TEXTBOXES[i].x, FRAME_TEXTBOXES[i].y, 
										FRAME_TEXTBOXES[i].scale, FRAME_TEXTBOXES[i].centered);
		add(FRAME_TEXTBOXES[i].textBox);
	}
	//Set textbox color to white
//	GXColor textColor = {255, 255, 255, 255};
//	FRAME_TEXTBOXES[0].textBox->setColor(&textColor);
	
	setDefaultFocus(FRAME_BUTTONS[5].button);
	setBackFunc(Func_ReturnFromSelectRomFrame);
	setEnabled(true);

#ifdef HW_RVL
	if(boxartTexCache == NULL) {
		boxartTexCache = (heap_cntrl*)malloc(sizeof(heap_cntrl));
		__lwp_heap_init(boxartTexCache, BOXART_ICON_LO,BOXART_ICON_SIZE, 32);
#ifdef SHOW_DEBUG
		DEBUG_registerHeap(boxartTexCache, "ART");
#endif
	}
#endif

	for (int i = 0; i<NUM_FILE_SLOTS; i++) {
		if (fileTextures[i])
#ifdef HW_RVL
			__lwp_heap_free(boxartTexCache, fileTextures[i]);
#else
			free(fileTextures[i]);
#endif
		fileTextures[i] = NULL;
	}
	dir_entries = NULL;
	rom_headers = NULL;
	
}

SelectRomFrame::~SelectRomFrame()
{
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
		delete FRAME_TEXTBOXES[i].textBox;
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}

}

void selectRomFrame_OpenDirectory(fileBrowser_file* dir);
void selectRomFrame_Error(fileBrowser_file* dir, int error_code);
void selectRomFrame_FillPage();
void selectRomFrame_LoadFile(int i);

void SelectRomFrame::activateSubmenu(int submenu)
{
	if (submenu) activeSubmenu = submenu;

	//Unselect all ROM source buttons
	for (int i = 0; i < 3; i++)
		FRAME_BUTTONS[i].button->setSelected(false);

#ifndef WII
	//Disable USB button for GC
	FRAME_BUTTONS[1].button->setActive(false);
#endif

	//Init textures
	for (int i = 0; i < NUM_FILE_SLOTS; i++)
	{
		int btn_ind = i+5;
		if(!fileTextures[i])
		{
#ifdef HW_RVL
			fileTextures[i] = (u8*)__lwp_heap_allocate(boxartTexCache,BOXART_TEX_SIZE);
#else
			fileTextures[i] = (u8*) memalign(32, BOXART_TEX_SIZE);
#endif
			memset(fileTextures[i], 0xFF, BOXART_TEX_SIZE);
			DCFlushRange(fileTextures[i], BOXART_TEX_SIZE);
			FRAME_BUTTONS[btn_ind].button->setBoxTexture(fileTextures[i]);
			FRAME_BUTTONS[btn_ind].button->setLabelColor((GXColor) {128,128,128,128});
		}
	}
	GX_InvalidateTexAll();
	
	menu::MessageBox::getInstance().fadeMessage("Searching for ROMs");

#ifdef WII	
	// Work out which device we have
	if(activeSubmenu == SUBMENU_DEFAULT) {
		romFile_init     = fileBrowser_libfat_init;
		romFile_topLevel = &topLevel_libfat_Default;
		romFile_deinit   = fileBrowser_libfatROM_deinit;
		activeSubmenu = SUBMENU_SD;

		if(!romFile_init( romFile_topLevel )) {
			if(romFile_deinit) romFile_deinit( romFile_topLevel );
			romFile_topLevel = &topLevel_libfat_USB;
			if(romFile_init( romFile_topLevel )) {
				activeSubmenu = SUBMENU_USB;
			}
		}
	}
#else
	activeSubmenu = SUBMENU_SD;
#endif
	
	switch (activeSubmenu)	//Display message saying which device we're opening
	{						
		case SUBMENU_SD:
			FRAME_BUTTONS[0].button->setSelected(true);
			break;
		case SUBMENU_USB:
#ifdef WII
			FRAME_BUTTONS[1].button->setSelected(true);
#endif
			break;
		case SUBMENU_DVD:
			FRAME_BUTTONS[2].button->setSelected(true);
			break;
	}
	//Draw one frame with message
	menu::Focus::getInstance().setFocusActive(false);
	menu::Focus::getInstance().setFreezeAction(true);
	menu::Cursor::getInstance().setFreezeAction(true);
	menu::Gui::getInstance().draw();
	menu::Focus::getInstance().setFocusActive(true);
	menu::Focus::getInstance().setFreezeAction(false);
	menu::Cursor::getInstance().setFreezeAction(false);
	menu::Focus::getInstance().clearPrimaryFocus();	

	switch (activeSubmenu)	//Init romFile pointers
	{						
		case SUBMENU_SD:
			// Deinit any existing romFile state
			if(romFile_deinit) romFile_deinit( romFile_topLevel );
			// Change all the romFile pointers
			romFile_topLevel = &topLevel_libfat_Default;
			romFile_readDir  = fileBrowser_libfat_readDir;
			romFile_readFile = fileBrowser_libfatROM_readFile;
			romFile_readHeader = fileBrowser_libfat_readFile;
			romFile_seekFile = fileBrowser_libfat_seekFile;
			romFile_init     = fileBrowser_libfat_init;
			romFile_deinit   = fileBrowser_libfatROM_deinit;
			// Make sure the romFile system is ready before we browse the filesystem
			romFile_deinit( romFile_topLevel );
			romFile_init( romFile_topLevel );
			selectRomFrame_OpenDirectory(romFile_topLevel);
			break;
		case SUBMENU_USB:
#ifdef WII
			// Deinit any existing romFile state
			if(romFile_deinit) romFile_deinit( romFile_topLevel );
			// Change all the romFile pointers
			romFile_topLevel = &topLevel_libfat_USB;
			romFile_readDir  = fileBrowser_libfat_readDir;
			romFile_readFile = fileBrowser_libfatROM_readFile;
			romFile_readHeader = fileBrowser_libfat_readFile;
			romFile_seekFile = fileBrowser_libfat_seekFile;
			romFile_init     = fileBrowser_libfat_init;
			romFile_deinit   = fileBrowser_libfatROM_deinit;
			// Make sure the romFile system is ready before we browse the filesystem
			romFile_deinit( romFile_topLevel );
			romFile_init( romFile_topLevel );
			selectRomFrame_OpenDirectory(romFile_topLevel);
#endif
			break;
		case SUBMENU_DVD:
			// Deinit any existing romFile state
			if(romFile_deinit) romFile_deinit( romFile_topLevel );
			// Change all the romFile pointers
			romFile_topLevel = &topLevel_DVD;
			romFile_readDir  = fileBrowser_DVD_readDir;
			romFile_readFile = fileBrowser_DVD_readFile;
			romFile_readHeader = fileBrowser_DVD_readFile;
			romFile_seekFile = fileBrowser_DVD_seekFile;
			romFile_init     = fileBrowser_DVD_init;
			romFile_deinit   = fileBrowser_DVD_deinit;
			// Make sure the romFile system is ready before we browse the filesystem
			romFile_init( romFile_topLevel );
			selectRomFrame_OpenDirectory(romFile_topLevel);
			break;
	}
}

static char* filenameFromAbsPath(char* absPath)
{
	char* filename = absPath;
	// Here we want to extract from the absolute path
	//   just the filename
	// First we move the pointer all the way to the end
	//   of the the string so we can work our way back
	while( *filename ) ++filename;
	// Now, just move it back to the last '/' or the start
	//   of the string
	while( filename != absPath && (*(filename-1) != '\\' && *(filename-1) != '/'))
		--filename;
	return filename;
}

void SelectRomFrame::drawChildren(menu::Graphics &gfx)
{
	if(isVisible())
	{
#ifdef HW_RVL
		WPADData* wiiPad = menu::Input::getInstance().getWpad();
#endif
		for (int i=0; i<4; i++)
		{
			u16 currentButtonsGC = PAD_ButtonsHeld(i);
			if (currentButtonsGC ^ previousButtonsGC[i])
			{
				u16 currentButtonsDownGC = (currentButtonsGC ^ previousButtonsGC[i]) & currentButtonsGC;
				previousButtonsGC[i] = currentButtonsGC;
				if (currentButtonsDownGC & PAD_TRIGGER_R)
				{
					//move to next set & return
					if(current_page+1 < max_page) 
					{
						current_page +=1;
						selectRomFrame_FillPage();
						menu::Focus::getInstance().clearPrimaryFocus();
					}
					break;
				}
				else if (currentButtonsDownGC & PAD_TRIGGER_L)
				{
					//move to the previous set & return
					if(current_page > 0) 
					{
						current_page -= 1;
						selectRomFrame_FillPage();
						menu::Focus::getInstance().clearPrimaryFocus();
					}
					break;
				}
			}
#ifdef HW_RVL
			else if (wiiPad[i].btns_h ^ previousButtonsWii[i])
			{
				u32 currentButtonsDownWii = (wiiPad[i].btns_h ^ previousButtonsWii[i]) & wiiPad[i].btns_h;
				previousButtonsWii[i] = wiiPad[i].btns_h;
				if (wiiPad[i].exp.type == WPAD_EXP_CLASSIC)
				{
					if (currentButtonsDownWii & WPAD_CLASSIC_BUTTON_FULL_R)
					{
						//move to next set & return
						if(current_page+1 < max_page) 
						{
							current_page +=1;
							selectRomFrame_FillPage();
							menu::Focus::getInstance().clearPrimaryFocus();
						}
						break;
					}
					else if (currentButtonsDownWii & WPAD_CLASSIC_BUTTON_FULL_L)
					{
						//move to the previous set & return
						if(current_page > 0) 
						{
							current_page -= 1;
							selectRomFrame_FillPage();
							menu::Focus::getInstance().clearPrimaryFocus();
						}
						break;
					}
				}
				else
				{
					if (currentButtonsDownWii & WPAD_BUTTON_PLUS)
					{
						//move to next set & return
						if(current_page+1 < max_page) 
						{
							current_page +=1;
							selectRomFrame_FillPage();
							menu::Focus::getInstance().clearPrimaryFocus();
						}
						break;
					}
					else if (currentButtonsDownWii & WPAD_BUTTON_MINUS)
					{
						//move to the previous set & return
						if(current_page > 0) 
						{
							current_page -= 1;
							selectRomFrame_FillPage();
							menu::Focus::getInstance().clearPrimaryFocus();
						}
						break;
					}
				}
			}
#endif //HW_RVL
		}

		FRAME_TEXTBOXES[1].textBoxString = FRAME_STRINGS[5];
		for (int i = 0; i < NUM_FILE_SLOTS; i++)
		{
			int btn_ind = i+5;
			if (FRAME_BUTTONS[btn_ind].button->getFocus() && ((current_page*NUM_FILE_SLOTS) + i < num_entries))
			{
				FRAME_TEXTBOXES[1].textBoxString = filenameFromAbsPath(dir_entries[i+(current_page*NUM_FILE_SLOTS)].name);
			}
		}
		
		//Draw buttons
		menu::ComponentList::const_iterator iteration;
		for (iteration = componentList.begin(); iteration != componentList.end(); iteration++)
		{
			(*iteration)->draw(gfx);
		}
	}
}

extern MenuContext *pMenuContext;

void Func_SR_SD()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SELECTROM,SelectRomFrame::SUBMENU_SD);
}

void Func_SR_USB()
{
#ifdef WII
	pMenuContext->setActiveFrame(MenuContext::FRAME_SELECTROM,SelectRomFrame::SUBMENU_USB);
#endif
}

void Func_SR_DVD()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SELECTROM,SelectRomFrame::SUBMENU_DVD);
}

void Func_SR_PrevPage()
{
	if(current_page > 0) current_page -= 1;
	selectRomFrame_FillPage();
}

void Func_SR_NextPage()
{
	if(current_page+1 < max_page) current_page +=1;
	selectRomFrame_FillPage();
}

void Func_ReturnFromSelectRomFrame()
{
	//Clean up memory
	for (int i = 0; i<NUM_FILE_SLOTS; i++)
	{
		if (fileTextures[i])
#ifdef HW_RVL
			__lwp_heap_free(boxartTexCache, fileTextures[i]);
#else
			free(fileTextures[i]);
#endif
		fileTextures[i] = NULL;
	}
	if(dir_entries){ free(dir_entries); dir_entries = NULL; }
	if(rom_headers){ free(rom_headers); rom_headers = NULL; }
		
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}

void Func_SR_Select1() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 0); }

void Func_SR_Select2() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 1); }

void Func_SR_Select3() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 2); }

void Func_SR_Select4() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 3); }

void Func_SR_Select5() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 4); }

void Func_SR_Select6() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 5); }

void Func_SR_Select7() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 6); }

void Func_SR_Select8() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 7); }

void Func_SR_Select9() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 8); }

void Func_SR_Select10() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 9); }

void Func_SR_Select11() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 10); }

void Func_SR_Select12() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 11); }

void Func_SR_Select13() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 12); }

void Func_SR_Select14() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 13); }

void Func_SR_Select15() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 14); }

void Func_SR_Select16() { selectRomFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 15); }

int loadROM(fileBrowser_file*);

static int dir_comparator(const void* _x, const void* _y){
	const fileBrowser_file* x = (const fileBrowser_file*)_x;
	const fileBrowser_file* y = (const fileBrowser_file*)_y;
	int xIsDir = x->attr & FILE_BROWSER_ATTR_DIR;
	int yIsDir = y->attr & FILE_BROWSER_ATTR_DIR;
	// Directories go on top, otherwise alphabetical
	if(xIsDir != yIsDir)
		return yIsDir - xIsDir;
	else
		return strcasecmp(x->name, y->name);
}

void selectRomFrame_OpenDirectory(fileBrowser_file* dir)
{
	// Free the old menu stuff
//	if(menu_items){  free(menu_items);  menu_items  = NULL; }
	if(dir_entries){ free(dir_entries); dir_entries = NULL; }
	if(rom_headers){ free(rom_headers); rom_headers = NULL; }
		
	// Read the directories and return on error
	num_entries = romFile_readDir(dir, &dir_entries, 1, 1);
	if(num_entries <= 0)
	{ 
		if(dir_entries) { free(dir_entries); dir_entries = NULL; } 
		selectRomFrame_Error(dir, num_entries); 
		return;
	}
	
	// Sort the listing
	qsort(dir_entries, num_entries, sizeof(fileBrowser_file), dir_comparator);
	
	// Read all headers
	rom_headers = (rom_header*) malloc(sizeof(rom_header)*num_entries);
	for(int i = 0; i < num_entries; i++) {
		fileBrowser_file f;
		memcpy(&f, &dir_entries[i], sizeof(fileBrowser_file));
		//print_gecko("reading header %s\r\n",&f.name[0]);
		romFile_seekFile(&f, 0, FILE_BROWSER_SEEK_SET);
		if(romFile_readHeader(&f, &rom_headers[i], sizeof(rom_header))!=sizeof(rom_header)) {
			break;	// give up on the first read error
		}
		byte_swap((char*)&rom_headers[i], sizeof(rom_header), init_byte_swap(*(u32*)&rom_headers[i]));
		//print_gecko("header CRC %08X\r\n",rom_headers[i].CRC1);
	}

	current_page = 0;
	max_page = (int)ceil((float)num_entries/NUM_FILE_SLOTS);
	selectRomFrame_FillPage();
}

void selectRomFrame_Error(fileBrowser_file* dir, int error_code)
{
	char feedback_string[256];
	//disable all buttons
//	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
//		FRAME_BUTTONS[i].button->setActive(false);
	for (int i = 0; i<NUM_FILE_SLOTS; i++)
		FRAME_BUTTONS[i+5].buttonString = FRAME_STRINGS[5];
	if(error_code == NO_HW_ACCESS) {
  	sprintf(feedback_string,"DVDX v2 not found");
	}
	else if(error_code == NO_DISC) {
  	sprintf(feedback_string,"NO Disc Inserted");
	}
	//set first entry to read 'error' and return to main menu
	else if(dir->name)
	  sprintf(feedback_string,"Error opening directory \"%s\"",&dir->name[0]);
	else
	  strcpy(feedback_string,"An error occured");
/*	FRAME_BUTTONS[2].buttonString = feedback_string;
	FRAME_BUTTONS[2].button->setClicked(Func_ReturnFromFileBrowserFrame);
	FRAME_BUTTONS[2].button->setActive(true);
	for (int i = 1; i<NUM_FILE_SLOTS; i++)
		FRAME_BUTTONS[i+2].buttonString = FRAME_STRINGS[2];
	FRAME_BUTTONS[2].button->setNextFocus(menu::Focus::DIRECTION_UP, NULL);
	FRAME_BUTTONS[2].button->setNextFocus(menu::Focus::DIRECTION_DOWN, NULL);
	pMenuContext->getFrame(MenuContext::FRAME_FILEBROWSER)->setDefaultFocus(FRAME_BUTTONS[2].button);
	menu::Focus::getInstance().clearPrimaryFocus();*/
	menu::MessageBox::getInstance().setMessage(feedback_string);
//	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
//	Func_ReturnFromSelectRomFrame();
}

void selectRomFrame_FillPage()
{
	//set entries according to page
	for (int i = 0; i < NUM_FILE_SLOTS; i++)
	{
		int btn_ind = i+5;
		if ((current_page*NUM_FILE_SLOTS) + i < num_entries)
		{
			if(dir_entries[i+(current_page*NUM_FILE_SLOTS)].attr & FILE_BROWSER_ATTR_DIR)
			{
				FRAME_BUTTONS[btn_ind].button->setLabelColor((GXColor) {255,50,50,255});
				memset(fileTextures[i], 0xFF, BOXART_TEX_SIZE);
				DCFlushRange(fileTextures[i], BOXART_TEX_SIZE);
			}
			else
			{
				FRAME_BUTTONS[btn_ind].button->setLabelColor((GXColor) {255,255,255,255});
				//print_gecko("Loading Boxart for %s with CRC: %08X\r\n",&rom_headers[i+(current_page*NUM_FILE_SLOTS)].nom,
				//			rom_headers[i+(current_page*NUM_FILE_SLOTS)].CRC1);
				//load boxart
				BOXART_Init();
				BOXART_LoadTexture(rom_headers[i+(current_page*NUM_FILE_SLOTS)].CRC1,(char*) fileTextures[i]);
				DCFlushRange(fileTextures[i], BOXART_TEX_SIZE);
			}
		}
		else
		{
			FRAME_BUTTONS[btn_ind].button->setLabelColor((GXColor) {255,255,255,128});
			memset(fileTextures[i], 0xFF, BOXART_TEX_SIZE);
			DCFlushRange(fileTextures[i], BOXART_TEX_SIZE);
		}
	}
	GX_InvalidateTexAll();

	//activate next/prev buttons
	if (current_page > 0) FRAME_BUTTONS[3].button->setActive(true);
	else				  FRAME_BUTTONS[3].button->setActive(false);
	if (current_page+1 < max_page) FRAME_BUTTONS[4].button->setActive(true);
	else						   FRAME_BUTTONS[4].button->setActive(false);

	//Fill in page number
	snprintf(FRAME_STRINGS[6],14,"Page %i of %i",current_page+1,max_page);
}

extern BOOL hasLoadedROM;
extern int rom_length;
extern int autoSaveLoaded;
void Func_SetPlayGame();
void Func_MMRefreshStateInfo();
void Func_MMPlayGame();

void selectRomFrame_LoadFile(int i)
{
	if (i >= num_entries)
		return;
	char feedback_string[256];
	if(dir_entries[i].attr & FILE_BROWSER_ATTR_DIR){
		// Here we are 'recursing' into a subdirectory
		// We have to do a little dance here to avoid a dangling pointer
		fileBrowser_file* dir = (fileBrowser_file*)malloc( sizeof(fileBrowser_file) );
		memcpy(dir, dir_entries+i, sizeof(fileBrowser_file));
		selectRomFrame_OpenDirectory(dir);
		free(dir);
		menu::Focus::getInstance().clearPrimaryFocus();
	} else {
		// We must select this file
		fileBrowser_file f;
		memcpy(&f, &dir_entries[i], sizeof(fileBrowser_file));
		//Clear memory from frame
		Func_ReturnFromSelectRomFrame();

		int ret = loadROM( &f );

		//Clear FB images
		memset(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_CURRENT_FB)->getTexture(), 0x00, FB_THUMB_SIZE);
		DCFlushRange(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_CURRENT_FB)->getTexture(), FB_THUMB_SIZE);
		Func_MMRefreshStateInfo();

		if(hasLoadedROM) 
		{
			Func_SetPlayGame();
			//Load boxart:
			BOXART_Init();
			BOXART_LoadTexture(ROM_HEADER.CRC1,(char*) menu::Resources::getInstance().getImage(menu::Resources::IMAGE_BOXART_FRONT)->getTexture());
			GX_InvalidateTexAll();
		}
		else //ClearBoxArt
		{
			memset(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_BOXART_FRONT)->getTexture(), 0x00, BOXART_TEX_SIZE);
		}
		DCFlushRange(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_BOXART_FRONT)->getTexture(), BOXART_TEX_SIZE);
		GX_InvalidateTexAll();

		if(!ret){	// If the read succeeded.

			strcpy(feedback_string, "Loaded ");
			strncat(feedback_string, filenameFromAbsPath(f.name), 36-7);

			char RomInfo[512] = "";
			char buffer [50];
			char buffer2 [50];
			strcat(RomInfo,feedback_string);
			sprintf(buffer,"\n\nRom name: %s\n",ROM_SETTINGS.goodname);
			strcat(RomInfo,buffer);
			sprintf(buffer,"Rom size: %d Mb\n",rom_length/1024/1024);
			strcat(RomInfo,buffer);
			if(ROM_HEADER.Manufacturer_ID == 'N') sprintf(buffer,"Manufacturer: Nintendo\n");
			else sprintf(buffer,"Manufacturer: %x\n", (unsigned int)(ROM_HEADER.Manufacturer_ID));
			strcat(RomInfo,buffer);
		    countrycodestring(ROM_HEADER.Country_code&0xFF, buffer2);
			sprintf(buffer,"Country: %s\n",buffer2);
			strcat(RomInfo,buffer);
//			sprintf(buffer,"CRCs: %8X %8X\n",ROM_HEADER.CRC1,ROM_HEADER.CRC2);
//			strcat(RomInfo,buffer);
			switch (autoSaveLoaded)
			{
			case NATIVESAVEDEVICE_NONE:
				break;
			case NATIVESAVEDEVICE_SD:
				strcat(RomInfo,"\nFound & loaded save from SD card\n");
				break;
			case NATIVESAVEDEVICE_USB:
				strcat(RomInfo,"\nFound & loaded save from USB device\n");
				break;
			case NATIVESAVEDEVICE_CARDA:
				strcat(RomInfo,"\nFound & loaded save from memcard in slot A\n");
				break;
			case NATIVESAVEDEVICE_CARDB:
				strcat(RomInfo,"\nFound & loaded save from memcard in slot B\n");
				break;
			}
			autoSaveLoaded = NATIVESAVEDEVICE_NONE;

			menu::MessageBox::getInstance().setMessage(RomInfo);
			
			//Launch game
			Func_MMPlayGame();
		}
		else		// If not.
		{
  		switch(ret) {
    		case ROM_CACHE_ERROR_READ:
			    strcpy(feedback_string,"A read error occured");
			    break;
			  case ROM_CACHE_INVALID_ROM:
			   strcpy(feedback_string,"Invalid ROM type");
			    break;
			  default:
			    strcpy(feedback_string,"An error has occured");
			    break;
		  }

			menu::MessageBox::getInstance().setMessage(feedback_string);
		}
	}
}
#endif