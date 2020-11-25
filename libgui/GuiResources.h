/**
 * Wii64 - GuiResources.h
 * Copyright (C) 2009, 2013, 2014 sepp256
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

#ifndef GUIRESOURCES_H
#define GUIRESOURCES_H

#include "GuiTypes.h"

namespace menu {

// 1/4 would be 160x120 = 19200 pix
// 1/8 would be 80x60 = 4800 pix
# define FB_THUMB_WD 160
# define FB_THUMB_HT 120
# define FB_THUMB_BPP 2 //4 or 2
# define FB_THUMB_FMT GX_TF_RGB565 //GX_TF_RGBA8 or GX_TF_RGB565
# define FB_THUMB_SIZE (FB_THUMB_WD*FB_THUMB_HT*FB_THUMB_BPP)

class Resources
{
public:
	Image* getImage(int image);
	static Resources& getInstance()
	{
		static Resources obj;
		return obj;
	}

	enum Images
	{
		IMAGE_DEFAULT_BUTTON=1,
		IMAGE_DEFAULT_BUTTONFOCUS,
		IMAGE_STYLEA_BUTTON,
		IMAGE_STYLEA_BUTTONFOCUS,
		IMAGE_STYLEA_BUTTONSELECTOFF,
		IMAGE_STYLEA_BUTTONSELECTOFFFOCUS,
		IMAGE_STYLEA_BUTTONSELECTON,
		IMAGE_STYLEA_BUTTONSELECTONFOCUS,
		IMAGE_MENU_BACKGROUND,
		IMAGE_LOGO,
		IMAGE_CONTROLLER_EMPTY,
		IMAGE_CONTROLLER_GAMECUBE,
#ifdef HW_RVL
		IMAGE_CONTROLLER_CLASSIC,
		IMAGE_CONTROLLER_WIIMOTENUNCHUCK,
		IMAGE_CONTROLLER_WIIMOTE,
		IMAGE_CONTROLLER_WIIUPRO,
#endif
		IMAGE_N64_CONTROLLER,
#if !(defined(HW_DOL) && defined(USE_EXPANSION))
		IMAGE_CURRENT_FB,
		IMAGE_STATE_FB,
		IMAGE_BOXART_FRONT,
		IMAGE_BOXART_SPINE,
		IMAGE_BOXART_BACK
#endif
	};

private:
	Resources();
	~Resources();
	Image *defaultButtonImage, *defaultButtonFocusImage;
	Image *styleAButtonImage, *styleAButtonFocusImage;
	Image *styleAButtonSelectOffImage, *styleAButtonSelectOffFocusImage;
	Image *styleAButtonSelectOnImage, *styleAButtonSelectOnFocusImage;
	Image *menuBackgroundImage;
	Image *logoImage;
	Image *controllerEmptyImage, *controllerGamecubeImage;
#ifdef HW_RVL
	Image *controllerClassicImage, *controllerWiimoteNunchuckImage;
	Image *controllerWiimoteImage, *controllerWiiUProImage;
#endif
	Image *n64ControllerImage;
#if !(defined(HW_DOL) && defined(USE_EXPANSION))
	Image *currentFramebufferImage, *stateFramebufferImage;
	u8	*currentFramebufferTexture, *stateFramebufferTexture;
	Image *boxartFrontImage, *boxartSpineImage, *boxartBackImage;
	u8	*boxartTexture;
#endif

};

} //namespace menu 

#endif
