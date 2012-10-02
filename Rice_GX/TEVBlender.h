/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Wii64 - TEVBlender.h                                                  *
 *   Wii64 homepage: http://code.google.com/p/mupen64gc/                   *
 *   Copyright (C) 2010, 2011, 2012 sepp256                                *
 *   Copyright (C) 2002 Rice1964                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef _TEV_BLENDER_H_
#define _TEV_BLENDER_H_

class OGLRender;

class CTEVBlender : public CBlender
{
public:
	void NormalAlphaBlender(void);
	void DisableAlphaBlender(void);
	void BlendFunc(uint32 srcFunc, uint32 desFunc);
	void Enable();
	void Disable();

protected:
	friend class OGLDeviceBuilder;
	CTEVBlender(CRender *pRender) : CBlender(pRender), m_pOGLRender((OGLRender*)pRender) {};
	~CTEVBlender() {};

	OGLRender *m_pOGLRender;
};

#endif



