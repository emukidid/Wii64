/*
Rice_GX - OGLRenderExt.cpp
Copyright (C) 2003 Rice1964
Copyright (C) 2010, 2011, 2012 sepp256 (Port to Wii/Gamecube/PS3)
Wii64 homepage: http://www.emulatemii.com
email address: sepp256@gmail.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __GX__
#include <SDL_opengl.h>
#endif //!__GX__

#include "stdafx.h"

extern Matrix g_MtxReal;
extern uObjMtxReal gObjMtxReal;

//========================================================================

void OGLRender::DrawText(const char* str, RECT *rect)
{
    return;
}

void OGLRender::DrawSpriteR_Render()    // With Rotation
{
    glViewportWrapper(0, windowSetting.statusBarHeightToUse, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight);

#ifndef __GX__
    GLboolean cullface = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    glBegin(GL_TRIANGLES);
    glColor4fv(gRDP.fvPrimitiveColor);

    OGLRender::TexCoord(g_texRectTVtx[0]);
    glVertex3f(g_texRectTVtx[0].x, g_texRectTVtx[0].y, -g_texRectTVtx[0].z);

    OGLRender::TexCoord(g_texRectTVtx[1]);
    glVertex3f(g_texRectTVtx[1].x, g_texRectTVtx[1].y, -g_texRectTVtx[1].z);

    OGLRender::TexCoord(g_texRectTVtx[2]);
    glVertex3f(g_texRectTVtx[2].x, g_texRectTVtx[2].y, -g_texRectTVtx[2].z);

    OGLRender::TexCoord(g_texRectTVtx[0]);
    glVertex3f(g_texRectTVtx[0].x, g_texRectTVtx[0].y, -g_texRectTVtx[0].z);

    OGLRender::TexCoord(g_texRectTVtx[2]);
    glVertex3f(g_texRectTVtx[2].x, g_texRectTVtx[2].y, -g_texRectTVtx[2].z);

    OGLRender::TexCoord(g_texRectTVtx[3]);
    glVertex3f(g_texRectTVtx[3].x, g_texRectTVtx[3].y, -g_texRectTVtx[3].z);

    glEnd();

    if( cullface ) glEnable(GL_CULL_FACE);
#else //!__GX__
	GX_SetCullMode (GX_CULL_NONE);

	GXColor GXcol;
	GXcol.r = (u8) ((gRDP.primitiveColor>>16)&0xFF);
	GXcol.g = (u8) ((gRDP.primitiveColor>>8)&0xFF);
	GXcol.b = (u8) ((gRDP.primitiveColor)&0xFF);
	GXcol.a = (u8) (gRDP.primitiveColor >>24);

	//set vertex description here
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);

		GX_Position3f32( g_texRectTVtx[0].x, g_texRectTVtx[0].y, -g_texRectTVtx[0].z );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_TexCoord2f32( g_texRectTVtx[0].tcord[0].u, g_texRectTVtx[0].tcord[0].v );

		GX_Position3f32( g_texRectTVtx[1].x, g_texRectTVtx[1].y, -g_texRectTVtx[1].z );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_TexCoord2f32( g_texRectTVtx[1].tcord[0].u, g_texRectTVtx[1].tcord[0].v );

		GX_Position3f32( g_texRectTVtx[2].x, g_texRectTVtx[2].y, -g_texRectTVtx[2].z );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_TexCoord2f32( g_texRectTVtx[2].tcord[0].u, g_texRectTVtx[2].tcord[0].v );

		GX_Position3f32( g_texRectTVtx[0].x, g_texRectTVtx[0].y, -g_texRectTVtx[0].z );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_TexCoord2f32( g_texRectTVtx[0].tcord[0].u, g_texRectTVtx[0].tcord[0].v );

		GX_Position3f32( g_texRectTVtx[2].x, g_texRectTVtx[2].y, -g_texRectTVtx[2].z );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_TexCoord2f32( g_texRectTVtx[2].tcord[0].u, g_texRectTVtx[2].tcord[0].v );

		GX_Position3f32( g_texRectTVtx[3].x, g_texRectTVtx[3].y, -g_texRectTVtx[3].z );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_TexCoord2f32( g_texRectTVtx[3].tcord[0].u, g_texRectTVtx[3].tcord[0].v );


	GX_End();

	GX_SetCullMode(gGX.GXcullMode);
#endif //__GX__
}


void OGLRender::DrawObjBGCopy(uObjBg &info)
{
    if( IsUsedAsDI(g_CI.dwAddr) )
    {
        printf("Unimplemented: write into Z buffer.  Was mostly commented out in Rice Video 6.1.0\n");
        return;
    }
    else
    {
        CRender::LoadObjBGCopy(info);
        CRender::DrawObjBGCopy(info);
    }
}


