/**
 * glN64_GX - FrameBuffer.cpp
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "Types.h"
#include "Textures.h"

struct FrameBuffer
{
	FrameBuffer *higher, *lower;

	CachedTexture *texture;

	u32 startAddress, endAddress;
	u32 size, width, height, changed;
	float scaleX, scaleY;
	BOOL fingerprint;	// FrameBuffer_CopyRdram() stamped this buffer
	BOOL isDepthBuffer;	// GLideN64's m_isDepthBuffer: saved at the depth image address
	u32 validityChecked;	// the frame its validity was last established at
	u32 refreshedFrame;	// the frame its texture was last re-captured from the EFB
};

struct FrameBufferInfo
{
	FrameBuffer *top, *bottom, *current;
	int numBuffers;
};

extern FrameBufferInfo frameBuffer;

void FrameBuffer_Init();
void FrameBuffer_Destroy();
void FrameBuffer_RemoveBuffersOfWidth( u32 width );
void FrameBuffer_SaveBuffer( u32 address, u16 size, u16 width, u16 height );
void FrameBuffer_RenderBuffer( u32 address );
void FrameBuffer_RestoreBuffer( u32 address, u16 size, u16 width );
void FrameBuffer_Remove( FrameBuffer *buffer );
void FrameBuffer_RemoveIntersections( FrameBuffer *current );
void FrameBuffer_RemoveBufferForTexture( CachedTexture *texture );
void FrameBuffer_InvalidateBuffer( u32 address );
FrameBuffer *FrameBuffer_FindBuffer( u32 address );
FrameBuffer *FrameBuffer_GetBuffer( u32 startAddress );
void FrameBuffer_ActivateBufferTexture( s16 t, FrameBuffer *buffer );
void FrameBuffer_ActivateBufferTextureBG( s16 t, FrameBuffer *buffer );
#ifdef __GX__
// GLideN64's FrameBuffer::copyRdram()/isValid(). Rather than snapshot the games
// pixels, an auxiliary buffer gets four words of our own stamped over its start and
// is later asked only whether they survived. Which is what makes it immune to the
// game rewriting its own frame buffer, as OoT's pause filter seems to.
void FrameBuffer_CopyRdram( FrameBuffer *buffer );
void FrameBuffer_CopyToRDRAM( u32 sourceAddress, u32 address, u32 width, u32 height );
BOOL FrameBuffer_IsValid( FrameBuffer *buffer );
// Display list counter, bumped in gDPFullSync(). Upstream has dwnd().getBuffersSwapCount()
extern u32 FB_frame;
// Restamp any buffer starting in [start,end] after deliberately rewriting RDRAM
// underneath it (upstream does the same after copyWhiteToRDRAM()).
void FrameBuffer_RestampMarkers( u32 start, u32 end );
// TRUE while this texture's buffer is being scanned out or rendered into. The
// texture cache asks before its eviction of last resort takes a frame buffer.
BOOL FrameBuffer_IsTextureLive( const CachedTexture *texture );
// Re-capture the EFB into this buffer's texture, at most once a frame and only
// while it is the buffer being rendered into.
void FrameBuffer_RefreshCurrent( FrameBuffer *buffer );
void FrameBuffer_RemoveBottom();
void FrameBuffer_MoveToTop( FrameBuffer *newtop );
void FrameBuffer_IncrementVIcount();
#endif //__GX__

#endif
