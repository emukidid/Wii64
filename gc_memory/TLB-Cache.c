/**
 * Wii64 - TLB-Cache.c
 * Copyright (C) 2007, 2008, 2009, 2026 emu_kidid
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * Tiny 2 stage TLB cache
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 * email address: emukidid@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#ifdef TINY_TLBCACHE

#include <gccore.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "tlb.h"
#include "TLB-Cache.h"
#include "../gui/DEBUG.h"

typedef unsigned long TLBEntry;

#define TLB_L1_BITS   10
#define TLB_L2_BITS   10
#define TLB_L1_SIZE   (1u << TLB_L1_BITS)
#define TLB_L2_SIZE   (1u << TLB_L2_BITS)
#define TLB_L2_MASK   (TLB_L2_SIZE - 1)

// entry contains VALID/WRITE flags + phys bits
static TLBEntry *tlb_cache_l1[TLB_L1_SIZE];
static u32 allocated = 0;

static inline void TLBCache_reset_level(TLBEntry **entry)
{
    for (u32 i = 0; i < TLB_L1_SIZE; ++i) {
        if (entry[i]) {
            free(entry[i]);
            entry[i] = NULL;
        }
    }
	allocated = 0;
}

void TLBCache_reset(void)
{
    TLBCache_reset_level(tlb_cache_l1);
}

u32 TLBCache_getSize() {
	return allocated;
}

static inline TLBEntry *TLBCache_get_slot(u32 vpn, int create)
{
    u32 l1_idx = vpn >> TLB_L2_BITS;
    u32 l2_idx = vpn & TLB_L2_MASK;

    TLBEntry *page = tlb_cache_l1[l1_idx];

    if (!page) {
        if (!create) {
            return NULL;
        }
        page = (TLBEntry *)calloc(TLB_L2_SIZE, sizeof(TLBEntry));
        if (!page) {
            // just say unmapped
            return NULL;
        }
		allocated += (TLB_L2_SIZE * sizeof(TLBEntry));
        tlb_cache_l1[l1_idx] = page;
    }

    return &page[l2_idx];
}

u32 TLBCache_get_r(u32 vpn)
{
    TLBEntry *slot = TLBCache_get_slot(vpn, 0);
    if (!slot) return 0;

    u32 e = *slot;
    if (!(e & TLB_VALID))
        return 0;

    return e;
}


u32 TLBCache_get_w(u32 vpn)
{
    TLBEntry *slot = TLBCache_get_slot(vpn, 0);
    if (!slot) return 0;

    u32 e = *slot;
    if (!(e & TLB_VALID)) return 0;
    if (!(e & TLB_WRITE)) return 0;

    return e;
}


void TLBCache_set_r(u32 vpn, u32 entry)
{
    if (!entry) {
        TLBEntry *slot = TLBCache_get_slot(vpn, 0);
        if (slot) *slot = 0;
        return;
    }

    TLBEntry *slot = TLBCache_get_slot(vpn, 1);
    if (slot) *slot = entry;
}


void TLBCache_set_w(u32 vpn, u32 entry)
{
    if (!entry) {
        TLBEntry *slot = TLBCache_get_slot(vpn, 0);
        if (slot) *slot = 0;
        return;
    }

    TLBEntry *slot = TLBCache_get_slot(vpn, 1);
    if (slot) *slot = entry;
}
#endif
