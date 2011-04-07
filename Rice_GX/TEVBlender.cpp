/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Wii64 - TEVBlender.h                                                 *
 *   Wii64 homepage: http://code.google.com/p/mupen64gc/                   *
 *   Copyright (C) 2010 sepp256                                            *
 *   Copyright (C) 2003 Rice1964                                           *
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

#ifndef __GX__
#include <SDL_opengl.h>
#endif //!__GX__

#include "stdafx.h"

//========================================================================
u8 GX_BlendFuncMaps [] =
{
    GX_BL_SRCALPHA,      //Nothing
    GX_BL_ZERO,          //BLEND_ZERO               = 1,
    GX_BL_ONE,           //BLEND_ONE                = 2,
    GX_BL_SRCCLR,        //BLEND_SRCCOLOR           = 3,
    GX_BL_INVSRCCLR,     //BLEND_INVSRCCOLOR        = 4,
    GX_BL_SRCALPHA,      //BLEND_SRCALPHA           = 5,
    GX_BL_INVSRCALPHA,   //BLEND_INVSRCALPHA        = 6,
    GX_BL_DSTALPHA,      //BLEND_DESTALPHA          = 7,
    GX_BL_INVDSTALPHA,   //BLEND_INVDESTALPHA       = 8,
    GX_BL_DSTCLR,        //BLEND_DESTCOLOR          = 9,
    GX_BL_INVDSTCLR,     //BLEND_INVDESTCOLOR       = 10,
    GX_BL_SRCALPHA,      //BLEND_SRCALPHASAT        = 11,
    GX_BL_SRCALPHA,      //BLEND_BOTHSRCALPHA       = 12,    
    GX_BL_SRCALPHA,      //BLEND_BOTHINVSRCALPHA    = 13,
};

void CTEVBlender::NormalAlphaBlender(void)
{
#ifndef __GX__
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#else //!__GX__
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); 
#endif //__GX__
}

void CTEVBlender::DisableAlphaBlender(void)
{
#ifndef __GX__
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
#else //!__GX__
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); 
#endif //__GX__
}

void CTEVBlender::BlendFunc(uint32 srcFunc, uint32 desFunc)
{
#ifndef __GX__
    glBlendFunc(DirectX_OGL_BlendFuncMaps[srcFunc], DirectX_OGL_BlendFuncMaps[desFunc]);
#else //!__GX__
	//TODO: Bring over additional functions from glN64
	GX_SetBlendMode(GX_BM_BLEND,(u8) GX_BlendFuncMaps[srcFunc],
		(u8) GX_BlendFuncMaps[desFunc], GX_LO_CLEAR); 

/*	u8 GXblenddstfactor, GXblendsrcfactor, GXblendmode;

	GXblendmode = GX_BM_BLEND;
			GXblendmode = GX_BM_NONE;
					GXblendsrcfactor = GX_BL_ONE;
					GXblenddstfactor = GX_BL_ONE;
		GX_SetBlendMode(GXblendmode, GXblendsrcfactor, GXblenddstfactor, GX_LO_CLEAR); 
		GX_SetColorUpdate(GX_ENABLE);
		GX_SetAlphaUpdate(GX_ENABLE);
		GX_SetDstAlpha(GX_DISABLE, 0xFF);*/

#endif //__GX__
}

void CTEVBlender::Enable()
{
#ifndef __GX__
    glEnable(GL_BLEND);
#else //!__GX__
	//Already enabled by calling BlendFunc()
#endif //__GX__
}

void CTEVBlender::Disable()
{
#ifndef __GX__
    glDisable(GL_BLEND);
#else //!__GX__
	GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); 
#endif //__GX__
}
