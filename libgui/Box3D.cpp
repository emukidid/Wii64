/**
 * Wii64 - Box3D.cpp
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

#include "Box3D.h"
#include "GuiResources.h"
#include "Image.h"
#include "GraphicsGX.h"
#include <math.h>
#include "ogc/lwp_watchdog.h"

namespace menu {

#define BOX3D_X1 30
#define BOX3D_Y1 22 //88/4
#define BOX3D_Z1 4 //16/4
//#define BOX3D_Y1 21
//#define BOX3D_Z1 6

// 'Box3D' vertex data
s8 Box3D_verts[] ATTRIBUTE_ALIGN (32) =
{ // x y z
  -BOX3D_X1, -BOX3D_Y1,  BOX3D_Z1,		// 0 (Front side, Z = +BOX3D_Z1)
   BOX3D_X1, -BOX3D_Y1,  BOX3D_Z1,		// 1	3  2
   BOX3D_X1,  BOX3D_Y1,  BOX3D_Z1,		// 2	0  1
  -BOX3D_X1,  BOX3D_Y1,  BOX3D_Z1,		// 3 (Back side, Z = -BOX3D_Z1)
  -BOX3D_X1, -BOX3D_Y1, -BOX3D_Z1,		// 4	6  7
   BOX3D_X1, -BOX3D_Y1, -BOX3D_Z1,		// 5	5  4
   BOX3D_X1,  BOX3D_Y1, -BOX3D_Z1,		// 6
  -BOX3D_X1,  BOX3D_Y1, -BOX3D_Z1,		// 7
};
//		   7   top    6
//		3          2
//		   4 front    5
//      0          1

// 'Box3D' normal data
s8 Box3D_norms[] ATTRIBUTE_ALIGN (32) =
{ // x y z
  0, 0,  BOX3D_Z1,		// 0 Front side
  0, 0, -BOX3D_Z1,		// 1 Back side
  0,  BOX3D_Y1, 0,		// 2 Top side
  0, -BOX3D_Y1, 0,		// 3 Bottom side
  -BOX3D_X1, 0, 0,		// 4 Left side
   BOX3D_X1, 0, 0,		// 5 Right side
};

// Box3D color data
u8 Box3D_colors[] ATTRIBUTE_ALIGN (32) =
{ // r, g, b, a
	255, 255, 255, 255,		// 0 white
	128, 128, 128, 200,		// 1 gray, semitransparent
};

// Box3D tex coord data
u8 Box3D_texcoords[] ATTRIBUTE_ALIGN (32) =
{ // s, t
	0x00, 0x00,	// 0 
	0x01, 0x00,	// 1 
	0x01, 0x01,	// 2 
	0x00, 0x01,	// 3 
};

#define LOGO_MODE_MAX 3

Box3D::Box3D()
		: x(0),
		  y(0),
		  size(2),
		  rotateAuto(0),
		  rotateX(0),
		  rotateY(0),
		  enableRotate(true),
		  transparency(255),
		  activeBoxartFrontImage(0),
		  activeBoxartSpineImage(0),
		  activeBoxartBackImage(0),
		  customBoxartFrontImage(0),
		  customBoxartSpineImage(0),
		  customBoxartBackImage(0)
{
	setVisible(false);
//	srand ( gettick() );
//	logoMode = rand() % LOGO_MODE_MAX;
}

Box3D::~Box3D()
{
	if(customBoxartFrontImage) 	delete customBoxartFrontImage;
	if(customBoxartSpineImage) 	delete customBoxartSpineImage;
	if(customBoxartBackImage) 	delete customBoxartBackImage;
}

void Box3D::setLocation(float newX, float newY, float newZ)
{
	x = newX;
	y = newY;
	z = newZ;
}

void Box3D::setEnableRotate(bool enable)
{
	enableRotate = enable;
}

void Box3D::setSize(float newSize)
{
	size = newSize;
}

void Box3D::setMode(int mode)
{
	logoMode = mode;
}

void Box3D::setTexture(u8* texture)
{
	if(customBoxartFrontImage)	{ delete customBoxartFrontImage; customBoxartFrontImage = NULL; }
	if(customBoxartSpineImage) 	{ delete customBoxartSpineImage; customBoxartSpineImage = NULL; }
	if(customBoxartBackImage) 	{ delete customBoxartBackImage;  customBoxartBackImage = NULL;  }
	
	if(texture)
	{
		customBoxartFrontImage = new Image(texture, 
									BOXART_TEX_WD, BOXART_TEX_FRONT_HT, BOXART_TEX_FMT, GX_CLAMP, GX_CLAMP, GX_FALSE);
		customBoxartSpineImage = new Image(texture+BOXART_TEX_FRONT_SIZE, 
									BOXART_TEX_WD, BOXART_TEX_SPINE_HT, BOXART_TEX_FMT, GX_CLAMP, GX_CLAMP, GX_FALSE);
		customBoxartBackImage = new Image(texture+BOXART_TEX_FRONT_SIZE+BOXART_TEX_SPINE_SIZE,
									BOXART_TEX_WD, BOXART_TEX_FRONT_HT, BOXART_TEX_FMT, GX_CLAMP, GX_CLAMP, GX_FALSE);
		activeBoxartFrontImage = customBoxartFrontImage;
		activeBoxartSpineImage = customBoxartSpineImage;
		activeBoxartBackImage  = customBoxartBackImage;
	} 
	else
	{
		activeBoxartFrontImage = Resources::getInstance().getImage(Resources::IMAGE_BOXART_FRONT);
		activeBoxartSpineImage = Resources::getInstance().getImage(Resources::IMAGE_BOXART_SPINE);
		activeBoxartBackImage  = Resources::getInstance().getImage(Resources::IMAGE_BOXART_BACK);
	}
	GX_InvalidateTexAll();
}

void Box3D::setTransparency(u8 color)
{
	transparency = color;
}

void Box3D::updateTime(float deltaTime)
{
	//Overload in Component class
	//Add interpolator class & update here?
}

void Box3D::drawComponent(Graphics& gfx)
{
	Mtx v, m, mv, mvi, tmp;            // view, model, modelview, modelview inverse, and perspective matrices
	guVector cam = { 0.0F, 0.0F, 0.0F }, 
		up = {0.0F, 1.0F, 0.0F}, 
		look = {0.0F, 0.0F, -1.0F},
		axisX = {1.0F, 0.0F, 0.0F},
		axisY = {0.0F, 1.0F, 0.0F};
	s8 stickX,stickY;
	u8 selColor = 0;

	guLookAt (v, &cam, &up, &look);
//	rotateAuto++;

	u16 pad0 = PAD_ButtonsDown(0);
	if (pad0 & PAD_TRIGGER_Z) logoMode = (logoMode+1) % 3;

	//libOGC was changed such that sticks are now clamped and don't have to be by us
	stickX = PAD_SubStickX(0);
	stickY = PAD_SubStickY(0);
//	if(stickX > 18 || stickX < -18) rotateX += stickX/32;
//	if(stickY > 18 || stickY < -18) rotateY += stickY/32;
	if(enableRotate)
	{
		rotateX += stickX/32;
		rotateY += stickY/32;
	}

	if (!isVisible())
		return;

	// move the logo out in front of us and rotate it
	guMtxIdentity (m);
	{
//		guMtxRotAxisDeg (tmp, &axisX, 25);			//change to isometric view
//		guMtxConcat (m, tmp, m);
		guMtxRotAxisDeg (tmp, &axisX, 180);			//flip rightside-up
		guMtxConcat (m, tmp, m);
		if(enableRotate)
		{
			guMtxRotAxisDeg (tmp, &axisX, -rotateY);
			guMtxConcat (m, tmp, m);
			guMtxRotAxisDeg (tmp, &axisY, rotateX);
			guMtxConcat (m, tmp, m);
			guMtxRotAxisDeg (tmp, &axisY, -rotateAuto);		//slowly rotate logo
			guMtxConcat (m, tmp, m);
		}
		guMtxScale (tmp, size, size, size);
		guMtxConcat (m, tmp, m);
	}
	guMtxTransApply (m, m, x, y, z); //Move box to x, y, z coords
	guMtxConcat (v, m, mv); //Generate modelview matrix
	// load the modelview matrix into matrix memory
	GX_LoadPosMtxImm (mv, GX_PNMTX0);

	guMtxInverse(mv,mvi);
	guMtxTranspose(mvi,mv);
	GX_LoadNrmMtxImm(mv, GX_PNMTX0);
	
	GX_SetCullMode (GX_CULL_BACK); // show only the outside facing quads
	GX_SetZMode(GX_ENABLE,GX_GEQUAL,GX_TRUE);

//	GX_SetLineWidth(8,GX_TO_ZERO);

	//Lighting WIP
	u32 theta = 180;
	u32 phi = 0;
//	GXColor litcol = (GXColor){ 0xD0, 0xD0, 0xD0, 0xFF }; // Light color 1
	GXColor litcol = (GXColor){ 0xBF, 0xBF, 0xBF, 0xFF }; // Light color 1
	GXColor ambcol = (GXColor){ 0x40, 0x40, 0x40, 0xFF }; // Ambient 1
//	GXColor matcol = (GXColor){ 0x80, 0x80, 0x80, 0xFF };  // Material 1
	GXColor matcol = (GXColor){ 0xFF, 0xFF, 0xFF, transparency };  // Material 1
	
	guVector lpos;
	f32 _theta,_phi;
	GXLightObj lobj;

	_theta = (f32)theta*M_PI/180.0f;
	_phi = (f32)phi*M_PI/180.0f;
	lpos.x = 1000.0f * cosf(_phi) * sinf(_theta);
	lpos.y = 1000.0f * sinf(_phi);
	lpos.z = 1000.0f * cosf(_phi) * cosf(_theta);
	guVecMultiply(v,&lpos,&lpos);

	GX_InitLightPos(&lobj,lpos.x,lpos.y,lpos.z);
	GX_InitLightColor(&lobj,litcol);
	GX_LoadLightObj(&lobj,GX_LIGHT0);
	
	// set number of rasterized color channels
	GX_SetNumChans(1);
	GX_SetChanCtrl(GX_COLOR0A0,GX_ENABLE,GX_SRC_REG,GX_SRC_REG,GX_LIGHT0,GX_DF_CLAMP,GX_AF_NONE);
	GX_SetChanAmbColor(GX_COLOR0A0,ambcol);
	GX_SetChanMatColor(GX_COLOR0A0,matcol);
	
	// setup the vertex descriptor
	GX_ClearVtxDesc ();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX0);
	GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc (GX_VA_NRM, GX_INDEX8);
	GX_SetVtxDesc (GX_VA_TEX0, GX_INDEX8);
	
	// setup the vertex attribute table
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S8, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_S8, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 0);
	
	// set the array stride
	GX_SetArray (GX_VA_POS, Box3D_verts, 3 * sizeof (s8));
	GX_SetArray (GX_VA_NRM, Box3D_norms, 3 * sizeof (s8));
	GX_SetArray (GX_VA_TEX0, Box3D_texcoords, 2 * sizeof (u8));
	
//	GX_SetNumChans (1); //2nd channel needed for lightings
	GX_SetNumTexGens (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	gfx.enableBlending(true);

//	Resources::getInstance().getImage(Resources::IMAGE_BOXART_FRONT)->activateImage(GX_TEXMAP0);
	activeBoxartFrontImage->activateImage(GX_TEXMAP0);
	GX_Begin (GX_QUADS, GX_VTXFMT0, 4);  //1 quads, so 4 verts
	drawQuad ( 0, 3, 2, 1, 3, 0, 1, 2, selColor, 0); //Front Side
	GX_End ();

//	Resources::getInstance().getImage(Resources::IMAGE_BOXART_BACK)->activateImage(GX_TEXMAP0);
	activeBoxartBackImage->activateImage(GX_TEXMAP0);
	GX_Begin (GX_QUADS, GX_VTXFMT0, 4);  //1 quads, so 4 verts
	drawQuad ( 5, 6, 7, 4, 1, 2, 3, 0, selColor, 1); //Back Side
	GX_End ();

//	Resources::getInstance().getImage(Resources::IMAGE_BOXART_SPINE)->activateImage(GX_TEXMAP0);
	activeBoxartSpineImage->activateImage(GX_TEXMAP0);
	GX_Begin (GX_QUADS, GX_VTXFMT0, 8);  //2 quads, so 8 verts
	drawQuad ( 4, 0, 1, 5, 3, 0, 1, 2, selColor, 3); //Bottom Side
	drawQuad ( 3, 7, 6, 2, 3, 0, 1, 2, selColor, 2); //Top Side
	GX_End ();

	//Draw sides
	GX_Begin (GX_QUADS, GX_VTXFMT0, 8);  //2 quads, so 8 verts
	drawQuad ( 4, 7, 3, 0, 2, 3, 0, 1, selColor, 4); //Bottom Side
	drawQuad ( 1, 2, 6, 5, 2, 3, 0, 1, selColor, 5); //Top Side
	GX_End ();
		
	//Reset GX state:
    GX_SetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHTNULL,GX_DF_NONE,GX_AF_NONE);
//	GX_SetLineWidth(6,GX_TO_ZERO);
	gfx.drawInit();
}

void Box3D::drawQuad(u8 v0, u8 v1, u8 v2, u8 v3, u8 st0, u8 st1, u8 st2, u8 st3, u8 c, u8 n)
{
	// draws a quad from 4 vertex idx and one color idx
	// one 8bit position idx
	GX_Position1x8 (v0);
	GX_Normal1x8 (n);
	// one 8bit color idx
	GX_TexCoord1x8 (st0);
	GX_Position1x8 (v1);
	GX_Normal1x8 (n);
	GX_TexCoord1x8 (st1);
	GX_Position1x8 (v2);
	GX_Normal1x8 (n);
	GX_TexCoord1x8 (st2);
	GX_Position1x8 (v3);
	GX_Normal1x8 (n);
	GX_TexCoord1x8 (st3);
}

void Box3D::drawLine(u8 v0, u8 v1, u8 c)
{
	// draws a line from 2 vertex idx and one color idx
	// one 8bit position idx
	GX_Position1x8 (v0);
	// one 8bit color idx
	GX_Color1x8 (c);
	GX_Position1x8 (v1);
	GX_Color1x8 (c);
}

} //namespace menu 
