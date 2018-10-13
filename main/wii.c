#include <ogc/ipc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <ogc/machine/processor.h>

#define _SHIFTR(v, s, w)	\
	((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))

#ifdef HW_RVL
f32 SYS_GetCoreMultiplier()
{
	u32 pvr,hid1;

	const f32 pll_cfg_table4[] = {
		 2.5,  7.5,  7.0,  1.0,
		 2.0,  6.5, 10.0,  4.5,
		 3.0,  5.5,  4.0,  5.0,
		 8.0,  6.0,  3.5,  0.0
	};
	const f32 pll_cfg_table5[] = {
		 0.0,  0.0,  5.0,  1.0,  2.0,  2.5,  3.0,  3.5,  4.0,  4.5,
		 5.0,  5.5,  6.0,  6.5,  7.0,  7.5,  8.0,  8.5,  9.0,  9.5,
		10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0,
		20.0,  0.0
	};

	pvr = mfpvr();
	hid1 = mfhid1();
	switch(_SHIFTR(pvr,20,12)) {
		case 0x000:
			switch(_SHIFTR(pvr,16,4)) {
				case 0x8:
					switch(_SHIFTR(pvr,12,4)) {
						case 0x7:
							return pll_cfg_table5[_SHIFTR(hid1,27,5)];
						default:
							return pll_cfg_table4[_SHIFTR(hid1,28,4)];
					}
					break;
			}
			break;
		case 0x700:
			return pll_cfg_table5[_SHIFTR(hid1,27,5)];
	}
	return 0.0;
}

u32 SYS_GetCoreFrequency()
{
	return (f64)(TB_BUS_CLOCK)*SYS_GetCoreMultiplier();
}
#endif
