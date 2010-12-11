/*
Copyright (C) 2003 Rice1964

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

#include "stdafx.h"

ConvertFunction     gConvertFunctions_GX_FullTMEM[ 8 ][ 4 ] = 
{
    // 4bpp             8bpp            16bpp               32bpp
    {  Convert4b_GX,    Convert8b_GX,   Convert16b_GX,      ConvertRGBA32_GX },     // RGBA
    {  NULL,            NULL,           ConvertYUV_GX,      NULL },                 // YUV
    {  Convert4b_GX,    Convert8b_GX,   NULL,               NULL },                 // CI
    {  Convert4b_GX,    Convert8b_GX,   Convert16b_GX,      NULL },                 // IA
    {  Convert4b_GX,    Convert8b_GX,   Convert16b_GX,      NULL },                 // I
    {  NULL,            NULL,           NULL,               NULL },                 // ?
    {  NULL,            NULL,           NULL,               NULL },                 // ?
    {  NULL,            NULL,           NULL,               NULL }                  // ?
};
ConvertFunction     gConvertFunctions_GX[ 8 ][ 4 ] = 
{
    // 4bpp             8bpp            16bpp               32bpp
    {  ConvertCI4_GX,   ConvertCI8_GX,  ConvertRGBA16_GX,   ConvertRGBA32_GX },     // RGBA
    {  NULL,            NULL,           ConvertYUV_GX,      NULL },                 // YUV
    {  ConvertCI4_GX,   ConvertCI8_GX,  NULL,               NULL },                 // CI
    {  ConvertIA4_GX,   ConvertIA8_GX,  ConvertIA16_GX,     NULL },                 // IA
    {  ConvertI4_GX,    ConvertI8_GX,   ConvertIA16_GX,     NULL },                 // I
    {  NULL,            NULL,           NULL,               NULL },                 // ?
    {  NULL,            NULL,           NULL,               NULL },                 // ?
    {  NULL,            NULL,           NULL,               NULL }                  // ?
};

ConvertFunction     gConvertTlutFunctions_GX[ 8 ][ 4 ] = 
{
    // 4bpp             8bpp            16bpp               32bpp
    {  ConvertCI4_GX,   ConvertCI8_GX,  ConvertRGBA16_GX,   ConvertRGBA32_GX },     // RGBA
    {  NULL,            NULL,           ConvertYUV_GX,      NULL },                 // YUV
    {  ConvertCI4_GX,   ConvertCI8_GX,  NULL,               NULL },                 // CI
    {  ConvertCI4_GX,   ConvertCI8_GX,  ConvertIA16_GX,     NULL },                 // IA
    {  ConvertCI4_GX,   ConvertCI8_GX,  ConvertIA16_GX,     NULL },                 // I
    {  NULL,            NULL,           NULL,               NULL },                 // ?
    {  NULL,            NULL,           NULL,               NULL },                 // ?
    {  NULL,            NULL,           NULL,               NULL }                  // ?
};

extern bool conkerSwapHack;

void ConvertRGBA16_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	// Convert RGBA5551 -> GX_TF_RGB5A3
	pTexture->GXtexfmt = GX_TF_RGB5A3;

	DrawInfo dInfo;

	// Copy of the base pointer
	uint16 * pSrc = (uint16*)(tinfo.pPhysicalAddress);

	uint8 * pByteSrc = (uint8 *)pSrc;
	if (!pTexture->StartUpdate(&dInfo))
		return;

	uint32 nFiddle;

	if (tinfo.bSwapped)
	{
		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			// dwDst points to start of destination row
			uint16 * dwDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						dwDst += 4;
						continue;
					}
					if ((ty&1) == 0)
						nFiddle = (S16<<1);
					else
						nFiddle = (S16<<1) | 0x4;
			
					// dwDst points to start of destination row
					//uint16 * dwDst = (uint16 *)((uint8 *)dInfo.lpSurface + ty*dInfo.lPitch);

					// DWordOffset points to the current dword we're looking at
					// (process 2 pixels at a time). May be a problem if we don't start on even pixel
					uint32 dwWordOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad * 2) + (x * 2);

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							dwDst++;
							dwWordOffset += 2;
							continue;
						}

						uint16 w = *(uint16 *)&pByteSrc[dwWordOffset ^ nFiddle];

						*dwDst++ = GXConvert555ToRGB5A3(w);

						// Increment word offset to point to the next two pixels
						dwWordOffset += 2;
					}
				}
			}
		}
	}
	else
	{
		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			// dwDst points to start of destination row
			uint16 * dwDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						dwDst += 4;
						continue;
					}

					// DWordOffset points to the current dword we're looking at
					// (process 2 pixels at a time). May be a problem if we don't start on even pixel
					uint32 dwWordOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad * 2) + (x * 2);

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							dwDst++;
							dwWordOffset += 2;
							continue;
						}
						uint16 w = *(uint16 *)&pByteSrc[dwWordOffset ^ (S16<<1)];

						*dwDst++ = GXConvert555ToRGB5A3(w);

						// Increment word offset to point to the next two pixels
						dwWordOffset += 2;
					}
				}
			}
		}
	}

	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();
}

void ConvertRGBA32_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	// Convert RGBA8 -> GX_TF_RGBA8
	pTexture->GXtexfmt = GX_TF_RGBA8;

	DrawInfo dInfo;
	if (!pTexture->StartUpdate(&dInfo))
		return;

	uint32 * pSrc = (uint32*)(tinfo.pPhysicalAddress);

	if( options.bUseFullTMEM )
	{
		Tile &tile = gRDP.tiles[tinfo.tileNo];

		uint32 *pWordSrc;
		if( tinfo.tileNo >= 0 )
		{
			pWordSrc = (uint32*)&g_Tmem.g_Tmem64bit[tile.dwTMem];

			for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
			{
				uint16 * dwDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);
				uint32 j = 0;

				for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4, j+=16)
				{
					for (uint32 k = 0; k < 4; k++)
					{
						uint32 ty = y+k;
						if (ty >= tinfo.HeightToLoad)
						{
							j += 4;
							continue;
						}

						uint32 nFiddle = ( ty&1 )? 0x2 : 0;
						int idx = tile.dwLine*4*ty + x;

						for (uint32 l = 0; l < 4; l++, idx++, j++)
						{
							uint32 tx = x+l;
							if (tx >= tinfo.WidthToLoad)
								continue;

							uint32 w = pWordSrc[idx^nFiddle];
							//w = 0xAABBGGRR
							dwDst[j]    = (u16) (((w >> 16) & 0x0000FF00) | (w & 0x000000FF)); //0xAARR
							dwDst[j+16] = (u16) (((w >> 16) & 0x000000FF) | (w & 0x0000FF00)); //0xGGBB
						}
					}
				}
			}
		}
	}
	else
	{
		if (tinfo.bSwapped)
		{
			for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
			{
                uint16 * dwDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);
				uint32 j = 0;
				uint8 *pS = (uint8 *)pSrc;
				for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4, j+=16)
				{
					for (uint32 k = 0; k < 4; k++)
					{
						uint32 ty = y+k;
						if (ty >= tinfo.HeightToLoad)
						{
							j += 4;
							continue;
						}
						int n = (ty+tinfo.TopToLoad) * tinfo.Pitch + (tinfo.LeftToLoad*4) + x*4;

						if ((ty%2) == 0)
						{
							for (uint32 l = 0; l < 4; l++, j++, n+=4)
							{
								uint32 tx = x+l;
								if (tx >= tinfo.WidthToLoad)
									continue;

								//(u32)*pS = 0xRRGGBBAA
								dwDst[j]    = (u16) ( ((pS[(n+3)^S8]&0xFF)<<8) | (pS[(n+0)^S8]&0xFF) ); //0xAARR
								dwDst[j+16] = (u16) ( ((pS[(n+1)^S8]&0xFF)<<8) | (pS[(n+2)^S8]&0xFF) ); //0xGGBB
							}
						}
						else
						{
							for (uint32 l = 0; l < 4; l++, j++, n+=4)
							{
								uint32 tx = x+l;
								if (tx >= tinfo.WidthToLoad)
									continue;

								//(u32)*pS = 0xRRGGBBAA
								dwDst[j]    = (u16) ( ((pS[(n+3)^(0x8|S8)]&0xFF)<<8) | (pS[(n+0)^(0x8|S8)]&0xFF) ); //0xAARR
								dwDst[j+16] = (u16) ( ((pS[(n+1)^(0x8|S8)]&0xFF)<<8) | (pS[(n+2)^(0x8|S8)]&0xFF) ); //0xGGBB
							}
						}
					}
				}
			}
		}
		else
		{
			for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
			{
				uint16 * dwDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);
				uint32 j = 0;
				uint8 *pS = (uint8 *)pSrc;
				for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4, j+=16)
				{
					for (uint32 k = 0; k < 4; k++)
					{
						uint32 ty = y+k;
						if (ty >= tinfo.HeightToLoad)
						{
							j += 4;
							continue;
						}
						int n = (ty+tinfo.TopToLoad) * tinfo.Pitch + (tinfo.LeftToLoad*4) + x*4;

						for (uint32 l = 0; l < 4; l++, j++, n+=4)
						{
							uint32 tx = x+l;
							if (tx >= tinfo.WidthToLoad)
								continue;

							//(u32)*pS = 0xRRGGBBAA
							dwDst[j]    = (u16) ( ((pS[(n+3)^S8]&0xFF)<<8) | (pS[(n+0)^S8]&0xFF) ); //0xAARR
							dwDst[j+16] = (u16) ( ((pS[(n+1)^S8]&0xFF)<<8) | (pS[(n+2)^S8]&0xFF) ); //0xGGBB
						}
					}
				}
			}
		}
	}

	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();
}

// E.g. Dear Mario text
// Copy, Score etc
void ConvertIA4_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	// Convert IA4 -> GX_TF_IA4
	pTexture->GXtexfmt = GX_TF_IA4;

	DrawInfo dInfo;
	uint32 nFiddle;

	uint8 * pSrc = (uint8*)(tinfo.pPhysicalAddress);

#ifdef _DEBUG
	if (((long)pSrc) % 4) TRACE0("Texture src addr is not aligned to 4 bytes, check me");
#endif

	if (!pTexture->StartUpdate(&dInfo))
		return;

	if (tinfo.bSwapped)
	{
		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			uint8 *pDst = (uint8 *)dInfo.lpSurface + y * dInfo.lPitch;

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=8)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 8;
						continue;
					}
					// For odd lines, swap words too
					if ((y%2) == 0)
						nFiddle = S8;
					else
						nFiddle = 0x4|S8;

					uint32 dwByteOffset = (ty+tinfo.TopToLoad) * tinfo.Pitch + (tinfo.LeftToLoad/2) + (x/2);

					for (uint32 l = 0; l < 8; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+(l>>1)) ^ nFiddle];
						b = (l & 1) ? (b & 0x0F) : (b >> 4);
						*pDst++ = ((OneToFour[(b & 0x01)] << 4) | ThreeToFour[(b >> 1)]);
					}
				}
			}
		}
	}
	else
	{
		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			uint8 *pDst = (uint8 *)dInfo.lpSurface + y * dInfo.lPitch;

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=8)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 8;
						continue;
					}

					uint32 dwByteOffset = (ty+tinfo.TopToLoad) * tinfo.Pitch + (tinfo.LeftToLoad/2) + (x/2);

					for (uint32 l = 0; l < 8; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+(l>>1)) ^ S8];
						b = (l & 1) ? (b & 0x0F) : (b >> 4);
						*pDst++ = ((OneToFour[(b & 0x01)] << 4) | ThreeToFour[(b >> 1)]);
					}
				}
			}
		}
	}

	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();

}

// E.g Mario's head textures
void ConvertIA8_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	// Convert IA8 -> GX_TF_IA4
	pTexture->GXtexfmt = GX_TF_IA4;

	DrawInfo dInfo;
	uint32 nFiddle;

	uint8 * pSrc = (uint8*)(tinfo.pPhysicalAddress);

#ifdef _DEBUG
	if (((long)pSrc) % 4) TRACE0("Texture src addr is not aligned to 4 bytes, check me");
#endif

	if (!pTexture->StartUpdate(&dInfo))
		return;


	if (tinfo.bSwapped)
	{
		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			uint8 *pDst = (uint8 *)dInfo.lpSurface + y * dInfo.lPitch;

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=8)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 8;
						continue;
					}
					// For odd lines, swap words too
					if ((y%2) == 0)
						nFiddle = S8;
					else
						nFiddle = 0x4|S8;

					// Points to current byte
					uint32 dwByteOffset = (ty+tinfo.TopToLoad) * tinfo.Pitch + (tinfo.LeftToLoad/2) + x;

					for (uint32 l = 0; l < 8; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+l) ^ nFiddle];
						*pDst++ = ((b & 0xf0) >> 4) | ((b & 0x0f) << 4);
					}
				}
			}
		}
	}
    else
	{
		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			uint8 *pDst = (uint8 *)dInfo.lpSurface + y * dInfo.lPitch;

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=8)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 8;
						continue;
					}

					// Points to current byte
					uint32 dwByteOffset = (ty+tinfo.TopToLoad) * tinfo.Pitch + (tinfo.LeftToLoad/2) + x;

					for (uint32 l = 0; l < 8; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+l) ^ S8];
						*pDst++ = ((b & 0xf0) >> 4) | ((b & 0x0f) << 4);
					}
				}
			}
		}
	}

	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();

}

// E.g. camera's clouds, shadows
void ConvertIA16_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	// Convert IA16 -> GX_TF_IA8
	pTexture->GXtexfmt = GX_TF_IA8;

	DrawInfo dInfo;
	uint32 nFiddle;

	uint16 * pSrc = (uint16*)(tinfo.pPhysicalAddress);
	uint8 * pByteSrc = (uint8 *)pSrc;

	if (!pTexture->StartUpdate(&dInfo))
		return;


	if (tinfo.bSwapped)
	{
		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			uint16 * pDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 4;
						continue;
					}
					if ((y%2) == 0)
						nFiddle = (S16<<1);
					else
						nFiddle = 0x4 | (S16<<1);

					// Points to current word
					uint32 dwWordOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad * 2) + x*2;

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint16 w = *(uint16 *)&pByteSrc[dwWordOffset^nFiddle];
						*pDst++ = (w >> 8) | ((w & 0xff) << 8);

						dwWordOffset += 2;
					}
				}
			}
		}
	}
	else
	{
		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			uint16 * pDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 4;
						continue;
					}

					// Points to current word
					uint32 dwWordOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad * 2) + x*2;

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint16 w = *(uint16 *)&pByteSrc[dwWordOffset^(S16<<1)];
						*pDst++ = (w >> 8) | ((w & 0xff) << 8);

						dwWordOffset += 2;
					}
				}
			}
		}
	}

	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();
}

// Used by MarioKart
void ConvertI4_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	// Convert I4 -> GX_TF_IA4
	pTexture->GXtexfmt = GX_TF_IA4;

	DrawInfo dInfo;
	uint32 nFiddle;

	uint8 * pSrc = (uint8*)(tinfo.pPhysicalAddress);

#ifdef _DEBUG
	if (((long) pSrc) % 4) TRACE0("Texture src addr is not aligned to 4 bytes, check me");
#endif

	if (!pTexture->StartUpdate(&dInfo))
		return;

	if (tinfo.bSwapped)
	{
		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			uint8 *pDst = (uint8 *)dInfo.lpSurface + y * dInfo.lPitch;

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=8)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 8;
						continue;
					}

					// Might not work with non-even starting X
					uint32 dwByteOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad/2) + (x/2);

					// For odd lines, swap words too
					if( !conkerSwapHack || (y&4) == 0 )
					{
						if ((y%2) == 0)
							nFiddle = S8;
						else
							nFiddle = 0x4|S8;
					}
					else
					{
						if ((y%2) == 1)
							nFiddle = S8;
						else
							nFiddle = 0x4|S8;
					}

					for (uint32 l = 0; l < 8; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+(l>>1)) ^ nFiddle];
						b = (l & 1) ? (b & 0x0F) : (b >> 4);
						*pDst++ = (b << 4) | b;
					}
				}
			}
		}
		conkerSwapHack = false;
	}
	else
	{
		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			uint8 *pDst = (uint8 *)dInfo.lpSurface + y * dInfo.lPitch;

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=8)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 8;
						continue;
					}

					uint32 dwByteOffset = (ty+tinfo.TopToLoad) * tinfo.Pitch + (tinfo.LeftToLoad/2) + (x/2);

					for (uint32 l = 0; l < 8; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+(l>>1)) ^ S8];
						b = (l & 1) ? (b & 0x0F) : (b >> 4);
						*pDst++ = ((OneToFour[(b & 0x01)] << 4) | ThreeToFour[(b >> 1)]);
					}
				}
			}
		}
	}

	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();
}

// Used by MarioKart
void ConvertI8_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	// Convert I8 -> GX_TF_IA8
	pTexture->GXtexfmt = GX_TF_IA8;

	DrawInfo dInfo;
	uint32 nFiddle;

	long pSrc = (long) tinfo.pPhysicalAddress;
	if (!pTexture->StartUpdate(&dInfo))
		return;


	if (tinfo.bSwapped)
	{
		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			uint8 *pDst = (uint8 *)dInfo.lpSurface + y * dInfo.lPitch;

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=8)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 8;
						continue;
					}
					if ((y%2) == 0)
						nFiddle = S8;
					else
						nFiddle = 0x4|S8;

					uint32 dwByteOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad + x;

					for (uint32 l = 0; l < 8; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = *(uint8*)((pSrc+dwByteOffset+l)^nFiddle);
						*pDst++ = ((uint16)b << 8) | b;
					}
				}
			}
		}
	}
	else
	{
		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			uint16 * pDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 4;
						continue;
					}

					uint32 dwByteOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad + x;

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = *(uint8*)((pSrc+dwByteOffset)^S8);
						*pDst++ = ((uint16)b << 8) | b;

						dwByteOffset++;
					}
				}
			}
		}
	}

	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();

}

//*****************************************************************************
// Convert CI4 images. We need to switch on the palette type
//*****************************************************************************
void    ConvertCI4_GX( CTexture * p_texture, const TxtrInfo & tinfo )
{
    if ( tinfo.TLutFmt == TLUT_FMT_RGBA16 )
    {
        ConvertCI4_RGBA16_GX( p_texture, tinfo );  
    }
    else if ( tinfo.TLutFmt == TLUT_FMT_IA16 )
    {
        ConvertCI4_IA16_GX( p_texture, tinfo );                    
    }
}

//*****************************************************************************
// Convert CI8 images. We need to switch on the palette type
//*****************************************************************************
void    ConvertCI8_GX( CTexture * p_texture, const TxtrInfo & tinfo )
{
    if ( tinfo.TLutFmt == TLUT_FMT_RGBA16 )
    {
        ConvertCI8_RGBA16_GX( p_texture, tinfo );  
    }
    else if ( tinfo.TLutFmt == TLUT_FMT_IA16 )
    {
        ConvertCI8_IA16_GX( p_texture, tinfo );                    
    }
}

// Used by Starfox intro
void ConvertCI4_RGBA16_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	// Convert RGBA16 -> GX_TF_RGB5A3
	pTexture->GXtexfmt = GX_TF_RGB5A3;

	DrawInfo dInfo;
	uint32 nFiddle;

	uint8 * pSrc = (uint8*)(tinfo.pPhysicalAddress);
	uint16 * pPal = (uint16 *)tinfo.PalAddress;
	bool bIgnoreAlpha = (tinfo.TLutFmt==TLUT_FMT_NONE);

	if (!pTexture->StartUpdate(&dInfo))
		return;

	if (tinfo.bSwapped)
	{
		for (uint32 y = 0; y <  tinfo.HeightToLoad; y+=4)
		{
			uint16 * pDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 4;
						continue;
					}
					if ((ty%2) == 0)
						nFiddle = S8;
					else
						nFiddle = 0x4|S8;

					uint32 dwByteOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + (x/2);

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+(l>>1)) ^ nFiddle];
						b = (l & 1) ? (b & 0x0f) : (b >> 4);

						if( bIgnoreAlpha )
							*pDst++ = GXConvert555ToRGB5A3((pPal[b^S16] | 0x0001));	// Remember palette is in different endian order!
						else
							*pDst++ = GXConvert555ToRGB5A3(pPal[b^S16]);	// Remember palette is in different endian order!
					}
				}
			}
		}
	}
	else
	{
		for (uint32 y = 0; y <  tinfo.HeightToLoad; y+=4)
		{
			uint16 * pDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 4;
						continue;
					}
					uint32 dwByteOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad/2) + (x/2);

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+(l>>1)) ^ S8];
						b = (l & 1) ? (b & 0x0f) : (b >> 4);

						if( bIgnoreAlpha )
							*pDst++ = GXConvert555ToRGB5A3((pPal[b^S16] | 0x0001));	// Remember palette is in different endian order!
						else
							*pDst++ = GXConvert555ToRGB5A3(pPal[b^S16]);	// Remember palette is in different endian order!
					}
				}
			}
		}
	}
	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();
}

// Used by Starfox intro
void ConvertCI4_IA16_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	// Convert IA16 -> GX_TF_IA8
	pTexture->GXtexfmt = GX_TF_IA8;

	DrawInfo dInfo;
	uint32 nFiddle;

	uint8 * pSrc = (uint8*)(tinfo.pPhysicalAddress);

#ifdef _DEBUG
	if (((long) pSrc) % 4) TRACE0("Texture src addr is not aligned to 4 bytes, check me");
#endif

	uint16 * pPal = (uint16 *)tinfo.PalAddress;
	bool bIgnoreAlpha = (tinfo.TLutFmt==TLUT_FMT_UNKNOWN);

	if (!pTexture->StartUpdate(&dInfo))
	return;

	if (tinfo.bSwapped)
	{
		for (uint32 y = 0; y <  tinfo.HeightToLoad; y+=4)
		{
			uint16 * pDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 4;
						continue;
					}
					if ((ty%2) == 0)
						nFiddle = S8;
					else
						nFiddle = 0x4|S8;

					uint32 dwByteOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad/2) + (x/2);

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+(l>>1)) ^ nFiddle];
						b = (l & 1) ? (b & 0x0f) : (b >> 4);

						uint16 w = pPal[b^S16];	// Remember palette is in different endian order!
						if( bIgnoreAlpha )
							*pDst++ = (w >> 8) | 0xff00;
						else
							*pDst++ = (w >> 8) | ((w & 0xff) << 8);
					}
				}
			}
		}
	}
	else
	{
		for (uint32 y = 0; y <  tinfo.HeightToLoad; y+=4)
		{
			uint16 * pDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 4;
						continue;
					}
					uint32 dwByteOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad/2) + (x/2);

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+(l>>1)) ^ S8];
						b = (l & 1) ? (b & 0x0f) : (b >> 4);

						uint16 w = pPal[b^S16];	// Remember palette is in different endian order!
						if( bIgnoreAlpha )
							*pDst++ = (w >> 8) | 0xff00;
						else
							*pDst++ = (w >> 8) | ((w & 0xff) << 8);
					}
				}
			}
		}
	}
	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();
}

// Used by MarioKart for Cars etc
void ConvertCI8_RGBA16_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	// Convert RGBA16 -> GX_TF_RGB5A3
	pTexture->GXtexfmt = GX_TF_RGB5A3;

	DrawInfo dInfo;
	uint32 nFiddle;

	uint8 * pSrc = (uint8*)(tinfo.pPhysicalAddress);

#ifdef _DEBUG
	if (((long) pSrc) % 4) TRACE0("Texture src addr is not aligned to 4 bytes, check me");
#endif

	uint16 * pPal = (uint16 *)tinfo.PalAddress;
	bool bIgnoreAlpha = (tinfo.TLutFmt==TLUT_FMT_NONE);

	if (!pTexture->StartUpdate(&dInfo))
		return;

	if (tinfo.bSwapped)
	{
		for (uint32 y = 0; y <  tinfo.HeightToLoad; y+=4)
		{
			uint16 * pDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 4;
						continue;
					}
					if ((ty%2) == 0)
						nFiddle = S8;
					else
						nFiddle = 0x4|S8;

					uint32 dwByteOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad + x;

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+l) ^ nFiddle];

						if( bIgnoreAlpha )
							*pDst++ = GXConvert555ToRGB5A3((pPal[b^S16] | 0x0001));	// Remember palette is in different endian order!
						else
							*pDst++ = GXConvert555ToRGB5A3(pPal[b^S16]);	// Remember palette is in different endian order!
					}
				}
			}
		}
	}
	else
	{
		for (uint32 y = 0; y <  tinfo.HeightToLoad; y+=4)
		{
			uint16 * pDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 4;
						continue;
					}
					uint32 dwByteOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad + x;

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+l) ^ S8];

						if( bIgnoreAlpha )
							*pDst++ = GXConvert555ToRGB5A3((pPal[b^S16] | 0x0001));	// Remember palette is in different endian order!
						else
							*pDst++ = GXConvert555ToRGB5A3(pPal[b^S16]);	// Remember palette is in different endian order!
					}
				}
			}
		}
	}
	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();
}

// Used by MarioKart for Cars etc
void ConvertCI8_IA16_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	// Convert IA16 -> GX_TF_IA8
	pTexture->GXtexfmt = GX_TF_IA8;

	DrawInfo dInfo;
	uint32 nFiddle;

	uint8 * pSrc = (uint8*)(tinfo.pPhysicalAddress);

#ifdef _DEBUG
	if (((long) pSrc) % 4) TRACE0("Texture src addr is not aligned to 4 bytes, check me");
#endif

	uint16 * pPal = (uint16 *)tinfo.PalAddress;
	bool bIgnoreAlpha = (tinfo.TLutFmt==TLUT_FMT_UNKNOWN);

	if (!pTexture->StartUpdate(&dInfo))
		return;

	if (tinfo.bSwapped)
	{
		for (uint32 y = 0; y <  tinfo.HeightToLoad; y+=4)
		{
			uint16 * pDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 4;
						continue;
					}
					if ((ty%2) == 0)
						nFiddle = S8;
					else
						nFiddle = 0x4|S8;

					uint32 dwByteOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad + x;

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+l) ^ nFiddle];

						uint16 w = pPal[b^S16];	// Remember palette is in different endian order!
						if( bIgnoreAlpha )
							*pDst++ = (w >> 8) | 0xff00;
						else
							*pDst++ = (w >> 8) | ((w & 0xff) << 8);
					}
				}
			}
		}
	}
	else
	{
		for (uint32 y = 0; y <  tinfo.HeightToLoad; y+=4)
		{
			uint16 * pDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						pDst += 4;
						continue;
					}
					uint32 dwByteOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad + x;

					for (uint32 l = 0; l < 4; l++)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
						{
							pDst++;
							continue;
						}
						uint8 b = pSrc[(dwByteOffset+l) ^ S8];

						uint16 w = pPal[b^S16];	// Remember palette is in different endian order!
						if( bIgnoreAlpha )
							*pDst++ = (w >> 8) | 0xff00;
						else
							*pDst++ = (w >> 8) | ((w & 0xff) << 8);
					}
				}
			}
		}
	}
	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();
}

void ConvertYUV_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	// Convert YUV16 -> GX_TF_RGBA8
	pTexture->GXtexfmt = GX_TF_RGBA8;

	DrawInfo dInfo;
	if (!pTexture->StartUpdate(&dInfo))
		return;

	uint32 nFiddle;

	if( options.bUseFullTMEM )
	{
		Tile &tile = gRDP.tiles[tinfo.tileNo];

		uint16 * pSrc;
		if( tinfo.tileNo >= 0 )
			pSrc = (uint16*)&g_Tmem.g_Tmem64bit[tile.dwTMem];
		else
			pSrc = (uint16*)(tinfo.pPhysicalAddress);

		uint8 * pByteSrc = (uint8 *)pSrc;

		for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
		{
			uint16 * dwDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);
			uint32 j = 0;

			for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4, j+=16)
			{
				for (uint32 k = 0; k < 4; k++)
				{
					uint32 ty = y+k;
					if (ty >= tinfo.HeightToLoad)
					{
						j += 4;
						continue;
					}
					nFiddle = ( ty&1 )? 0x4 : 0;
					int dwWordOffset = tinfo.tileNo>=0? tile.dwLine*8*ty + (2*x) : ((ty+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad * 2) + (2*x);

					for (uint32 l = 0; l < 4; l+=2, dwWordOffset+=4, j+=2)
					{
						uint32 tx = x+l;
						if (tx >= tinfo.WidthToLoad)
							continue;

						int y0 = *(uint8*)&pByteSrc[(dwWordOffset+(2^S8))^nFiddle];
						int y1 = *(uint8*)&pByteSrc[(dwWordOffset+(0^S8))^nFiddle];
						int u0 = *(uint8*)&pByteSrc[(dwWordOffset+(3^S8))^nFiddle];
						int v0 = *(uint8*)&pByteSrc[(dwWordOffset+(1^S8))^nFiddle];

						uint32 w = ConvertYUV16ToR8G8B8_GX(y0,u0,v0);
						//w = 0xAARRGGBB
						dwDst[j]    = (u16) ((w >> 16) & 0x0000FFFF); //0xAARR
						dwDst[j+16] = (u16) (w & 0x0000FFFF); //0xGGBB
						w = ConvertYUV16ToR8G8B8_GX(y1,u0,v0);
						dwDst[j+1]  = (u16) ((w >> 16) & 0x0000FFFF); //0xAARR
						dwDst[j+17] = (u16) (w & 0x0000FFFF); //0xGGBB
					}
				}
			}
		}
	}
	else
	{
		uint16 * pSrc = (uint16*)(tinfo.pPhysicalAddress);
		uint8 * pByteSrc = (uint8 *)pSrc;

		if (tinfo.bSwapped)
		{
			for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
			{
                uint16 * dwDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);
				uint32 j = 0;
				for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4, j+=16)
				{
					for (uint32 k = 0; k < 4; k++)
					{
						uint32 ty = y+k;
						if (ty >= tinfo.HeightToLoad)
						{
							j += 4;
							continue;
						}
						if ((ty&1) == 0)
							nFiddle = S8;
						else
							nFiddle = 0x4|S8;

						// DWordOffset points to the current dword we're looking at
						// (process 2 pixels at a time). May be a problem if we don't start on even pixel
						uint32 dwWordOffset = ((ty+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad*2) + (x*2);

						for (uint32 l = 0; l < 4; l+=2, dwWordOffset+=4, j+=2)
						{
							uint32 tx = x+l;
							if (tx >= tinfo.WidthToLoad)
								continue;

							int y0 = *(uint8*)&pByteSrc[(dwWordOffset+(3^S8))^nFiddle];
							int v0 = *(uint8*)&pByteSrc[(dwWordOffset+(2^S8))^nFiddle];
							int y1 = *(uint8*)&pByteSrc[(dwWordOffset+(1^S8))^nFiddle];
							int u0 = *(uint8*)&pByteSrc[(dwWordOffset+(0^S8))^nFiddle];

							uint32 w = ConvertYUV16ToR8G8B8_GX(y0,u0,v0);
							//w = 0xAARRGGBB
							dwDst[j]    = (u16) ((w >> 16) & 0x0000FFFF); //0xAARR
							dwDst[j+16] = (u16) (w & 0x0000FFFF); //0xGGBB
							w = ConvertYUV16ToR8G8B8_GX(y1,u0,v0);
							dwDst[j+1]  = (u16) ((w >> 16) & 0x0000FFFF); //0xAARR
							dwDst[j+17] = (u16) (w & 0x0000FFFF); //0xGGBB
						}
					}
				}
			}
		}
		else
		{
			for (uint32 y = 0; y < tinfo.HeightToLoad; y+=4)
			{
				uint16 * dwDst = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);
				uint32 j = 0;
				for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4, j+=16)
				{
					for (uint32 k = 0; k < 4; k++)
					{
						uint32 ty = y+k;
						if (ty >= tinfo.HeightToLoad)
						{
							j += 4;
							continue;
						}
						uint32 dwByteOffset = (ty * 32) + (x*2);

						for (uint32 l = 0; l < 4; l+=2, dwByteOffset+=4, j+=2)
						{
							uint32 tx = x+l;
							if (tx >= tinfo.WidthToLoad)
								continue;

							int y0 = *(uint8*)&pByteSrc[dwByteOffset+(3^S8)];
							int v0 = *(uint8*)&pByteSrc[dwByteOffset+(2^S8)];
							int y1 = *(uint8*)&pByteSrc[dwByteOffset+(1^S8)];
							int u0 = *(uint8*)&pByteSrc[dwByteOffset+(0^S8)];

							uint32 w = ConvertYUV16ToR8G8B8_GX(y0,u0,v0);
							//w = 0xAARRGGBB
							dwDst[j]    = (u16) ((w >> 16) & 0x0000FFFF); //0xAARR
							dwDst[j+16] = (u16) (w & 0x0000FFFF); //0xGGBB
							w = ConvertYUV16ToR8G8B8_GX(y1,u0,v0);
							dwDst[j+1]  = (u16) ((w >> 16) & 0x0000FFFF); //0xAARR
							dwDst[j+17] = (u16) (w & 0x0000FFFF); //0xGGBB
						}
					}
				}
			}
		}
	}

	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();
}

uint32 ConvertYUV16ToR8G8B8_GX(int Y, int U, int V)
{
    uint32 A= 1;

    /*
    int R = int(g_convc0 *(Y-16) + g_convc1 * V);
    int G = int(g_convc0 *(Y-16) + g_convc2 * U - g_convc3 * V);
    int B = int(g_convc0 *(Y-16) + g_convc4 * U);
    */

    int R = int(Y + (1.370705f * (V-128)));
    int G = int(Y - (0.698001f * (V-128)) - (0.337633f * (U-128)));
    int B = int(Y + (1.732446f * (U-128)));

    R = R<0 ? 0 : R;
    G = G<0 ? 0 : G;
    B = B<0 ? 0 : B;

    uint32 R2 = R>255 ? 255 : R;
    uint32 G2 = G>255 ? 255 : G;
    uint32 B2 = B>255 ? 255 : B;

    return COLOR_RGBA(R2, G2, B2, 0xFF*A);
//#define COLOR_RGBA(r,g,b,a) (((r&0xFF)<<16) | ((g&0xFF)<<8) | ((b&0xFF)<<0) | ((a&0xFF)<<24))
}

// Used by Starfox intro
void Convert4b_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	uint8 GXtileX;

	if( gRDP.otherMode.text_tlut>=2 || ( tinfo.Format != TXT_FMT_IA && tinfo.Format != TXT_FMT_I) )
	{
		GXtileX = 4;
		if( tinfo.TLutFmt == TLUT_FMT_IA16 )
			pTexture->GXtexfmt = GX_TF_IA8;
		else
			pTexture->GXtexfmt = GX_TF_RGB5A3;
	}
	else
	{
		GXtileX = 8;
		pTexture->GXtexfmt = GX_TF_IA4;
	}

	DrawInfo dInfo;
	if (!pTexture->StartUpdate(&dInfo)) 
		return;

	uint16 * pPal = (uint16 *)tinfo.PalAddress;
	bool bIgnoreAlpha = (tinfo.TLutFmt==TLUT_FMT_UNKNOWN);
	if( tinfo.Format <= TXT_FMT_CI ) bIgnoreAlpha = (tinfo.TLutFmt==TLUT_FMT_NONE);

	Tile &tile = gRDP.tiles[tinfo.tileNo];

	uint8 *pByteSrc = tinfo.tileNo >= 0 ? (uint8*)&g_Tmem.g_Tmem64bit[tile.dwTMem] : (uint8*)(tinfo.pPhysicalAddress);

	for (uint32 y = 0; y <  tinfo.HeightToLoad; y+=4)
	{
		uint16 * pDst16 = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);
		uint8 * pDst8 = (uint8 *)dInfo.lpSurface + y*dInfo.lPitch;

		for (uint32 x = 0; x < tinfo.WidthToLoad; x+=GXtileX)
		{
			for (uint32 k = 0; k < 4; k++)
			{
				uint32 ty = y+k;
				if (ty >= tinfo.HeightToLoad)
				{
					pDst16 += 4;
					pDst8 += 8;
					continue;
				}
				uint32 nFiddle;
				if( tinfo.tileNo < 0 )  
				{
					if (tinfo.bSwapped)
					{
						if ((ty%2) == 0)
							nFiddle = S8;
						else
							nFiddle = 0x4|S8;
					}
					else
					{
						nFiddle = S8;
					}
				}
				else
				{
					nFiddle = ( ty&1 )? 0x4 : 0;
				}

		        int idx = tinfo.tileNo>=0 ? tile.dwLine*8*ty  + (x/2): ((ty+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad/2) + (x/2);

				for (uint32 l = 0; l < GXtileX; l++)
				{
					uint32 tx = x+l;
					if (tx >= tinfo.WidthToLoad)
					{
						pDst16++;
						pDst8++;
						continue;
					}
					uint8 b = pByteSrc[(idx+(l>>1)) ^ nFiddle];
					b = (l & 1) ? (b & 0x0F) : (b >> 4);

					if( gRDP.otherMode.text_tlut>=2 || ( tinfo.Format != TXT_FMT_IA && tinfo.Format != TXT_FMT_I) )
					{
						if( tinfo.TLutFmt == TLUT_FMT_IA16 )
						{
							if( tinfo.tileNo>=0 )
							{
								uint16 w = g_Tmem.g_Tmem16bit[0x400+tinfo.Palette*0x40+(b<<2)];
								if( bIgnoreAlpha )
									*pDst16++ = (w >> 8) | 0xff00;
								else
									*pDst16++ = (w >> 8) | ((w & 0xff) << 8);
							}
							else
							{
								uint16 w = pPal[b^S16];
								if( bIgnoreAlpha )
									*pDst16++ = (w >> 8) | 0xff00;
								else
									*pDst16++ = (w >> 8) | ((w & 0xff) << 8);
							}
						}
						else
						{
							if( tinfo.tileNo>=0 )
							{
								if( bIgnoreAlpha )
									*pDst16++ = GXConvert555ToRGB5A3((g_Tmem.g_Tmem16bit[0x400+tinfo.Palette*0x40+(b<<2)] | 0x0001));
								else
									*pDst16++ = GXConvert555ToRGB5A3(g_Tmem.g_Tmem16bit[0x400+tinfo.Palette*0x40+(b<<2)]);
							}
							else
							{
								if( bIgnoreAlpha )
									*pDst16++ = GXConvert555ToRGB5A3((pPal[b^S16] | 0x0001));
								else
									*pDst16++ = GXConvert555ToRGB5A3(pPal[b^S16]);
							}
						}
					}
					else if( tinfo.Format == TXT_FMT_IA )
					{
						if( bIgnoreAlpha )
							*pDst8++ = (0xf0 | ThreeToFour[(b >> 1)]);
						else
							*pDst8++ = ((OneToFour[(b & 0x01)] << 4) | ThreeToFour[(b >> 1)]);
					}
					else    // if( tinfo.Format == TXT_FMT_I )
					{
						if( bIgnoreAlpha )
							*pDst8++ = 0xf0 | b;
						else
							*pDst8++ = (b << 4) | b;
					}
				}
			}
		}
	}

	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();
}

void Convert8b_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	uint8 GXtileX;

	if( gRDP.otherMode.text_tlut>=2 || ( tinfo.Format != TXT_FMT_IA && tinfo.Format != TXT_FMT_I) )
	{
		GXtileX = 4;
		if( tinfo.TLutFmt == TLUT_FMT_IA16 )
			pTexture->GXtexfmt = GX_TF_IA8;
		else
			pTexture->GXtexfmt = GX_TF_RGB5A3;
	}
	else if( tinfo.Format == TXT_FMT_IA )
	{
		GXtileX = 8;
		pTexture->GXtexfmt = GX_TF_IA4;
	}
	else    // if( tinfo.Format == TXT_FMT_I )
	{
		GXtileX = 4;
		pTexture->GXtexfmt = GX_TF_IA8;
	}

	DrawInfo dInfo;
	if (!pTexture->StartUpdate(&dInfo)) 
		return;

	uint16 * pPal = (uint16 *)tinfo.PalAddress;
	bool bIgnoreAlpha = (tinfo.TLutFmt==TLUT_FMT_UNKNOWN);
	if( tinfo.Format <= TXT_FMT_CI ) bIgnoreAlpha = (tinfo.TLutFmt==TLUT_FMT_NONE);

	Tile &tile = gRDP.tiles[tinfo.tileNo];

	uint8 *pByteSrc;
	if( tinfo.tileNo >= 0 )
	{
		pByteSrc = (uint8*)&g_Tmem.g_Tmem64bit[tile.dwTMem];
	}
	else
	{
		pByteSrc = (uint8*)(tinfo.pPhysicalAddress);
	}

	for (uint32 y = 0; y <  tinfo.HeightToLoad; y+=4)
	{
		uint16 * pDst16 = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);
		uint8 * pDst8 = (uint8 *)dInfo.lpSurface + y*dInfo.lPitch;

		for (uint32 x = 0; x < tinfo.WidthToLoad; x+=GXtileX)
		{
			for (uint32 k = 0; k < 4; k++)
			{
				uint32 ty = y+k;
				if (ty >= tinfo.HeightToLoad)
				{
					pDst16 += 4;
					pDst8 += 8;
					continue;
				}
				uint32 nFiddle;
				if( tinfo.tileNo < 0 )  
				{
					if (tinfo.bSwapped)
					{
						if ((ty%2) == 0)
							nFiddle = S8;
						else
							nFiddle = 0x4|S8;
					}
					else
					{
						nFiddle = S8;
					}
				}
				else
				{
					nFiddle = ( ty&1 )? 0x4 : 0;
				}

		        int idx = tinfo.tileNo>=0 ? tile.dwLine*8*ty  + x : ((ty+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad + x;

				for (uint32 l = 0; l < GXtileX; l++)
				{
					uint32 tx = x+l;
					if (tx >= tinfo.WidthToLoad)
					{
						pDst16++;
						pDst8++;
						continue;
					}
					uint8 b = pByteSrc[(idx+l) ^ nFiddle];

					if( gRDP.otherMode.text_tlut>=2 || ( tinfo.Format != TXT_FMT_IA && tinfo.Format != TXT_FMT_I) )
					{
						if( tinfo.TLutFmt == TLUT_FMT_IA16 )
						{
							if( tinfo.tileNo>=0 )
							{
								uint16 w = g_Tmem.g_Tmem16bit[0x400+(b<<2)];
								if( bIgnoreAlpha )
									*pDst16++ = (w >> 8) | 0xff00;
								else
									*pDst16++ = (w >> 8) | ((w & 0xff) << 8);
							}
							else
							{
								uint16 w = pPal[b^S16];
								if( bIgnoreAlpha )
									*pDst16++ = (w >> 8) | 0xff00;
								else
									*pDst16++ = (w >> 8) | ((w & 0xff) << 8);
							}
						}
						else
						{
							if( tinfo.tileNo>=0 )
							{
								if( bIgnoreAlpha )
									*pDst16++ = GXConvert555ToRGB5A3((g_Tmem.g_Tmem16bit[0x400+(b<<2)] | 0x0001));
								else
									*pDst16++ = GXConvert555ToRGB5A3(g_Tmem.g_Tmem16bit[0x400+(b<<2)]);
							}
							else
							{
								if( bIgnoreAlpha )
									*pDst16++ = GXConvert555ToRGB5A3((pPal[b^S16] | 0x0001));
								else
									*pDst16++ = GXConvert555ToRGB5A3(pPal[b^S16]);
							}
						}
					}
					else if( tinfo.Format == TXT_FMT_IA )
					{
						if( bIgnoreAlpha )
							*pDst8++ = ((b & 0xf0) >> 4) | 0xf0;
						else
							*pDst8++ = ((b & 0xf0) >> 4) | ((b & 0x0f) << 4);
					}
					else    // if( tinfo.Format == TXT_FMT_I )
					{
						if( bIgnoreAlpha )
							*pDst16++ = 0xff00 | b;
						else
							*pDst16++ = ((uint16)b << 8) | b;
					}
				}
			}
		}
	}

	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();
}


void Convert16b_GX(CTexture *pTexture, const TxtrInfo &tinfo)
{
	if( tinfo.Format == TXT_FMT_RGBA )
		pTexture->GXtexfmt = GX_TF_RGB5A3;
	else if( tinfo.Format == TXT_FMT_YUV ) {}
	else if( tinfo.Format >= TXT_FMT_IA )
		pTexture->GXtexfmt = GX_TF_IA8;

	DrawInfo dInfo;
	if (!pTexture->StartUpdate(&dInfo)) 
		return;

	Tile &tile = gRDP.tiles[tinfo.tileNo];

	uint16 *pWordSrc;
	if( tinfo.tileNo >= 0 )
		pWordSrc = (uint16*)&g_Tmem.g_Tmem64bit[tile.dwTMem];
	else
		pWordSrc = (uint16*)(tinfo.pPhysicalAddress);

	for (uint32 y = 0; y <  tinfo.HeightToLoad; y+=4)
	{
		uint16 * pDst16 = (uint16 *)((uint8 *)dInfo.lpSurface + y*dInfo.lPitch);

		for (uint32 x = 0; x < tinfo.WidthToLoad; x+=4)
		{
			for (uint32 k = 0; k < 4; k++)
			{
				uint32 ty = y+k;
				if (ty >= tinfo.HeightToLoad)
				{
					pDst16 += 4;
					continue;
				}
				uint32 nFiddle;
				if( tinfo.tileNo < 0 )  
				{
					if (tinfo.bSwapped)
					{
						if ((ty&1) == 0)
							nFiddle = S16;
						else
							nFiddle = 0x2|S16;
					}
					else
					{
						nFiddle = S16;
					}
				}
				else
				{
					nFiddle = ( ty&1 )? 0x2 : 0;
				}

				int idx = tinfo.tileNo>=0? tile.dwLine*4*ty + x: (((ty+tinfo.TopToLoad) * tinfo.Pitch)>>1) + tinfo.LeftToLoad + x;

				for (uint32 l = 0; l < 4; l++)
				{
					uint32 tx = x+l;
					if (tx >= tinfo.WidthToLoad)
					{
						pDst16++;
						continue;
					}
					uint16 w = pWordSrc[(idx+l)^nFiddle];
					uint16 w2 = tinfo.tileNo>=0? ((w>>8)|(w<<8)) : w;

					if( tinfo.Format == TXT_FMT_RGBA )
						*pDst16++ = GXConvert555ToRGB5A3(w2);
					else if( tinfo.Format == TXT_FMT_YUV ) {}
					else if( tinfo.Format >= TXT_FMT_IA )
						*pDst16++ = (w2 >> 8) | ((w2 & 0xff) << 8);
				}
			}
		}
	}

	pTexture->EndUpdate(&dInfo);
	pTexture->SetOthersVariables();
}
