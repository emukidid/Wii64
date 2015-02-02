/* Invalid_Code.c - Uses 1/8th the memory as the char hash table
   by Mike Slegeir for Mupen64-GC / MEM2 ver by emu_kidid
 */

#include "Invalid_Code.h"
#ifndef HW_RVL
#include "../gc_memory/ARAM.h"
#else
#include "../gc_memory/MEM2.h"
#endif
unsigned char *const invalid_code = (unsigned char *)(INVCODE_LO);

int inline invalid_code_get(int block_num){
	return invalid_code[block_num];
}

void inline invalid_code_set(int block_num, int value){
	invalid_code[block_num] = value;
}


