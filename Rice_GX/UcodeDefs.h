/*
Rice_GX - UcodeDefs.h
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

#ifndef _UCODE_DEFS_H_
#define _UCODE_DEFS_H_

typedef struct {
    union {
        unsigned int w0;
        struct {
#ifndef _BIG_ENDIAN
            unsigned int arg0:24;
            unsigned int cmd:8;
#else // !_BIG_ENDIAN - Big Endian fix.
            unsigned int cmd:8;
            unsigned int arg0:24;
#endif // _BIG_ENDIAN
        };
    };
    unsigned int w1;
} Gwords;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int w0;
    unsigned int v2:8;
    unsigned int v1:8;
    unsigned int v0:8;
    unsigned int flag:8;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int w0;
    unsigned int flag:8;
    unsigned int v0:8;
    unsigned int v1:8;
    unsigned int v2:8;
#endif // _BIG_ENDIAN
} GGBI0_Tri1;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int v0:8;
    unsigned int v1:8;
    unsigned int v2:8;
    unsigned int cmd:8;
    unsigned int pad:24;
    unsigned int flag:8;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int cmd:8;
    unsigned int v2:8;
    unsigned int v1:8;
	unsigned int v0:8;
    unsigned int flag:8;
    unsigned int pad:24;
#endif // _BIG_ENDIAN
} GGBI2_Tri1;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int :1;
    unsigned int v3:7;
    unsigned int :1;
    unsigned int v4:7;
    unsigned int :1;
    unsigned int v5:7;
    unsigned int cmd:8;
    unsigned int :1;
    unsigned int v0:7;
    unsigned int :1;
    unsigned int v1:7;
    unsigned int :1;
    unsigned int v2:7;
    unsigned int flag:8;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int cmd:8;
    unsigned int v5:7;
    unsigned int :1;
    unsigned int v4:7;
    unsigned int :1;
    unsigned int v3:7;
	unsigned int :1;
    unsigned int flag:8;
    unsigned int v2:7;
    unsigned int :1;
    unsigned int v1:7;
    unsigned int :1;
    unsigned int v0:7;
    unsigned int :1;
#endif // _BIG_ENDIAN
} GGBI2_Tri2;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int w0;
    unsigned int v2:8;
    unsigned int v1:8;
    unsigned int v0:8;
    unsigned int v3:8;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int w0;
    unsigned int v3:8;
    unsigned int v0:8;
    unsigned int v1:8;
    unsigned int v2:8;
#endif // _BIG_ENDIAN
} GGBI0_Ln3DTri2;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int v5:8;
    unsigned int v4:8;
    unsigned int v3:8;
    unsigned int cmd:8;

    unsigned int v2:8;
    unsigned int v1:8;
    unsigned int v0:8;
    unsigned int flag:8;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int cmd:8;
    unsigned int v3:8;
    unsigned int v4:8;
    unsigned int v5:8;

    unsigned int flag:8;
    unsigned int v0:8;
    unsigned int v1:8;
    unsigned int v2:8;
#endif // _BIG_ENDIAN
} GGBI1_Tri2;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int v3:8;
    unsigned int v4:8;
    unsigned int v5:8;
    unsigned int cmd:8;

    unsigned int v0:8;
    unsigned int v1:8;
    unsigned int v2:8;
    unsigned int flag:8;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int cmd:8;
    unsigned int v5:8;
    unsigned int v4:8;
	unsigned int v3:8;

    unsigned int flag:8;
    unsigned int v2:8;
    unsigned int v1:8;
    unsigned int v0:8;
#endif // _BIG_ENDIAN
} GGBI2_Line3D;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int len:16;
    unsigned int v0:4;
    unsigned int n:4;
    unsigned int cmd:8;
    unsigned int addr;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int cmd:8;
    unsigned int n:4;
    unsigned int v0:4;
    unsigned int len:16;
    unsigned int addr;
#endif // _BIG_ENDIAN
} GGBI0_Vtx;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int len:10;
    unsigned int n:6;
    unsigned int :1;
    unsigned int v0:7;
    unsigned int cmd:8;
    unsigned int addr;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int cmd:8;
    unsigned int v0:7;
    unsigned int :1;
    unsigned int n:6;
	unsigned int len:10;
    unsigned int addr;
#endif // _BIG_ENDIAN
} GGBI1_Vtx;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int vend:8;
    unsigned int :4;
    unsigned int n:8;
    unsigned int :4;
    unsigned int cmd:8;
    unsigned int addr;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int cmd:8;
    unsigned int :4;
    unsigned int n:8;
    unsigned int :4;
	unsigned int vend:8;
    unsigned int addr;
#endif // _BIG_ENDIAN
} GGBI2_Vtx;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int    width:12;
    unsigned int    :7;
    unsigned int    siz:2;
    unsigned int    fmt:3;
    unsigned int    cmd:8;
    unsigned int    addr;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int    cmd:8;
    unsigned int    fmt:3;
    unsigned int    siz:2;
    unsigned int    :7;
    unsigned int    width:12;
    unsigned int    addr;
#endif // _BIG_ENDIAN
} GSetImg;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int    prim_level:8;
    unsigned int    prim_min_level:8;
    unsigned int    pad:8;
    unsigned int    cmd:8;

    union {
        unsigned int    color;
        struct {
            unsigned int fillcolor:16;
            unsigned int fillcolor2:16;
        };
        struct {
            unsigned int a:8;
            unsigned int b:8;
            unsigned int g:8;
            unsigned int r:8;
        };
    };
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int    cmd:8;
    unsigned int    pad:8;
    unsigned int    prim_min_level:8;
	unsigned int    prim_level:8;

    union {
        unsigned int    color;
        struct {
            unsigned int fillcolor2:16;
            unsigned int fillcolor:16;
        };
        struct {
            unsigned int r:8;
            unsigned int g:8;
            unsigned int b:8;
            unsigned int a:8;
        };
    };
#endif // _BIG_ENDIAN
} GSetColor;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int    :16;
    unsigned int    param:8;
    unsigned int    cmd:8;
    unsigned int    addr;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int    cmd:8;
    unsigned int    param:8;
    unsigned int    :16;
    unsigned int    addr;
#endif // _BIG_ENDIAN
} GGBI0_Dlist;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int    len:16;
    unsigned int    projection:1;
    unsigned int    load:1;
    unsigned int    push:1;
    unsigned int    :5;
    unsigned int    cmd:8;
    unsigned int    addr;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int    cmd:8;
    unsigned int    :5;
    unsigned int    push:1;
    unsigned int    load:1;
    unsigned int    projection:1;
	unsigned int    len:16;
    unsigned int    addr;
#endif // _BIG_ENDIAN
} GGBI0_Matrix;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int    :24;
    unsigned int    cmd:8;
    unsigned int    projection:1;
    unsigned int    :31;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int    cmd:8;
    unsigned int    :24;
    unsigned int    :31;
    unsigned int    projection:1;
#endif // _BIG_ENDIAN
} GGBI0_PopMatrix;

typedef struct {
#ifndef _BIG_ENDIAN
    union {
        struct {
            unsigned int    param:8;
            unsigned int    len:16;
            unsigned int    cmd:8;
        };
        struct {
            unsigned int    nopush:1;
            unsigned int    load:1;
            unsigned int    projection:1;
            unsigned int    :5;
            unsigned int    len2:16;
            unsigned int    cmd2:8;
        };
    };
    unsigned int    addr;
#else // !_BIG_ENDIAN - Big Endian fix.
    union {
        struct {
            unsigned int    cmd:8;
            unsigned int    len:16;
            unsigned int    param:8;
        };
        struct {
            unsigned int    cmd2:8;
            unsigned int    len2:16;
            unsigned int    :5;
            unsigned int    projection:1;
            unsigned int    load:1;
			unsigned int    nopush:1;
        };
    };
    unsigned int    addr;
#endif // _BIG_ENDIAN
} GGBI2_Matrix;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int    type:8;
    unsigned int    offset:16;
    unsigned int    cmd:8;
    unsigned int    value;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int    cmd:8;
    unsigned int    offset:16;
	unsigned int    type:8;
    unsigned int    value;
#endif // _BIG_ENDIAN
} GGBI0_MoveWord;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int    offset:16;
    unsigned int    type:8;
    unsigned int    cmd:8;
    unsigned int    value;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int    cmd:8;
    unsigned int    type:8;
	unsigned int    offset:16;
    unsigned int    value;
#endif // _BIG_ENDIAN
} GGBI2_MoveWord;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int    enable_gbi0:1;
    unsigned int    enable_gbi2:1;
    unsigned int    :6;
    unsigned int    tile:3;
    unsigned int    level:3;
    unsigned int    :10;
    unsigned int    cmd:8;
    unsigned int    scaleT:16;
    unsigned int    scaleS:16;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int    cmd:8;
    unsigned int    :10;
    unsigned int    level:3;
    unsigned int    tile:3;
    unsigned int    :6;
    unsigned int    enable_gbi2:1;
	unsigned int    enable_gbi0:1;
    unsigned int    scaleS:16;
    unsigned int    scaleT:16;
#endif // _BIG_ENDIAN
} GTexture;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int    tl:12;
    unsigned int    sl:12;
    unsigned int    cmd:8;

    unsigned int    th:12;
    unsigned int    sh:12;
    unsigned int    tile:3;
    unsigned int    pad:5;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int    cmd:8;
    unsigned int    sl:12;
    unsigned int    tl:12;

    unsigned int    pad:5;
    unsigned int    tile:3;
    unsigned int    sh:12;
    unsigned int    th:12;
#endif // _BIG_ENDIAN
} Gloadtile;

typedef struct {
#ifndef _BIG_ENDIAN
    unsigned int    tmem:9;
    unsigned int    line:9;
    unsigned int    pad0:1;
    unsigned int    siz:2;
    unsigned int    fmt:3;
    unsigned int    cmd:8;

    unsigned int    shifts:4;
    unsigned int    masks:4;
    unsigned int    ms:1;
    unsigned int    cs:1;
    unsigned int    shiftt:4;
    unsigned int    maskt:4;
    unsigned int    mt:1;
    unsigned int    ct:1;
    unsigned int    palette:4;
    unsigned int    tile:3;
    unsigned int    pad1:5;
#else // !_BIG_ENDIAN - Big Endian fix.
    unsigned int    cmd:8;
    unsigned int    fmt:3;
    unsigned int    siz:2;
    unsigned int    pad0:1;
    unsigned int    line:9;
	unsigned int    tmem:9;

    unsigned int    pad1:5;
    unsigned int    tile:3;
    unsigned int    palette:4;
    unsigned int    ct:1;
    unsigned int    mt:1;
    unsigned int    maskt:4;
    unsigned int    shiftt:4;
    unsigned int    cs:1;
    unsigned int    ms:1;
    unsigned int    masks:4;
    unsigned int    shifts:4;
#endif // _BIG_ENDIAN
} Gsettile;

typedef union {
    Gwords          words;
    GGBI0_Tri1      tri1;
    GGBI0_Ln3DTri2  ln3dtri2;
    GGBI1_Tri2      gbi1tri2;
    GGBI2_Tri1      gbi2tri1;
    GGBI2_Tri2      gbi2tri2;
    GGBI2_Line3D    gbi2line3d;
    GGBI0_Vtx       gbi0vtx;
    GGBI1_Vtx       gbi1vtx;
    GGBI2_Vtx       gbi2vtx;
    GSetImg         setimg;
    GSetColor       setcolor;
    GGBI0_Dlist     gbi0dlist;
    GGBI0_Matrix    gbi0matrix;
    GGBI0_PopMatrix gbi0popmatrix;
    GGBI2_Matrix    gbi2matrix;
    GGBI0_MoveWord  gbi0moveword;
    GGBI2_MoveWord  gbi2moveword;
    GTexture        texture;
    Gloadtile       loadtile;
    Gsettile        settile;
    /*
    Gdma        dma;
    Gsegment    segment;
    GsetothermodeH  setothermodeH;
    GsetothermodeL  setothermodeL;
    Gtexture    texture;
    Gperspnorm  perspnorm;
    Gsetcombine setcombine;
    Gfillrect   fillrect;
    Gsettile    settile;
    Gloadtile   loadtile;
    Gsettilesize    settilesize;
    Gloadtlut   loadtlut;
    */
    long long int   force_structure_alignment;
} Gfx;

typedef union {
#ifndef _BIG_ENDIAN
    struct {
        unsigned int    w0;
        unsigned int    w1;
        unsigned int    w2;
        unsigned int    w3;
    };
    struct {
        unsigned int    yl:12;  /* Y coordinate of upper left   */
        unsigned int    xl:12;  /* X coordinate of upper left   */
        unsigned int    cmd:8;  /* command          */

        unsigned int    yh:12;  /* Y coordinate of lower right  */
        unsigned int    xh:12;  /* X coordinate of lower right  */
        unsigned int    tile:3; /* Tile descriptor index    */
        unsigned int    pad1:5; /* Padding          */

        unsigned int    t:16;   /* T texture coord at top left  */
        unsigned int    s:16;   /* S texture coord at top left  */

        unsigned int    dtdy:16;/* Change in T per change in Y  */
        unsigned int    dsdx:16;/* Change in S per change in X  */
    };
#else // !_BIG_ENDIAN - Big Endian fix.
    struct {
        unsigned int    w0;
        unsigned int    w1;
        unsigned int    w2;
        unsigned int    w3;
    };
    struct {
        unsigned int    cmd:8;  /* command          */
        unsigned int    xl:12;  /* X coordinate of upper left   */
        unsigned int    yl:12;  /* Y coordinate of upper left   */

        unsigned int    pad1:5; /* Padding          */
        unsigned int    tile:3; /* Tile descriptor index    */
        unsigned int    xh:12;  /* X coordinate of lower right  */
        unsigned int    yh:12;  /* Y coordinate of lower right  */

        unsigned int    s:16;   /* S texture coord at top left  */
        unsigned int    t:16;   /* T texture coord at top left  */

        unsigned int    dsdx:16;/* Change in S per change in X  */
        unsigned int    dtdy:16;/* Change in T per change in Y  */
    };
#endif // _BIG_ENDIAN
} Gtexrect;

#endif

