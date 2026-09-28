/**
 * glN64_GX - DepthCopy.h
 *
 * Wii64 homepage: https://www.github.com/emukidid/wii64
 *
**/

#ifndef DEPTHCOPY_H
#define DEPTHCOPY_H

#include "../main/winlnxdefs.h"
#include "Types.h"

void DepthCopy_Reset();
BOOL DepthCopy_Active();
void DepthCopy_BeginDList();
void DepthCopy_EndDList();
void DepthCopy_NoteDepthClear();
void DepthCopy_NoteDepthImageWritten();

#endif // DEPTHCOPY_H
