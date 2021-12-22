/**
 * Wii64 - Box3D.h
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
#ifndef BOX3D_H
#define BOX3D_H

//#include "GuiTypes.h"
#include "Component.h"

namespace menu {

class Box3D : public Component
{
public:
	Box3D();
	~Box3D();
	void setLocation(float x, float y, float z);
	void setEnableRotate(bool enable);
	void setSize(float size);
	void setMode(int mode);
	void setTexture(u8* texture);
	void setTransparency(u8 color);
	void updateTime(float deltaTime);
	void drawComponent(Graphics& gfx);
/*	enum LogoMode
	{
		LOGO_N=0,
		LOGO_M,
		LOGO_W
	};*/

private:
	void drawQuad(u8 v0, u8 v1, u8 v2, u8 v3, u8 st0, u8 st1, u8 st2, u8 st3, u8 c, u8 n);
	void drawLine(u8 v0, u8 v1, u8 c);
	int logoMode;
	float x, y, z, size;
	float rotateAuto, rotateX, rotateY;
	bool enableRotate;
	u8 transparency;
	Image *activeBoxartFrontImage, *activeBoxartSpineImage, *activeBoxartBackImage;
	Image *customBoxartFrontImage, *customBoxartSpineImage, *customBoxartBackImage;

};

} //namespace menu 

#endif
#endif
