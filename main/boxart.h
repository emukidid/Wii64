/* boxart.h 
	- Boxart texture loading from a binary compilation
	by emu_kidid
 */

#ifndef BOXART_H
#define BOXART_H

#define BOXART_TEX_WD 	120
#define BOXART_TEX_HT 	84
#define BOXART_TEX_BPP 	2
#define BOXART_TEX_SIZE	(BOXART_TEX_WD*BOXART_TEX_HT*BOXART_TEX_BPP)
#define BOXART_TEX_FMT 	GX_TF_RGB565

void BOXART_Init();
void BOXART_DeInit();
void BOXART_LoadTexture(u32 CRC, char *buffer);

#endif

