/**
 * Wii64 - SelectRomFrame.h
 * Copyright (C) 2009, 2010, 2013 sepp256
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
#if !(defined(GC_BASIC))
#ifndef SELECTROMFRAME_H
#define SELECTROMFRAME_H

#include "../libgui/Frame.h"
#include "MenuTypes.h"

class SelectRomFrame : public menu::Frame
{
public:
	SelectRomFrame();
	~SelectRomFrame();
	void activateSubmenu(int submenu);
	void drawChildren(menu::Graphics& gfx);

	enum SelectRomSubmenus
	{
		SUBMENU_DEFAULT=0,
		SUBMENU_SD,
		SUBMENU_USB,
		SUBMENU_DVD
	};

private:
	int activeSubmenu;
	u16 previousButtonsGC[4];
	u32 previousButtonsWii[4];

};

#endif
#endif
