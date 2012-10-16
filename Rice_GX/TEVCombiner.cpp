/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Wii64 - TEVCombiner.cpp                                               *
 *   Wii64 homepage: http://code.google.com/p/mupen64gc/                   *
 *   Copyright (C) 2010, 2011, 2012 sepp256                                *
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
typedef struct {
	u8 Cin;
	u8 CinA;	//Used for MUX_ALPHAREPLICATE flag
	u8 Ain;
	u8 KCsel;
	u8 KCselA;	//Used for MUX_ALPHAREPLICATE flag
	u8 KAsel;
} RGBMapType;

#define MUX_GX_PREV		(MUX_UNK+1)
#define MUX_GX_TEMPREG0	(MUX_UNK+2)
#define MUX_GX_TEMPREG1	(MUX_UNK+3)

RGBMapType RGBArgsMap[] =
{//	ColIn,			ColInA,			AlphaIn,		KCsel,				KCselA,				KAsel
	{GX_CC_ZERO,	GX_CC_ZERO,		GX_CA_ZERO,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_0
	{GX_CC_ONE,		GX_CC_ONE,		GX_CA_KONST,	GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_1
	{GX_CC_C0,		GX_CC_A0,		GX_CA_A0,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_COMBINED,
	{GX_CC_TEXC,	GX_CC_TEXA,		GX_CA_TEXA,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_TEXEL0,
	{GX_CC_TEXC,	GX_CC_TEXA,		GX_CA_TEXA,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_TEXEL1,
	{GX_CC_KONST,	GX_CC_KONST,	GX_CA_KONST,	GX_TEV_KCSEL_K0,	GX_TEV_KCSEL_K0_A,	GX_TEV_KASEL_K0_A	},	//MUX_PRIM,
	{GX_CC_RASC,	GX_CC_RASA,		GX_CA_RASA,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_SHADE,
	{GX_CC_KONST,	GX_CC_KONST,	GX_CA_KONST,	GX_TEV_KCSEL_K1,	GX_TEV_KCSEL_K1_A,	GX_TEV_KASEL_K1_A	},	//MUX_ENV,
	{GX_CC_A0,		GX_CC_A0,		GX_CA_A0,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_COMBALPHA,
	{GX_CC_TEXA,	GX_CC_TEXA,		GX_CA_TEXA,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_T0_ALPHA,
	{GX_CC_TEXA,	GX_CC_TEXA,		GX_CA_TEXA,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_T1_ALPHA,
	{GX_CC_KONST,	GX_CC_KONST,	GX_CA_KONST,	GX_TEV_KCSEL_K0_A,	GX_TEV_KCSEL_K0_A,	GX_TEV_KASEL_K0_A	},	//MUX_PRIM_ALPHA,
	{GX_CC_RASA,	GX_CC_RASA,		GX_CA_RASA,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_SHADE_ALPHA,
	{GX_CC_KONST,	GX_CC_KONST,	GX_CA_KONST,	GX_TEV_KCSEL_K1_A,	GX_TEV_KCSEL_K1_A,	GX_TEV_KASEL_K1_A	},	//MUX_ENV_ALPHA,
	{GX_CC_KONST,	GX_CC_KONST,	GX_CA_KONST,	GX_TEV_KCSEL_K2,	GX_TEV_KCSEL_K2,	GX_TEV_KASEL_K2_A	},	//MUX_LODFRAC,
	{GX_CC_KONST,	GX_CC_KONST,	GX_CA_KONST,	GX_TEV_KCSEL_K3,	GX_TEV_KCSEL_K3,	GX_TEV_KASEL_K3_A	},	//MUX_PRIMLODFRAC,
	{GX_CC_RASC,	GX_CC_RASC,		GX_CA_RASA,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_K5
	{GX_CC_RASC,	GX_CC_RASC,		GX_CA_RASA,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_UNK
	{GX_CC_CPREV,	GX_CC_CPREV,	GX_CA_APREV,	GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_GX_PREV
	{GX_CC_C1,		GX_CC_C1,		GX_CA_A1,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_GX_TEMPREG0
	{GX_CC_C2,		GX_CC_C2,		GX_CA_A2,		GX_TEV_KCSEL_1,		GX_TEV_KCSEL_1,		GX_TEV_KASEL_1		},	//MUX_GX_TEMPREG1
};

//========================================================================
CTEVColorCombiner::CTEVColorCombiner(CRender *pRender) :
	CColorCombiner(pRender),
	m_pOGLRender((OGLRender*)pRender),
	m_lastIndex(-1),
	m_dwLastMux0(0), m_dwLastMux1(0)
{
	m_pDecodedMux = new COGLDecodedMux;
	m_pDecodedMux->m_maxConstants = 2; //Only used for setting const textures
//    m_pDecodedMux->m_maxConstants = 0; //Only used for setting const textures
//    m_pDecodedMux->m_maxTextures = 1;  //Only used for setting const textures
#ifdef __GX__
	gGX.GXmultiTex = false;
#endif //__GX__
}

CTEVColorCombiner::~CTEVColorCombiner()
{
    m_vCompiledSettings.clear();
	delete m_pDecodedMux;
	m_pDecodedMux = NULL;
}

bool CTEVColorCombiner::Initialize(void)
{
    m_supportedStages = 16; //If >1, then single stage combines are split
    m_bSupportMultiTexture = true;

//    COGLGraphicsContext *pcontext = (COGLGraphicsContext *)(CGraphicsContext::g_pGraphicsContext);

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
//			GX_LoadTexObj(&pTexture->GXtex, 0); // t = 0 is GX_TEXMAP0 and t = 1 is GX_TEXMAP1
#endif //__GX__
        }
	    else
		{
			m_pOGLRender->BindTexture((COGLTexture*)gTextureManager.GetBlackTexture()->pTexture, 0);
#ifdef _DEBUG
			DebuggerAppendMsg("Check me, texture is NULL");
#endif
		}
		//Set Modulate TEV mode
		GX_SetNumTevStages(1);
		GX_SetNumChans (1);
		GX_SetNumTexGens (1);
		gGX.GXmultiTex = false;
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
		gGX.GXmultiTex = false;
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
    }
    else
    {
		m_pOGLRender->BindTexture((COGLTexture*)gTextureManager.GetBlackTexture()->pTexture, 0);
#ifdef _DEBUG
		DebuggerAppendMsg("Check me, texture is NULL");
#endif
    }

	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	//Set Replace TEV mode
	GX_SetNumTevStages(1);
	GX_SetNumChans (0);
	GX_SetNumTexGens (1);
	gGX.GXmultiTex = false;
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
	gGX.GXmultiTex = false;
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
}

void CTEVColorCombiner::InitCombinerCycle12(void)
{
#if 0 //Simplified Cycle12 Combiner
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

#else //Cycle12 combiner from COGLColorCombinerNvidia::InitCombinerCycle12(void)

	bool combinerIsChanged = false;

#if 0 //def SHOW_DEBUG
	if( m_pDecodedMux->m_dwMux0 == 0x0026a004 && m_pDecodedMux->m_dwMux1 == 0x1f1093fb )
	{
		sprintf(txtbuffer,"\r\nInitCombinerCycle12: detected Mux0 0x%8x, Mux1 0x%8x and converted\r\n", m_pDecodedMux->m_dwMux0, m_pDecodedMux->m_dwMux1);
		DEBUG_print(txtbuffer,DBG_USBGECKO);
		m_pDecodedMux->m_dwMux0 = 0x00FFFFFF; //0, 0, 0, 1
		m_pDecodedMux->m_dwMux1 = 0xFFFF7DBE; //0, 0, 0, 1
	}
#endif //SHOW_DEBUG
	
	if( m_pDecodedMux->m_dwMux0 != m_dwLastMux0 || m_pDecodedMux->m_dwMux1 != m_dwLastMux1  || m_lastIndex < 0 )
	{
		combinerIsChanged = true;
		m_lastIndex = FindCompiledMux();
		if( m_lastIndex < 0 )       // Can not found
		{
			m_lastIndex = ParseDecodedMux();
#ifdef SHOW_DEBUG
//			DisplayTEVMuxString(m_lastIndex);
#endif //SHOW_DEBUG
		}

		m_dwLastMux0 = m_pDecodedMux->m_dwMux0;
		m_dwLastMux1 = m_pDecodedMux->m_dwMux1;
	}

	if( m_bCycleChanged || combinerIsChanged || gRDP.texturesAreReloaded || gRDP.colorsAreReloaded )
	{
#ifdef SHOW_DEBUG
//		sprintf(txtbuffer,"InitCombCyc12: m_bCycCh %d, combIsCh %d, gRDP.texRL %d, gRDP.colRL %d\r\n",
//				m_bCycleChanged, combinerIsChanged, gRDP.texturesAreReloaded, gRDP.colorsAreReloaded);
//		DEBUG_print(txtbuffer,DBG_USBGECKO);
#endif //SHOW_DEBUG
		if( m_bCycleChanged || combinerIsChanged )
		{
			GenerateCombinerSettingConstants(m_lastIndex);
			GenerateCombinerSetting(m_lastIndex);
		}
		else if( gRDP.colorsAreReloaded )
		{
			GenerateCombinerSettingConstants(m_lastIndex);
		}

		m_pOGLRender->SetAllTexelRepeatFlag();

		gRDP.colorsAreReloaded = false;
		gRDP.texturesAreReloaded = false; 
#ifdef SHOW_DEBUG
//		DisplayTEVMuxString(m_lastIndex);
#endif //SHOW_DEBUG
    }

#ifdef SHOW_DEBUG
	//DisplayTEVMuxString(m_lastIndex);
#endif //SHOW_DEBUG
#endif
}

//void COGLColorCombinerNvidia::ParseDecodedMux(NVRegisterCombinerParserType &result) // Compile the decodedMux into NV register combiner setting
int CTEVColorCombiner::ParseDecodedMux()
{
	//TEV specs/limitations:
	//There are 16 TEV stages.
	//Each TEV stage can only have 1 texture, 1 vertex color, and 1 konst color input.
	//Color/Alpha channels share stage inputs.
	//There are 4 (unsigned) konst color slots.
	//There are 3 (signed) registers available.
	//Signed values can only be added between stages.

	//TEV Input Mappings:
	//TEX0 <- MUX_TEXEL0
	//TEX1 <- MUX_TEXEL1
	//TEX2 <- Ztex (?)
	//TexN <- color inputs (?)
	//KREG0 <- MUX_PRIM
	//KREG1 <- MUX_ENV
	//KREG2 <- MUX_LODFRAC
	//KREG3 <- MUX_PRIMLODFRAC
	//REG0 <- MUX_COMBINED
	//REG1 <-
	//REG2 <- backup PRIM/ENV/LODFRAC ??
	//VtxClr0 <- MUX_SHADE, MUX_K5, MUX_UNK
	//VtxClr1 <-

	//Tasks:
	//Get current DecodedMux
	//Fill out TEVCombinerSaveType res
	//return index to saved result

	//TODO: There could be a problem for MUX_COMPLEMENT in the alpha channel when multiplying by MUX_1 because a KONST register must be used for alpha.
	
#define NextAvailable(stage, rgbalpha, arg) \
	while ( (((arg&MUX_MASK)==MUX_TEXEL0 || (arg&MUX_MASK)==MUX_T0_ALPHA) && res.units[stage].tex==1) || \
		    (((arg&MUX_MASK)==MUX_TEXEL1 || (arg&MUX_MASK)==MUX_T1_ALPHA) && res.units[stage].tex==0) ) { \
		res.units[stage].Combs[rgbalpha].argD = MUX_GX_PREV; \
		if ( stage > 0 ) res.units[stage].ops[rgbalpha].clamp = res.units[stage-1].ops[rgbalpha].clamp; \
		stage++; } 

    TEVCombinerSaveType res;
	for (int stage=0; stage<16; stage++)
	{ //These are the default settings
		for (int i=0; i<4; i++)
		{
			res.units[stage].rgbComb.args[i] = CM_IGNORE_BYTE;
			res.units[stage].alphaComb.args[i] = CM_IGNORE_BYTE;
			res.units[stage].rgbIn.args[i] = GX_CC_ZERO;
			res.units[stage].alphaIn.args[i] = GX_CA_ZERO;
		}
		res.units[stage].rgbOp.tevop = GX_TEV_ADD;
		res.units[stage].rgbOp.tevbias = GX_TB_ZERO;
		res.units[stage].rgbOp.tevscale = GX_CS_SCALE_1;
		res.units[stage].rgbOp.clamp = GX_TRUE;
		res.units[stage].rgbOp.tevregid = GX_TEVPREV;
		res.units[stage].alphaOp.tevop = GX_TEV_ADD;
		res.units[stage].alphaOp.tevbias = GX_TB_ZERO;
		res.units[stage].alphaOp.tevscale = GX_CS_SCALE_1;
		res.units[stage].alphaOp.clamp = GX_TRUE;
		res.units[stage].alphaOp.tevregid = GX_TEVPREV;
		res.units[stage].order.texcoord = GX_TEXCOORDNULL;
		res.units[stage].order.texmap = GX_TEXMAP_NULL;
		res.units[stage].order.color = GX_COLORNULL;
		res.units[stage].Kcol = GX_TEV_KCSEL_1;
		res.units[stage].Kalpha = GX_TEV_KASEL_1;
		res.units[stage].tex = -1;
	}
	res.numUnitsUsed[0] = 0;
	res.numUnitsUsed[1] = 0;
	COGLDecodedMux &mux = *(COGLDecodedMux*)m_pDecodedMux;

	for( int rgbalpha = 0; rgbalpha<2; rgbalpha++ ) //
	{
		int stage = 0;
		int tempreg = 0;

		for ( int cycle = 0; cycle<2; cycle++ )
		{
			CombinerFormatType type = m_pDecodedMux->splitType[cycle*2+rgbalpha];
			N64CombinerType &m = m_pDecodedMux->m_n64Combiners[cycle*2+rgbalpha];

			switch( type )
			{
			case CM_FMT_TYPE_NOT_USED:
				res.units[stage].Combs[rgbalpha].argD = MUX_COMBINED;
				stage++;
				break;
			case CM_FMT_TYPE_D:             // = A
				NextAvailable(stage, rgbalpha, m.d);
				if( !(m.d&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argD = m.d;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.d;
				}
				stage++;
				break;
			case CM_FMT_TYPE_A_ADD_D:           // = A+D
				NextAvailable(stage, rgbalpha, m.a);
				if( !(m.a&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argD = m.a;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.a;
				}
				stage++;
				NextAvailable(stage, rgbalpha, m.d);
				if( !(m.d&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argA = m.d;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.d;
				}
				res.units[stage].Combs[rgbalpha].argD = MUX_GX_PREV;
				stage++;
				break;
			case CM_FMT_TYPE_A_SUB_B:           // = A-B
				NextAvailable(stage, rgbalpha, m.a);
				if( !(m.a&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argD = m.a;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.a;
				}
				stage++;
				NextAvailable(stage, rgbalpha, m.b);
				if( !(m.b&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argA = m.b;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.b;
				}
				res.units[stage].Combs[rgbalpha].argD = MUX_GX_PREV;
				res.units[stage].ops[rgbalpha].tevop = GX_TEV_SUB;
				stage++;
				break;
			case CM_FMT_TYPE_A_MOD_C:           // = A*C
				NextAvailable(stage, rgbalpha, m.a);
				if( !(m.a&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argD = m.a;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.a;
				}
				stage++;
				NextAvailable(stage, rgbalpha, m.c);
				if( !(m.c&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argB = m.c;
					res.units[stage].Combs[rgbalpha].argC = MUX_GX_PREV;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_GX_PREV;
					res.units[stage].Combs[rgbalpha].argC = m.c;
				}
				stage++;
				break;
			case CM_FMT_TYPE_A_MOD_C_ADD_D: // = A*C+D
				NextAvailable(stage, rgbalpha, m.a);
				if( !(m.a&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argD = m.a;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.a;
				}
				stage++;
				NextAvailable(stage, rgbalpha, m.c);
				if( !(m.c&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argB = m.c;
					res.units[stage].Combs[rgbalpha].argC = MUX_GX_PREV;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_GX_PREV;
					res.units[stage].Combs[rgbalpha].argC = m.c;
				}
				res.units[stage].ops[rgbalpha].clamp = GX_FALSE;
				stage++;
				NextAvailable(stage, rgbalpha, m.d);
				if( !(m.d&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argA = m.d;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.d;
				}
				res.units[stage].Combs[rgbalpha].argD = MUX_GX_PREV;
				stage++;
				break;
			case CM_FMT_TYPE_A_LERP_B_C:        // = (A-B)*C+B = A*C - B*C + B = A*C + B*(1-C)
				NextAvailable(stage, rgbalpha, m.a);
				if( !(m.a&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argD = m.a;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.a;
				}
				res.units[stage].ops[rgbalpha].tevregid = GX_TEVREG1 + tempreg;
				stage++;
				NextAvailable(stage, rgbalpha, m.b);
				if( !(m.b&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argD = m.b;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.b;
				}
				stage++;
				NextAvailable(stage, rgbalpha, m.c);
				if( !(m.c&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_GX_PREV;
					res.units[stage].Combs[rgbalpha].argB = MUX_GX_TEMPREG0 + tempreg;
					res.units[stage].Combs[rgbalpha].argC = m.c;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_GX_TEMPREG0 + tempreg;
					res.units[stage].Combs[rgbalpha].argB = MUX_GX_PREV;
					res.units[stage].Combs[rgbalpha].argC = m.c;
				}
				stage++;
				tempreg++;
				break;
			case CM_FMT_TYPE_A_SUB_B_ADD_D: // = A-B+D
				NextAvailable(stage, rgbalpha, m.a);
				if( !(m.a&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argD = m.a;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.a;
				}
				stage++;
				NextAvailable(stage, rgbalpha, m.d);
				if( !(m.d&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argA = m.d;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.d;
				}
				res.units[stage].Combs[rgbalpha].argD = MUX_GX_PREV;
				res.units[stage].ops[rgbalpha].clamp = GX_FALSE;
				stage++;
				NextAvailable(stage, rgbalpha, m.b);
				if( !(m.b&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argA = m.b;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.b;
				}
				res.units[stage].Combs[rgbalpha].argD = MUX_GX_PREV;
				res.units[stage].ops[rgbalpha].tevop = GX_TEV_SUB;
				stage++;
				break;
			case CM_FMT_TYPE_A_SUB_B_MOD_C: // = (A-B)*C = A*C - B*C
				NextAvailable(stage, rgbalpha, m.c);
				if( !(m.c&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argD = m.c;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.c;
				}
				res.units[stage].ops[rgbalpha].tevregid = GX_TEVREG1 + tempreg;
				stage++;
				NextAvailable(stage, rgbalpha, m.a);
				if( !(m.a&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argB = m.a;
					res.units[stage].Combs[rgbalpha].argC = MUX_GX_TEMPREG0 + tempreg;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_GX_TEMPREG0 + tempreg;
					res.units[stage].Combs[rgbalpha].argC = m.a;
				}
				res.units[stage].ops[rgbalpha].clamp = GX_FALSE;
				stage++;
				NextAvailable(stage, rgbalpha, m.b);
				if( !(m.b&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argB = m.b;
					res.units[stage].Combs[rgbalpha].argC = MUX_GX_TEMPREG0 + tempreg;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_GX_TEMPREG0 + tempreg;
					res.units[stage].Combs[rgbalpha].argC = m.b;
				}
				res.units[stage].Combs[rgbalpha].argD = MUX_GX_PREV;
				res.units[stage].ops[rgbalpha].tevop = GX_TEV_SUB;
				stage++;
				tempreg++;
				break;
			case CM_FMT_TYPE_A_B_C_D:           // = (A-B)*C+D = A*C - B*C + D
			default:
				NextAvailable(stage, rgbalpha, m.c);
				if( !(m.c&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argD = m.c;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.c;
				}
				res.units[stage].ops[rgbalpha].tevregid = GX_TEVREG1 + tempreg;
				stage++;
				NextAvailable(stage, rgbalpha, m.a);
				if( !(m.a&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argB = m.a;
					res.units[stage].Combs[rgbalpha].argC = MUX_GX_TEMPREG0 + tempreg;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_GX_TEMPREG0 + tempreg;
					res.units[stage].Combs[rgbalpha].argC = m.a;
				}
				res.units[stage].ops[rgbalpha].clamp = GX_FALSE;
				stage++;
				NextAvailable(stage, rgbalpha, m.b);
				if( !(m.b&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argB = m.b;
					res.units[stage].Combs[rgbalpha].argC = MUX_GX_TEMPREG0 + tempreg;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_GX_TEMPREG0 + tempreg;
					res.units[stage].Combs[rgbalpha].argC = m.b;
				}
				res.units[stage].Combs[rgbalpha].argD = MUX_GX_PREV;
				res.units[stage].ops[rgbalpha].tevop = GX_TEV_SUB;
				res.units[stage].ops[rgbalpha].clamp = GX_FALSE;
				stage++;
				NextAvailable(stage, rgbalpha, m.d);
				if( !(m.d&MUX_COMPLEMENT) )
				{
					res.units[stage].Combs[rgbalpha].argA = m.d;
				}
				else
				{
					res.units[stage].Combs[rgbalpha].argA = MUX_1;
					res.units[stage].Combs[rgbalpha].argC = m.d;
				}
				res.units[stage].Combs[rgbalpha].argD = MUX_GX_PREV;
				stage++;
				tempreg++;
				break;
			}

			if (cycle == 0)
				res.units[stage-1].ops[rgbalpha].tevregid = GX_TEVREG0; //Save MUX_COMBINED

		}
		//Update TEV order
		res.numUnitsUsed[rgbalpha] = stage;
		for (int i = 0; i<res.numUnitsUsed[rgbalpha]; i++)
		{
			for (int j = 0; j<4; j++)
			{
				if (((res.units[i].Combs[rgbalpha].args[j]&MUX_MASK) == MUX_TEXEL0) || 
					((res.units[i].Combs[rgbalpha].args[j]&MUX_MASK) == MUX_T0_ALPHA) ) 
				{
					res.units[i].tex = 0;
					res.units[i].order.texcoord = GX_TEXCOORD0;
					res.units[i].order.texmap = GX_TEXMAP0;
				}
				if (((res.units[i].Combs[rgbalpha].args[j]&MUX_MASK) == MUX_TEXEL1) || 
					((res.units[i].Combs[rgbalpha].args[j]&MUX_MASK) == MUX_T1_ALPHA) ) 
				{
					res.units[i].tex = 1;
					res.units[i].order.texcoord = GX_TEXCOORD1;
					res.units[i].order.texmap = GX_TEXMAP1;
				}
				if (((res.units[i].Combs[rgbalpha].args[j]&MUX_MASK) == MUX_SHADE) || 
					((res.units[i].Combs[rgbalpha].args[j]&MUX_MASK) == MUX_SHADE_ALPHA) || 
					((res.units[i].Combs[rgbalpha].args[j]&MUX_MASK) == MUX_K5) || 
					((res.units[i].Combs[rgbalpha].args[j]&MUX_MASK) == MUX_UNK) ) 
				{
					res.units[i].order.color = GX_COLOR0A0;
				}
			}
		}
	}

	//Calculate number of TEV stages
	res.numOfUnits = max(res.numUnitsUsed[0], res.numUnitsUsed[1]);

	//Fill in unused TEV stages
	for (int i = 0; i<2; i++)
		for (int j = res.numUnitsUsed[i]; j<res.numOfUnits; j++)
			res.units[j].Combs[i].argD = MUX_GX_PREV;

	res.primIsUsed = mux.isUsed(MUX_PRIM);
	res.envIsUsed = mux.isUsed(MUX_ENV);
	res.lodFracIsUsed = mux.isUsed(MUX_LODFRAC);
	res.primLodFracIsUsed = mux.isUsed(MUX_PRIMLODFRAC);

	//Save Parsed Result
	res.dwMux0 = m_pDecodedMux->m_dwMux0;
	res.dwMux1 = m_pDecodedMux->m_dwMux1;

	for( int n=0; n<res.numOfUnits; n++ )
	{
		for( int i=0; i<4; i++ )
		{
			if( res.units[n].rgbComb.args[i] != CM_IGNORE_BYTE )
			{
				if (res.units[n].rgbComb.args[i] & MUX_ALPHAREPLICATE)
					res.units[n].rgbIn.args[i] = RGBArgsMap[(res.units[n].rgbComb.args[i]&MUX_MASK)].CinA;
				else
					res.units[n].rgbIn.args[i] = RGBArgsMap[(res.units[n].rgbComb.args[i]&MUX_MASK)].Cin;
				if (res.units[n].rgbIn.args[i] == GX_CC_KONST) 
				{
					if (res.units[n].rgbComb.args[i] & MUX_ALPHAREPLICATE)
						res.units[n].Kcol = RGBArgsMap[(res.units[n].rgbComb.args[i]&MUX_MASK)].KCselA;
					else
						res.units[n].Kcol = RGBArgsMap[(res.units[n].rgbComb.args[i]&MUX_MASK)].KCsel;
				}
			}
			if( res.units[n].alphaComb.args[i] != CM_IGNORE_BYTE )
			{
				res.units[n].alphaIn.args[i] = RGBArgsMap[(res.units[n].alphaComb.args[i]&MUX_MASK)].Ain;
				if (res.units[n].alphaIn.args[i] == GX_CA_KONST) 
					res.units[n].Kalpha = RGBArgsMap[(res.units[n].alphaComb.args[i]&MUX_MASK)].KAsel;
			}
		}
	}

	m_vCompiledSettings.push_back(res);
	m_lastIndex = m_vCompiledSettings.size()-1;

	return m_lastIndex;
}

void CTEVColorCombiner::GenerateCombinerSetting(int index)
{
	TEVCombinerSaveType &res = m_vCompiledSettings[index];

	COGLTexture* pTexture = NULL;
	COGLTexture* pTexture1 = NULL;
	int numTex = 0;

	if( m_bTex0Enabled || m_bTex1Enabled || gRDP.otherMode.cycle_type  == CYCLE_TYPE_COPY )
	{
		if( m_bTex0Enabled || gRDP.otherMode.cycle_type  == CYCLE_TYPE_COPY )
		{
			pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
			if( pTexture )  
				m_pOGLRender->BindTexture(pTexture, 0);
			else
				m_pOGLRender->BindTexture((COGLTexture*)gTextureManager.GetBlackTexture()->pTexture, 0);
			numTex = 1;
		}
		if( m_bTex1Enabled )
		{
			pTexture1 = g_textures[(gRSP.curTile+1)&7].m_pCOGLTexture;
			if( pTexture1 ) 
				m_pOGLRender->BindTexture(pTexture1, 1);
			else
				m_pOGLRender->BindTexture((COGLTexture*)gTextureManager.GetBlackTexture()->pTexture, 1);
			numTex = 2;
		}
	}

#if 0 //def SHOW_DEBUG
//	if( m_pDecodedMux->m_dwMux0 == 0x0026a004 && m_pDecodedMux->m_dwMux1 == 0x1f1093fb )
	if( m_pDecodedMux->m_dwMux0 == 0x00272c04 && m_pDecodedMux->m_dwMux1 == 0x1f1093ff )
	{
		sprintf(txtbuffer,"\r\nGenCombSetng: detected Mux0 0x%8x, Mux1 0x%8x, pTex0 0x%8x, pTex1 0x%8x\r\n", m_pDecodedMux->m_dwMux0, m_pDecodedMux->m_dwMux1, pTexture, pTexture1);
		DEBUG_print(txtbuffer,DBG_USBGECKO);
	}
#endif //SHOW_DEBUG
	
	GX_SetNumTevStages(res.numOfUnits);
//	GX_SetNumTevStages(res.numOfUnits+1);
	GX_SetNumChans (1);
	GX_SetNumTexGens (numTex);
//	GX_SetNumTexGens (numTex+1);
	gGX.GXmultiTex = (numTex == 2) ? true : false;

	for( int i=0; i<res.numOfUnits; i++ )
	{
		GX_SetTevOrder (GX_TEVSTAGE0+i, res.units[i].order.texcoord, res.units[i].order.texmap, res.units[i].order.color);
#if 0 //for debugging
		if( m_pDecodedMux->m_dwMux0 == 0x00267e60 && m_pDecodedMux->m_dwMux1 == 0x350cf37f && i==res.numOfUnits-1) //OOT Intro Index1
		{
//			GX_SetTevOrder (GX_TEVSTAGE0+i, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
//			GX_SetTevColorIn(GX_TEVSTAGE0+i, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ONE);
//			GX_SetTevAlphaIn(GX_TEVSTAGE0+i, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA); //GX_CA_RASA, GX_CA_KONST
//			GX_SetTevKAlphaSel(GX_TEVSTAGE0+i, GX_TEV_KASEL_K0_A);
		}
		else
		{
//			GX_SetTevOrder (GX_TEVSTAGE0+i, res.units[i].order.texcoord, res.units[i].order.texmap, res.units[i].order.color);
//			GX_SetTevColorIn(GX_TEVSTAGE0+i, res.units[i].rgbIn.a, res.units[i].rgbIn.b, res.units[i].rgbIn.c, res.units[i].rgbIn.d);
//			GX_SetTevAlphaIn(GX_TEVSTAGE0+i, res.units[i].alphaIn.a, res.units[i].alphaIn.b, res.units[i].alphaIn.c, res.units[i].alphaIn.d);
//			GX_SetTevKAlphaSel(GX_TEVSTAGE0+i, res.units[i].Kalpha);
		}
#endif
		GX_SetTevColorIn(GX_TEVSTAGE0+i, res.units[i].rgbIn.a, res.units[i].rgbIn.b, res.units[i].rgbIn.c, res.units[i].rgbIn.d);
		GX_SetTevAlphaIn(GX_TEVSTAGE0+i, res.units[i].alphaIn.a, res.units[i].alphaIn.b, res.units[i].alphaIn.c, res.units[i].alphaIn.d);
		GX_SetTevColorOp(GX_TEVSTAGE0+i, res.units[i].rgbOp.tevop, res.units[i].rgbOp.tevbias, 
			res.units[i].rgbOp.tevscale, res.units[i].rgbOp.clamp, res.units[i].rgbOp.tevregid);
		GX_SetTevAlphaOp(GX_TEVSTAGE0+i, res.units[i].alphaOp.tevop, res.units[i].alphaOp.tevbias, 
			res.units[i].alphaOp.tevscale, res.units[i].alphaOp.clamp, res.units[i].alphaOp.tevregid);
		GX_SetTevKColorSel(GX_TEVSTAGE0+i, res.units[i].Kcol);
		GX_SetTevKAlphaSel(GX_TEVSTAGE0+i, res.units[i].Kalpha);
	}

	if( 0 ) //m_pDecodedMux->m_dwMux0 == 0x00267e60 && m_pDecodedMux->m_dwMux1 == 0x350cf37f ) //OOT Intro Index1
	{
//		GX_SetTevColorIn(GX_TEVSTAGE0+res.numOfUnits-1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C2);
		GX_SetTevColorIn(GX_TEVSTAGE0+res.numOfUnits-1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ONE);
	}

	//Configure final stage for Ztexture
//	GX_SetTevOp ((u8) res.numOfUnits, GX_PASSCLR);
//	GX_SetTevOrder ((u8) res.numOfUnits, GX_TEXMAP2, GX_TEXMAP2, GX_COLORNULL);

/*	GX_SetTevOp(GX_TEVSTAGE0,GX_REPLACE);//GXTevMode);
//	if( pTexture )
	//	GX_LoadTexObj(&pTexture->GXtex, 0); // t = 0 is GX_TEXMAP0 and t = 1 is GX_TEXMAP1

	GX_SetNumTevStages(1);
	GX_SetNumChans (1);
	GX_SetNumTexGens (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
*/
#ifndef __GX__
    for( int i=0; i<res.numOfUnits; i++ )
    {
        glActiveTexture(GL_TEXTURE0_ARB+i);
        m_pOGLRender->EnableTexUnit(i,TRUE);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
        ApplyFor1Unit(res.units[i]);
    }

    if( res.numOfUnits < m_maxTexUnits )
    {
        for( int i=res.numOfUnits; i<m_maxTexUnits; i++ )
        {
            glActiveTexture(GL_TEXTURE0_ARB+i);
            m_pOGLRender->DisBindTexture(0, i);
            m_pOGLRender->EnableTexUnit(i,FALSE);
        }
    }
#endif //!__GX__
}

void CTEVColorCombiner::GenerateCombinerSettingConstants(int index)
{
	TEVCombinerSaveType &res = m_vCompiledSettings[index];

	//float *fv;
	//float tempf[4];
	GXColor GXcol;
	uint32 dwColor;

	if( res.primIsUsed )
	{
		//fv = GetPrimitiveColorfv(); // CONSTANT COLOR
		dwColor = GetPrimitiveColor();
		GXcol.r = (u8) ((dwColor>>16)&0xFF);
		GXcol.g = (u8) ((dwColor>>8)&0xFF);
		GXcol.b = (u8) (dwColor&0xFF);
		GXcol.a = (u8) (dwColor>>24);
		GX_SetTevKColor(GX_KCOLOR0, GXcol);
#if 0//def SHOW_DEBUG
		sprintf(txtbuffer,"\r\nGenCombConst: prim (%d,%d,%d,%d)\r\n",GXcol.r,GXcol.g,GXcol.b,GXcol.a);
		DEBUG_print(txtbuffer,DBG_USBGECKO);
#endif
	}
	if( res.envIsUsed )
	{
		//fv = GetEnvColorfv();   // CONSTANT COLOR
		dwColor = GetEnvColor();
//		dwColor = 0xFFFFFFFF; //Testing..
		GXcol.r = (u8) ((dwColor>>16)&0xFF);
		GXcol.g = (u8) ((dwColor>>8)&0xFF);
		GXcol.b = (u8) (dwColor&0xFF);
		GXcol.a = (u8) (dwColor>>24);
		GX_SetTevKColor(GX_KCOLOR1, GXcol);
#if 0//def SHOW_DEBUG
		sprintf(txtbuffer,"\r\nGenCombConst: env (%d,%d,%d,%d)\r\n",GXcol.r,GXcol.g,GXcol.b,GXcol.a);
		DEBUG_print(txtbuffer,DBG_USBGECKO);
#endif
	}
	if( res.lodFracIsUsed )
	{
		//float frac = gRDP.LODFrac / 255.0f;
		//tempf[0] = tempf[1] = tempf[2] = tempf[3] = frac;
		//fv = &tempf[0];
		GXcol.r = GXcol.g = GXcol.b = GXcol.a = (u8) (gRDP.LODFrac&0xFF);
		GX_SetTevKColor(GX_KCOLOR2, GXcol);
#if 0//def SHOW_DEBUG
		sprintf(txtbuffer,"\r\nGenCombConst: lodfrac (%d,%d,%d,%d)\r\n",GXcol.r,GXcol.g,GXcol.b,GXcol.a);
		DEBUG_print(txtbuffer,DBG_USBGECKO);
#endif
    }
	if( res.primLodFracIsUsed )
	{
		//float frac2 = gRDP.primLODFrac / 255.0f;
		GXcol.r = GXcol.g = GXcol.b = GXcol.a = (u8) (gRDP.primLODFrac&0xFF);
		GX_SetTevKColor(GX_KCOLOR3, GXcol);
#if 0//def SHOW_DEBUG
		sprintf(txtbuffer,"\r\nGenCombConst: primlodfrac (%d,%d,%d,%d)\r\n",GXcol.r,GXcol.g,GXcol.b,GXcol.a);
		DEBUG_print(txtbuffer,DBG_USBGECKO);
#endif
    }
}

int CTEVColorCombiner::FindCompiledMux()
{
#ifdef _DEBUG
    if( debuggerDropCombiners )
    {
        m_vCompiledSettings.clear();
        //m_dwLastMux0 = m_dwLastMux1 = 0;
        debuggerDropCombiners = false;
    }
#endif
    for( uint32 i=0; i<m_vCompiledSettings.size(); i++ )
    {
        if( m_vCompiledSettings[i].dwMux0 == m_pDecodedMux->m_dwMux0 && m_vCompiledSettings[i].dwMux1 == m_pDecodedMux->m_dwMux1 )
            return (int)i;
    }

    return -1;
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

extern const char* muxTypeStrs[];
extern const char *translatedCombTypes[];

#ifdef SHOW_DEBUG
const char * tevColorStrs[] =
{
	"GX_CC_CPREV",
	"GX_CC_APREV",
	"GX_CC_C0",
	"GX_CC_A0",
	"GX_CC_C1",
	"GX_CC_A1",
	"GX_CC_C2",
	"GX_CC_A2",
	"GX_CC_TEXC",
	"GX_CC_TEXA",
	"GX_CC_RASC",
	"GX_CC_RASA",
	"GX_CC_ONE",
	"GX_CC_HALF",
	"GX_CC_KONST",
	"GX_CC_ZERO",
};

const char * tevAlphaStrs[] =
{
	"GX_CA_APREV",
	"GX_CA_A0",
	"GX_CA_A1",
	"GX_CA_A2",
	"GX_CA_TEXA",
	"GX_CA_RASA",
	"GX_CA_KONST",
	"GX_CA_ZERO",
};
#endif

void CTEVColorCombiner::DisplayTEVMuxString(int index)
{
#ifdef SHOW_DEBUG
	//int debug_line = DBG_RICE;

	//sprintf(txtbuffer,"DispTEVMuxStr: index %d, Mux0 %8x, Mux1 %8x\r\n", index, m_dwLastMux0, m_dwLastMux1);
	sprintf(txtbuffer,"\r\nDispTEVMuxStr: index %d, Mux0 0x%8x, Mux1 0x%8x\r\n", index, m_pDecodedMux->m_dwMux0, m_pDecodedMux->m_dwMux1);
	DEBUG_print(txtbuffer,DBG_USBGECKO);

	for( int rgbalpha = 0; rgbalpha<2; rgbalpha++ ) 
	{
		for ( int cycle = 0; cycle<2; cycle++ )
		{
			CombinerFormatType type = m_pDecodedMux->splitType[cycle*2+rgbalpha];
			N64CombinerType &m = m_pDecodedMux->m_n64Combiners[cycle*2+rgbalpha];

			sprintf(txtbuffer,"RGBA %d Cycle %d Type %s:\r\n", rgbalpha, cycle, muxTypeStrs[type]); 
			DEBUG_print(txtbuffer,DBG_USBGECKO);
			sprintf(txtbuffer," abcd = %s%s(0x%2x), %s%s(0x%2x), %s%s(0x%2x), %s%s(0x%2x)\r\n", 
				translatedCombTypes[m.a&MUX_MASK], (m.a&(~MUX_MASK))?"*":" ", m.a,
				translatedCombTypes[m.b&MUX_MASK], (m.b&(~MUX_MASK))?"*":" ", m.b,
				translatedCombTypes[m.c&MUX_MASK], (m.c&(~MUX_MASK))?"*":" ", m.c,
				translatedCombTypes[m.d&MUX_MASK], (m.d&(~MUX_MASK))?"*":" ", m.d);
			DEBUG_print(txtbuffer,DBG_USBGECKO);
		}
	}

	TEVCombinerSaveType &res = m_vCompiledSettings[index];

	COGLTexture* pTexture = NULL;
	COGLTexture* pTexture1 = NULL;
	int numTex = 0;

	if( m_bTex0Enabled || m_bTex1Enabled || gRDP.otherMode.cycle_type  == CYCLE_TYPE_COPY )
	{
		if( m_bTex0Enabled || gRDP.otherMode.cycle_type  == CYCLE_TYPE_COPY )
		{
			pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
			if( 0 )//pTexture )  
			{
				m_pOGLRender->BindTexture(pTexture, 0);
//				GX_LoadTexObj(&pTexture->GXtex, 0); // t = 0 is GX_TEXMAP0 and t = 1 is GX_TEXMAP1
			}
//			numTex++;
			numTex = 1;
		}
		if( m_bTex1Enabled )
		{
			pTexture1 = g_textures[(gRSP.curTile+1)&7].m_pCOGLTexture;
			if( 0 )//pTexture1 ) 
			{
				m_pOGLRender->BindTexture(pTexture1, 1);
//				GX_LoadTexObj(&pTexture1->GXtex, 1); // t = 0 is GX_TEXMAP0 and t = 1 is GX_TEXMAP1
			}
//			numTex++;
			numTex = 2;
		}
	}

	sprintf(txtbuffer,"TEV Stages: %d, Texs: %d (%8x,%8x), prim %d, env %d, lod %d, primlod %d\r\n", res.numOfUnits, numTex, (u32)pTexture, (u32)pTexture1, res.primIsUsed, res.envIsUsed, res.lodFracIsUsed, res.primLodFracIsUsed);
	DEBUG_print(txtbuffer,DBG_USBGECKO);

	for( int i=0; i<res.numOfUnits; i++ )
	{
		sprintf(txtbuffer,"Stage%d: texmap %d, col %d, KCsel %d, KAsel %d\r\n",
			i, res.units[i].order.texmap, res.units[i].order.color, res.units[i].Kcol, res.units[i].Kalpha);
		DEBUG_print(txtbuffer,DBG_USBGECKO);
		sprintf(txtbuffer," Cin: %s, %s, %s, %s, Op %d, Clamp %d, Reg %d\r\n",
			tevColorStrs[res.units[i].rgbIn.a], tevColorStrs[res.units[i].rgbIn.b], tevColorStrs[res.units[i].rgbIn.c], tevColorStrs[res.units[i].rgbIn.d],
			res.units[i].rgbOp.tevop, res.units[i].rgbOp.clamp, res.units[i].rgbOp.tevregid);
		DEBUG_print(txtbuffer,DBG_USBGECKO);
		sprintf(txtbuffer," Ain: %s, %s, %s, %s, Op %d, Clamp %d, Reg %d\r\n", 
			tevAlphaStrs[res.units[i].alphaIn.a], tevAlphaStrs[res.units[i].alphaIn.b], tevAlphaStrs[res.units[i].alphaIn.c], tevAlphaStrs[res.units[i].alphaIn.d],
			res.units[i].alphaOp.tevop, res.units[i].alphaOp.clamp, res.units[i].alphaOp.tevregid);
		DEBUG_print(txtbuffer,DBG_USBGECKO);
	}
#endif //SHOW_DEBUG
}

#ifdef _DEBUG
extern const char *translatedCombTypes[];
void CTEVColorCombiner::DisplaySimpleMuxString(void)
{
    TRACE0("\nSimplified Mux\n");
    m_pDecodedMux->DisplaySimpliedMuxString("Used");
}
#endif

