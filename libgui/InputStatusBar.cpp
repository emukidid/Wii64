/**
 * Wii64 - InputStatusBar.cpp
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

#include "InputStatusBar.h"
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

InputStatusBar::InputStatusBar(float x, float y)
		: x(x),
		  y(y)
{
						//Focus color			Inactive color		  Active color			Selected color		  Label color
//	GXColor colors[5] = {{255, 100, 100, 255}, {255, 255, 255,  70}, {255, 255, 255, 130}, {255, 255, 255, 255}, {255, 255, 255, 255}};

}

InputStatusBar::~InputStatusBar()
{
}

void InputStatusBar::drawComponent(Graphics& gfx)
{
//	GXColor activeColor = (GXColor) {255, 255, 255, 255};
	GXColor inactiveColor = (GXColor) {192, 192, 192, 192};
	GXColor controllerColors[5] = {	{  1,  29, 169, 255}, //blue
									{229,  29,  19, 255}, //orange/red
									{  8, 147,  48, 255}, //green
									{255, 192,   1, 255}, //yellow/gold
									{150, 150, 255, 255}};
//	char statusText[50];
	Image* statusIcon = NULL;
	char buffer [51];

	//Update controller availability
	for (int i = 0; i < 4; i++)
	{
		switch (padType[i])
		{
		case PADTYPE_GAMECUBE:
			controller_GC.available[(int)padAssign[i]] = (gc_connected & (1<<padAssign[i])) ? 1 : 0;
			if (controller_GC.available[(int)padAssign[i]])
			{
//				gfx.setColor(activeColor);
//				IplFont::getInstance().drawInit(activeColor);
				gfx.setColor(controllerColors[i]);
				IplFont::getInstance().drawInit(controllerColors[i]);
			}
			else
			{
				gfx.setColor(inactiveColor);
				IplFont::getInstance().drawInit(inactiveColor);
			}
			statusIcon = Resources::getInstance().getImage(Resources::IMAGE_CONTROLLER_GAMECUBE);
//			sprintf (statusText, "Pad%d: GC%d", i+1, padAssign[i]+1);
			break;
#ifdef HW_RVL
		case PADTYPE_WII:
			u32 type;
			s32 err;
			err = WPAD_Probe((int)padAssign[i], &type);
			controller_Classic.available[(int)padAssign[i]] = (err == WPAD_ERR_NONE && type == WPAD_EXP_CLASSIC) ? 1 : 0;
			controller_WiimoteNunchuk.available[(int)padAssign[i]] = (err == WPAD_ERR_NONE && type == WPAD_EXP_NUNCHUK) ? 1 : 0;
			controller_Wiimote.available[(int)padAssign[i]] = (err == WPAD_ERR_NONE && type == WPAD_EXP_NONE) ? 1 : 0;
			if (controller_Classic.available[(int)padAssign[i]])
			{
				assign_controller(i, &controller_Classic, (int)padAssign[i]);
//				gfx.setColor(activeColor);
//				IplFont::getInstance().drawInit(activeColor);
				gfx.setColor(controllerColors[i]);
				IplFont::getInstance().drawInit(controllerColors[i]);
				statusIcon = Resources::getInstance().getImage(Resources::IMAGE_CONTROLLER_CLASSIC);
//				sprintf (statusText, "Pad%d: CC%d", i+1, padAssign[i]+1);
			}
			else if (controller_WiimoteNunchuk.available[(int)padAssign[i]])
			{
				assign_controller(i, &controller_WiimoteNunchuk, (int)padAssign[i]);
//				gfx.setColor(activeColor);
//				IplFont::getInstance().drawInit(activeColor);
				gfx.setColor(controllerColors[i]);
				IplFont::getInstance().drawInit(controllerColors[i]);
				statusIcon = Resources::getInstance().getImage(Resources::IMAGE_CONTROLLER_WIIMOTENUNCHUCK);
//				sprintf (statusText, "Pad%d: WM+N%d", i+1, padAssign[i]+1);
			}
			else if (controller_Wiimote.available[(int)padAssign[i]])
			{
				assign_controller(i, &controller_Wiimote, (int)padAssign[i]);
//				gfx.setColor(activeColor);
//				IplFont::getInstance().drawInit(activeColor);
				gfx.setColor(controllerColors[i]);
				IplFont::getInstance().drawInit(controllerColors[i]);
				statusIcon = Resources::getInstance().getImage(Resources::IMAGE_CONTROLLER_WIIMOTE);
//				sprintf (statusText, "Pad%d: WM%d", i+1, padAssign[i]+1);
			}
			else
			{
				gfx.setColor(inactiveColor);
				IplFont::getInstance().drawInit(inactiveColor);
				statusIcon = Resources::getInstance().getImage(Resources::IMAGE_CONTROLLER_WIIMOTE);
//				sprintf (statusText, "Pad%d: Wii%d", i+1, padAssign[i]+1);
			}

			break;
#endif
		case PADTYPE_NONE:
			gfx.setColor(inactiveColor);
			IplFont::getInstance().drawInit(inactiveColor);
			statusIcon = Resources::getInstance().getImage(Resources::IMAGE_CONTROLLER_EMPTY);
//			sprintf (statusText, "Pad%d: None", i+1);
			break;
		}
//		IplFont::getInstance().drawString((int) 540, (int) 150+30*i, statusText, 1.0, true);
//		IplFont::getInstance().drawString((int) box_x+width/2, (int) 215+30*i, statusText, 1.0, true);
		int base_x = x + 53*i;
		int base_y = y;
		//draw numbers
		sprintf(buffer,"%d",i+1);
		IplFont::getInstance().drawString((int) base_x+36, (int) base_y+10, buffer, 0.8, true);
		if (padType[i]!=PADTYPE_NONE)
		{
			sprintf(buffer,"%d",padAssign[i]+1);
			IplFont::getInstance().drawString((int) base_x+37, (int) base_y+52, buffer, 0.8, true);
		}
		//draw icon
		statusIcon->activateImage(GX_TEXMAP0);
		GX_SetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_RASC);
		GX_SetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
		GX_SetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_RASA,GX_CA_TEXA,GX_CA_ZERO);
		GX_SetTevAlphaOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
		gfx.enableBlending(true);
		gfx.drawImage(0, base_x, base_y, 48, 64, 0, 1, 0, 1);
	}
}

void InputStatusBar::updateTime(float deltaTime)
{
	//Overload in Component class
	//Add interpolator class & update here?
}

Component* InputStatusBar::updateFocus(int direction, int buttonsPressed)
{
	return NULL;
}

} //namespace menu 
