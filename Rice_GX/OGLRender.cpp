/*
Rice_GX - OGLRender.cpp
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
#endif //__GX__

#include "stdafx.h"

// Fix me, use OGL internal L/T and matrix stack
// Fix me, use OGL lookupAt function
// Fix me, use OGL DisplayList

UVFlagMap OGLXUVFlagMaps[] =
{
#ifndef __GX__
{TEXTURE_UV_FLAG_WRAP, GL_REPEAT},
{TEXTURE_UV_FLAG_MIRROR, GL_MIRRORED_REPEAT_ARB},
{TEXTURE_UV_FLAG_CLAMP, GL_CLAMP},
#else //!__GX__
{TEXTURE_UV_FLAG_WRAP, GX_REPEAT},
{TEXTURE_UV_FLAG_MIRROR, GX_MIRROR},
{TEXTURE_UV_FLAG_CLAMP, GX_CLAMP},
#endif //__GX__
};

#ifdef __GX__
COGLTexture** g_curBoundTex; //Needed to clear m_curBoundTex when deleting textures
#endif //__GX__

//===================================================================
OGLRender::OGLRender()
{
    COGLGraphicsContext *pcontext = (COGLGraphicsContext *)(CGraphicsContext::g_pGraphicsContext);
    m_bSupportFogCoordExt = pcontext->m_bSupportFogCoord;
    m_bMultiTexture = pcontext->m_bSupportMultiTexture;
    m_bSupportClampToEdge = false;
    for( int i=0; i<8; i++ )
    {
        m_curBoundTex[i]=0;
        m_texUnitEnabled[i]=FALSE;
    }
#ifndef __GX__
    m_bEnableMultiTexture = false;
#else //!__GX__
    m_bEnableMultiTexture = true;
#endif //__GX__

#ifdef __GX__ //Maybe this can be moved to Initialize()
	gGX.GXenableZmode = GX_DISABLE;
	gGX.GXZfunc = GX_ALWAYS;
	gGX.GXZupdate = GX_FALSE;

	g_curBoundTex = m_curBoundTex;
#endif //__GX__
}

OGLRender::~OGLRender()
{
    ClearDeviceObjects();
}

bool OGLRender::InitDeviceObjects()
{
    // enable Z-buffer by default
    ZBufferEnable(true);
    return true;
}

bool OGLRender::ClearDeviceObjects()
{
    return true;
}

void OGLRender::Initialize(void)
{
#ifndef __GX__
	//TODO: Implement in GX
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glViewportWrapper(0, windowSetting.statusBarHeightToUse, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight);

    COGLGraphicsContext *pcontext = (COGLGraphicsContext *)(CGraphicsContext::g_pGraphicsContext);
    if( pcontext->IsExtensionSupported("GL_IBM_texture_mirrored_repeat") )
    {
        OGLXUVFlagMaps[TEXTURE_UV_FLAG_MIRROR].realFlag = GL_MIRRORED_REPEAT_IBM;
    }
    else if( pcontext->IsExtensionSupported("ARB_texture_mirrored_repeat") )
    {
        OGLXUVFlagMaps[TEXTURE_UV_FLAG_MIRROR].realFlag = GL_MIRRORED_REPEAT_ARB;
    }
    else
    {
        OGLXUVFlagMaps[TEXTURE_UV_FLAG_MIRROR].realFlag = GL_REPEAT;
    }

    if( pcontext->IsExtensionSupported("GL_ARB_texture_border_clamp") || pcontext->IsExtensionSupported("GL_EXT_texture_edge_clamp") )
    {
        m_bSupportClampToEdge = true;
        OGLXUVFlagMaps[TEXTURE_UV_FLAG_CLAMP].realFlag = GL_CLAMP_TO_EDGE;
    }
    else
    {
        m_bSupportClampToEdge = false;
        OGLXUVFlagMaps[TEXTURE_UV_FLAG_CLAMP].realFlag = GL_CLAMP;
    }

    glVertexPointer( 4, GL_FLOAT, sizeof(float)*5, &(g_vtxProjected5[0][0]) );
    glEnableClientState( GL_VERTEX_ARRAY );

    if( m_bMultiTexture )
    {
        glClientActiveTextureARB( GL_TEXTURE0_ARB );
        glTexCoordPointer( 2, GL_FLOAT, sizeof( TLITVERTEX ), &(g_vtxBuffer[0].tcord[0].u) );
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );

        glClientActiveTextureARB( GL_TEXTURE1_ARB );
        glTexCoordPointer( 2, GL_FLOAT, sizeof( TLITVERTEX ), &(g_vtxBuffer[0].tcord[1].u) );
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    }
    else
    {
        glTexCoordPointer( 2, GL_FLOAT, sizeof( TLITVERTEX ), &(g_vtxBuffer[0].tcord[0].u) );
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    }

    if (m_bSupportFogCoordExt)
    {
        glFogCoordPointerEXT( GL_FLOAT, sizeof(float)*5, &(g_vtxProjected5[0][4]) );
        glEnableClientState( GL_FOG_COORDINATE_ARRAY_EXT );
        glFogi( GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT );
        glFogi(GL_FOG_MODE, GL_LINEAR); // Fog Mode
        glFogf(GL_FOG_DENSITY, 1.0f); // How Dense Will The Fog Be
        glHint(GL_FOG_HINT, GL_FASTEST); // Fog Hint Value
        glFogi( GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT );
        glFogf( GL_FOG_START, 0.0f );
        glFogf( GL_FOG_END, 1.0f );
    }

    //glColorPointer( 1, GL_UNSIGNED_BYTE, sizeof(TLITVERTEX), &g_vtxBuffer[0].r);
    glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof(uint8)*4, &(g_oglVtxColors[0][0]) );
    glEnableClientState( GL_COLOR_ARRAY );

    if( pcontext->IsExtensionSupported("GL_NV_depth_clamp") )
    {
        glEnable(GL_DEPTH_CLAMP_NV);
    }
#else //!__GX__
	//Reset projection matrix
	guMtxIdentity(gGX.GXcombW);
//	guMtxIdentity(OGL.GXprojWnear);
	guMtxIdentity(gGX.GXprojIdent);
	//Reset Modelview matrix
	guMtxIdentity(gGX.GXmodelViewIdent);
//	guOrtho(OGL.GXprojIdent, -1, 1, -1, 1, 1.0f, -1.0f);
	//N64 Z clip space is backwards, so mult z components by -1
	//N64 Z [-1,1] whereas GC Z [-1,0], so mult by 0.5 and shift by -0.5
	gGX.GXcombW[3][2] = -1;
	gGX.GXcombW[3][3] = 0;
//	OGL.GXprojWnear[2][2] = 0.0f;
//	OGL.GXprojWnear[2][3] = -0.5f;
//	OGL.GXprojWnear[3][2] = -1.0f;
//	OGL.GXprojWnear[3][3] = 0.0f;
	gGX.GXprojIdent[2][2] = GXprojZScale; //0.5;
	gGX.GXprojIdent[2][3] = GXprojZOffset; //-0.5;
	gGX.GXpolyOffset = false;

    glViewportWrapper(0, windowSetting.statusBarHeightToUse, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight);

	GX_LoadProjectionMtx(gGX.GXprojIdent, GX_ORTHOGRAPHIC);
	GX_LoadPosMtxImm(gGX.GXmodelViewIdent,GX_PNMTX0);
//	GX_SetViewport((f32) 0,(f32) 0,(f32) 640,(f32) 480, 0.0f, 1.0f); // <- remove this

	GX_SetClipMode(GX_CLIP_ENABLE);
//	GX_SetClipMode(GX_CLIP_DISABLE);

	//These are here temporarily until combining/blending is sorted out...
	//Set PassColor TEV mode
	GX_SetNumChans (1);
	GX_SetNumTexGens (0);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	//Set CopyModeDraw blend modes here
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);

	GX_SetZMode(GX_ENABLE,GX_LEQUAL,GX_TRUE);

#endif //__GX__
}
//===================================================================
TextureFilterMap OglTexFilterMap[2]=
{
#ifndef __GX__
    {FILTER_POINT, GL_NEAREST},
    {FILTER_LINEAR, GL_LINEAR},
#else //!__GX__
    {FILTER_POINT, GX_NEAR},
    {FILTER_LINEAR, GX_LINEAR},
#endif //__GX__
};

void OGLRender::ApplyTextureFilter()
{
    static uint32 minflag=0xFFFF, magflag=0xFFFF;
#ifndef __GX__
    static uint32 mtex;

    if( m_texUnitEnabled[0] )
    {
        if( mtex != m_curBoundTex[0] )
        {
            mtex = m_curBoundTex[0];
            minflag = m_dwMinFilter;
            magflag = m_dwMagFilter;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OglTexFilterMap[m_dwMinFilter].realFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OglTexFilterMap[m_dwMagFilter].realFilter);
        }
        else
        {
            if( minflag != (unsigned int)m_dwMinFilter )
            {
                minflag = m_dwMinFilter;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OglTexFilterMap[m_dwMinFilter].realFilter);
            }
            if( magflag != (unsigned int)m_dwMagFilter )
            {
                magflag = m_dwMagFilter;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OglTexFilterMap[m_dwMagFilter].realFilter);
            }   
        }
    }
#else //!__GX__
    static COGLTexture* mtex;

    if( m_texUnitEnabled[0] )
    {
        if( mtex != m_curBoundTex[0] )
        {
            mtex = m_curBoundTex[0];
            minflag = m_dwMinFilter;
            magflag = m_dwMagFilter;

            if (mtex) 
			{
				GX_InitTexObjFilterMode(&mtex->GXtex,(u8) OglTexFilterMap[m_dwMinFilter].realFilter,
					(u8) OglTexFilterMap[m_dwMagFilter].realFilter);
				GXreloadTex[0] = true;
			}
        }
        else if( (minflag != (unsigned int)m_dwMinFilter) || (magflag != (unsigned int)m_dwMagFilter) )
        {
            minflag = m_dwMinFilter;
            magflag = m_dwMagFilter;
            if (mtex) 
			{
				GX_InitTexObjFilterMode(&mtex->GXtex,(u8) OglTexFilterMap[m_dwMinFilter].realFilter,
					(u8) OglTexFilterMap[m_dwMagFilter].realFilter);
				GXreloadTex[0] = true;
			}
        }
    }
#endif //!__GX__
}

void OGLRender::SetShadeMode(RenderShadeMode mode)
{
#ifndef __GX__
	//TODO: Implement in GX
    if( mode == SHADE_SMOOTH )
        glShadeModel(GL_SMOOTH);
    else
        glShadeModel(GL_FLAT);
#endif //!__GX__
}

void OGLRender::ZBufferEnable(BOOL bZBuffer)
{
    gRSP.bZBufferEnabled = bZBuffer;
    if( g_curRomInfo.bForceDepthBuffer )
        bZBuffer = TRUE;
#ifndef __GX__
    if( bZBuffer )
    {
        glDepthMask(GL_TRUE);
        //glEnable(GL_DEPTH_TEST);
        glDepthFunc( GL_LEQUAL );
    }
    else
    {
        glDepthMask(GL_FALSE);
        //glDisable(GL_DEPTH_TEST);
        glDepthFunc( GL_ALWAYS );
    }
#else //!__GX__
    if( bZBuffer )
	{
		gGX.GXenableZmode = GX_ENABLE;
		gGX.GXZfunc = GX_LEQUAL;
	}
    else
	{
		gGX.GXenableZmode = GX_DISABLE;
		gGX.GXZfunc = GX_ALWAYS;
	}
	GX_SetZMode(gGX.GXenableZmode,gGX.GXZfunc,gGX.GXZupdate);
#endif //__GX__
}

void OGLRender::ClearBuffer(bool cbuffer, bool zbuffer)
{
#ifndef __GX__
    uint32 flag=0;
    if( cbuffer )   flag |= GL_COLOR_BUFFER_BIT;
    if( zbuffer )   flag |= GL_DEPTH_BUFFER_BIT;
    float depth = ((gRDP.originalFillColor&0xFFFF)>>2)/(float)0x3FFF;
    glClearDepth(depth);
    glClear(flag);
#else //!__GX__
	gGX.GXclearColorBuffer = cbuffer;
	gGX.GXclearColor = (GXColor) { 0, 0, 0, 0 };
	gGX.GXclearDepthBuffer = zbuffer;
	gGX.GXclearDepth = ((gRDP.originalFillColor&0xFFFF)>>2)/(float)0x3FFF;
	if (gGX.GXclearColorBuffer || gGX.GXclearDepthBuffer)
		GXclearEFB();
#endif //__GX__
}

void OGLRender::ClearZBuffer(float depth)
{
#ifndef __GX__
    uint32 flag=GL_DEPTH_BUFFER_BIT;
    glClearDepth(depth);
    glClear(flag);
#else //!__GX__
	//This function is only ever called with depth = 1.0f
	gGX.GXclearDepthBuffer = true;
	gGX.GXclearDepth = depth;
	GXclearEFB();
#endif //__GX__
}

void OGLRender::SetZCompare(BOOL bZCompare)
{
    if( g_curRomInfo.bForceDepthBuffer )
        bZCompare = TRUE;

    gRSP.bZBufferEnabled = bZCompare;
#ifndef __GX__
    if( bZCompare == TRUE )
        //glEnable(GL_DEPTH_TEST);
        glDepthFunc( GL_LEQUAL );
    else
        //glDisable(GL_DEPTH_TEST);
        glDepthFunc( GL_ALWAYS );
#else //!__GX__
    if( bZCompare == TRUE )
		gGX.GXZfunc = GX_LEQUAL;
    else
		gGX.GXZfunc = GX_ALWAYS;
	GX_SetZMode(gGX.GXenableZmode,gGX.GXZfunc,gGX.GXZupdate);
#endif //__GX__
}

void OGLRender::SetZUpdate(BOOL bZUpdate)
{
    if( g_curRomInfo.bForceDepthBuffer )
        bZUpdate = TRUE;

#ifndef __GX__
    if( bZUpdate )
    {
        //glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }
    else
    {
        glDepthMask(GL_FALSE);
    }
#else //!__GX__
	if( bZUpdate )
		gGX.GXZupdate = GX_TRUE;
	else
		gGX.GXZupdate = GX_FALSE;
	GX_SetZMode(gGX.GXenableZmode,gGX.GXZfunc,gGX.GXZupdate);
#endif //__GX__
}

void OGLRender::ApplyZBias(int bias)
{
#ifndef __GX__
    float f1 = bias > 0 ? -3.0f : 0.0f;  // z offset = -3.0 * max(abs(dz/dx),abs(dz/dy)) per pixel delta z slope
    float f2 = bias > 0 ? -3.0f : 0.0f;  // z offset += -3.0 * 1 bit
	//TODO: Implement in GX
    if (bias > 0)
        glEnable(GL_POLYGON_OFFSET_FILL);  // enable z offsets
    else
        glDisable(GL_POLYGON_OFFSET_FILL);  // disable z offsets
    glPolygonOffset(f1, f2);  // set bias functions
#else //!__GX__
	if (bias > 0)
	{
		gGX.GXpolyOffset = true;
		gGX.GXupdateMtx = true;
	}
	else 
	{
		gGX.GXpolyOffset = false;
		gGX.GXupdateMtx = true;
	}
#endif //__GX__
}

void OGLRender::SetZBias(int bias)
{
#if defined(_DEBUG)
    if( pauseAtNext == true )
      DebuggerAppendMsg("Set zbias = %d", bias);
#endif
    // set member variable and apply the setting in opengl
    m_dwZBias = bias;
    ApplyZBias(bias);
}

void OGLRender::SetAlphaRef(uint32 dwAlpha)
{
    if (m_dwAlpha != dwAlpha)
    {
        m_dwAlpha = dwAlpha;
#ifndef __GX__
        glAlphaFunc(GL_GEQUAL, (float)dwAlpha);
#else //!__GX__
		//TODO: Double-check range of dwAlpha... should be 0~255
		//TODO: Set Z compare location based on Ztex
		GX_SetZCompLoc(GX_FALSE); //Zcomp after texturing
		gGX.GXalphaFunc = GX_GEQUAL;
		gGX.GXalphaRef = (u8) dwAlpha;
		GX_SetAlphaCompare(gGX.GXalphaFunc,gGX.GXalphaRef,GX_AOP_AND,GX_ALWAYS,0);
#endif //__GX__
    }
}

void OGLRender::ForceAlphaRef(uint32 dwAlpha)
{
#ifndef __GX__
    float ref = dwAlpha/255.0f;
    glAlphaFunc(GL_GEQUAL, ref);
#else //!__GX__
	//TODO: Set Z compare location based on Ztex
	GX_SetZCompLoc(GX_FALSE); //Zcomp after texturing
	gGX.GXalphaFunc = GX_GEQUAL;
	gGX.GXalphaRef = (u8) dwAlpha;
	GX_SetAlphaCompare(gGX.GXalphaFunc,gGX.GXalphaRef,GX_AOP_AND,GX_ALWAYS,0);
#endif //__GX__
}

void OGLRender::SetFillMode(FillMode mode)
{
#ifndef __GX__
	//TODO: Implement in GX
    if( mode == RICE_FILLMODE_WINFRAME )
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif //!__GX__
}

void OGLRender::SetCullMode(bool bCullFront, bool bCullBack)
{
    CRender::SetCullMode(bCullFront, bCullBack);
#ifndef __GX__
    if( bCullFront && bCullBack )
    {
        glCullFace(GL_FRONT_AND_BACK);
        glEnable(GL_CULL_FACE);
    }
    else if( bCullFront )
    {
        glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);
    }
    else if( bCullBack )
    {
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
#else // !__GX__
	if( bCullFront && bCullBack )
		gGX.GXcullMode = GX_CULL_ALL;
	else if( bCullFront )
		gGX.GXcullMode = GX_CULL_BACK;	// GC vertex winding is backwards.
	else if( bCullBack )
		gGX.GXcullMode = GX_CULL_FRONT;	// GC vertex winding is backwards.
	else
		gGX.GXcullMode = GX_CULL_NONE;
	GX_SetCullMode(gGX.GXcullMode);
#endif // __GX__
}

bool OGLRender::SetCurrentTexture(int tile, CTexture *handler,uint32 dwTileWidth, uint32 dwTileHeight, TxtrCacheEntry *pTextureEntry)
{
    RenderTexture &texture = g_textures[tile];
    texture.pTextureEntry = pTextureEntry;

    if( handler!= NULL  && texture.m_lpsTexturePtr != handler->GetTexture() )
    {
        texture.m_pCTexture = handler;
        texture.m_lpsTexturePtr = handler->GetTexture();

        texture.m_dwTileWidth = dwTileWidth;
        texture.m_dwTileHeight = dwTileHeight;

        if( handler->m_bIsEnhancedTexture )
        {
            texture.m_fTexWidth = (float)pTextureEntry->pTexture->m_dwCreatedTextureWidth;
            texture.m_fTexHeight = (float)pTextureEntry->pTexture->m_dwCreatedTextureHeight;
        }
        else
        {
            texture.m_fTexWidth = (float)handler->m_dwCreatedTextureWidth;
            texture.m_fTexHeight = (float)handler->m_dwCreatedTextureHeight;
        }
    }
    
    return true;
}

bool OGLRender::SetCurrentTexture(int tile, TxtrCacheEntry *pEntry)
{
    if (pEntry != NULL && pEntry->pTexture != NULL)
    {   
        SetCurrentTexture( tile, pEntry->pTexture,  pEntry->ti.WidthToCreate, pEntry->ti.HeightToCreate, pEntry);
        return true;
    }
    else
    {
        SetCurrentTexture( tile, NULL, 64, 64, NULL );
        return false;
    }
    return true;
}

void OGLRender::SetAddressUAllStages(uint32 dwTile, TextureUVFlag dwFlag)
{
    SetTextureUFlag(dwFlag, dwTile);
}

void OGLRender::SetAddressVAllStages(uint32 dwTile, TextureUVFlag dwFlag)
{
    SetTextureVFlag(dwFlag, dwTile);
}

void OGLRender::SetTexWrapS(int unitno,GLuint flag)
{
    static GLuint mflag;
#ifndef __GX__
	static GLuint mtex;
#else //!__GX__
	static COGLTexture* mtex;
#endif //__GX__
#ifdef _DEBUG
    if( unitno != 0 )
    {
        DebuggerAppendMsg("Check me, unitno != 0 in base ogl");
    }
#endif
    if( m_curBoundTex[0] != mtex || mflag != flag )
    {
        mtex = m_curBoundTex[0];
        mflag = flag;
#ifndef __GX__
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, flag);
#else //!__GX__
		if (mtex) mtex->GXwrapS = (u8) flag;
        if (mtex) 
		{
			GX_InitTexObjWrapMode(&mtex->GXtex, mtex->GXwrapS, mtex->GXwrapT);
			GXreloadTex[0] = true;
		}
#endif //__GX__
    }
}
void OGLRender::SetTexWrapT(int unitno,GLuint flag)
{
    static GLuint mflag;
#ifndef __GX__
	static GLuint mtex;
#else //!__GX__
	static COGLTexture* mtex;
#endif //__GX__
    if( m_curBoundTex[0] != mtex || mflag != flag )
    {
        mtex = m_curBoundTex[0];
        mflag = flag;
#ifndef __GX__
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, flag);
#else //!__GX__
		if (mtex) mtex->GXwrapT = (u8) flag;
        if (mtex) 
		{
			GX_InitTexObjWrapMode(&mtex->GXtex, mtex->GXwrapS, mtex->GXwrapT);
			GXreloadTex[0] = true;
		}
#endif //__GX__
    }
}

void OGLRender::SetTextureUFlag(TextureUVFlag dwFlag, uint32 dwTile)
{
    TileUFlags[dwTile] = dwFlag;
    if( dwTile == gRSP.curTile )    // For basic OGL, only support the 1st texel
    {
        COGLTexture* pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
        if( pTexture )
        {
            EnableTexUnit(0,TRUE);
#ifndef __GX__
            BindTexture(pTexture->m_dwTextureName, 0);
#else //!__GX__
            BindTexture(pTexture, 0);
#endif //__GX__
        }
        SetTexWrapS(0, OGLXUVFlagMaps[dwFlag].realFlag);
    }
}
void OGLRender::SetTextureVFlag(TextureUVFlag dwFlag, uint32 dwTile)
{
    TileVFlags[dwTile] = dwFlag;
    if( dwTile == gRSP.curTile )    // For basic OGL, only support the 1st texel
    {
        COGLTexture* pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
        if( pTexture ) 
        {
            EnableTexUnit(0,TRUE);
#ifndef __GX__
            BindTexture(pTexture->m_dwTextureName, 0);
#else //!__GX__
            BindTexture(pTexture, 0);
#endif //__GX__
        }
        SetTexWrapT(0, OGLXUVFlagMaps[dwFlag].realFlag);
    }
}

// Basic render drawing functions

bool OGLRender::RenderTexRect()
{
    glViewportWrapper(0, windowSetting.statusBarHeightToUse, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight);

#ifndef __GX__
    GLboolean cullface = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    glBegin(GL_TRIANGLE_FAN);

    float depth = -(g_texRectTVtx[3].z*2-1);

    glColor4f(g_texRectTVtx[3].r, g_texRectTVtx[3].g, g_texRectTVtx[3].b, g_texRectTVtx[3].a);
    TexCoord(g_texRectTVtx[3]);
    glVertex3f(g_texRectTVtx[3].x, g_texRectTVtx[3].y, depth);
    
    glColor4f(g_texRectTVtx[2].r, g_texRectTVtx[2].g, g_texRectTVtx[2].b, g_texRectTVtx[2].a);
    TexCoord(g_texRectTVtx[2]);
    glVertex3f(g_texRectTVtx[2].x, g_texRectTVtx[2].y, depth);

    glColor4f(g_texRectTVtx[1].r, g_texRectTVtx[1].g, g_texRectTVtx[1].b, g_texRectTVtx[1].a);
    TexCoord(g_texRectTVtx[1]);
    glVertex3f(g_texRectTVtx[1].x, g_texRectTVtx[1].y, depth);

    glColor4f(g_texRectTVtx[0].r, g_texRectTVtx[0].g, g_texRectTVtx[0].b, g_texRectTVtx[0].a);
    TexCoord(g_texRectTVtx[0]);
    glVertex3f(g_texRectTVtx[0].x, g_texRectTVtx[0].y, depth);

    glEnd();

    if( cullface ) glEnable(GL_CULL_FACE);
#else //!__GX__
	GX_SetCullMode (GX_CULL_NONE);

    float depth = -(g_texRectTVtx[3].z*2-1);

	GXloadTextures();

	//set vertex description here
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	if (gGX.GXmultiTex) GX_SetVtxDesc(GX_VA_TEX1MTXIDX, GX_TEXMTX0);
//	GX_SetVtxDesc(GX_VA_TEX2MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	if (gGX.GXmultiTex) GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
//	GX_SetVtxDesc(GX_VA_TEX2, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	if (gGX.GXmultiTex) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
//	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	for (int i = 3; i>=0; i--)
	{
		//glVertex3f(g_texRectTVtx[3].x, g_texRectTVtx[3].y, depth);
		GX_Position3f32( g_texRectTVtx[i].x, g_texRectTVtx[i].y, depth );
		//glColor4f(g_texRectTVtx[3].r, g_texRectTVtx[3].g, g_texRectTVtx[3].b, g_texRectTVtx[3].a);
/*		GX_Color4u8( (u8) (g_texRectTVtx[i].r*255),
			(u8) (g_texRectTVtx[i].g*255),
			(u8) (g_texRectTVtx[i].b*255),
			(u8) (g_texRectTVtx[i].a*255));*/
		GX_Color4u8( (u8) (g_texRectTVtx[i].r),
			(u8) (g_texRectTVtx[i].g),
			(u8) (g_texRectTVtx[i].b),
			(u8) (g_texRectTVtx[i].a));
		//TexCoord(g_texRectTVtx[3]);
		//glTexCoord2f(vtxInfo.tcord[0].u, vtxInfo.tcord[0].v);
		GX_TexCoord2f32( g_texRectTVtx[i].tcord[0].u, g_texRectTVtx[i].tcord[0].v );
		if (gGX.GXmultiTex) GX_TexCoord2f32( g_texRectTVtx[i].tcord[1].u, g_texRectTVtx[i].tcord[1].v );
	}
	GX_End();

	GX_SetCullMode(gGX.GXcullMode);

#ifdef RICE_STATS
	DEBUG_stats(RICE_STATS+0, "RenderTexRect", STAT_TYPE_ACCUM, 1);
#endif
//	printf("RenderTexRect\n");

#endif //__GX__

    return true;
}

bool OGLRender::RenderFillRect(uint32 dwColor, float depth)
{
#ifndef __GX__
    float a = (dwColor>>24)/255.0f;
    float r = ((dwColor>>16)&0xFF)/255.0f;
    float g = ((dwColor>>8)&0xFF)/255.0f;
    float b = (dwColor&0xFF)/255.0f;
    glViewportWrapper(0, windowSetting.statusBarHeightToUse, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight);

    GLboolean cullface = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(r,g,b,a);
    glVertex4f(m_fillRectVtx[0].x, m_fillRectVtx[1].y, depth, 1);
    glVertex4f(m_fillRectVtx[1].x, m_fillRectVtx[1].y, depth, 1);
    glVertex4f(m_fillRectVtx[1].x, m_fillRectVtx[0].y, depth, 1);
    glVertex4f(m_fillRectVtx[0].x, m_fillRectVtx[0].y, depth, 1);
    glEnd();

    if( cullface ) glEnable(GL_CULL_FACE);
#else //!__GX__
	GXColor GXcol;
	GXcol.r = (u8) ((dwColor>>16)&0xFF);
	GXcol.g = (u8) ((dwColor>>8)&0xFF);
	GXcol.b = (u8) (dwColor&0xFF);
	GXcol.a = (u8) (dwColor>>24);

    glViewportWrapper(0, windowSetting.statusBarHeightToUse, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight);

	GX_SetCullMode (GX_CULL_NONE);

	//set vertex description here
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32( m_fillRectVtx[0].x, m_fillRectVtx[1].y, depth );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_Position3f32( m_fillRectVtx[1].x, m_fillRectVtx[1].y, depth );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_Position3f32( m_fillRectVtx[1].x, m_fillRectVtx[0].y, depth );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_Position3f32( m_fillRectVtx[0].x, m_fillRectVtx[0].y, depth );
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
	GX_End();

	GX_SetCullMode(gGX.GXcullMode);

#ifdef RICE_STATS
	DEBUG_stats(RICE_STATS+1, "RenderFillRect", STAT_TYPE_ACCUM, 1);
#endif
//	printf("RenderFillRect\n");

#endif //__GX__

    return true;
}

bool OGLRender::RenderLine3D()
{
    ApplyZBias(0);  // disable z offsets

#ifndef __GX__
    glBegin(GL_TRIANGLE_FAN);

    glColor4ub(m_line3DVtx[1].r, m_line3DVtx[1].g, m_line3DVtx[1].b, m_line3DVtx[1].a);
    glVertex3f(m_line3DVector[3].x, m_line3DVector[3].y, -m_line3DVtx[1].z);
    glVertex3f(m_line3DVector[2].x, m_line3DVector[2].y, -m_line3DVtx[0].z);
    
    glColor4ub(m_line3DVtx[0].r, m_line3DVtx[0].g, m_line3DVtx[0].b, m_line3DVtx[0].a);
    glVertex3f(m_line3DVector[1].x, m_line3DVector[1].y, -m_line3DVtx[1].z);
    glVertex3f(m_line3DVector[0].x, m_line3DVector[0].y, -m_line3DVtx[0].z);

    glEnd();
#else //!__GX__
	//set vertex description here
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32( m_line3DVector[3].x, m_line3DVector[3].y, -m_line3DVtx[1].z );
		GX_Color4u8( m_line3DVtx[1].r,m_line3DVtx[1].g, m_line3DVtx[1].b, m_line3DVtx[1].a );
		GX_Position3f32( m_line3DVector[2].x, m_line3DVector[2].y, -m_line3DVtx[0].z );
		GX_Color4u8( m_line3DVtx[1].r,m_line3DVtx[1].g, m_line3DVtx[1].b, m_line3DVtx[1].a );
		GX_Position3f32( m_line3DVector[1].x, m_line3DVector[1].y, -m_line3DVtx[1].z );
		GX_Color4u8( m_line3DVtx[0].r,m_line3DVtx[0].g, m_line3DVtx[0].b, m_line3DVtx[0].a );
		GX_Position3f32( m_line3DVector[0].x, m_line3DVector[0].y, -m_line3DVtx[0].z );
		GX_Color4u8( m_line3DVtx[0].r,m_line3DVtx[0].g, m_line3DVtx[0].b, m_line3DVtx[0].a );
	GX_End();
#ifdef RICE_STATS
	DEBUG_stats(RICE_STATS+2, "RenderLine3D", STAT_TYPE_ACCUM, 1);
#endif
//	printf("RenderLine3D\n");

#endif //__GX__

    ApplyZBias(m_dwZBias);          // set Z offset back to previous value

    return true;
}

extern FiddledVtx * g_pVtxBase;

// This is so weired that I can not do vertex transform by myself. I have to use
// OpenGL internal transform
bool OGLRender::RenderFlushTris()
{
    if( !m_bSupportFogCoordExt )    
        SetFogFlagForNegativeW();
    else
    {
        if( !gRDP.bFogEnableInBlender && gRSP.bFogEnabled )
        {
#ifndef __GX__  //This is redundant with CRender::DrawTriangles()
            glDisable(GL_FOG);
#endif //!__GX__
        }
    }

    ApplyZBias(m_dwZBias);                    // set the bias factors

    glViewportWrapper(windowSetting.vpLeftW, windowSetting.uDisplayHeight-windowSetting.vpTopW-windowSetting.vpHeightW+windowSetting.statusBarHeightToUse, windowSetting.vpWidthW, windowSetting.vpHeightW, false);
    //if options.bOGLVertexClipper == FALSE )
    {
#ifndef __GX__
		//TODO: Implement in GX
        glDrawElements( GL_TRIANGLES, gRSP.numVertices, GL_UNSIGNED_INT, g_vtxIndex );
#else //!__GX__
#ifdef RICE_STATS
		DEBUG_stats(RICE_STATS+3, "RenderFlush Tris - gRSP.numVert", STAT_TYPE_ACCUM, gRSP.numVertices);
#endif


//		printf("RenderFlushTris: gRSP.numVert = %d\n",gRSP.numVertices);

	//These are here temporarily until combining/blending is sorted out...
	//Set PassColor TEV mode
/*	GX_SetNumChans (1);
	GX_SetNumTexGens (0);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);*/
	//Set CopyModeDraw blend modes here
/*	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);

	GX_SetZMode(GX_ENABLE,GX_LEQUAL,GX_TRUE);
*/

	GXloadTextures();

	//set vertex description here
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
	if (gGX.GXmultiTex) GX_SetVtxDesc(GX_VA_TEX1MTXIDX, GX_TEXMTX0);
//	if (combiner.usesT0) GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
//	if (combiner.usesT1) GX_SetVtxDesc(GX_VA_TEX1MTXIDX, GX_TEXMTX0);
//	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
//	GX_SetVtxDesc(GX_VA_TEX1MTXIDX, GX_TEXMTX0);
//	GX_SetVtxDesc(GX_VA_TEX2MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
//	if (lighting) GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	if (gGX.GXmultiTex) GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
//	if (combiner.usesT0) GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
//	if (combiner.usesT1) GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
//	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
//	GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
//	GX_SetVtxDesc(GX_VA_TEX2, GX_DIRECT);

	//set vertex attribute formats here
	//TODO: These only need to be set once during init.
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
//	if (lighting) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
//	if (combiner.usesT0) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
//	if (combiner.usesT1) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	if (gGX.GXmultiTex) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
//	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0);

    //glVertexPointer( 4, GL_FLOAT, sizeof(float)*5, &(g_vtxProjected5[0][0]) );
    //glEnableClientState( GL_VERTEX_ARRAY );
    //glColorPointer( 1, GL_UNSIGNED_BYTE, sizeof(TLITVERTEX), &g_vtxBuffer[0].r);
    //glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof(uint8)*4, &(g_oglVtxColors[0][0]) );
    //glEnableClientState( GL_COLOR_ARRAY );
	GXColor GXcol = (GXColor) {0,0,255,255};
	float invW;

//	GX_Begin(GX_LINESTRIP, GX_VTXFMT0, gRSP.numVertices);
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, gRSP.numVertices);
	for (int i = 0; i < (int) gRSP.numVertices; i++) {
		if(gGX.GXuseCombW)
			GX_Position3f32( g_vtxProjected5[g_vtxIndex[i]][0], g_vtxProjected5[g_vtxIndex[i]][1], -g_vtxProjected5[g_vtxIndex[i]][3] );
//			GX_Position3f32( OGL.vertices[i].x, OGL.vertices[i].y, -OGL.vertices[i].w );
		else
		{
			invW = (g_vtxProjected5[g_vtxIndex[i]][3] != 0) ? 1/g_vtxProjected5[g_vtxIndex[i]][3] : 0.0f;
			//Fix for NoN tri's
			GX_Position3f32( g_vtxProjected5[g_vtxIndex[i]][0]*invW, g_vtxProjected5[g_vtxIndex[i]][1]*invW, max(-1.0f,g_vtxProjected5[g_vtxIndex[i]][2]*invW) );
//			GX_Position3f32( g_vtxProjected5[g_vtxIndex[i]][0]*invW, g_vtxProjected5[g_vtxIndex[i]][1]*invW, g_vtxProjected5[g_vtxIndex[i]][2]*invW );
//			invW = (OGL.vertices[i].w != 0) ? 1/OGL.vertices[i].w : 0.0f;
//			GX_Position3f32( OGL.vertices[i].x*invW, OGL.vertices[i].y*invW, OGL.vertices[i].z*invW );
#if 0//def SHOW_DEBUG
			sprintf(txtbuffer,"\r\nVertex %i: x = %f, y = %f, z = %f, InvW = %f", i, g_vtxProjected5[g_vtxIndex[i]][0], g_vtxProjected5[g_vtxIndex[i]][1], g_vtxProjected5[g_vtxIndex[i]][2], invW);
			DEBUG_print(txtbuffer,DBG_USBGECKO);
			sprintf(txtbuffer,"\r\nVertex %i: xw = %f, y = %f, z = %f", i, g_vtxProjected5[g_vtxIndex[i]][0]*invW, g_vtxProjected5[g_vtxIndex[i]][1]*invW, g_vtxProjected5[g_vtxIndex[i]][2]*invW);
			DEBUG_print(txtbuffer,DBG_USBGECKO);
#endif
		}
//		GX_Position3f32(OGL.vertices[i].x/OGL.vertices[i].w, OGL.vertices[i].y/OGL.vertices[i].w, OGL.vertices[i].z/OGL.vertices[i].w);
//		GXcol.r = (u8) (min(OGL.vertices[i].color.r,1.0)*255);
//		GXcol.g = (u8) (min(OGL.vertices[i].color.g,1.0)*255);
//		GXcol.b = (u8) (min(OGL.vertices[i].color.b,1.0)*255);
//		GXcol.a = (u8) (min(OGL.vertices[i].color.a,1.0)*255);
		GXcol.r = g_oglVtxColors[g_vtxIndex[i]][0];
		GXcol.g = g_oglVtxColors[g_vtxIndex[i]][1];
		GXcol.b = g_oglVtxColors[g_vtxIndex[i]][2];
		GXcol.a = g_oglVtxColors[g_vtxIndex[i]][3];
		//GXcol = (GXColor) {100,100,200,128};
#ifdef SHOW_DEBUG
//		sprintf(txtbuffer,"Vtx Color: {%d,%d,%d,%d} \r\n", g_oglVtxColors[g_vtxIndex[i]][0], g_oglVtxColors[g_vtxIndex[i]][1], g_oglVtxColors[g_vtxIndex[i]][2], g_oglVtxColors[g_vtxIndex[i]][3]);
//		DEBUG_print(txtbuffer,DBG_USBGECKO);
#endif
//		GX_Color4u8(g_oglVtxColors[g_vtxIndex[i]][0], g_oglVtxColors[g_vtxIndex[i]][1], g_oglVtxColors[g_vtxIndex[i]][2], g_oglVtxColors[g_vtxIndex[i]][3]); 
		GX_Color4u8(GXcol.r, GXcol.g, GXcol.b, GXcol.a); 
		GX_TexCoord2f32(g_vtxBuffer[i].tcord[0].u,g_vtxBuffer[i].tcord[0].v);
		if (gGX.GXmultiTex) GX_TexCoord2f32(g_vtxBuffer[i].tcord[1].u,g_vtxBuffer[i].tcord[1].v);
//		if (combiner.usesT0) GX_TexCoord2f32(OGL.vertices[i].s0,OGL.vertices[i].t0);
//		if (combiner.usesT1) GX_TexCoord2f32(OGL.vertices[i].s1,OGL.vertices[i].t1);
//		if (combiner.usesT0) GX_TexCoord2f32(OGL.vertices[i].s0,OGL.vertices[i].t0);
//		else GX_TexCoord2f32(0.0f,0.0f);
//		if (combiner.usesT1) GX_TexCoord2f32(OGL.vertices[i].s1,OGL.vertices[i].t1);
//		else GX_TexCoord2f32(0.0f,0.0f);
//		GX_TexCoord2f32(0.0f,0.0f);
	}
	GX_End();

#endif //__GX__
    }
/*  else
    {
        //ClipVertexesOpenGL();
        // Redo the index
        // Set the array
        glVertexPointer( 4, GL_FLOAT, sizeof(float)*5, &(g_vtxProjected5Clipped[0][0]) );
        glEnableClientState( GL_VERTEX_ARRAY );

        glClientActiveTextureARB( GL_TEXTURE0_ARB );
        glTexCoordPointer( 2, GL_FLOAT, sizeof( TLITVERTEX ), &(g_clippedVtxBuffer[0].tcord[0].u) );
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );

        glClientActiveTextureARB( GL_TEXTURE1_ARB );
        glTexCoordPointer( 2, GL_FLOAT, sizeof( TLITVERTEX ), &(g_clippedVtxBuffer[0].tcord[1].u) );
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );

        glDrawElements( GL_TRIANGLES, gRSP.numVertices, GL_UNSIGNED_INT, g_vtxIndex );

        // Reset the array
        glClientActiveTextureARB( GL_TEXTURE0_ARB );
        glTexCoordPointer( 2, GL_FLOAT, sizeof( TLITVERTEX ), &(g_vtxBuffer[0].tcord[0].u) );
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );

        glClientActiveTextureARB( GL_TEXTURE1_ARB );
        glTexCoordPointer( 2, GL_FLOAT, sizeof( TLITVERTEX ), &(g_vtxBuffer[0].tcord[1].u) );
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );

        glVertexPointer( 4, GL_FLOAT, sizeof(float)*5, &(g_vtxProjected5[0][0]) );
        glEnableClientState( GL_VERTEX_ARRAY );
    }
*/

    if( !m_bSupportFogCoordExt )    
        RestoreFogFlag();
    else
    {
        if( !gRDP.bFogEnableInBlender && gRSP.bFogEnabled )
        {
#ifndef __GX__  //This is redundant with CRender::DrawTriangles()
            glEnable(GL_FOG);
#endif //!__GX__
        }
    }
    return true;
}

void OGLRender::DrawSimple2DTexture(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, COLOR dif, COLOR spe, float z, float rhw)
{
    if( status.bVIOriginIsUpdated == true && currentRomOptions.screenUpdateSetting==SCREEN_UPDATE_AT_1ST_PRIMITIVE )
    {
        status.bVIOriginIsUpdated=false;
        CGraphicsContext::Get()->UpdateFrame();
        DEBUGGER_PAUSE_AND_DUMP_NO_UPDATE(NEXT_SET_CIMG,{DebuggerAppendMsg("Screen Update at 1st Simple2DTexture");});
    }

    StartDrawSimple2DTexture(x0, y0, x1, y1, u0, v0, u1, v1, dif, spe, z, rhw);

#ifndef __GX__
    GLboolean cullface = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    glViewportWrapper(0, windowSetting.statusBarHeightToUse, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight);

    glBegin(GL_TRIANGLES);
    float a = (g_texRectTVtx[0].dcDiffuse >>24)/255.0f;
    float r = ((g_texRectTVtx[0].dcDiffuse>>16)&0xFF)/255.0f;
    float g = ((g_texRectTVtx[0].dcDiffuse>>8)&0xFF)/255.0f;
    float b = (g_texRectTVtx[0].dcDiffuse&0xFF)/255.0f;
    glColor4f(r,g,b,a);

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

	glViewportWrapper(0, windowSetting.statusBarHeightToUse, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight);

	GXColor GXcol;
	GXcol.r = (u8) ((g_texRectTVtx[0].dcDiffuse>>16)&0xFF);
	GXcol.g = (u8) ((g_texRectTVtx[0].dcDiffuse>>8)&0xFF);
	GXcol.b = (u8) (g_texRectTVtx[0].dcDiffuse&0xFF);
	GXcol.a = (u8) (g_texRectTVtx[0].dcDiffuse >>24);

	GXloadTextures();

	//set vertex description here
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_TEX0MTXIDX, GX_TEXMTX0);
//	GX_SetVtxDesc(GX_VA_TEX1MTXIDX, GX_TEXMTX0);
//	GX_SetVtxDesc(GX_VA_TEX2MTXIDX, GX_TEXMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
//	GX_SetVtxDesc(GX_VA_TEX1, GX_DIRECT);
//	GX_SetVtxDesc(GX_VA_TEX2, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
//	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
//	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0);
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

#ifdef RICE_STATS
	DEBUG_stats(RICE_STATS+4, "DrawSimple2DTexture", STAT_TYPE_ACCUM, 1);
#endif
//	printf("DrawSimple2DTexture\n");

#endif //__GX__
}

void OGLRender::DrawSimpleRect(LONG nX0, LONG nY0, LONG nX1, LONG nY1, uint32 dwColor, float depth, float rhw) //This function is never called..
{
    StartDrawSimpleRect(nX0, nY0, nX1, nY1, dwColor, depth, rhw);

#ifndef __GX__
	//TODO: Replace with GX
    GLboolean cullface = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    glBegin(GL_TRIANGLE_FAN);

    float a = (dwColor>>24)/255.0f;
    float r = ((dwColor>>16)&0xFF)/255.0f;
    float g = ((dwColor>>8)&0xFF)/255.0f;
    float b = (dwColor&0xFF)/255.0f;
    glColor4f(r,g,b,a);
    glVertex3f(m_simpleRectVtx[1].x, m_simpleRectVtx[0].y, -depth);
    glVertex3f(m_simpleRectVtx[1].x, m_simpleRectVtx[1].y, -depth);
    glVertex3f(m_simpleRectVtx[0].x, m_simpleRectVtx[1].y, -depth);
    glVertex3f(m_simpleRectVtx[0].x, m_simpleRectVtx[0].y, -depth);
    
    glEnd();

    if( cullface ) glEnable(GL_CULL_FACE);
#else //!__GX__
#ifdef RICE_STATS
	DEBUG_stats(RICE_STATS+5, "DrawSimpleRect", STAT_TYPE_ACCUM, 1);
#endif
//	printf("DrawSimpleRect\n");

#endif //__GX__
}

void OGLRender::InitCombinerBlenderForSimpleRectDraw(uint32 tile) //This function is never called..
{
    //glEnable(GL_CULL_FACE);
    EnableTexUnit(0,FALSE);
#ifndef __GX__
	//TODO: Replace with GX
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glEnable(GL_ALPHA_TEST);
#endif //__GX__
}

COLOR OGLRender::PostProcessDiffuseColor(COLOR curDiffuseColor)
{
    uint32 color = curDiffuseColor;
    uint32 colorflag = m_pColorCombiner->m_pDecodedMux->m_dwShadeColorChannelFlag;
    uint32 alphaflag = m_pColorCombiner->m_pDecodedMux->m_dwShadeAlphaChannelFlag;
    if( colorflag+alphaflag != MUX_0 )
    {
#if 1 //ndef _BIG_ENDIAN //Maybe no endian problem... This is testing whether colorflag or alphaflag are pointers
        if( (colorflag & 0xFFFFFF00) == 0 && (alphaflag & 0xFFFFFF00) == 0 )
#else // !_BIG_ENDIAN
        if( (colorflag & 0x00FFFFFF) == 0 && (alphaflag & 0x00FFFFFF) == 0 )
#endif // _BIG_ENDIAN
        {
            color = (m_pColorCombiner->GetConstFactor(colorflag, alphaflag, curDiffuseColor));
        }
        else
            color = (CalculateConstFactor(colorflag, alphaflag, curDiffuseColor));
#if 0 //def SHOW_DEBUG
		sprintf(txtbuffer,"\r\nPostProcDiffCol: cflag 0x%8x, aflag 0x%8x, color 0x%8x\r\n",colorflag, alphaflag, color);
		DEBUG_print(txtbuffer,DBG_USBGECKO);
#endif
    }

    //return (color<<8)|(color>>24);
    return color;
}

COLOR OGLRender::PostProcessSpecularColor()
{
    return 0;
}

void OGLRender::SetViewportRender()
{
    glViewportWrapper(windowSetting.vpLeftW, windowSetting.uDisplayHeight-windowSetting.vpTopW-windowSetting.vpHeightW+windowSetting.statusBarHeightToUse, windowSetting.vpWidthW, windowSetting.vpHeightW);
}

void OGLRender::RenderReset()
{
    CRender::RenderReset();

#ifndef __GX__
	//TODO: Replace with GX
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight, 0, -1, 1);

    // position viewer 
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#else //!__GX__
	//This is messed up somehow...
	Mtx44 GXprojection;
	guMtxIdentity(GXprojection);
	guOrtho(GXprojection, 0, windowSetting.uDisplayHeight, 0, windowSetting.uDisplayWidth, -1.0f, 1.0f);
	GX_LoadProjectionMtx(GXprojection, GX_ORTHOGRAPHIC); 
	GX_LoadPosMtxImm(gGX.GXmodelViewIdent,GX_PNMTX0);

	//This is called at the beginning of every DList, so initialize GX stuff here:
	// init fog
	gGX.GXfogStartZ = -1.0f;
	gGX.GXfogEndZ = 1.0f;
	gGX.GXfogColor = (GXColor){0,0,0,255};
	gGX.GXfogType = GX_FOG_NONE;
	GX_SetFog(gGX.GXfogType,gGX.GXfogStartZ,gGX.GXfogEndZ,-1.0,1.0,gGX.GXfogColor);
	gGX.GXupdateFog = false;

#endif //__GX__
}

void OGLRender::SetAlphaTestEnable(BOOL bAlphaTestEnable)
{
#ifndef __GX__
	//TODO: REplace with GX
#ifdef _DEBUG
    if( bAlphaTestEnable && debuggerEnableAlphaTest )
#else
    if( bAlphaTestEnable )
#endif
        glEnable(GL_ALPHA_TEST);
    else
        glDisable(GL_ALPHA_TEST);
#else //!__GX__
    if( bAlphaTestEnable )
	{
		GX_SetAlphaCompare(gGX.GXalphaFunc,gGX.GXalphaRef,GX_AOP_AND,GX_ALWAYS,0);
		GX_SetZCompLoc(GX_FALSE); //Zcomp after texturing
	}
    else
	{
		GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
		GX_SetZCompLoc(GX_TRUE); //Zcomp before texturing for better performance
	}
#endif //__GX__
}

#ifndef __GX__
void OGLRender::BindTexture(GLuint texture, int unitno)
#else //!__GX__
void OGLRender::BindTexture(COGLTexture *texture, int unitno)
#endif //__GX__
{
#ifdef _DEBUG
    if( unitno != 0 )
    {
        DebuggerAppendMsg("Check me, base ogl bind texture, unit no != 0");
    }
#endif
    if( m_curBoundTex[0] != texture )
    {
#ifndef __GX__
		//TODO: Figure out where to load GX textures
        glBindTexture(GL_TEXTURE_2D,texture);
#endif //!__GX__
        m_curBoundTex[0] = texture;
#ifdef __GX__
		GXreloadTex[0] = true;
#endif //__GX__
    }
}

#ifndef __GX__
void OGLRender::DisBindTexture(GLuint texture, int unitno)
{
    //EnableTexUnit(0,FALSE);
    //glBindTexture(GL_TEXTURE_2D, 0);  //Not to bind any texture
}
#else //!__GX__ //This Function is never called by TEVCombiner
void OGLRender::DisBindTexture(COGLTexture *texture, int unitno) { }
#endif //__GX__

void OGLRender::EnableTexUnit(int unitno, BOOL flag)
{
#ifdef _DEBUG
    if( unitno != 0 )
    {
        DebuggerAppendMsg("Check me, in the base ogl render, unitno!=0");
    }
#endif
    if( m_texUnitEnabled[0] != flag )
    {
        m_texUnitEnabled[0] = flag;
#ifndef __GX__
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

void OGLRender::TexCoord2f(float u, float v) //This function is never called
{
#ifndef __GX__
	//TODO: Replace with GX
    glTexCoord2f(u, v);
#endif //!__GX__
}

void OGLRender::TexCoord(TLITVERTEX &vtxInfo)
{
#ifndef __GX__
	//TODO: Replace with GX
    glTexCoord2f(vtxInfo.tcord[0].u, vtxInfo.tcord[0].v);
#endif //!__GX__
}

void OGLRender::UpdateScissor()
{
    if( options.bEnableHacks && g_CI.dwWidth == 0x200 && gRDP.scissor.right == 0x200 && g_CI.dwWidth>(*g_GraphicsInfo.VI_WIDTH_REG & 0xFFF) )
    {
        // Hack for RE2
        uint32 width = *g_GraphicsInfo.VI_WIDTH_REG & 0xFFF;
        uint32 height = (gRDP.scissor.right*gRDP.scissor.bottom)/width;
#ifndef __GX__
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, int(height*windowSetting.fMultY+windowSetting.statusBarHeightToUse),
            int(width*windowSetting.fMultX), int(height*windowSetting.fMultY) );
#else //!__GX__
		//Notes: windowSetting.statusBarHeightToUse = 0 for fullscreen mode; uly may be incorrect
		float ulx = gGX.GXorigX;
		float uly = gGX.GXorigY;
		float lrx = max(gGX.GXorigX + min((width*windowSetting.fMultX) * gGX.GXscaleX,gGX.GXwidth), 0);
		float lry = max(gGX.GXorigY + min((height*windowSetting.fMultY) * gGX.GXscaleY,gGX.GXheight), 0);
		GX_SetScissor((u32) ulx,(u32) uly,(u32) (lrx - ulx),(u32) (lry - uly));
#ifdef SHOW_DEBUG
//		sprintf(txtbuffer,"\r\nUpdateScissor: width %d, height %d, fMultX %f, fMultY %f, statBarHt %d, ulx %f, uly %f, lrx %f, lry %f\r\n",
//			width, height, windowSetting.fMultX, windowSetting.fMultY, windowSetting.statusBarHeightToUse,
//			ulx, uly, lrx, lry);
//		DEBUG_print(txtbuffer,DBG_USBGECKO);
#endif //SHOW_DEBUG
#endif //__GX__
    }
    else
    {
        UpdateScissorWithClipRatio();
    }
}

void OGLRender::ApplyRDPScissor(bool force)
{
    if( !force && status.curScissor == RDP_SCISSOR )    return;

    if( options.bEnableHacks && g_CI.dwWidth == 0x200 && gRDP.scissor.right == 0x200 && g_CI.dwWidth>(*g_GraphicsInfo.VI_WIDTH_REG & 0xFFF) )
    {
        // Hack for RE2
        uint32 width = *g_GraphicsInfo.VI_WIDTH_REG & 0xFFF;
        uint32 height = (gRDP.scissor.right*gRDP.scissor.bottom)/width;
#ifndef __GX__
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, int(height*windowSetting.fMultY+windowSetting.statusBarHeightToUse),
            int(width*windowSetting.fMultX), int(height*windowSetting.fMultY) );
#else //!__GX__
		//Notes: windowSetting.statusBarHeightToUse = 0 for fullscreen mode; uly may be incorrect
		float ulx = gGX.GXorigX;
		float uly = gGX.GXorigY;
		float lrx = max(gGX.GXorigX + min((width*windowSetting.fMultX) * gGX.GXscaleX,gGX.GXwidth), 0);
		float lry = max(gGX.GXorigY + min((height*windowSetting.fMultY) * gGX.GXscaleY,gGX.GXheight), 0);
		GX_SetScissor((u32) ulx,(u32) uly,(u32) (lrx - ulx),(u32) (lry - uly));
#ifdef SHOW_DEBUG
//		sprintf(txtbuffer,"\r\nApplyRDPScissor: width %d, height %d, fMultX %f, fMultY %f, statBarHt %d, ulx %f, uly %f, lrx %f, lry %f\r\n",
//			width, height, windowSetting.fMultX, windowSetting.fMultY, windowSetting.statusBarHeightToUse,
//			ulx, uly, lrx, lry);
//		DEBUG_print(txtbuffer,DBG_USBGECKO);
#endif //SHOW_DEBUG
#endif //__GX__
    }
    else
    {
#ifndef __GX__
        glScissor(int(gRDP.scissor.left*windowSetting.fMultX), int((windowSetting.uViHeight-gRDP.scissor.bottom)*windowSetting.fMultY+windowSetting.statusBarHeightToUse),
            int((gRDP.scissor.right-gRDP.scissor.left)*windowSetting.fMultX), int((gRDP.scissor.bottom-gRDP.scissor.top)*windowSetting.fMultY ));
#else //!__GX__
		//Notes: windowSetting.statusBarHeightToUse = 0 for fullscreen mode
		float ulx = max(gGX.GXorigX + (gRDP.scissor.left*windowSetting.fMultX) * gGX.GXscaleX, 0);
		float uly = max(gGX.GXorigY + (gRDP.scissor.top*windowSetting.fMultY) * gGX.GXscaleY, 0);
		float lrx = max(gGX.GXorigX + min((gRDP.scissor.right*windowSetting.fMultX) * gGX.GXscaleX,gGX.GXwidth), 0);
		float lry = max(gGX.GXorigY + min((gRDP.scissor.bottom*windowSetting.fMultY) * gGX.GXscaleY,gGX.GXheight), 0);
		GX_SetScissor((u32) ulx,(u32) uly,(u32) (lrx - ulx),(u32) (lry - uly));
#ifdef SHOW_DEBUG
//		sprintf(txtbuffer,"\r\nApplyRDPScissor: RSP.lft %d, RDP.rt %d, RDP.top %d, RDP.bot %d, uViHeight %d, \r\n    fMultX %f, fMultY %f, statBarHt %d, ulx %f, uly %f, lrx %f, lry %f\r\n",
//			gRDP.scissor.left, gRDP.scissor.right, gRDP.scissor.top, gRDP.scissor.bottom, windowSetting.uViHeight,
//			windowSetting.fMultX, windowSetting.fMultY, windowSetting.statusBarHeightToUse, ulx, uly, lrx, lry);
//		DEBUG_print(txtbuffer,DBG_USBGECKO);
#endif //SHOW_DEBUG
#endif //__GX__
    }

    status.curScissor = RDP_SCISSOR;
}

void OGLRender::ApplyScissorWithClipRatio(bool force)
{
    if( !force && status.curScissor == RSP_SCISSOR )    return;

#ifndef __GX__
    glEnable(GL_SCISSOR_TEST);
    glScissor(windowSetting.clipping.left, int((windowSetting.uViHeight-gRSP.real_clip_scissor_bottom)*windowSetting.fMultY)+windowSetting.statusBarHeightToUse,
        windowSetting.clipping.width, windowSetting.clipping.height);
#else //!__GX__
	//Notes: windowSetting.statusBarHeightToUse = 0 for fullscreen mode
	float ulx = max(gGX.GXorigX + windowSetting.clipping.left * gGX.GXscaleX, 0);
	float uly = max(gGX.GXorigY + ((gRSP.real_clip_scissor_top)*windowSetting.fMultY) * gGX.GXscaleY, 0);
	float lrx = max(gGX.GXorigX + min((windowSetting.clipping.left+windowSetting.clipping.width) * gGX.GXscaleX,gGX.GXwidth), 0);
	float lry = max(gGX.GXorigY + min(((gRSP.real_clip_scissor_top*windowSetting.fMultY) + windowSetting.clipping.height) * gGX.GXscaleY,gGX.GXheight), 0);
	GX_SetScissor((u32) ulx,(u32) uly,(u32) (lrx - ulx),(u32) (lry - uly));
#ifdef SHOW_DEBUG
//	sprintf(txtbuffer,"\r\nApplyScissorWithClipRatio: clip.lft %d, clip.wd %d, clip.ht %d, uViHeight %d, gRSP.clip_bot %d, fMultY %f, \r\n    statBarHt %d, ulx %f, uly %f, lrx %f, lry %f\r\n",
//		windowSetting.clipping.left, windowSetting.clipping.width, windowSetting.clipping.height, 
//		windowSetting.uViHeight, gRSP.real_clip_scissor_bottom, windowSetting.fMultY,
//		windowSetting.statusBarHeightToUse, ulx, uly, lrx, lry);
//	DEBUG_print(txtbuffer,DBG_USBGECKO);
#endif //SHOW_DEBUG
#endif //__GX__

    status.curScissor = RSP_SCISSOR;
}

void OGLRender::SetFogMinMax(float fMin, float fMax)
{
#ifndef __GX__
	//TODO: Replace with GX
    glFogf(GL_FOG_START, gRSPfFogMin); // Fog Start Depth
    glFogf(GL_FOG_END, gRSPfFogMax); // Fog End Depth
#else //!__GX__
	gGX.GXfogStartZ = gRSPfFogMin;
	gGX.GXfogEndZ = gRSPfFogMax;

	GX_SetFog(gGX.GXfogType,gGX.GXfogStartZ,gGX.GXfogEndZ,-1.0,1.0,gGX.GXfogColor);
#endif //__GX__
}

void OGLRender::TurnFogOnOff(bool flag)
{
#ifndef __GX__
    if( flag )
        glEnable(GL_FOG);
    else
        glDisable(GL_FOG);
#else //!__GX__
    if( flag )
		gGX.GXfogType = GX_FOG_ORTHO_LIN;
    else
		gGX.GXfogType = GX_FOG_NONE;
//		gGX.GXfogType = GX_FOG_ORTHO_LIN;

	GX_SetFog(gGX.GXfogType,gGX.GXfogStartZ,gGX.GXfogEndZ,-1.0,1.0,gGX.GXfogColor);
#endif //__GX__
}

void OGLRender::SetFogEnable(bool bEnable)
{
    DEBUGGER_IF_DUMP( (gRSP.bFogEnabled != (bEnable==TRUE) && logFog ), TRACE1("Set Fog %s", bEnable? "enable":"disable"));

    gRSP.bFogEnabled = bEnable&&options.bEnableFog;

#ifndef __GX__
    if( gRSP.bFogEnabled )
    {
        //TRACE2("Enable fog, min=%f, max=%f",gRSPfFogMin,gRSPfFogMax );
        glFogfv(GL_FOG_COLOR, gRDP.fvFogColor); // Set Fog Color
        glFogf(GL_FOG_START, gRSPfFogMin); // Fog Start Depth
        glFogf(GL_FOG_END, gRSPfFogMax); // Fog End Depth
        glEnable(GL_FOG);
    }
    else
    {
        glDisable(GL_FOG);
    }
#else //!__GX__
	if( gRSP.bFogEnabled )
	{
		gGX.GXfogType = GX_FOG_ORTHO_LIN;
#if 0 //def SHOW_DEBUG
		sprintf(txtbuffer,"SetFog: StartZ %f, EndZ %f, color (%d,%d,%d,%d), fo %f, fm %f", gGX.GXfogStartZ, gGX.GXfogEndZ, gGX.GXfogColor.r, gGX.GXfogColor.g, gGX.GXfogColor.b, gGX.GXfogColor.a, gRSPfFogMin, gRSPfFogMax);
		DEBUG_print(txtbuffer,DBG_RSPINFO1);
#endif
	}
	else
		gGX.GXfogType = GX_FOG_NONE;

	GX_SetFog(gGX.GXfogType,gGX.GXfogStartZ,gGX.GXfogEndZ,-1.0,1.0,gGX.GXfogColor);
#endif //__GX__
}

void OGLRender::SetFogColor(uint32 r, uint32 g, uint32 b, uint32 a)
{
    gRDP.fogColor = COLOR_RGBA(r, g, b, a); 
    gRDP.fvFogColor[0] = r/255.0f;      //r
    gRDP.fvFogColor[1] = g/255.0f;      //g
    gRDP.fvFogColor[2] = b/255.0f;          //b
    gRDP.fvFogColor[3] = a/255.0f;      //a
#ifndef __GX__
    glFogfv(GL_FOG_COLOR, gRDP.fvFogColor); // Set Fog Color
#else //!__GX__
	gGX.GXfogColor.r = (u8) (r & 0xff);
	gGX.GXfogColor.g = (u8) (g & 0xff);
	gGX.GXfogColor.b = (u8) (b & 0xff);
	gGX.GXfogColor.a = (u8) (a & 0xff);
	//gGX.GXupdateFog = true;

	GX_SetFog(gGX.GXfogType,gGX.GXfogStartZ,gGX.GXfogEndZ,-1.0,1.0,gGX.GXfogColor);
#endif //__GX__
}

void OGLRender::DisableMultiTexture()
{
#ifndef __GX__
    glActiveTexture(GL_TEXTURE1_ARB);
    EnableTexUnit(1,FALSE);
    glActiveTexture(GL_TEXTURE0_ARB);
    EnableTexUnit(0,FALSE);
    glActiveTexture(GL_TEXTURE0_ARB);
    EnableTexUnit(0,TRUE);
#else //!__GX__
    EnableTexUnit(1,FALSE);
    EnableTexUnit(0,FALSE);
    EnableTexUnit(0,TRUE);
#endif //__GX__
}

void OGLRender::EndRendering(void)
{
#ifndef __GX__
	//TODO: Replace with GX
    glFlush();
#endif //!__GX__
    if( CRender::gRenderReferenceCount > 0 ) 
        CRender::gRenderReferenceCount--;
}

void OGLRender::glViewportWrapper(GLint x, GLint y, GLsizei width, GLsizei height, bool flag)
{
    static GLint mx=0,my=0;
    static GLsizei m_width=0, m_height=0;
    static bool mflag=true;

#ifndef __GX__
	if( x!=mx || y!=my || width!=m_width || height!=m_height || mflag!=flag)
#else //!__GX__
    if( x!=mx || y!=my || width!=m_width || height!=m_height || mflag!=flag || gGX.GXupdateMtx)
#endif //__GX__
    {
        mx=x;
        my=y;
        m_width=width;
        m_height=height;
        mflag=flag;
#ifndef __GX__
		//TODO: Replace with GX
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        if( flag )  glOrtho(0, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight, 0, -1, 1);
        glViewport(x,y,width,height);
#else //!__GX__
		if( flag )
		{
			//Used for rectangles
			Mtx44 GXprojection;
			guMtxIdentity(GXprojection);
			guOrtho(GXprojection, 0, windowSetting.uDisplayHeight, 0, windowSetting.uDisplayWidth, -1.0f, 1.0f);
			if(gGX.GXpolyOffset)
				GXprojection[2][3] -= GXpolyOffsetFactor;
			GX_LoadProjectionMtx(GXprojection, GX_ORTHOGRAPHIC); 
//			GX_LoadPosMtxImm(OGL.GXmodelViewIdent,GX_PNMTX0);
		}
		else
		{
			//Used for triangles
			//Update MV & P Matrices
//			if(gGX.GXupdateMtx)  //TODO: uncomment this? Maybe set GXupdateMtx above in if(flag)
			{
				if(gGX.GXpolyOffset)
				{
					if(gGX.GXuseCombW)
					{
//						if(!OGL.GXuseProjWnear)
						{
							//TODO: use PS matrix copy?
//							CopyMatrix( gGX.GXprojTemp, gGX.GXcombW );
							guMtxCopy( gGX.GXcombW, gGX.GXprojTemp  ); //void c_guMtxCopy(Mtx src,Mtx dst);
							gGX.GXprojTemp[2][2] += GXpolyOffsetFactor;
							GX_LoadProjectionMtx(gGX.GXprojTemp, GX_PERSPECTIVE); 
						}
/*						else
						{
							GX_LoadProjectionMtx(OGL.GXprojWnear, GX_PERSPECTIVE); 
						}
*/					}
					else
					{
						//TODO: use PS matrix copy?
//						CopyMatrix( gGX.GXprojTemp, gGX.GXprojIdent );
						guMtxCopy( gGX.GXprojIdent, gGX.GXprojTemp  ); //void c_guMtxCopy(Mtx src,Mtx dst);
						gGX.GXprojTemp[2][3] -= GXpolyOffsetFactor;
						GX_LoadProjectionMtx(gGX.GXprojTemp, GX_ORTHOGRAPHIC); 
					}
				}
				else
				{
					//TODO: handle this case when !G_ZBUFFER
/*					if(!(gSP.geometryMode & G_ZBUFFER))
					{
						CopyMatrix( OGL.GXprojTemp, OGL.GXproj );
						OGL.GXprojTemp[2][2] =  0.0;
						OGL.GXprojTemp[2][3] = -1.0;
						if(OGL.GXprojTemp[3][2] == -1)
							GX_LoadProjectionMtx(OGL.GXprojTemp, GX_PERSPECTIVE);
						else
							GX_LoadProjectionMtx(OGL.GXprojTemp, GX_ORTHOGRAPHIC); 
					}*/
					if(gGX.GXuseCombW)
					{
//						if(!OGL.GXuseProjWnear)
						{
							GX_LoadProjectionMtx(gGX.GXcombW, GX_PERSPECTIVE); 
						}
/*						else
						{
							GX_LoadProjectionMtx(OGL.GXprojWnear, GX_PERSPECTIVE); 
						}
*/					}
					else
						GX_LoadProjectionMtx(gGX.GXprojIdent, GX_ORTHOGRAPHIC); 
				}
				gGX.GXupdateMtx = false;
			}
		}
		//Possibly separate this from gGX.GXupdateMtx to improve speed?
		GX_SetViewport((f32) (gGX.GXorigX + x),(f32) (gGX.GXorigY + (windowSetting.uDisplayHeight-(y+height))*gGX.GXscaleY),
			(f32) (gGX.GXscaleX * width),(f32) (gGX.GXscaleY * height), 0.0f, 1.0f);
#endif //__GX__
    }
}

void OGLRender::CaptureScreen(char *filename)
{
#ifndef __GX__
    unsigned char *buffer = (unsigned char*)malloc( windowSetting.uDisplayWidth * windowSetting.uDisplayHeight * 3 );

    GLint oldMode;
    glGetIntegerv( GL_READ_BUFFER, &oldMode );
    glReadBuffer( GL_FRONT );
    glReadPixels( 0, windowSetting.statusBarHeightToUse, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, buffer );
    glReadBuffer( oldMode );

    SaveRGBBufferToFile(filename, buffer, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight);

    free( buffer );
#endif //!__GX__
}

void OGLRender::GXloadTextures()
{
	if (GXreloadTex[0] && m_curBoundTex[0] && m_curBoundTex[0]->GXinited)	GX_LoadTexObj(&m_curBoundTex[0]->GXtex, 0); // t = 0 is GX_TEXMAP0 and t = 1 is GX_TEXMAP1
	if (GXreloadTex[1] && m_curBoundTex[1] && m_curBoundTex[1]->GXinited)	GX_LoadTexObj(&m_curBoundTex[1]->GXtex, 1); // t = 0 is GX_TEXMAP0 and t = 1 is GX_TEXMAP1
}

extern GXRModeObj *vmode, *rmode;

void OGLRender::GXclearEFB()
{
#if 0 //def __GX__
		static int callcount = 0;
		sprintf(txtbuffer,"GXclearEFB %d", callcount++);
		DEBUG_print(txtbuffer,DBG_RICE); 
#endif //__GX__

	//Note: EFB is RGB8, so no need to clear alpha
	if(gGX.GXclearColorBuffer)	GX_SetColorUpdate(GX_ENABLE);
	else						GX_SetColorUpdate(GX_DISABLE);
	if(gGX.GXclearDepthBuffer)	GX_SetZMode(GX_ENABLE,GX_GEQUAL,GX_TRUE);
	else						GX_SetZMode(GX_ENABLE,GX_GEQUAL,GX_FALSE);

	unsigned int cycle_type = gRDP.otherMode.cycle_type;
	gRDP.otherMode.cycle_type = CYCLE_TYPE_FILL;
	m_pColorCombiner->InitCombinerMode();
	m_pAlphaBlender->Disable();

	GX_SetZTexture(GX_ZT_DISABLE,GX_TF_Z16,0);	//GX_ZT_DISABLE or GX_ZT_REPLACE; set in gDP.cpp
	GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
	GX_SetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
	GX_SetFog(GX_FOG_NONE,0.1,1.0,0.0,1.0,(GXColor){0,0,0,255});
	GX_SetViewport((f32) gGX.GXorigX,(f32) gGX.GXorigY,(f32) gGX.GXwidth,(f32) gGX.GXheight, 0.0f, 1.0f);
	//The OGL renderer doesn't disable scissoring
//	GX_SetScissor((u32) 0,(u32) 0,(u32) windowSetting.uDisplayWidth+1,(u32) windowSetting.uDisplayHeight+1);	//Disable Scissor
	GX_SetCullMode (GX_CULL_NONE);
	Mtx44 GXprojection;
	guMtxIdentity(GXprojection);
//	guOrtho(GXprojection, 0, OGL.height-1, 0, OGL.width-1, 0.0f, 1.0f);
	guOrtho(GXprojection, 0, windowSetting.uDisplayHeight-1, 0, windowSetting.uDisplayWidth-1, 0.0f, 1.0f);
//	guOrtho(GXprojection, 0, 480, 0, 640, 0.0f, 1.0f);
	GX_LoadProjectionMtx(GXprojection, GX_ORTHOGRAPHIC); 
	GX_LoadPosMtxImm(gGX.GXmodelViewIdent,GX_PNMTX0);

	//set vertex description here
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
//	f32 ZmaxDepth = (f32) -0xFFFFFF/0x1000000;
	f32 ZmaxDepth = ((f32) -0xFFFFFF/0x1000000) * gGX.GXclearDepth;

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32(-1.0f, -1.0f, ZmaxDepth);
		GX_Color4u8(gGX.GXclearColor.r, gGX.GXclearColor.g, gGX.GXclearColor.b, gGX.GXclearColor.a); 
		GX_Position3f32((f32) windowSetting.uDisplayWidth+1, -1.0f, ZmaxDepth);
//		GX_Position3f32((f32) OGL.width+1, -1.0f, ZmaxDepth);
		GX_Color4u8(gGX.GXclearColor.r, gGX.GXclearColor.g, gGX.GXclearColor.b, gGX.GXclearColor.a); 
		GX_Position3f32((f32) windowSetting.uDisplayWidth+1,(f32) windowSetting.uDisplayHeight+1, ZmaxDepth);
//		GX_Position3f32((f32) OGL.width+1,(f32) OGL.height+1, ZmaxDepth);
		GX_Color4u8(gGX.GXclearColor.r, gGX.GXclearColor.g, gGX.GXclearColor.b, gGX.GXclearColor.a); 
		GX_Position3f32(-1.0f,(f32) windowSetting.uDisplayHeight+1, ZmaxDepth);
//		GX_Position3f32(-1.0f,(f32) OGL.height+1, ZmaxDepth);
		GX_Color4u8(gGX.GXclearColor.r, gGX.GXclearColor.g, gGX.GXclearColor.b, gGX.GXclearColor.a); 
	GX_End();

	//TODO: implement the following line?
	//if (gGX.GXclearColorBuffer) VI.EFBcleared = true;
	gGX.GXclearColorBuffer = false;
	gGX.GXclearDepthBuffer = false;
	gGX.GXupdateMtx = true;
	gGX.GXupdateFog = true;
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetCullMode(gGX.GXcullMode);
	//TODO: reset the following
//	OGL_UpdateViewport();
//	gDP.changed |= CHANGED_SCISSOR | CHANGED_COMBINE | CHANGED_RENDERMODE;	//Restore scissor in OGL_UpdateStates() before drawing next geometry.
	//CHANGED_SCISSOR
	GX_SetZMode(gGX.GXenableZmode,gGX.GXZfunc,gGX.GXZupdate);
	gRDP.otherMode.cycle_type = cycle_type;


/*	OGL.GXclearBufferTex = (u8*) memalign(32,640*480/2);
	GX_SetCopyClear (OGL.GXclearColor, 0xFFFFFF);
	if(OGL.GXclearColorBuffer)	GX_SetColorUpdate(GX_ENABLE);
	else						GX_SetColorUpdate(GX_DISABLE);
	if(OGL.GXclearDepthBuffer)	GX_SetZMode(GX_ENABLE,GX_ALWAYS,GX_TRUE);
	else						GX_SetZMode(GX_ENABLE,GX_ALWAYS,GX_FALSE);
	GX_SetTexCopySrc(0, 0, 640, 480);
	GX_SetTexCopyDst(640, 480, GX_TF_I4, GX_FALSE);
	GX_CopyTex(OGL.GXclearBufferTex, GX_TRUE);
	GX_PixModeSync();
//	GX_DrawDone();
	GX_SetColorUpdate(GX_ENABLE);
	free(OGL.GXclearBufferTex);
	gDP.changed |= CHANGED_RENDERMODE;*/
}