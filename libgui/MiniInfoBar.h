/**
 * Wii64 - MiniInfoBar.h
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
#if !(defined(GC_BASIC))
#ifndef MINIINFOBAR_H
#define MINIINFOBAR_H

//#include "GuiTypes.h"
#include "Component.h"

namespace menu {

class MiniInfoBar : public Component
{
public:
	MiniInfoBar();
	~MiniInfoBar();
	void updateTime(float deltaTime);
	void drawComponent(Graphics& gfx);
	Component* updateFocus(int direction, int buttonsPressed);

private:

};

} //namespace menu 

#endif
#endif
