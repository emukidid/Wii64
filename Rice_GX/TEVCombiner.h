/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Wii64 - TEVCombiner.h                                                 *
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

#ifndef _TEV_COMBINER_H_
#define _TEV_COMBINER_H_

#include "Combiner.h"

typedef union 
{
	struct {
		uint8   argA;
		uint8   argB;
		uint8   argC;
		uint8   argD;
	};
	uint8 args[4];
} TEVCombType;

typedef union 
{
	struct {
		u8   a;
		u8   b;
		u8   c;
		u8   d;
	};
	u8 args[4];
} TEVInType;

typedef union 
{
	struct {
		u8   tevop;
		u8   tevbias;
		u8   tevscale;
		u8   clamp;
		u8   tevregid;
	};
	u8 args[5];
} TEVOpType;

typedef struct {
	u8	texcoord;
	u32	texmap;
	u8	color;
} TEVOrderType;

typedef struct {
	union {
		struct {
			uint8   rgbArgA;
			uint8   rgbArgB;
			uint8   rgbArgC;
			uint8   rgbArgD;
			uint8   alphaArgA;
			uint8   alphaArgB;
			uint8   alphaArgC;
			uint8   alphaArgD;
		};
		struct {
			TEVCombType rgbComb;
			TEVCombType alphaComb;
		};
		TEVCombType Combs[2];
	};

	union { //GX_SetTevColorIn
		struct {
			TEVInType	rgbIn;
			TEVInType	alphaIn;
		};
		TEVInType inputs[2];
	};

	union { //GX_SetTevColorOp
		struct {
			TEVOpType	rgbOp;
			TEVOpType	alphaOp;
		};
		TEVOpType ops[2];
	};

	union { //GX_SetTevKColorSel
		struct {
			u8	Kcol;
			u8	Kalpha;
		};
		u8 Ksel[2];
	};

	TEVOrderType order; //GX_SetTevOrder

	int     tex;
//	bool    textureIsUsed;
    //float scale;      //Will not be used
} TEVCombinerType;


typedef struct {
	uint32  dwMux0;
	uint32  dwMux1;
	TEVCombinerType units[16];
	int     numUnitsUsed[2];
	int     numOfUnits;

	bool    primIsUsed;
	bool    envIsUsed;
	bool    lodFracIsUsed;
	bool    primLodFracIsUsed;
} TEVCombinerSaveType;


class OGLRender;

class CTEVColorCombiner : public CColorCombiner
{
public:
	bool Initialize(void);
	void InitCombinerBlenderForSimpleTextureDraw(uint32 tile=0);
protected:
	friend class OGLDeviceBuilder;

	void DisableCombiner(void);
	void InitCombinerCycleCopy(void);
	void InitCombinerCycleFill(void);
	void InitCombinerCycle12(void);
	void GenerateCombinerSetting(int index);
	void GenerateCombinerSettingConstants(int index);
	int ParseDecodedMux();

	CTEVColorCombiner(CRender *pRender);
	~CTEVColorCombiner();
	OGLRender *m_pOGLRender;

	int m_lastIndex;	//Index of saved TEV setting
	uint32 m_dwLastMux0;
	uint32 m_dwLastMux1;

	void DisplayTEVMuxString(int index);

#ifdef _DEBUG
	void DisplaySimpleMuxString(void);
#endif

	std::vector<TEVCombinerSaveType>	m_vCompiledSettings;

private:
	int FindCompiledMux();

};

#endif
