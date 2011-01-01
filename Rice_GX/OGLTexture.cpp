/*
Copyright (C) 2003 Rice1964

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

#include "stdafx.h"

COGLTexture::COGLTexture(uint32 dwWidth, uint32 dwHeight, TextureUsage usage) :
    CTexture(dwWidth,dwHeight,usage),
    m_glFmt(GL_RGBA)
{
    // Fix me, if usage is AS_RENDER_TARGET, we need to create pbuffer instead of regular texture

    m_dwTextureFmt = TEXTURE_FMT_A8R8G8B8;  // Always use 32bit to load texture

#ifndef __GX__ //TODO: Implement naming for GX textures
    glGenTextures( 1, &m_dwTextureName );
#endif //!__GX_

    // Make the width and height be the power of 2
    uint32 w;
    for (w = 1; w < dwWidth; w <<= 1);
    m_dwCreatedTextureWidth = w;
    for (w = 1; w < dwHeight; w <<= 1);
    m_dwCreatedTextureHeight = w;
    
    if (dwWidth*dwHeight > 256*256)
        TRACE4("Large texture: (%d x %d), created as (%d x %d)", 
            dwWidth, dwHeight,m_dwCreatedTextureWidth,m_dwCreatedTextureHeight);
    
    m_fYScale = (float)m_dwCreatedTextureHeight/(float)m_dwHeight;
    m_fXScale = (float)m_dwCreatedTextureWidth/(float)m_dwWidth;

#ifndef __GX__ //GX texture buffer is allocated during StartUpdate()
    m_pTexture = malloc(m_dwCreatedTextureWidth * m_dwCreatedTextureHeight * GetPixelSize());

    switch( options.textureQuality )
    {
    case TXT_QUALITY_DEFAULT:
        if( options.colorQuality == TEXTURE_FMT_A4R4G4B4 ) 
            m_glFmt = GL_RGBA4;
        break;
    case TXT_QUALITY_32BIT:
        break;
    case TXT_QUALITY_16BIT:
            m_glFmt = GL_RGBA4;
        break;
    };
#else //!__GX__
	//m_dwCreatedTextureWidth & m_dwCreatedTextureHeight are the same as the N64 textures for GX
	//This is the default for class Texture

	//m_pTexture will be allocated later once GX texture format is decided
    //m_pTexture = malloc(m_dwCreatedTextureWidth * m_dwCreatedTextureHeight * GetPixelSize());
#endif //__GX__
    LOG_TEXTURE(TRACE2("New texture: (%d, %d)", dwWidth, dwHeight));
}

COGLTexture::~COGLTexture()
{
    // Fix me, if usage is AS_RENDER_TARGET, we need to destroy the pbuffer

#ifndef __GX__ //GX Texture buffers are allocated in CTexture class
    glDeleteTextures(1, &m_dwTextureName );
    if (m_pTexture) free(m_pTexture);
    m_pTexture = NULL;
#endif //!__GX__
    m_dwWidth = 0;
    m_dwHeight = 0;
}

bool COGLTexture::StartUpdate(DrawInfo *di)
{
    if (m_pTexture == NULL)
#ifndef __GX__
        return false;
#else //!__GX__
		GXallocateTexture();
#endif //__GX__

    di->dwHeight = (uint16)m_dwHeight;
    di->dwWidth = (uint16)m_dwWidth;
    di->dwCreatedHeight = m_dwCreatedTextureHeight;
    di->dwCreatedWidth = m_dwCreatedTextureWidth;
    di->lpSurface = m_pTexture;
    di->lPitch = GetPixelSize()*m_dwCreatedTextureWidth;

    return true;
}

void COGLTexture::EndUpdate(DrawInfo *di)
{
#ifndef __GX__
    glBindTexture(GL_TEXTURE_2D, m_dwTextureName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Copy the image data from main memory to video card texture memory
#ifndef _BIG_ENDIAN
    glTexImage2D(GL_TEXTURE_2D, 0, m_glFmt, m_dwCreatedTextureWidth, m_dwCreatedTextureHeight, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, m_pTexture);
#else //!_BIG_ENDIAN
	//Fix Me: Currently 16bit textures are never used in Rice...
	//Change to GL_UNSIGNED_INT_8_8_8_8 for X forwarding to X86
    glTexImage2D(GL_TEXTURE_2D, 0, m_glFmt, m_dwCreatedTextureWidth, m_dwCreatedTextureHeight, 0, GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8_REV, m_pTexture);
#endif //_BIG_ENDIAN
#else //!__GX__
	//void GX_InitTexObj(GXTexObj *obj,void *img_ptr,u16 wd,u16 ht,u8 fmt,u8 wrap_s,u8 wrap_t,u8 mipmap);
	//if (OGL.GXuseMinMagNearest) GX_InitTexObjLOD(&texture->GXtex, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
	//GX_LoadTexObj(&texture->GXtex, t); // t = 0 is GX_TEXMAP0 and t = 1 is GX_TEXMAP1

	if(m_pTexture != NULL)
		DCFlushRange(m_pTexture, GXtextureBytes);

	GX_InitTexObj(&GXtex, m_pTexture, (u16) m_dwCreatedTextureWidth, (u16) m_dwCreatedTextureHeight, GXtexfmt, GXwrapS, GXwrapT, GX_FALSE);
#endif //__GX__
}


// Keep in mind that the real texture is not scaled to fix the created opengl texture yet.
// when the image is need to be scaled, ScaleImageToSurface in CTexure will be called to 
// scale the image automatically

