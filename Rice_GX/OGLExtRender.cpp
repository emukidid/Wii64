/*
Rice_GX - OGLExtRender.cpp
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
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#endif //!__GX__

#include "stdafx.h"

void COGLExtRender::Initialize(void)
{
    OGLRender::Initialize();

#ifndef __GX__
	//TODO: Implement in GX
    // Initialize multitexture
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&m_maxTexUnits);
#else //!__GX__
    m_maxTexUnits = 8;
#endif //__GX__

    for( int i=0; i<8; i++ )
        m_textureUnitMap[i] = -1;
    m_textureUnitMap[0] = 0;    // T0 is usually using texture unit 0
    m_textureUnitMap[1] = 1;    // T1 is usually using texture unit 1
}


#ifndef __GX__
void COGLExtRender::BindTexture(GLuint texture, int unitno)
#else //!__GX__
void COGLExtRender::BindTexture(COGLTexture *texture, int unitno)
#endif //__GX__
{
    if( m_bEnableMultiTexture )
    {
        if( unitno < m_maxTexUnits )
        {
            if( m_curBoundTex[unitno] != texture )
            {
#ifndef __GX__
                //TODO: Figure out where to load GX textures
                glActiveTexture(GL_TEXTURE0_ARB+unitno);
                glBindTexture(GL_TEXTURE_2D,texture);
#endif //!__GX__
                m_curBoundTex[unitno] = texture;
#ifdef __GX__
				GXreloadTex[unitno] = true;
#endif //__GX__
            }
        }
    }
    else
    {
        OGLRender::BindTexture(texture, unitno);
    }
}

#ifndef __GX__
void COGLExtRender::DisBindTexture(GLuint texture, int unitno)
{
    if( m_bEnableMultiTexture )
    {
        glActiveTexture(GL_TEXTURE0_ARB+unitno);
        glBindTexture(GL_TEXTURE_2D, 0);    //Not to bind any texture
    }
    else
        OGLRender::DisBindTexture(texture, unitno);
}
#else //!__GX__ //This Function is never called by TEVCombiner
void COGLExtRender::DisBindTexture(COGLTexture *texture, int unitno) { }
#endif //__GX__

void COGLExtRender::TexCoord2f(float u, float v) //This function is never called
{
    if( m_bEnableMultiTexture )
    {
        for( int i=0; i<8; i++ )
        {
            if( m_textureUnitMap[i] >= 0 )
            {
#ifndef __GX__
		//TODO: Implement in GX
                glMultiTexCoord2f(GL_TEXTURE0_ARB+i, u, v);
#endif //!__GX__
            }
        }
    }
    else
        OGLRender::TexCoord2f(u,v);
}

void COGLExtRender::TexCoord(TLITVERTEX &vtxInfo)
{
    if( m_bEnableMultiTexture )
    {
        for( int i=0; i<8; i++ )
        {
            if( m_textureUnitMap[i] >= 0 )
            {
#ifndef __GX__
		//TODO: Implement in GX
                glMultiTexCoord2fv(GL_TEXTURE0_ARB+i, &(vtxInfo.tcord[m_textureUnitMap[i]].u));
#endif //!__GX__
            }
        }
    }
    else
        OGLRender::TexCoord(vtxInfo);
}


void COGLExtRender::SetTexWrapS(int unitno,GLuint flag)
{
    static GLuint mflag[8];
#ifndef __GX__
    static GLuint mtex[8];
#else //!__GX__
	static COGLTexture* mtex[8];
#endif //__GX__
    if( m_curBoundTex[unitno] != mtex[unitno] || mflag[unitno] != flag )
    {
        mtex[unitno] = m_curBoundTex[unitno];
        mflag[unitno] = flag;
#ifndef __GX__
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, flag);
#else //!__GX__
		mtex[unitno]->GXwrapS = (u8) flag;
		if (mtex[unitno]) 
		{
			GX_InitTexObjWrapMode(&mtex[unitno]->GXtex, mtex[unitno]->GXwrapS, mtex[unitno]->GXwrapT);
			GXreloadTex[unitno] = true;
		}
#endif //__GX__
    }
}
void COGLExtRender::SetTexWrapT(int unitno,GLuint flag)
{
    static GLuint mflag[8];
#ifndef __GX__
    static GLuint mtex[8];
#else //!__GX__
	static COGLTexture* mtex[8];
#endif //__GX__
    if( m_curBoundTex[unitno] != mtex[unitno] || mflag[unitno] != flag )
    {
        mtex[unitno] = m_curBoundTex[unitno];
        mflag[unitno] = flag;
#ifndef __GX__
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, flag);
#else //!__GX__
		mtex[unitno]->GXwrapT = (u8) flag;
        if (mtex[unitno]) 
		{
			GX_InitTexObjWrapMode(&mtex[unitno]->GXtex, mtex[unitno]->GXwrapS, mtex[unitno]->GXwrapT);
			GXreloadTex[unitno] = true;
		}
#endif //__GX__
    }
}

extern UVFlagMap OGLXUVFlagMaps[];
void COGLExtRender::SetTextureUFlag(TextureUVFlag dwFlag, uint32 dwTile)
{
    TileUFlags[dwTile] = dwFlag;
    if( !m_bEnableMultiTexture )
    {
        OGLRender::SetTextureUFlag(dwFlag, dwTile);
        return;
    }

    int tex;
    if( dwTile == gRSP.curTile )
        tex=0;
    else if( dwTile == ((gRSP.curTile+1)&7) )
        tex=1;
    else
    {
        if( dwTile == ((gRSP.curTile+2)&7) )
            tex=2;
        else if( dwTile == ((gRSP.curTile+3)&7) )
            tex=3;
        else
        {
            TRACE2("Incorrect tile number for OGL SetTextureUFlag: cur=%d, tile=%d", gRSP.curTile, dwTile);
            return;
        }
    }

    for( int textureNo=0; textureNo<8; textureNo++)
    {
        if( m_textureUnitMap[textureNo] == tex )
        {
#ifndef __GX__
			//TODO: Implement in GX
            glActiveTexture(GL_TEXTURE0_ARB+textureNo);
#endif //!__GX__
            COGLTexture* pTexture = g_textures[(gRSP.curTile+tex)&7].m_pCOGLTexture;
            if( pTexture ) 
            {
                EnableTexUnit(textureNo,TRUE);
#ifndef __GX__
                BindTexture(pTexture->m_dwTextureName, textureNo);
#else //!__GX__
                BindTexture(pTexture, textureNo);
#endif //__GX__
            }
            SetTexWrapS(textureNo, OGLXUVFlagMaps[dwFlag].realFlag);
            m_bClampS[textureNo] = dwFlag==TEXTURE_UV_FLAG_CLAMP?true:false;
        }
    }
}

void COGLExtRender::SetTextureVFlag(TextureUVFlag dwFlag, uint32 dwTile)
{
    TileVFlags[dwTile] = dwFlag;
    if( !m_bEnableMultiTexture )
    {
        OGLRender::SetTextureVFlag(dwFlag, dwTile);
        return;
    }

    int tex;
    if( dwTile == gRSP.curTile )
        tex=0;
    else if( dwTile == ((gRSP.curTile+1)&7) )
        tex=1;
    else
    {
        if( dwTile == ((gRSP.curTile+2)&7) )
            tex=2;
        else if( dwTile == ((gRSP.curTile+3)&7) )
            tex=3;
        else
        {
            TRACE2("Incorrect tile number for OGL SetTextureVFlag: cur=%d, tile=%d", gRSP.curTile, dwTile);
            return;
        }
    }
    
    for( int textureNo=0; textureNo<8; textureNo++)
    {
        if( m_textureUnitMap[textureNo] == tex )
        {
            COGLTexture* pTexture = g_textures[(gRSP.curTile+tex)&7].m_pCOGLTexture;
            if( pTexture )
            {
                EnableTexUnit(textureNo,TRUE);
#ifndef __GX__
                BindTexture(pTexture->m_dwTextureName, textureNo);
#else //!__GX__
                BindTexture(pTexture, textureNo);
#endif //__GX__
            }
            SetTexWrapT(textureNo, OGLXUVFlagMaps[dwFlag].realFlag);
            m_bClampT[textureNo] = dwFlag==TEXTURE_UV_FLAG_CLAMP?true:false;
        }
    }
}

void COGLExtRender::EnableTexUnit(int unitno, BOOL flag)
{
    if( m_texUnitEnabled[unitno] != flag )
    {
        m_texUnitEnabled[unitno] = flag;
#ifndef __GX__
		//TODO: Implement in GX
        glActiveTexture(GL_TEXTURE0_ARB+unitno);
        if( flag == TRUE )
            glEnable(GL_TEXTURE_2D);
        else
            glDisable(GL_TEXTURE_2D);
#else //!__GX__
		GXactiveTexUnit = unitno;
		GXtexUnitEnabled[GXactiveTexUnit] = flag;
#endif //__GX__
    }
}

void COGLExtRender::ApplyTextureFilter()
{
    static uint32 minflag[8], magflag[8];
#ifndef __GX__
    static uint32 mtex[8];
    for( int i=0; i<m_maxTexUnits; i++ )
    {
        int iMinFilter = (m_dwMinFilter == FILTER_LINEAR ? GL_LINEAR : GL_NEAREST);
        int iMagFilter = (m_dwMagFilter == FILTER_LINEAR ? GL_LINEAR : GL_NEAREST);
        if( m_texUnitEnabled[i] )
        {
            if( mtex[i] != m_curBoundTex[i] )
            {
                mtex[i] = m_curBoundTex[i];
                glActiveTexture(GL_TEXTURE0_ARB+i);
                minflag[i] = m_dwMinFilter;
                magflag[i] = m_dwMagFilter;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, iMinFilter);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, iMagFilter);
            }
            else
            {
                if( minflag[i] != (unsigned int)m_dwMinFilter )
                {
                    minflag[i] = m_dwMinFilter;
                    glActiveTexture(GL_TEXTURE0_ARB+i);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, iMinFilter);
                }
                if( magflag[i] != (unsigned int)m_dwMagFilter )
                {
                    magflag[i] = m_dwMagFilter;
                    glActiveTexture(GL_TEXTURE0_ARB+i);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, iMagFilter);
                }
            }
        }
    }
#else //!__GX__
    static COGLTexture* mtex[8];
    for( int i=0; i<m_maxTexUnits; i++ )
    {
        int iMinFilter = (m_dwMinFilter == FILTER_LINEAR ? GX_LINEAR : GX_NEAR);
        int iMagFilter = (m_dwMagFilter == FILTER_LINEAR ? GX_LINEAR : GX_NEAR);
        if( m_texUnitEnabled[i] )
        {
            if( mtex[i] != m_curBoundTex[i] )
            {
                mtex[i] = m_curBoundTex[i];
                minflag[i] = m_dwMinFilter;
                magflag[i] = m_dwMagFilter;
                if (mtex[i]) 
				{
					GX_InitTexObjFilterMode(&mtex[i]->GXtex,(u8) iMinFilter,(u8) iMagFilter);
					GXreloadTex[i] = true;
				}
            }
            else if( (minflag[i] != (unsigned int)m_dwMinFilter) || (magflag[i] != (unsigned int)m_dwMagFilter) )
            {
                minflag[i] = m_dwMinFilter;
                magflag[i] = m_dwMagFilter;
                if (mtex[i]) 
				{
					GX_InitTexObjFilterMode(&mtex[i]->GXtex,(u8) iMinFilter,(u8) iMagFilter);
					GXreloadTex[i] = true;
				}
            }
        }
    }
#endif //!__GX__
}

void COGLExtRender::SetTextureToTextureUnitMap(int tex, int unit)
{
    if( unit < 8 && (tex >= -1 || tex <= 1))
        m_textureUnitMap[unit] = tex;
}

