/*
Rice_GX - RSP_S2DEX.h
Copyright (C) 2002 Rice1964
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

#ifndef _RSP_S2DEX_H_
#define _RSP_S2DEX_H_

#define S2DEX_BG_1CYC           0x01
#define S2DEX_BG_COPY           0x02
#define S2DEX_OBJ_RECTANGLE     0x03
#define S2DEX_OBJ_SPRITE        0x04
#define S2DEX_OBJ_MOVEMEM       0x05
#define S2DEX_SELECT_DL         0xb0
#define S2DEX_OBJ_RENDERMODE    0xb1
#define S2DEX_OBJ_RECTANGLE_R   0xb2
#define S2DEX_OBJ_LOADTXTR      0xc1
#define S2DEX_OBJ_LDTX_SPRITE   0xc2
#define S2DEX_OBJ_LDTX_RECT     0xc3
#define S2DEX_OBJ_LDTX_RECT_R   0xc4
#define S2DEX_RDPHALF_0         0xe4

#define S2DEX_OBJLT_TXTRBLOCK   0x00001033
#define S2DEX_OBJLT_TXTRTILE    0x00fc1034
#define S2DEX_OBJLT_TLUT        0x00000030
#define S2DEX_BGLT_LOADBLOCK    0x0033
#define S2DEX_BGLT_LOADTILE     0xfff4

typedef struct  {       //Intel format
#ifndef _BIG_ENDIAN
  uint32    type;   
  uint32    image;
  
  uint16    tsize;  
  uint16    tmem;   
  
  uint16    sid;    
  uint16    tline;  

  uint32    flag;   
  uint32    mask;   
#else // !_BIG_ENDIAN - Big Endian fix.
  uint32    type;   
  uint32    image;
  
  uint16    tmem;   
  uint16    tsize;  
  
  uint16    tline;  
  uint16    sid;    

  uint32    flag;   
  uint32    mask;   
#endif // _BIG_ENDIAN
} uObjTxtrBlock;    

typedef struct  {       //Intel Format
#ifndef _BIG_ENDIAN
  uint32    type;   
  uint32    image;

  uint16    twidth; 
  uint16    tmem;   

  uint16    sid;    
  uint16    theight;

  uint32    flag;   
  uint32    mask;   
#else // !_BIG_ENDIAN - Big Endian fix.
  uint32    type;   
  uint32    image;

  uint16    tmem;   
  uint16    twidth; 

  uint16    theight;
  uint16    sid;    

  uint32    flag;   
  uint32    mask;   
#endif // _BIG_ENDIAN
} uObjTxtrTile;         // 24 bytes

typedef struct  {       // Intel Format
#ifndef _BIG_ENDIAN
  uint32    type;   
  uint32    image;
  
  uint16    pnum;   
  uint16    phead;  
  
  uint16    sid;    
  uint16   zero;    
  
  uint32    flag;   
  uint32    mask;   
#else // !_BIG_ENDIAN - Big Endian fix.
  uint32    type;   
  uint32    image;
  
  uint16    phead;  
  uint16    pnum;   
  
  uint16   zero;    
  uint16    sid;    
  
  uint32    flag;   
  uint32    mask;   
#endif // _BIG_ENDIAN
} uObjTxtrTLUT;     

typedef union {
  uObjTxtrBlock      block;
  uObjTxtrTile       tile;
  uObjTxtrTLUT       tlut;
} uObjTxtr;

typedef struct {        // Intel format
#ifndef _BIG_ENDIAN
  uint16  scaleW;       
  short  objX;          
  
  uint16  paddingX;     
  uint16  imageW;       
  
  uint16  scaleH;       
  short  objY;          
  
  uint16  paddingY;     
  uint16  imageH;       
  
  uint16  imageAdrs;    
  uint16  imageStride;  

  uint8   imageFlags;   
  uint8   imagePal;     
  uint8   imageSiz;     
  uint8   imageFmt;     
#else // !_BIG_ENDIAN - Big Endian fix.
  short  objX;          
  uint16  scaleW;       
  
  uint16  imageW;       
  uint16  paddingX;     
  
  short  objY;          
  uint16  scaleH;       
  
  uint16  imageH;       
  uint16  paddingY;     
  
  uint16  imageStride;  
  uint16  imageAdrs;    

  uint8   imageFmt;     
  uint8   imageSiz;     
  uint8   imagePal;     
  uint8   imageFlags;   
#endif // _BIG_ENDIAN
} uObjSprite;           


typedef struct  {
  uObjTxtr  txtr;
  uObjSprite    sprite;
} uObjTxSprite;     /* 48 bytes */

typedef struct {        // Intel format
#ifndef _BIG_ENDIAN
  s32       A, B, C, D; 

  short     Y;          
  short     X;          

  uint16   BaseScaleY;  
  uint16   BaseScaleX;  
#else // !_BIG_ENDIAN - Big Endian fix.
  s32       A, B, C, D; 

  short     X;          
  short     Y;          

  uint16   BaseScaleX;  
  uint16   BaseScaleY;  
#endif // _BIG_ENDIAN
} uObjMtx;              

typedef struct {
  float   A, B, C, D;
  float   X;        
  float   Y;        
  float   BaseScaleX;
  float   BaseScaleY;
} uObjMtxReal;

typedef struct {        //Intel format
#ifndef _BIG_ENDIAN
  short   Y;            
  short   X;            
  uint16   BaseScaleY;  
  uint16   BaseScaleX;  
#else // !_BIG_ENDIAN - Big Endian fix.
  short   X;            
  short   Y;            
  uint16   BaseScaleX;  
  uint16   BaseScaleY;  
#endif // _BIG_ENDIAN
} uObjSubMtx;           

typedef struct  {       // Intel Format
#ifndef _BIG_ENDIAN
  uint16    imageW;     
  uint16    imageX;     

  uint16    frameW;     
  short     frameX;     

  uint16    imageH;     
  uint16    imageY;     

  uint16    frameH;     
  short     frameY;     

  uint32    imagePtr;   

  uint8     imageSiz;   
  uint8     imageFmt;   
  uint16    imageLoad;  

  uint16    imageFlip;  
  uint16    imagePal;   

  uint16    tmemH;      
  uint16    tmemW;      
  uint16    tmemLoadTH; 
  uint16    tmemLoadSH; 
  uint16    tmemSize;   
  uint16    tmemSizeW;  
#else // !_BIG_ENDIAN - Big Endian fix.
  uint16    imageX;     
  uint16    imageW;     

  short     frameX;     
  uint16    frameW;     

  uint16    imageY;     
  uint16    imageH;     

  short     frameY;     
  uint16    frameH;     

  uint32    imagePtr;   

  uint16    imageLoad;  
  uint8     imageFmt;   
  uint8     imageSiz;   

  uint16    imagePal;   
  uint16    imageFlip;  

  uint16    tmemW;      
  uint16    tmemH;      
  uint16    tmemLoadSH; 
  uint16    tmemLoadTH; 
  uint16    tmemSizeW;  
  uint16    tmemSize;   
#endif // _BIG_ENDIAN
} uObjBg;               

typedef struct  {   // Intel Format
#ifndef _BIG_ENDIAN
  uint16    imageW;     
  uint16    imageX;     

  uint16    frameW;     
  short     frameX;     

  uint16    imageH;     
  uint16    imageY;     

  uint16    frameH;     
  short     frameY;     

  uint32    imagePtr;   

  uint8     imageSiz;   
  uint8     imageFmt;   
  uint16    imageLoad;  

  uint16    imageFlip;  
  uint16    imagePal;   

  uint16    scaleH;     
  uint16    scaleW;     

  s32       imageYorig; 
  uint8     padding[4];
#else // !_BIG_ENDIAN - Big Endian fix.
  uint16    imageX;     
  uint16    imageW;     

  short     frameX;     
  uint16    frameW;     

  uint16    imageY;     
  uint16    imageH;     

  short     frameY;     
  uint16    frameH;     

  uint32    imagePtr;   

  uint16    imageLoad;  
  uint8     imageFmt;   
  uint8     imageSiz;   

  uint16    imagePal;   
  uint16    imageFlip;  

  uint16    scaleW;     
  uint16    scaleH;     

  s32       imageYorig; 
  uint8     padding[4];
#endif // _BIG_ENDIAN
} uObjScaleBg;

#endif

