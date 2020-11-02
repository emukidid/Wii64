#include <ogc/ipc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <ogc/machine/processor.h>

#define _SHIFTR(v, s, w)	\
	((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))

#ifdef HW_RVL
u32 SYS_GetCoreFrequency()
{
	return (f64)(TB_BUS_CLOCK)*SYS_GetCoreMultiplier();
}
#endif
