/**
 * Wii64 - MiniMenuFrame.h
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

#ifndef MINIMENUFRAME_H
#define MINIMENUFRAME_H

#include "../libgui/Frame.h"
#include "../libgui/InputStatusBar.h"
#include "../libgui/MiniInfoBar.h"
#include "MenuTypes.h"

class MiniMenuFrame : public menu::Frame
{
public:
	MiniMenuFrame();
	~MiniMenuFrame();
	void drawChildren(menu::Graphics& gfx);

private:
	menu::MiniInfoBar *miniInfoBar;
	menu::InputStatusBar *inputStatusBar;
	u16 previousButtonsGC[4];
	u32 previousButtonsWii[4];
};

#endif
