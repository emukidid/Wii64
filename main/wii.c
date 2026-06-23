#include <ogc/ipc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <ogc/machine/processor.h>

#define _SHIFTR(v, s, w)	\
	((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))
