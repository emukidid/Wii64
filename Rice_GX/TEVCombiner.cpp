/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - OGLCombiner.cpp                                         *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
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
uint32 GX_BlendFuncMaps [] =
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

//========================================================================
CTEVColorCombiner::CTEVColorCombiner(CRender *pRender) :
	CColorCombiner(pRender),
	m_pOGLRender((OGLRender*)pRender),
	m_lastIndex(-1),
	m_dwLastMux0(0), m_dwLastMux1(0)
{
    m_pDecodedMux = new COGLDecodedMux;
//    m_pDecodedMux->m_maxConstants = 0; //Only used for setting const textures
//    m_pDecodedMux->m_maxTextures = 1;  //Only used for setting const textures
}

CTEVColorCombiner::~CTEVColorCombiner()
{
    delete m_pDecodedMux;
    m_pDecodedMux = NULL;
}

bool CTEVColorCombiner::Initialize(void)
{
    m_supportedStages = 16; //If >1, then single stage combines are split
    m_bSupportMultiTexture = true;

    COGLGraphicsContext *pcontext = (COGLGraphicsContext *)(CGraphicsContext::g_pGraphicsContext);

    return true;
}

void CTEVColorCombiner::DisableCombiner(void)
{
	//TODO: Implement in GX
    m_pOGLRender->DisableMultiTexture();
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE, GL_ZERO);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR); 
    
    if( m_bTexelsEnable )
    {
        COGLTexture* pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
        if( pTexture ) 
        {
            m_pOGLRender->EnableTexUnit(0,TRUE);
#ifndef __GX__
            m_pOGLRender->BindTexture(pTexture->m_dwTextureName, 0);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            m_pOGLRender->SetAllTexelRepeatFlag();
#else //!__GX__
            m_pOGLRender->BindTexture(pTexture, 0);
            m_pOGLRender->SetAllTexelRepeatFlag();
			//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			GX_LoadTexObj(&pTexture->GXtex, 0); // t = 0 is GX_TEXMAP0 and t = 1 is GX_TEXMAP1
#endif //__GX__
        }
#ifdef _DEBUG
        else
        {
            DebuggerAppendMsg("Check me, texture is NULL but it is enabled");
        }
#endif
		//Set Modulate TEV mode
		GX_SetNumTevStages(1);
		GX_SetNumChans (1);
		GX_SetNumTexGens (1);
		GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
    }
    else
    {
		//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		m_pOGLRender->EnableTexUnit(0,FALSE);
		//Set PassColor TEV mode
		GX_SetNumTevStages(1);
		GX_SetNumChans (1);
		GX_SetNumTexGens (0);
		GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
    }
}

void CTEVColorCombiner::InitCombinerCycleCopy(void)
{
    m_pOGLRender->DisableMultiTexture();
    m_pOGLRender->EnableTexUnit(0,TRUE);
    COGLTexture* pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
    if( pTexture )
    {
        //m_pOGLRender->BindTexture(pTexture->m_dwTextureName, 0);
        m_pOGLRender->BindTexture(pTexture, 0);
        m_pOGLRender->SetTexelRepeatFlags(gRSP.curTile);
		GX_LoadTexObj(&pTexture->GXtex, 0); // t = 0 is GX_TEXMAP0 and t = 1 is GX_TEXMAP1
    }
#ifdef _DEBUG
    else
    {
        DebuggerAppendMsg("Check me, texture is NULL");
    }
#endif

	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	//Set Replace TEV mode
	GX_SetNumTevStages(1);
	GX_SetNumChans (0);
	GX_SetNumTexGens (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORZERO);
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
}

void CTEVColorCombiner::InitCombinerCycleFill(void)
{
	//TODO: Call this over all TexUnits??
//    for( int i=0; i<m_supportedStages; i++ )
//    {
        //glActiveTexture(GL_TEXTURE0_ARB+i);
//        m_pOGLRender->EnableTexUnit(i,FALSE);
//    }

    m_pOGLRender->DisableMultiTexture();
    m_pOGLRender->EnableTexUnit(0,FALSE);

	//Set PassColor TEV mode
	GX_SetNumTevStages(1);
	GX_SetNumChans (1);
	GX_SetNumTexGens (0);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
}

void CTEVColorCombiner::InitCombinerCycle12(void)
{
#ifdef __GX__
//	sprintf(txtbuffer,"CTEVColorCombiner::InitCombinerCycle12");
//	DEBUG_print(txtbuffer,DBG_RICE+4); 
#endif //__GX__
#ifndef __GX__
	//TODO: Implement in GX
    m_pOGLRender->DisableMultiTexture();
    if( !m_bTexelsEnable )
    {
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        m_pOGLRender->EnableTexUnit(0,FALSE);
        return;
    }

    uint32 mask = 0x1f;
    COGLTexture* pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
    if( pTexture )
    {
        m_pOGLRender->EnableTexUnit(0,TRUE);
        m_pOGLRender->BindTexture(pTexture->m_dwTextureName, 0);
        m_pOGLRender->SetAllTexelRepeatFlag();
    }
#ifdef _DEBUG
    else
    {
        DebuggerAppendMsg("Check me, texture is NULL");
    }
#endif

    bool texIsUsed = m_pDecodedMux->isUsed(MUX_TEXEL0);
    bool shadeIsUsedInColor = m_pDecodedMux->isUsedInCycle(MUX_SHADE, 0, COLOR_CHANNEL);
    bool texIsUsedInColor = m_pDecodedMux->isUsedInCycle(MUX_TEXEL0, 0, COLOR_CHANNEL);

    if( texIsUsed )
    {
        // Parse the simplified the mux, because the OGL 1.1 combiner function is so much
        // limited, we only parse the 1st N64 combiner setting and only the RGB part

        N64CombinerType & comb = m_pDecodedMux->m_n64Combiners[0];
        switch( m_pDecodedMux->mType )
        {
        case CM_FMT_TYPE_NOT_USED:
        case CM_FMT_TYPE_D:             // = A
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            break;
        case CM_FMT_TYPE_A_ADD_D:           // = A+D
            if( shadeIsUsedInColor && texIsUsedInColor )
            {
                if( m_bSupportAdd )
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
                else
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
            }
            else if( texIsUsedInColor )
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            }
            else
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            break;
        case CM_FMT_TYPE_A_SUB_B:           // = A-B
            if( shadeIsUsedInColor && texIsUsedInColor )
            {
                if( m_bSupportSubtract )
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_SUBTRACT_ARB);
                else
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
            }
            else if( texIsUsedInColor )
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            }
            else
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            break;
        case CM_FMT_TYPE_A_MOD_C:           // = A*C
        case CM_FMT_TYPE_A_MOD_C_ADD_D: // = A*C+D
            if( shadeIsUsedInColor && texIsUsedInColor )
            {
                if( ((comb.c & mask) == MUX_SHADE && !(comb.c&MUX_COMPLEMENT)) || 
                    ((comb.a & mask) == MUX_SHADE && !(comb.a&MUX_COMPLEMENT)) )
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                else
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            }
            else if( texIsUsedInColor )
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            }
            else
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            break;
        case CM_FMT_TYPE_A_LERP_B_C:    // = A*C+D
            if( (comb.b&mask) == MUX_SHADE && (comb.c&mask)==MUX_TEXEL0 && ((comb.a&mask)==MUX_PRIM||(comb.a&mask)==MUX_ENV))
            {
                float *fv;
                if( (comb.a&mask)==MUX_PRIM )
                {
                    fv = GetPrimitiveColorfv();
                }
                else
                {
                    fv = GetEnvColorfv();
                }

                glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR,fv);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
                break;
            }
        default:        // = (A-B)*C+D
            if( shadeIsUsedInColor )
            {
                if( ((comb.c & mask) == MUX_SHADE && !(comb.c&MUX_COMPLEMENT)) || 
                    ((comb.a & mask) == MUX_SHADE && !(comb.a&MUX_COMPLEMENT)) )
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                else
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            }
            else
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            }
            break;
        }
    }
    else
    {
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
#else //!__GX__
	//TODO: Implement in GX

	u8 GXTevMode = GX_MODULATE;
    COGLTexture* pTexture = NULL;

    m_pOGLRender->DisableMultiTexture();
    if( !m_bTexelsEnable )
    {
        //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        //m_pOGLRender->EnableTexUnit(0,FALSE);
        //return;
		GXTevMode = GX_MODULATE;
		pTexture = NULL;
    }

    uint32 mask = 0x1f;
    pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
    if( pTexture )
    {
        m_pOGLRender->EnableTexUnit(0,TRUE);
        m_pOGLRender->BindTexture(pTexture, 0);
        m_pOGLRender->SetAllTexelRepeatFlag();
    }
#ifdef _DEBUG
    else
    {
        DebuggerAppendMsg("Check me, texture is NULL");
    }
#endif

    bool texIsUsed = m_pDecodedMux->isUsed(MUX_TEXEL0);
    bool shadeIsUsedInColor = m_pDecodedMux->isUsedInCycle(MUX_SHADE, 0, COLOR_CHANNEL);
    bool texIsUsedInColor = m_pDecodedMux->isUsedInCycle(MUX_TEXEL0, 0, COLOR_CHANNEL);

    if( texIsUsed )
    {
        // Parse the simplified the mux, because the OGL 1.1 combiner function is so much
        // limited, we only parse the 1st N64 combiner setting and only the RGB part

        N64CombinerType & comb = m_pDecodedMux->m_n64Combiners[0];
        switch( m_pDecodedMux->mType )
        {
        case CM_FMT_TYPE_NOT_USED:
        case CM_FMT_TYPE_D:             // = A
            //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			GXTevMode = GX_MODULATE;
            break;
        case CM_FMT_TYPE_A_ADD_D:           // = A+D
            if( shadeIsUsedInColor && texIsUsedInColor )
            {
				GXTevMode = GX_MODULATE;
                //if( m_bSupportAdd )
				//    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
                //else
                //    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
            }
            else if( texIsUsedInColor )
            {
				GXTevMode = GX_REPLACE;
                //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            }
            else
				GXTevMode = GX_MODULATE;
                //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            break;
        case CM_FMT_TYPE_A_SUB_B:           // = A-B
            if( shadeIsUsedInColor && texIsUsedInColor )
            {
//                if( m_bSupportSubtract )
					GXTevMode = GX_MODULATE;
                    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_SUBTRACT_ARB);
//                else
//					GXTevMode = GX_BLEND;
                    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
            }
            else if( texIsUsedInColor )
            {
				GXTevMode = GX_REPLACE;
                //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            }
            else
				GXTevMode = GX_MODULATE;
                //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            break;
        case CM_FMT_TYPE_A_MOD_C:           // = A*C
        case CM_FMT_TYPE_A_MOD_C_ADD_D: // = A*C+D
            if( shadeIsUsedInColor && texIsUsedInColor )
            {
                if( ((comb.c & mask) == MUX_SHADE && !(comb.c&MUX_COMPLEMENT)) || 
                    ((comb.a & mask) == MUX_SHADE && !(comb.a&MUX_COMPLEMENT)) )
					GXTevMode = GX_MODULATE;
                    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                else
					GXTevMode = GX_REPLACE;
                    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            }
            else if( texIsUsedInColor )
            {
				GXTevMode = GX_REPLACE;
                //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            }
            else
				GXTevMode = GX_MODULATE;
                //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            break;
        case CM_FMT_TYPE_A_LERP_B_C:    // = A*C+D
            if( (comb.b&mask) == MUX_SHADE && (comb.c&mask)==MUX_TEXEL0 && ((comb.a&mask)==MUX_PRIM||(comb.a&mask)==MUX_ENV))
            {
                float *fv;
                if( (comb.a&mask)==MUX_PRIM )
                {
                    fv = GetPrimitiveColorfv();
                }
                else
                {
                    fv = GetEnvColorfv();
                }

                //glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR,fv);
                //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
				GXTevMode = GX_BLEND;
                break;
            }
        default:        // = (A-B)*C+D
            if( shadeIsUsedInColor )
            {
                if( ((comb.c & mask) == MUX_SHADE && !(comb.c&MUX_COMPLEMENT)) || 
                    ((comb.a & mask) == MUX_SHADE && !(comb.a&MUX_COMPLEMENT)) )
					GXTevMode = GX_MODULATE;
                    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                else
					GXTevMode = GX_REPLACE;
                    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            }
            else
            {
				GXTevMode = GX_REPLACE;
                //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            }
            break;
        }
    }
    else
    {
		GXTevMode = GX_MODULATE;
        //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

	
	GX_SetTevOp(GX_TEVSTAGE0,GXTevMode);
	if( pTexture )
		GX_LoadTexObj(&pTexture->GXtex, 0); // t = 0 is GX_TEXMAP0 and t = 1 is GX_TEXMAP1

	GX_SetNumTevStages(1);
	GX_SetNumChans (1);
	GX_SetNumTexGens (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

#endif //__GX__
}

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

	u8 GXblenddstfactor, GXblendsrcfactor, GXblendmode;

/*	GXblendmode = GX_BM_BLEND;
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

void CTEVColorCombiner::InitCombinerBlenderForSimpleTextureDraw(uint32 tile)
{
#ifndef __GX__
	//TODO: Implement in GX
    m_pOGLRender->DisableMultiTexture();
    if( g_textures[tile].m_pCTexture )
    {
        m_pOGLRender->EnableTexUnit(0,TRUE);
        glBindTexture(GL_TEXTURE_2D, ((COGLTexture*)(g_textures[tile].m_pCTexture))->m_dwTextureName);
    }
    m_pOGLRender->SetAllTexelRepeatFlag();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // Linear Filtering
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // Linear Filtering

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#else //!__GX__
	//This is called only by CRender::DrawFrameBuffer() in RenderExt.cpp
#endif //__GX__
    m_pOGLRender->SetAlphaTestEnable(FALSE);
}

#ifdef _DEBUG
extern const char *translatedCombTypes[];
void CTEVColorCombiner::DisplaySimpleMuxString(void)
{
    TRACE0("\nSimplified Mux\n");
    m_pDecodedMux->DisplaySimpliedMuxString("Used");
}
#endif

