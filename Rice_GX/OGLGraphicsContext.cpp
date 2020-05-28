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

#ifndef __GX__
#include <SDL_opengl.h>
#else //!__GX__
# ifdef MENU_V2
#include "../libgui/IPLFont.h"
#include "../menu/MenuResources.h"
# else // MENU_V2
#include "../gui/font.h"
# endif //!MENU_V2
#include "../gui/DEBUG.h"
#include "../main/timers.h"
#endif // __GX__

#include "stdafx.h"

#include "liblinux/BMGLibPNG.h"

#ifndef __GX__
#include "../main/version.h"
#endif //!__GX__

COGLGraphicsContext::COGLGraphicsContext() :
#ifndef __GX__
    m_pScreen(0),
#endif //!__GX__
    m_bSupportMultiTexture(false),
    m_bSupportTextureEnvCombine(false),
    m_bSupportSeparateSpecularColor(false),
    m_bSupportSecondColor(false),
    m_bSupportFogCoord(false),
    m_bSupportTextureObject(false),
    m_bSupportRescaleNormal(false),
    m_bSupportLODBias(false),
    m_bSupportTextureMirrorRepeat(false),
    m_bSupportTextureLOD(false),
    m_bSupportNVRegisterCombiner(false),
    m_bSupportBlendColor(false),
    m_bSupportBlendSubtract(false),
    m_bSupportNVTextureEnvCombine4(false),
    m_pVendorStr(NULL),
    m_pRenderStr(NULL),
    m_pExtensionStr(NULL),
    m_pVersionStr(NULL)
{
}


COGLGraphicsContext::~COGLGraphicsContext()
{
}

bool COGLGraphicsContext::Initialize(HWND hWnd, HWND hWndStatus, uint32 dwWidth, uint32 dwHeight, BOOL bWindowed )
{
#ifndef __GX__
    printf("Initializing OpenGL Device Context\n");
#endif //!__GX__
    Lock();

    CGraphicsContext::Get()->m_supportTextureMirror = false;
    CGraphicsContext::Initialize(hWnd, hWndStatus, dwWidth, dwHeight, bWindowed );

    if( bWindowed )
    {
        windowSetting.statusBarHeightToUse = windowSetting.statusBarHeight;
        windowSetting.toolbarHeightToUse = windowSetting.toolbarHeight;
    }
    else
    {
        windowSetting.statusBarHeightToUse = 0;
        windowSetting.toolbarHeightToUse = 0;
    }

#ifndef __GX__
    int  depthBufferDepth = options.OpenglDepthBufferSetting;
    int  colorBufferDepth = 32;
    if( options.colorQuality == TEXTURE_FMT_A4R4G4B4 ) colorBufferDepth = 16;

   // init sdl & gl
   const SDL_VideoInfo *videoInfo;
   Uint32 videoFlags = 0;
   
   /* Initialize SDL */
   printf("(II) Initializing SDL video subsystem...\n");
   if(SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
     {
    printf("(EE) Error initializing SDL video subsystem: %s\n", SDL_GetError());
    return false;
     }
   
   /* Video Info */
   printf("(II) Getting video info...\n");
   if(!(videoInfo = SDL_GetVideoInfo()))
     {
    printf("(EE) Video query failed: %s\n", SDL_GetError());
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    return false;
     }
   /* Setting the video mode */
   videoFlags |= SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE;
   
   if(videoInfo->hw_available)
     videoFlags |= SDL_HWSURFACE;
   else
     videoFlags |= SDL_SWSURFACE;
   
   if(videoInfo->blit_hw)
     videoFlags |= SDL_HWACCEL;
   
   if(!bWindowed)
     videoFlags |= SDL_FULLSCREEN;
   
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, colorBufferDepth);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBufferDepth);
   
   printf("(II) Setting video mode %dx%d...\n", (int)windowSetting.uDisplayWidth, (int)windowSetting.uDisplayHeight);
   if(!(m_pScreen = SDL_SetVideoMode(windowSetting.uDisplayWidth, windowSetting.uDisplayHeight, colorBufferDepth, videoFlags)))
     {
    printf("(EE) Error setting video mode %dx%d: %s\n", (int)windowSetting.uDisplayWidth, (int)windowSetting.uDisplayHeight, SDL_GetError());
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    return false;
     }
   
   char caption[500];
   sprintf(caption, "RiceVideoLinux N64 Plugin %s", MUPEN_VERSION);
   SDL_WM_SetCaption(caption, caption);
#endif //!__GX__

   SetWindowMode();

    InitState();
    InitOGLExtension();
    sprintf(m_strDeviceStats, "%s - %s : %s", m_pVendorStr, m_pRenderStr, m_pVersionStr);
    TRACE0(m_strDeviceStats);
#ifndef __GX__
    printf("%s\n", m_strDeviceStats);
#endif //!__GX__

    Unlock();

#ifndef __GX__
    Clear(CLEAR_COLOR_AND_DEPTH_BUFFER);    // Clear buffers
    UpdateFrame();
    Clear(CLEAR_COLOR_AND_DEPTH_BUFFER);
    UpdateFrame();
#else //!__GX__
	//Don't clear the screen immediately after ROM load
	//TODO: Make sure that EFB/xfb are cleared before rendering first screen
#endif //__GX__
    
    m_bReady = true;
    status.isVertexShaderEnabled = false;

    return true;
}

void COGLGraphicsContext::InitState(void)
{
#ifndef __GX__
	//TODO: Implement in GX
    m_pRenderStr = glGetString(GL_RENDERER);;
    m_pExtensionStr = glGetString(GL_EXTENSIONS);;
    m_pVersionStr = glGetString(GL_VERSION);;
    m_pVendorStr = glGetString(GL_VENDOR);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);

    glShadeModel(GL_SMOOTH);

    //position viewer 
    //glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();

    glDisable(GL_ALPHA_TEST);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);

    glFrontFace(GL_CCW);
    glDisable(GL_CULL_FACE);
    glDisable(GL_NORMALIZE);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    glDepthRange(-1, 1);
#endif //!__GX__
}

void COGLGraphicsContext::InitOGLExtension(void)
{
#ifndef __GX__
    // important extension features, it is very bad not to have these feature
    m_bSupportMultiTexture = IsExtensionSupported("GL_ARB_multitexture");
    m_bSupportTextureEnvCombine = IsExtensionSupported("GL_EXT_texture_env_combine");
    
    m_bSupportSeparateSpecularColor = IsExtensionSupported("GL_EXT_separate_specular_color");
    m_bSupportSecondColor = IsExtensionSupported("GL_EXT_secondary_color");
    m_bSupportFogCoord = IsExtensionSupported("GL_EXT_fog_coord");
    m_bSupportTextureObject = IsExtensionSupported("GL_EXT_texture_object");

    // Optional extension features
    m_bSupportRescaleNormal = IsExtensionSupported("GL_EXT_rescale_normal");
    m_bSupportLODBias = IsExtensionSupported("GL_EXT_texture_lod_bias");

    // Nvidia only extension features (optional)
    m_bSupportNVRegisterCombiner = IsExtensionSupported("GL_NV_register_combiners");
    m_bSupportTextureMirrorRepeat = IsExtensionSupported("GL_IBM_texture_mirrored_repeat") || IsExtensionSupported("ARB_texture_mirrored_repeat");
    m_supportTextureMirror = m_bSupportTextureMirrorRepeat;
    m_bSupportTextureLOD = IsExtensionSupported("GL_EXT_texture_lod");
    m_bSupportBlendColor = IsExtensionSupported("GL_EXT_blend_color");
    m_bSupportBlendSubtract = IsExtensionSupported("GL_EXT_blend_subtract");
    m_bSupportNVTextureEnvCombine4 = IsExtensionSupported("GL_NV_texture_env_combine4");
#else //!__GX__
    // important extension features, it is very bad not to have these feature
    m_bSupportMultiTexture = true;
    m_bSupportTextureEnvCombine = true;
    
    m_bSupportSeparateSpecularColor = true;
    m_bSupportSecondColor = true;
    m_bSupportFogCoord = true;
    m_bSupportTextureObject = true;

    // Optional extension features
    m_bSupportRescaleNormal = false; //need to look this up: IsExtensionSupported("GL_EXT_rescale_normal");
    m_bSupportLODBias = true;

    // Nvidia only extension features (optional)
	m_bSupportNVRegisterCombiner = false;
    m_bSupportTextureMirrorRepeat = true; //not sure about this
    m_supportTextureMirror = m_bSupportTextureMirrorRepeat;
    m_bSupportTextureLOD = true;
    m_bSupportBlendColor = true;
    m_bSupportBlendSubtract = true;
    m_bSupportNVTextureEnvCombine4 = false;
#endif //__GX__
}

bool COGLGraphicsContext::IsExtensionSupported(const char* pExtName)
{
    if( strstr((const char*)m_pExtensionStr, pExtName) != NULL )
        return true;
    else
        return false;
}

bool COGLGraphicsContext::IsWglExtensionSupported(const char* pExtName)
{
    if( m_pWglExtensionStr == NULL )
        return false;

    if( strstr((const char*)m_pWglExtensionStr, pExtName) != NULL )
        return true;
    else
        return false;
}


void COGLGraphicsContext::CleanUp()
{
#ifndef __GX__
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    m_pScreen = NULL;
#endif //!__GX__
    m_bReady = false;
}


void COGLGraphicsContext::Clear(ClearFlag dwFlags, uint32 color, float depth)
{
#ifndef __GX__
    uint32 flag=0;
    if( dwFlags&CLEAR_COLOR_BUFFER )    flag |= GL_COLOR_BUFFER_BIT;
    if( dwFlags&CLEAR_DEPTH_BUFFER )    flag |= GL_DEPTH_BUFFER_BIT;

    float r = ((color>>16)&0xFF)/255.0f;
    float g = ((color>> 8)&0xFF)/255.0f;
    float b = ((color    )&0xFF)/255.0f;
    float a = ((color>>24)&0xFF)/255.0f;

	glClearColor(r, g, b, a);
    glClearDepth(depth);
    glClear(flag);  //Clear color buffer and depth buffer
#else //!__GX__
    if( dwFlags&CLEAR_COLOR_BUFFER )    gGX.GXclearColorBuffer = true;
    if( dwFlags&CLEAR_DEPTH_BUFFER )    gGX.GXclearDepthBuffer = true;

	gGX.GXclearColor.r = (u8) ((color>>16)&0xFF);
	gGX.GXclearColor.g = (u8) ((color>> 8)&0xFF);
	gGX.GXclearColor.b = (u8) ((color    )&0xFF);
	gGX.GXclearColor.a = (u8) ((color>>24)&0xFF);

	gGX.GXclearDepth = depth;
	if (gGX.GXclearColorBuffer || gGX.GXclearDepthBuffer)
		CRender::GetRender()->GXclearEFB();
#endif //__GX__
}

#ifdef __GX__
//Functions needed for UpdateFrame
extern "C" {
extern int enableLoadIcon;
extern long long gettime();
extern unsigned int diff_sec(long long start,long long end);
};

extern char printToScreen;
extern char showFPSonScreen;
extern GXRModeObj *rmode;

struct VIInfo
{
	unsigned int* xfb[2];
	int which_fb;
	bool updateOSD;
	bool copy_fb;
};

VIInfo VI;

void VI_GX_setFB(unsigned int* fb1, unsigned int* fb2){
	VI.xfb[0] = fb1;
	VI.xfb[1] = fb2;
}

extern timers Timers;

void VI_GX_showFPS(){
	static char caption[25];

	sprintf(caption, "%.1f VI/s, %.1f FPS",Timers.vis,Timers.fps);
	
	GXColor fontColor = {150,255,150,255};
#ifndef MENU_V2
	write_font_init_GX(fontColor);
	if(showFPSonScreen)
		write_font(10,35,caption, 1.0);
#else
	menu::IplFont::getInstance().drawInit(fontColor);
	if(showFPSonScreen)
		menu::IplFont::getInstance().drawString(20,40,caption, 1.0, false);
#endif

	//reset swap table from GUI/DEBUG
//	GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
	GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);

/*	static long long lastTick=0;
	static int frames=0;
	static int VIs=0;
	static char caption[20];
	
	long long nowTick = gettime();
	VIs++;
//	if (VI.updateOSD)
//		frames++;
	if (diff_sec(lastTick,nowTick)>=1) {
		sprintf(caption, "%02d VI/s, %02d FPS",VIs,frames);
		frames = 0;
		VIs = 0;
		lastTick = nowTick;
	}
	
//	if (VI.updateOSD)
//	{
		GXColor fontColor = {150,255,150,255};
		write_font_init_GX(fontColor);
		if(showFPSonScreen)
			write_font(10,35,caption, 1.0);
		//write_font(10,10,caption,xfb,which_fb);

		//reset swap table from GUI/DEBUG
		GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
		GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
//	}*/
}

#ifdef HW_DOL
void VI_GX_showLoadIcon()
{
	if (enableLoadIcon == 0) return;
	enableLoadIcon = 0;

	float x = 530;
	float y = 30;
	float width = 80;
	float height = 56;

	Mtx44 GXprojection2D;
	Mtx GXmodelView2D;

	GXTexObj texObj;
//	GX_InitTexObj(&texObj, LoadingTexture, width, height, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObj(&texObj, LoadingTexture, width, height, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);

	guMtxIdentity(GXmodelView2D);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX2);
	guOrtho(GXprojection2D, 0, 480, 0, 640, 0, 1);
	GX_LoadProjectionMtx(GXprojection2D, GX_ORTHOGRAPHIC); //load current 2D projection matrix

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_PTNMTXIDX, GX_PNMTX2);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//set vertex attribute formats here
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	//disable textures
	GX_SetNumChans (0);
	GX_SetNumTexGens (1);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	//set blend mode
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); //Fix src alpha
	GX_SetColorUpdate(GX_ENABLE);
	GX_SetAlphaUpdate(GX_ENABLE);
	GX_SetDstAlpha(GX_DISABLE, 0xFF);
	GX_SetZMode(GX_DISABLE,GX_ALWAYS,GX_FALSE);
	GX_SetZTexture(GX_ZT_DISABLE,GX_TF_Z16,0);	//GX_ZT_DISABLE or GX_ZT_REPLACE; set in gDP.cpp
	GX_SetZCompLoc(GX_TRUE);	// Do Z-compare before texturing.
	//set cull mode
	GX_SetCullMode (GX_CULL_NONE);

	GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
		GX_Position2f32(x, y);
		GX_TexCoord2f32(0,0);
		GX_Position2f32(x+width, y);
		GX_TexCoord2f32(1,0);
		GX_Position2f32(x+width, y+height);
		GX_TexCoord2f32(1,1);
		GX_Position2f32(x, y+height);
		GX_TexCoord2f32(0,1);
	GX_End();
}
#endif

extern char text[DEBUG_TEXT_HEIGHT][DEBUG_TEXT_WIDTH];

void VI_GX_showDEBUG()
{
	int i = 0;
	GXColor fontColor = {150, 255, 150, 255};
#ifdef SHOW_DEBUG
	DEBUG_update();
#ifndef MENU_V2
	write_font_init_GX(fontColor);
	if(printToScreen)
		for (i=0;i<DEBUG_TEXT_HEIGHT;i++)
			write_font(10,(10*i+60),text[i], 0.5); 
#else
	menu::IplFont::getInstance().drawInit(fontColor);
	if(printToScreen)
		for (i=0;i<DEBUG_TEXT_HEIGHT;i++)
//			menu::IplFont::getInstance().drawString(10,(15*i+0),text[i], 0.8, false); 
			menu::IplFont::getInstance().drawString(20,(10*i+65),text[i], 0.5, false); 
#endif
#endif //SHOW_DEBUG

	//Reset any stats in DEBUG_stats
//	DEBUG_stats(8, "RecompCache Blocks Freed", STAT_TYPE_CLEAR, 1);

   //reset swap table from GUI/DEBUG
//	GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
	GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
/*
//	if (VI.updateOSD)
//	{
		int i = 0;
		GXColor fontColor = {150, 255, 150, 255};
		DEBUG_update();
#ifndef MENU_V2
		write_font_init_GX(fontColor);
		if(printToScreen)
			for (i=0;i<DEBUG_TEXT_HEIGHT;i++)
				write_font(10,(10*i+60),text[i], 0.5); 
#else
		menu::IplFont::getInstance().drawInit(fontColor);
		if(printToScreen)
			for (i=0;i<DEBUG_TEXT_HEIGHT;i++)
				menu::IplFont::getInstance().drawString(10,(10*i+60),text[i], 0.5, false); 
#endif
		
	   //reset swap table from GUI/DEBUG
		GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
		GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
//	}*/
}

void VI_GX_showStats()
{
/*	if (VI.updateOSD)
	{
		sprintf(txtbuffer,"texCache: %d bytes in %d cached textures; %d FB textures",cache.cachedBytes,cache.numCached,frameBuffer.numBuffers);
		DEBUG_print(txtbuffer,DBG_CACHEINFO); 
	}*/
}

void VI_GX_PreRetraceCallback(u32 retraceCnt)
{
	if(VI.copy_fb)
	{
		VIDEO_SetNextFramebuffer(VI.xfb[VI.which_fb]);
		VIDEO_Flush();
		VI.which_fb ^= 1;
		VI.copy_fb = false;
	}
}

#endif //__GX__

void COGLGraphicsContext::UpdateFrame(bool swaponly)
{
    status.gFrameCount++;

#ifndef __GX__
	//TODO: Implement in GX
    glFlush();
    OPENGL_CHECK_ERRORS;
#endif //!__GX__
    //glFinish();
    //wglSwapIntervalEXT(0);

    /*
    if (debuggerPauseCount == countToPause)
    {
        static int iShotNum = 0;
        // get width, height, allocate buffer to store image
        int width = windowSetting.uDisplayWidth;
        int height = windowSetting.uDisplayHeight;
        printf("Saving debug images: width=%i  height=%i\n", width, height);
        short *buffer = (short *) malloc(((width+3)&~3)*(height+1)*4);
        glReadBuffer( GL_FRONT );
        // set up a BMGImage struct
        struct BMGImageStruct img;
        memset(&img, 0, sizeof(BMGImageStruct));
        InitBMGImage(&img);
        img.bits = (unsigned char *) buffer;
        img.bits_per_pixel = 32;
        img.height = height;
        img.width = width;
        img.scan_width = width * 4;
        // store the RGB color image
        char chFilename[64];
        sprintf(chFilename, "dbg_rgb_%03i.png", iShotNum);
        glReadPixels(0,0,width,height, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
        WritePNG(chFilename, img);
        // store the Z buffer
        sprintf(chFilename, "dbg_Z_%03i.png", iShotNum);
        glReadPixels(0,0,width,height, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, buffer);
        //img.bits_per_pixel = 16;
        //img.scan_width = width * 2;
        WritePNG(chFilename, img);
        // dump a subset of the Z data
        for (int y = 0; y < 480; y += 16)
        {
            for (int x = 0; x < 640; x+= 16)
                printf("%4hx ", buffer[y*640 + x]);
            printf("\n");
        }
        printf("\n");
        // free memory and get out of here
        free(buffer);
        iShotNum++;
        }
    */

   
   // if emulator defined a render callback function, call it before buffer swap
   if(renderCallback)
       (*renderCallback)();

#ifndef __GX__
	//TODO: Implement in GX
   SDL_GL_SwapBuffers();
#else

//	VI_GX_cleanUp();
//	VI_GX_showStats();
   //TODO: Move the following and others to a "clean up" function
	// Set viewport to whole EFB and scissor to N64 frame for OSD
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	GX_SetScissor((u32) gGX.GXorigX,(u32) gGX.GXorigY,(u32) gGX.GXwidth,(u32) gGX.GXheight);	//Set Scissor to render plane for DEBUG prints
#ifdef HW_DOL
	VI_GX_showLoadIcon();
#endif
	VI_GX_showFPS();
	VI_GX_showDEBUG();
	// Set viewport to N64 frame and disable scissor CopyDisp
	GX_SetViewport((f32) gGX.GXorigX,(f32) gGX.GXorigY,(f32) gGX.GXwidth,(f32) gGX.GXheight, 0.0f, 1.0f);
	GX_SetScissor((u32) 0,(u32) 0,(u32) windowSetting.uDisplayWidth+1,(u32) windowSetting.uDisplayHeight+1);	//Disable Scissor
//	if(VI.updateOSD)
//	{
//		if(VI.copy_fb)
//			VIDEO_WaitVSync();
		GX_SetCopyClear ((GXColor){0,0,0,255}, 0xFFFFFF);
//		GX_CopyDisp (VI.xfb[VI.which_fb], GX_TRUE);	//clear the EFB before executing new Dlist
		GX_CopyDisp (VI.xfb[VI.which_fb], GX_FALSE);	//clear the EFB before executing new Dlist
		GX_DrawDone(); //Wait until EFB->XFB copy is complete
//		doCaptureScreen();
		VI.updateOSD = false;
		VI.copy_fb = true;
//	}

#endif //!__GX__
   
   /*if(options.bShowFPS)
     {
    static unsigned int lastTick=0;
    static int frames=0;
    unsigned int nowTick = SDL_GetTicks();
    frames++;
    if(lastTick + 5000 <= nowTick)
      {
         char caption[200];
         sprintf(caption, "RiceVideoLinux N64 Plugin %s - %.3f VI/S", MUPEN_VERSION, frames/5.0);
         SDL_WM_SetCaption(caption, caption);
         frames = 0;
         lastTick = nowTick;
      }
     }*/

#ifndef __GX__
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);
    if( !g_curRomInfo.bForceScreenClear ) 
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
#else //!__GX__
	//TODO: Integrate this with above GX_CopyDisp
    if( !g_curRomInfo.bForceScreenClear ) 
    {
		gGX.GXclearDepthBuffer = true;
		gGX.GXclearDepth = 1.0;
		CRender::GetRender()->GXclearEFB();
    }
#endif //__GX__
    else
        needCleanScene = true;

    status.bScreenIsDrawn = false;
}

bool COGLGraphicsContext::SetFullscreenMode()
{
    windowSetting.uDisplayWidth = windowSetting.uFullScreenDisplayWidth;
    windowSetting.uDisplayHeight = windowSetting.uFullScreenDisplayHeight;
    windowSetting.statusBarHeightToUse = 0;
    windowSetting.toolbarHeightToUse = 0;
    return true;
}

bool COGLGraphicsContext::SetWindowMode()
{
    windowSetting.uDisplayWidth = windowSetting.uWindowDisplayWidth;
    windowSetting.uDisplayHeight = windowSetting.uWindowDisplayHeight;
    windowSetting.statusBarHeightToUse = windowSetting.statusBarHeight;
    windowSetting.toolbarHeightToUse = windowSetting.toolbarHeight;
    return true;
}
int COGLGraphicsContext::ToggleFullscreen()
{
#ifndef __GX__
	//TODO: Implement in GX
   if(SDL_WM_ToggleFullScreen(m_pScreen) == 1)
     {
    m_bWindowed = 1 - m_bWindowed;
    if(m_bWindowed)
      SetWindowMode();
    else
      SetFullscreenMode();
     }
#endif //!__GX__

    return m_bWindowed?0:1;
}

// This is a static function, will be called when the plugin DLL is initialized
void COGLGraphicsContext::InitDeviceParameters()
{
    status.isVertexShaderEnabled = false;   // Disable it for now
}

