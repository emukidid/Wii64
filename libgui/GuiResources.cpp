/**
 * Wii64 - GuiResources.cpp
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

#include "GuiResources.h"
#include "Image.h"
#include "resources.h"
#include "../menu/MenuResources.h"

namespace menu {

Resources::Resources()
{
	defaultButtonImage = new Image(ButtonTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	styleAButtonImage = new Image(StyleAButtonTexture, 8, 56, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	styleAButtonFocusImage = new Image(StyleAButtonFocusTexture, 8, 56, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	styleAButtonSelectOffImage = new Image(StyleAButtonSelectOffTexture, 8, 56, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	styleAButtonSelectOffFocusImage = new Image(StyleAButtonSelectOffFocusTexture, 8, 56, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	styleAButtonSelectOnImage = new Image(StyleAButtonSelectOnTexture, 8, 56, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	styleAButtonSelectOnFocusImage = new Image(StyleAButtonSelectOnFocusTexture, 8, 56, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	menuBackgroundImage = new Image(BackgroundTexture, 424, 240, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
#ifdef HW_RVL
	logoImage = new Image(LogoTexture, 144, 52, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
#else
	logoImage = new Image(LogoTexture, 192, 52, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
#endif
	controllerEmptyImage = new Image(ControlEmptyTexture, 48, 64, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
	controllerGamecubeImage = new Image(ControlGamecubeTexture, 48, 64, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
#ifdef HW_RVL
	controllerClassicImage = new Image(ControlClassicTexture, 48, 64, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
	controllerWiimoteNunchuckImage = new Image(ControlWiimoteNunchuckTexture, 48, 64, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
	controllerWiimoteImage = new Image(ControlWiimoteTexture, 48, 64, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
	controllerWiiUProImage = new Image(ControlWiiUProTexture, 48, 64, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
#endif
	n64ControllerImage = new Image(N64ControllerTexture, 208, 200, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);

#if !(defined(GC_BASIC))
	//Thumbnail images for current FB and state FB
	currentFramebufferTexture = (u8*) memalign(32, FB_THUMB_SIZE);
	stateFramebufferTexture = (u8*) memalign(32, FB_THUMB_SIZE);
	memset(currentFramebufferTexture, 0x00, FB_THUMB_SIZE);
	memset(stateFramebufferTexture, 0x00, FB_THUMB_SIZE);
	DCFlushRange(currentFramebufferTexture, FB_THUMB_SIZE);
	DCFlushRange(stateFramebufferTexture, FB_THUMB_SIZE);
	currentFramebufferImage = new Image(currentFramebufferTexture, FB_THUMB_WD, FB_THUMB_HT, FB_THUMB_FMT, GX_CLAMP, GX_CLAMP, GX_FALSE);
	stateFramebufferImage = new Image(stateFramebufferTexture, FB_THUMB_WD, FB_THUMB_HT, FB_THUMB_FMT, GX_CLAMP, GX_CLAMP, GX_FALSE);

	//Image for boxart texture
	boxartTexture = (u8*) memalign(32, BOXART_TEX_SIZE);
	memset(boxartTexture, 0xFF, BOXART_TEX_SIZE);
	DCFlushRange(boxartTexture, BOXART_TEX_SIZE);
	boxartFrontImage = new Image(boxartTexture, 
								BOXART_TEX_WD, BOXART_TEX_FRONT_HT, BOXART_TEX_FMT, GX_CLAMP, GX_CLAMP, GX_FALSE);
	boxartSpineImage = new Image(boxartTexture+BOXART_TEX_FRONT_SIZE, 
								BOXART_TEX_WD, BOXART_TEX_SPINE_HT, BOXART_TEX_FMT, GX_CLAMP, GX_CLAMP, GX_FALSE);
	boxartBackImage = new Image(boxartTexture+BOXART_TEX_FRONT_SIZE+BOXART_TEX_SPINE_SIZE,
								BOXART_TEX_WD, BOXART_TEX_FRONT_HT, BOXART_TEX_FMT, GX_CLAMP, GX_CLAMP, GX_FALSE);
#endif
}

Resources::~Resources()
{
	delete defaultButtonImage;
	delete defaultButtonFocusImage;
	delete styleAButtonImage;
	delete styleAButtonFocusImage;
	delete styleAButtonSelectOffImage;
	delete styleAButtonSelectOffFocusImage;
	delete styleAButtonSelectOnImage;
	delete styleAButtonSelectOnFocusImage;
	delete menuBackgroundImage;
	delete logoImage;
	delete controllerEmptyImage;
	delete controllerGamecubeImage;
#ifdef HW_RVL
	delete controllerClassicImage;
	delete controllerWiimoteNunchuckImage;
	delete controllerWiimoteImage;
	delete controllerWiiUProImage;
#endif
	delete n64ControllerImage;
#if !(defined(GC_BASIC))
	delete currentFramebufferImage;
	delete stateFramebufferImage;
	delete boxartFrontImage;
	delete boxartSpineImage;
	delete boxartBackImage;
	free(currentFramebufferTexture);
	free(stateFramebufferTexture);
	free(boxartTexture);
#endif
}

Image* Resources::getImage(int image)
{
	Image* returnImage = NULL;
	switch (image)
	{
	case IMAGE_DEFAULT_BUTTON:
		returnImage = defaultButtonImage;
		break;
	case IMAGE_DEFAULT_BUTTONFOCUS:
		returnImage = defaultButtonFocusImage;
		break;
	case IMAGE_STYLEA_BUTTON:
		returnImage = styleAButtonImage;
		break;
	case IMAGE_STYLEA_BUTTONFOCUS:
		returnImage = styleAButtonFocusImage;
		break;
	case IMAGE_STYLEA_BUTTONSELECTOFF:
		returnImage = styleAButtonSelectOffImage;
		break;
	case IMAGE_STYLEA_BUTTONSELECTOFFFOCUS:
		returnImage = styleAButtonSelectOffFocusImage;
		break;
	case IMAGE_STYLEA_BUTTONSELECTON:
		returnImage = styleAButtonSelectOnImage;
		break;
	case IMAGE_STYLEA_BUTTONSELECTONFOCUS:
		returnImage = styleAButtonSelectOnFocusImage;
		break;
	case IMAGE_MENU_BACKGROUND:
		returnImage = menuBackgroundImage;
		break;
	case IMAGE_LOGO:
		returnImage = logoImage;
		break;
	case IMAGE_CONTROLLER_EMPTY:
		returnImage = controllerEmptyImage;
		break;
	case IMAGE_CONTROLLER_GAMECUBE:
		returnImage = controllerGamecubeImage;
		break;
#ifdef HW_RVL
	case IMAGE_CONTROLLER_CLASSIC:
		returnImage = controllerClassicImage;
		break;
	case IMAGE_CONTROLLER_WIIMOTENUNCHUCK:
		returnImage = controllerWiimoteNunchuckImage;
		break;
	case IMAGE_CONTROLLER_WIIMOTE:
		returnImage = controllerWiimoteImage;
		break;
	case IMAGE_CONTROLLER_WIIUPRO:
		returnImage = controllerWiiUProImage;
		break;
#endif
	case IMAGE_N64_CONTROLLER:
		returnImage = n64ControllerImage;
		break;
#if !(defined(GC_BASIC))
	case IMAGE_CURRENT_FB:
		returnImage = currentFramebufferImage;
		break;
	case IMAGE_STATE_FB:
		returnImage = stateFramebufferImage;
		break;
	case IMAGE_BOXART_FRONT:
		returnImage = boxartFrontImage;
		break;
	case IMAGE_BOXART_SPINE:
		returnImage = boxartSpineImage;
		break;
	case IMAGE_BOXART_BACK:
		returnImage = boxartBackImage;
		break;
#endif
	}
	return returnImage;
}

} //namespace menu 
