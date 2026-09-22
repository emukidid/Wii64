/**
 * glN64_GX - Config.h
 * Copyright (C) 2003 Orkin
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 *
**/

#ifndef CONFIG_H
#define CONFIG_H

#include "Types.h"

struct Config
{
	struct
	{
		u32 hacks;
	} generalEmulation;
};

extern Config config;

#define hack_subscreen				(1<<6)  //Fix subscreen delay in Zelda OOT and Doubutsu no Mori
#define hack_clearAloneDepthBuffer	(1<<3)  //Force clear depth buffer if there is no frame buffer for it. Multiplayer in GE and PD.
#define hack_rectDepthBufferCopyPD	(1<<8)  //Perfect Dark's depth buffer reads, which drive its light coronas
#define hack_fbTextureOffset		(1<<23) //Offset Conker's shadow in CBFD and the Bob-ombs in Mario Tennis
#define hack_fbCopyToRDRAM			(1<<24) //Write the EFB back to RDRAM for real. Majora's Mask pictograph
#define hack_doNotResetOtherModeH	(1<<14) //Don't reset othermode.h at dlist start. Quake and Quake 2
#define hack_doNotResetOtherModeL	(1<<15) //Don't reset othermode.l at dlist start. Quake

void Config_LoadConfig();
void Config_DoConfig();
#endif
