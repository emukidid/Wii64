/**
 * Wii64 - Button.h
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

#ifndef BUTTON_H
#define BUTTON_H

//#include "GuiTypes.h"
#include "Component.h"

#define BTN_DEFAULT menu::Button::BUTTON_DEFAULT
#define BTN_A_NRM menu::Button::BUTTON_STYLEA_NORMAL
#define BTN_A_SEL menu::Button::BUTTON_STYLEA_SELECT
#if !(defined(GC_BASIC))
#define BTN_BOX3D menu::Button::BUTTON_BOX3D
#endif

typedef void (*ButtonFunc)( void );

namespace menu {

class Button : public Component
{
public:
	Button(int style, char** label, float x, float y, float width, float height);
	~Button();
	void setActive(bool active);
	bool getActive();
	void setSelected(bool selected);
	void setReturn(ButtonFunc returnFn);
	void doReturn();
	void setClicked(ButtonFunc clickedFn);
	void doClicked();
	void setText(char** strPtr);
	void setFontSize(float size);
	void setLabelMode(int mode);
	void setLabelScissor(int scissor);
	void setNormalImage(Image *image);
	void setFocusImage(Image *image);
	void setSelectedImage(Image *image);
	void setSelectedFocusImage(Image *image);
	void setBoxTexture(u8* texture);
	void setBoxSize(float size, float focusSize);
	void updateTime(float deltaTime);
	void drawComponent(Graphics& gfx);
	Component* updateFocus(int direction, int buttonsPressed);
	void setButtonColors(GXColor *colors);
	void setLabelColor(GXColor color);
	enum LabelMode
	{
		LABEL_CENTER=0,
		LABEL_LEFT,
		LABEL_SCROLL,
		LABEL_SCROLLONFOCUS
	};

	enum ButtonStyle
	{
		BUTTON_DEFAULT=0,
		BUTTON_STYLEA_NORMAL,
		BUTTON_STYLEA_SELECT
#if !(defined(GC_BASIC))
		,BUTTON_BOX3D
#endif
	};

private:
	bool active, selected;
	Image	*normalImage;
	Image	*focusImage;
	Image	*selectedImage;
	Image	*selectedFocusImage;
	char** buttonText;
	int buttonStyle, labelMode, labelScissor;
	unsigned long StartTime;
	float x, y, width, height, fontSize;
	float boxSize, boxFocusSize;
	GXColor	focusColor, inactiveColor, activeColor, selectedColor, labelColor;
	ButtonFunc clickedFunc, returnFunc;
	u8*	boxTexture;

};

} //namespace menu 

#endif
