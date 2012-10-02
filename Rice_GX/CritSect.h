/*
Rice_GX - CritSect.h
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

#if !defined(CRITSECT_H)
#define CRITSECT_H

#ifndef __GX__
#include <SDL.h>
#endif //__GX__

class CCritSect
{
 public:
   CCritSect()
     {
#ifndef __GX__
    cs = SDL_CreateMutex();
#else //!__GX__
    LWP_MutexInit(&cs,GX_FALSE);
#endif //__GX__
    locked = 0;
     }
   
   ~CCritSect()
     {
#ifndef __GX__
    SDL_DestroyMutex(cs);
#else //!__GX__
    LWP_MutexDestroy(cs);
#endif //__GX__
     }
   
   void Lock()
     {
#ifndef __GX__
    SDL_LockMutex(cs);
#else //!__GX__
    LWP_MutexLock(cs);
#endif //__GX__
    locked = 1;
     }
   
   void Unlock()
     {
    locked = 0;
#ifndef __GX__
    SDL_UnlockMutex(cs);
#else //!__GX__
	LWP_MutexUnlock(cs);
#endif //__GX__
     }
   
   bool IsLocked()
     {
    return (locked != 0);
     }
   
 protected:
#ifndef __GX__
   SDL_mutex *cs;
#else //!__GX__
   mutex_t cs;
#endif //__GX__
   int locked;
};

#endif // !defined(CRITSECT_H)

