/**
 * Wii64 - MiniMenuFrame.cpp
 * Copyright (C) 2013 sepp256
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
#include "MiniMenuFrame.h"
#include "SettingsFrame.h"
#include "../libgui/Button.h"
#include "../libgui/TextBox.h"
#include "../libgui/Gui.h"
#include "../libgui/GuiResources.h"
#include "../libgui/InputStatusBar.h"
#include "../libgui/resources.h"
//#include "../libgui/InputManager.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"
#include "../main/wii64config.h"
#ifdef DEBUGON
# include <debug.h>
#endif
extern "C" {
#include "../gc_memory/memory.h"
#include "../gc_memory/Saves.h"
#include "../main/plugin.h"
#include "../main/rom.h"
#include "../main/savestates.h"
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-CARD.h"
#include "../r4300/Recomp-Cache.h"
}

void Func_MMResetROM();
void Func_MMRefreshStateInfo();
void Func_MMSaveState();
void Func_MMLoadState();
void Func_MMExitToLoader();
void Func_MMSelectROM();
void Func_MMAdvancedMenu();
void Func_MMControls();
void Func_MMPlayGame();
void Func_MMRomInfo();

#define NUM_FRAME_BUTTONS 8
#define FRAME_BUTTONS miniMenuFrameButtons
#define FRAME_STRINGS miniMenuFrameStrings
#define NUM_FRAME_TEXTBOXES 4
#define FRAME_TEXTBOXES miniMenuFrameTextBoxes

char FRAME_STRINGS[9][50] = //[16] =
	{ "Reset",
	  "Save State",
	  "Load State 0",
	  "Quit",
	  "New ROM",
	  "Advanced",
	  "(Home) Quit",
	  "(B) Resume Game",
	  "Slot 0"};

//                   [Reset]
// [Save State]    <Game Info>    [Load State]
// [Save State]    <Game Info>    [Load State]
// [Quit]         [Select ROM]      [Advanced]
//  "<home_icon>/<start> Quit"  "<b> Resume"
//
//Behavior:
// - On emulator load, attempt to autoload the last ROM
// - L/R buttons should cycle Save State slot
// - By default Nothing or "Select ROM" is highlighted
// - left/right/up/down should move to Save/Load/Reset/Resume first
// - Home button brings up question "Exit Wii64?"

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
{ //	button	buttonStyle	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				returnFunc
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[0],	240.0,	 60.0,	160.0,	56.0,	 4,	 7,	 1,	 2,	Func_MMResetROM,		Func_MMPlayGame }, // Reset ROM
	{	NULL,	BTN_DEFAULT,NULL,	 			60.0,	103.0,	180.0,	185.0,	 0,	 3,	-1,	 4,	Func_MMSaveState,		Func_MMPlayGame }, // Save State
	{	NULL,	BTN_DEFAULT,NULL,				400.0,	103.0,	180.0,	185.0,	 0,	 5,	 4,	-1,	Func_MMLoadState,		Func_MMPlayGame }, // Load State
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[3],	 50.0,	350.0,	150.0,	56.0,	 1,	-1,	-1,	 6,	Func_MMExitToLoader,	Func_MMPlayGame }, // Quit
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[4],	240.0,	270.0,	160.0,	56.0,	 7,	 6,	 1,	 2,	Func_MMSelectROM,		Func_MMPlayGame }, // Select ROM
	{	NULL,	BTN_A_NRM,	FRAME_STRINGS[5],	440.0,	350.0,	150.0,	56.0,	 2,	-1,	 6,	-1,	Func_MMAdvancedMenu,	Func_MMPlayGame }, // Advanced
	{	NULL,	BTN_DEFAULT,NULL,				205.0,	330.0,	230.0,	84.0,	 4,	-1,	 3,	 5,	Func_MMControls,		Func_MMPlayGame }, // Controller Settings
	{	NULL,	BTN_BOX3D,	NULL,				260.0,	136.0,	120.0,	88.0,	 0,	 4,	 1,	 2,	Func_MMRomInfo,			Func_MMPlayGame }, // Boxart Button
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
	{	NULL,	FRAME_STRINGS[1],	150.0,	263.0,	 1.0,	true }, // Save State 
	{	NULL,	FRAME_STRINGS[2],	490.0,	263.0,	 1.0,	true }, // Load State 
	{	NULL,	FRAME_STRINGS[6],	200.0,	430.0,	 1.0,	true }, // (Home) Quit
	{	NULL,	FRAME_STRINGS[7],	440.0,	430.0,	 1.0,	true }, // (B) Resume
//	{	NULL,	FRAME_STRINGS[8],	490.0,	320.0,	 1.0,	true }, // Save State Slot #
};

MiniMenuFrame::MiniMenuFrame()
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
	
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
	{
		FRAME_TEXTBOXES[i].textBox = new menu::TextBox(&FRAME_TEXTBOXES[i].textBoxString, 
										FRAME_TEXTBOXES[i].x, FRAME_TEXTBOXES[i].y, 
										FRAME_TEXTBOXES[i].scale, FRAME_TEXTBOXES[i].centered);
		add(FRAME_TEXTBOXES[i].textBox);
	}
	//Set color to white for State button labels
	GXColor textColor = {255, 255, 255, 255};
	FRAME_TEXTBOXES[0].textBox->setColor(&textColor);
	FRAME_TEXTBOXES[1].textBox->setColor(&textColor);

	//Set size of boxart button
	FRAME_BUTTONS[7].button->setBoxSize(2.0, 2.4);
	FRAME_BUTTONS[7].button->setLabelColor((GXColor) {128,128,128,128});
	
	miniInfoBar = new menu::MiniInfoBar();
	add(miniInfoBar);
	inputStatusBar = new menu::InputStatusBar(216,340);
	add(inputStatusBar);
	
	setDefaultFocus(FRAME_BUTTONS[4].button);
	setBackFunc(Func_MMPlayGame);
	setEnabled(true);

}

MiniMenuFrame::~MiniMenuFrame()
{
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
		delete FRAME_TEXTBOXES[i].textBox;
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
	delete inputStatusBar;
	delete miniInfoBar;
}

static unsigned int which_slot = 0;
static bool state_exists;
static char state_date[10], state_time[10];
extern BOOL hasLoadedROM;

void MiniMenuFrame::drawChildren(menu::Graphics &gfx)
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
					//move to next save state slot
					which_slot = (which_slot+1) %10;
					Func_MMRefreshStateInfo();
					break;
				}
				else if (currentButtonsDownGC & PAD_TRIGGER_L)
				{
					//move to the previous save state slot
					which_slot = (which_slot==0) ? 9 : (which_slot-1);
					Func_MMRefreshStateInfo();
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
						//move to next save state slot
						which_slot = (which_slot+1) %10;
						Func_MMRefreshStateInfo();
						break;
					}
					else if (currentButtonsDownWii & WPAD_CLASSIC_BUTTON_FULL_L)
					{
						//move to the previous save state slot
						which_slot = (which_slot==0) ? 9 : (which_slot-1);
						Func_MMRefreshStateInfo();
						break;
					}
				}
				else
				{
					if (currentButtonsDownWii & WPAD_BUTTON_PLUS)
					{
						//move to next save state slot
						which_slot = (which_slot+1) %10;
						Func_MMRefreshStateInfo();
						break;
					}
					else if (currentButtonsDownWii & WPAD_BUTTON_MINUS)
					{
						//move to the previous save state slot
						which_slot = (which_slot==0) ? 9 : (which_slot-1);
						Func_MMRefreshStateInfo();
						break;
					}
				}
			}
#endif //HW_RVL
		}

	//Set boxart button transparency
	if(!hasLoadedROM) 	FRAME_BUTTONS[7].button->setLabelColor((GXColor) {128,128,128,90});
	else				FRAME_BUTTONS[7].button->setLabelColor((GXColor) {255,255,255,255});

		//Draw buttons
		menu::ComponentList::const_iterator iteration;
		for (iteration = componentList.begin(); iteration != componentList.end(); iteration++)
		{
			(*iteration)->draw(gfx);
		}
	}
}

extern MenuContext *pMenuContext;

extern "C" {
void cpu_init();
void cpu_deinit();
}

void Func_SetPlayGame();

void Func_MMResetROM()
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

void Func_MMRefreshStateInfo()
{
	savestates_select_slot(which_slot);
	FRAME_STRINGS[8][5] = which_slot + '0';
	FRAME_STRINGS[2][11] = which_slot + '0';
	if(!hasLoadedROM)
	{ //Clear State FB Image
		memset(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_STATE_FB)->getTexture(), 0x00, FB_THUMB_SIZE);
		DCFlushRange(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_STATE_FB)->getTexture(), FB_THUMB_SIZE);
		GX_InvalidateTexAll();
		state_exists = false;
		return;
	}
	if(savestates_load_header(which_slot, menu::Resources::getInstance().getImage(menu::Resources::IMAGE_STATE_FB)->getTexture(), state_date, state_time))
	{
		memset(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_STATE_FB)->getTexture(), 0x00, FB_THUMB_SIZE);
		state_exists = false;
	}
	else
		state_exists = true;
	DCFlushRange(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_STATE_FB)->getTexture(), FB_THUMB_SIZE);
	GX_InvalidateTexAll();
}

void Func_MMSaveState()
{
	char question[50];
	if(!hasLoadedROM)
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
		return;
	}
	if(state_exists)
	{
		sprintf(question,"Replace state saved on\n%8s at %5s?", state_date, state_time);
		if(!menu::MessageBox::getInstance().askMessage(question))
			return;
	}
	if(!savestates_exists(which_slot, SAVESTATE)) {
		menu::MessageBox::getInstance().setMessage("Failed to create save state");
	}
	else {
		savestates_save(which_slot, menu::Resources::getInstance().getImage(menu::Resources::IMAGE_CURRENT_FB)->getTexture());
		Func_MMRefreshStateInfo();
		menu::MessageBox::getInstance().setMessage("State Saved Successfully");
	}
}

void Func_MMLoadState()
{
	char question[50];
	if(!hasLoadedROM)
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
		return;
	}
	if(state_exists)
	{
		sprintf(question,"Load state saved on\n%8s at %5s?", state_date, state_time);
		if(!menu::MessageBox::getInstance().askMessage(question))
			return;
	}
	else
		return;
	if(!savestates_exists(which_slot, LOADSTATE)) {
		menu::MessageBox::getInstance().setMessage("Save doesn't exist");
	}
	else {
		savestates_queue_load(which_slot);
		savestates_load_header(which_slot, menu::Resources::getInstance().getImage(menu::Resources::IMAGE_STATE_FB)->getTexture(),NULL,NULL);
		memcpy(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_CURRENT_FB)->getTexture(), menu::Resources::getInstance().getImage(menu::Resources::IMAGE_STATE_FB)->getTexture(), FB_THUMB_SIZE);
		DCFlushRange(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_CURRENT_FB)->getTexture(), FB_THUMB_SIZE);
		GX_InvalidateTexAll();
		menu::MessageBox::getInstance().setMessage("Game will resume from save state");
	}
}

extern char shutdown;

void Func_MMExitToLoader()
{
	if(menu::MessageBox::getInstance().askMessage("Are you sure you want to exit to loader?"))
		shutdown = 2;
}

void Func_MMSelectROM()
{
//	pMenuContext->setActiveFrame(MenuContext::FRAME_LOADROM);
	pMenuContext->setActiveFrame(MenuContext::FRAME_SELECTROM, SelectRomFrame::SUBMENU_DEFAULT);
}

void Func_MMAdvancedMenu()
{
	pMenuContext->setUseMiniMenu(false);
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}

void Func_MMControls()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS, SettingsFrame::SUBMENU_INPUT);
}

extern BOOL hasLoadedROM;
extern int rom_length;

void Func_MMRomInfo()
{
	if(!hasLoadedROM)
	{
		menu::MessageBox::getInstance().setMessage("No ROM loaded");
		return;
	}
	
	char RomInfo[256] = "";
	char buffer [50];
	char buffer2 [50];
	
	sprintf(buffer,"Rom name: %s\n",ROM_SETTINGS.goodname);
	strcat(RomInfo,buffer);
	sprintf(buffer,"Rom size: %d Mb\n",rom_length/1024/1024);
	strcat(RomInfo,buffer);
	if(ROM_HEADER.Manufacturer_ID == 'N') sprintf(buffer,"Manufacturer: Nintendo\n");
	else sprintf(buffer,"Manufacturer: %x\n", (unsigned int)(ROM_HEADER.Manufacturer_ID));
	strcat(RomInfo,buffer);
	countrycodestring(ROM_HEADER.Country_code&0xFF, buffer2);
	sprintf(buffer,"Country: %s\n",buffer2);
	strcat(RomInfo,buffer);
	sprintf(buffer,"CRCs: %08X %08X\n",(unsigned int)ROM_HEADER.CRC1,(unsigned int)ROM_HEADER.CRC2);
	strcat(RomInfo,buffer);

	menu::MessageBox::getInstance().setMessage(RomInfo);
}

extern "C" {
void pauseAudio(void);  void pauseInput(void);
void resumeAudio(void); void resumeInput(void);
void go(void); 
}

extern char menuActive;
extern char autoSave;
extern BOOL sramWritten;
extern BOOL eepromWritten;
extern BOOL mempakWritten;
extern BOOL flashramWritten;
extern "C" unsigned int usleep(unsigned int us);

void Func_MMPlayGame()
{
	if(!hasLoadedROM)
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
		return;
	}

	//Wait until 'A' or 'B'button released before play/resume game
	menu::Focus::getInstance().setFocusActive(false);
	menu::Focus::getInstance().setFreezeAction(true);
	menu::Cursor::getInstance().setFreezeAction(true);
	int buttonHeld = 1;
	while(buttonHeld)
	{
		buttonHeld = 0;
		menu::Gui::getInstance().draw();
		for (int i=0; i<4; i++)
		{
			if(PAD_ButtonsHeld(i) & (PAD_BUTTON_A | PAD_BUTTON_B)) buttonHeld++;
#ifdef HW_RVL
			WPADData* wiiPad = WPAD_Data(i);
			if(wiiPad->err == WPAD_ERR_NONE && wiiPad->btns_h & (WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A)) buttonHeld++;
			if(wiiPad->err == WPAD_ERR_NONE && wiiPad->btns_h & (WPAD_BUTTON_B | WPAD_CLASSIC_BUTTON_B)) buttonHeld++;
#endif
		}
	}
	menu::Focus::getInstance().setFocusActive(true);
	menu::Focus::getInstance().setFreezeAction(false);
	menu::Cursor::getInstance().setFreezeAction(false);
	menu::Focus::getInstance().clearPrimaryFocus();	

	menu::Gui::getInstance().gfx->clearEFB((GXColor){0, 0, 0, 0xFF}, 0x000000);
	BOXART_DeInit();
	
	pauseRemovalThread();
	resumeAudio();
	resumeInput();
	menuActive = 0;
#ifdef DEBUGON
	_break();
#endif
	go();
	RecompCache_Release(4*1024*1024);
#ifdef DEBUGON
	_break();
#endif
	//Create Thumbnail of last Framebuffer
	menu::Gui::getInstance().gfx->copyFBTex(menu::Resources::getInstance().getImage(menu::Resources::IMAGE_CURRENT_FB)->getTexture(), 
											FB_THUMB_WD, FB_THUMB_HT, FB_THUMB_FMT, FB_THUMB_BPP);

	menuActive = 1;
	pauseInput();
	pauseAudio();
  continueRemovalThread();
	
  if(autoSave==AUTOSAVE_ENABLE) {
    if(flashramWritten || sramWritten || eepromWritten || mempakWritten) {  //something needs saving
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
    	if (result==amountSaves) {  //saved all of them ok	
    		switch (nativeSaveDevice)
    		{
    			case NATIVESAVEDEVICE_SD:
    				menu::MessageBox::getInstance().fadeMessage("Automatically saved to SD card");
    				break;
    			case NATIVESAVEDEVICE_USB:
    				menu::MessageBox::getInstance().fadeMessage("Automatically saved to USB device");
    				break;
    			case NATIVESAVEDEVICE_CARDA:
    				menu::MessageBox::getInstance().fadeMessage("Automatically saved to memcard in Slot A");
    				break;
    			case NATIVESAVEDEVICE_CARDB:
    				menu::MessageBox::getInstance().fadeMessage("Automatically saved to memcard in Slot B");
    				break;
    		}
    		flashramWritten = sramWritten = eepromWritten = mempakWritten = 0;  //nothing new written since save
  		}
  	  else		
  	    menu::MessageBox::getInstance().setMessage("Failed to Save"); //one or more failed to save
      
    }
  }
//	FRAME_BUTTONS[5].buttonString = FRAME_STRINGS[6];
	menu::Cursor::getInstance().clearCursorFocus();
}

/*void Func_SetPlayGame()
{
	FRAME_BUTTONS[5].buttonString = FRAME_STRINGS[5];
}

void Func_SetResumeGame()
{
	FRAME_BUTTONS[5].buttonString = FRAME_STRINGS[6];
}*/
