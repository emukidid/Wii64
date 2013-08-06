/**
 * Wii64 - MiniInfoBar.cpp
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

#include "MiniInfoBar.h"
#include "GuiResources.h"
#include "GraphicsGX.h"
#include "IPLFont.h"
#include "Image.h"
#include "FocusManager.h"
#include <math.h>
#include <gccore.h>
#include "../main/wii64config.h"

extern "C" {
#include "../gc_input/controller.h"
#include "../main/rom.h"
}

namespace menu {

MiniInfoBar::MiniInfoBar()
{
						//Focus color			Inactive color		  Active color			Selected color		  Label color
//	GXColor colors[5] = {{255, 100, 100, 255}, {255, 255, 255,  70}, {255, 255, 255, 130}, {255, 255, 255, 255}, {255, 255, 255, 255}};

}

MiniInfoBar::~MiniInfoBar()
{
}

extern "C" BOOL hasLoadedROM;
extern "C" char autoSave;
extern "C" BOOL sramWritten;
extern "C" BOOL eepromWritten;
extern "C" BOOL mempakWritten;
extern "C" BOOL flashramWritten;

void MiniInfoBar::drawComponent(Graphics& gfx)
{
/*	int box_x = 50;
	int box_y = 0;
	int width = 235;
	int height = 340;
	int labelScissor = 5;
	GXColor activeColor = (GXColor) {255, 255, 255, 255};
	//Draw Main Info Box
	GXColor boxColor = (GXColor) {87, 90, 100,128};
	gfx.setTEV(GX_PASSCLR);
	gfx.enableBlending(true);
	gfx.setColor(boxColor);
	gfx.fillRect(box_x, box_y, width, height);
	//Write ROM Status Info
	gfx.enableScissor(box_x + labelScissor, box_y, width - 2*labelScissor, height);
	char buffer [51];
	int text_y = 100;
	if(!hasLoadedROM)
	{
		IplFont::getInstance().drawInit(activeColor);
		sprintf(buffer,"No ROM Loaded");
		IplFont::getInstance().drawString((int) box_x + 15, (int) text_y, buffer, 0.8, false);
	}
	else
	{
		IplFont::getInstance().drawInit(activeColor);
		sprintf(buffer,"%s",ROM_SETTINGS.goodname);
//		IplFont::getInstance().drawString((int) box_x + 15, (int) text_y, buffer, 0.8, false);
		text_y += 20*IplFont::getInstance().drawStringWrap((int) box_x + 15, (int) text_y, buffer, 0.8, false, width - 2*15, 20);
	    countrycodestring(ROM_HEADER->Country_code&0xFF, buffer);
		text_y += 13;
		IplFont::getInstance().drawString((int) box_x + 15, (int) text_y, buffer, 0.7, false);
		if (autoSave)
			sprintf(buffer,"AutoSave Enabled");
		else if (!flashramWritten && !sramWritten && !eepromWritten && !mempakWritten)
			sprintf(buffer,"Nothing to Save");
		else
			sprintf(buffer,"Game Needs Saving");
		text_y += 25;
		IplFont::getInstance().drawString((int) box_x + 15, (int) text_y, buffer, 0.7, false);
	}
	gfx.disableScissor();*/

	//draw Wii64 logo
	Resources::getInstance().getImage(Resources::IMAGE_LOGO)->activateImage(GX_TEXMAP0);
	gfx.setTEV(GX_REPLACE);
	gfx.enableBlending(true);
#ifdef HW_RVL
//	gfx.drawImage(0, 75, 380, 144, 52, 0, 1, 0, 1);
	gfx.drawImage(0, 40, 40, 144, 52, 0, 1, 0, 1);
#else
//	gfx.drawImage(0, 75, 380, 192, 52, 0, 1, 0, 1);
	gfx.drawImage(0, 40, 40, 192, 52, 0, 1, 0, 1);
#endif
	gfx.setTEV(GX_PASSCLR);

	//draw current FB thumbnail
	Resources::getInstance().getImage(Resources::IMAGE_CURRENT_FB)->activateImage(GX_TEXMAP0);
	gfx.setTEV(GX_REPLACE);
	gfx.enableBlending(true);
	gfx.drawImage(0, 70, 114, 160, 120, 0, 1, 0, 1);
	gfx.setTEV(GX_PASSCLR);
	
	//draw state FB thumbnail
	Resources::getInstance().getImage(Resources::IMAGE_STATE_FB)->activateImage(GX_TEXMAP0);
	gfx.setTEV(GX_REPLACE);
	gfx.enableBlending(true);
	gfx.drawImage(0, 410, 114, 160, 120, 0, 1, 0, 1);
	gfx.setTEV(GX_PASSCLR);
	
}

void MiniInfoBar::updateTime(float deltaTime)
{
	//Overload in Component class
	//Add interpolator class & update here?
}

Component* MiniInfoBar::updateFocus(int direction, int buttonsPressed)
{
	return NULL;
}

} //namespace menu 
