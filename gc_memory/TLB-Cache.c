/**
 * Wii64 - TLB-Cache.c
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * Tiny TLB cache. It keeps 16 * 1024(*4) ranges of entries.
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
#include "TLB-Cache.h"
#include "../gui/DEBUG.h"

#define TLB_NUM_BLOCKS                           128
#define TLB_W_TYPE                                1
#define TLB_R_TYPE                                2
#define CACHED_TLB_ENTRIES                     1024
#define CACHED_TLB_SIZE      CACHED_TLB_ENTRIES * 4

static u32    tlb_block[TLB_NUM_BLOCKS][CACHED_TLB_ENTRIES] __attribute__((aligned(32)));
static u32    tlb_block_type[TLB_NUM_BLOCKS];
static u32    tlb_block_addr[TLB_NUM_BLOCKS];

void TLBCache_init(void){
	int i = 0, j = 0;
	for(i=0;i<TLB_NUM_BLOCKS;i++) {
		for(j=0;j<CACHED_TLB_ENTRIES;j++)
			tlb_block[i][j] = 0;
		tlb_block_type[i] = 0;
		tlb_block_addr[i] = 0;
	}
}

void TLBCache_deinit(void){
	TLBCache_init();
}

static inline int calc_block_addr(unsigned int page){
  return ((page - (page % CACHED_TLB_ENTRIES)) * 4);
}

static inline int calc_index(unsigned int page){
  return page % CACHED_TLB_ENTRIES;
}

static unsigned int TLBCache_get(int type, unsigned int page){
  int i = 0;
  // it must exist
  for(i=0; i<TLB_NUM_BLOCKS; i++) {
    if((tlb_block_type[i]==type) && (tlb_block_addr[i]==calc_block_addr(page))) {
      return tlb_block[i][calc_index(page)];
    }
  }
  return 0;
}

unsigned int TLBCache_get_r(unsigned int page){
  return TLBCache_get(TLB_R_TYPE, page);
}

unsigned int TLBCache_get_w(unsigned int page){
  return TLBCache_get(TLB_W_TYPE, page);
}

static inline void TLBCache_set(int type, unsigned int page, unsigned int val){
  int i = 0;
  // check if it exists already
  for(i=0; i<TLB_NUM_BLOCKS; i++) {
    if(tlb_block_type[i]==type && tlb_block_addr[i]==calc_block_addr(page)) {
      tlb_block[i][calc_index(page)] = val;
      return;
    }
  }
  // else, populate a tlb block
  for(i=0; i<TLB_NUM_BLOCKS; i++) {
    if(!tlb_block_type[i]) {
      tlb_block_type[i] = type;
      tlb_block_addr[i] = calc_block_addr(page);
      tlb_block[i][calc_index(page)] = val;
      return;
    }
  }
  // If we hit here, this is big trouble (just need to increase the amount of entries..)
  *(u32*)0 = 0x13370002;
}

void TLBCache_set_r(unsigned int page, unsigned int val){
  TLBCache_set(TLB_R_TYPE, page, val);
}

void TLBCache_set_w(unsigned int page, unsigned int val){
  TLBCache_set(TLB_W_TYPE, page, val);
}

#endif


