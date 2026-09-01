/**
 * Mupen64 - interupt.c
 * Copyright (C) 2002 Hacktarux
 *               2010 emu_kidid
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *                emukidid@gmail.com
 * 
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
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
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#include <stdio.h>
#include <stdlib.h>
#include "r4300.h"
#include "macros.h"
#include "interupt.h"
#include "../gc_memory/dma.h"

//Non-zero while an RSP task is locked
unsigned int interrupt_unsafe_state = 0;
#include "exception.h"
#include "../config.h"
#include "../main/plugin.h"
#include "../main/guifuncs.h"
#include "../main/savestates.h"
#include "../gc_memory/memory.h"

#define QUEUE_SIZE 8
static int SPECIAL_done = 0;
static interupt_queue *q = NULL;
static interupt_queue *qstack[QUEUE_SIZE];
static unsigned int qstackindex = 0;
static interupt_queue *qbase = NULL;

static interupt_queue* queue_malloc(size_t Bytes)
{
       if (qstackindex >= QUEUE_SIZE) // should never happen
       {
               return malloc(Bytes);
       }
       interupt_queue* newQueue = qstack[qstackindex];
       qstackindex ++;

       return newQueue;
}

static void queue_free(interupt_queue *qToFree)
{
       if (qToFree < qbase || qToFree >= qbase + sizeof(interupt_queue) * QUEUE_SIZE)
       {
               free(qToFree); //must be a non-stack memory allocation
               return;
       }
       qstackindex --;
       qstack[qstackindex] = qToFree;
}

static void clear_queue(void)
{
    int i;
    q = NULL;
    for (i =0; i < QUEUE_SIZE; i++)
    {
       qstack[i] = &qbase[i];
    }
    qstackindex = 0;
}

// from mupen64plus
unsigned long add_random_interupt_time(void)
{
  return randomize_interrupt ? (unsigned long)(rand() & 0x3F) : 0;
}

void print_queue()
{
  interupt_queue *aux;
  printf("------------------ %x\n", (unsigned int)Count);
  aux = q;
  while (aux != NULL) {
    printf("Count:%x, %x\n", (unsigned int)aux->count, aux->type);
    aux = aux->next;
  }
  printf("------------------\n");
}

int before_event(unsigned long evt1, unsigned long evt2, int type2)
{
  if(evt1 - Count < 0x80000000) {
    if(evt2 - Count < 0x80000000) {
      if((evt1 - Count) < (evt2 - Count)) {
        return 1;
      }
      else {
        return 0;
      }
    }
    else {
      if((Count - evt2) < 0x10000000) {
        if((type2 == SPECIAL_INT) && (SPECIAL_done)) {
          return 1;
        }
        else {
          return 0;
        }     
      }
      else {
        return 1;
      }
    }
  }
  else {
    return 0;
  }
}

void add_interupt_event(int type, unsigned long delay)
{
  unsigned long count = Count + delay;
  int special = 0;
  
  if(type == SPECIAL_INT) {
    special = 1;
  }
  if(Count > 0x80000000) {
    SPECIAL_done = 0;
  }
     
  if (get_event(type)) {
    //printf("two events of type %x in queue\n", type);
  }
   
  interupt_queue *aux = q;
  if (q == NULL) {
    q = queue_malloc(sizeof(interupt_queue));
    q->next = NULL;
    q->count = count;
    q->type = type;
	r4300.next_interrupt = q->count;
    return;
  }
   
  if(before_event(count, q->count, q->type) && !special) {
    q = queue_malloc(sizeof(interupt_queue));
    q->next = aux;
    q->count = count;
    q->type = type;
	r4300.next_interrupt = q->count;
    return;
  }

  while (aux->next != NULL && (!before_event(count, aux->next->count, aux->next->type) || special)) {
    aux = aux->next;
  }

  if (aux->next == NULL) {
    aux->next = queue_malloc(sizeof(interupt_queue));
    aux = aux->next;
    aux->next = NULL;
    aux->count = count;
    aux->type = type;
  }
  else {
    interupt_queue *aux2;
    if (type != SPECIAL_INT) {
      while(aux->next != NULL && aux->next->count == count) {
        aux = aux->next;
      }
    }
    aux2 = aux->next;
    aux->next = queue_malloc(sizeof(interupt_queue));
    aux = aux->next;
    aux->next = aux2;
    aux->count = count;
    aux->type = type;
  }
}

void add_interupt_event_count(int type, unsigned long count)
{
  add_interupt_event(type, (count - Count));
}

void remove_interupt_event()
{
  interupt_queue *aux = q->next;
  if(q->type == SPECIAL_INT) {
    SPECIAL_done = 1;
  }
  queue_free(q);
  q = aux;
  r4300.next_interrupt = (q != NULL) ? q->count : 0;
}

// Returns a ptr to the event's count
unsigned long *get_event(int type)
{
  interupt_queue *aux = q;
  if (q == NULL) {
    return NULL;
  }
  if (q->type == type) {
    return &q->count;
  }
  while (aux->next != NULL && aux->next->type != type) {
    aux = aux->next;
  }
  if (aux->next != NULL) {
    return &aux->next->count;
  }
  return NULL;
}

void remove_event(int type)
{
  interupt_queue *aux = q;
  if (q == NULL) return;
  if (q->type == type) {
    aux = aux->next;
    queue_free(q);
    q = aux;
    return;
  }
  while (aux->next != NULL && aux->next->type != type) {
    aux = aux->next;
  }
  if (aux->next != NULL) { // it's a type int
    interupt_queue *aux2 = aux->next->next;
    queue_free(aux->next);
    aux->next = aux2;
  }
}

void translate_event_queue(unsigned long base)
{
  interupt_queue *aux;
  remove_event(COMPARE_INT);
  remove_event(SPECIAL_INT);
  aux=q;
  while (aux != NULL) {
    aux->count = (aux->count - Count)+base;
    aux = aux->next;
  }

  Count = base;
  add_interupt_event_count(SPECIAL_INT, (Count & 0x80000000) ^ 0x80000000);
  // Add count_per_op to avoid wrong event order in case Count == Compare
  Count += count_per_op;
  add_interupt_event_count(COMPARE_INT, Compare);
  Count -= count_per_op;
}

// save the queue (for save states)
int save_eventqueue_infos(char *buf)
{
  int len = 0;
  interupt_queue *aux = q;
  if (q == NULL) {
    *((unsigned long*)&buf[0]) = 0xFFFFFFFF;
    return 4;
  }
  while (aux != NULL) {
    memcpy(buf+len  , &aux->type , 4);
    memcpy(buf+len+4, &aux->count, 4);
    len += 8;
    aux = aux->next;
  }
  *((unsigned long*)&buf[len]) = 0xFFFFFFFF;
  return len+4;
}

// load the queue (for save states)
void load_eventqueue_infos(char *buf)
{
  unsigned int len = 0, type = 0, count = 0;
  clear_queue();
  while (*((unsigned long*)&buf[len]) != 0xFFFFFFFF) {
    type  = *((unsigned long*)&buf[len]);
    count = *((unsigned long*)&buf[len+4]);
    add_interupt_event_count(type, count);
    len += 8;
  }
  remove_event(SPECIAL_INT);
  add_interupt_event_count(SPECIAL_INT, (Count & 0x80000000) ^ 0x80000000);
}

void init_interupt()
{
  SPECIAL_done = 1;
  r4300.next_vi = r4300.next_interrupt = 5000;
  r4300.vi_field = 0;
  if (qbase != NULL) free(qbase);
  qbase = (interupt_queue *) malloc(sizeof(interupt_queue) * QUEUE_SIZE );
  memset(qbase,0,sizeof(interupt_queue) * QUEUE_SIZE );
  qstackindex=0;
  clear_queue();
  add_interupt_event_count(SPECIAL_INT, 0x80000000);
}

void check_interupt()
{
  if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg) {
    Cause = (Cause | 0x400) & 0xFFFFFF83;
  }
  else {
    Cause &= ~0x400;
  }
  if ((Status & 7) != 1) {
    return;
  }
  if (q != NULL && q->type == CHECK_INT) {
    return;
  }
  if (Status & Cause & 0xFF00) {
    if(q == NULL) {
      q = queue_malloc(sizeof(interupt_queue));
      q->next = NULL;
      q->count = Count;
      q->type = CHECK_INT;
    }
    else {
      interupt_queue* aux = queue_malloc(sizeof(interupt_queue));
      aux->next = q;
      aux->count = Count;
      aux->type = CHECK_INT;
      q = aux;
    }
    r4300.next_interrupt = Count;
  }
}

// Just wrapping up some common code
int chk_status(int chk) {
  if(chk) {
    if (MI_register.mi_intr_reg & MI_register.mi_intr_mask_reg) {
      Cause = (Cause | 0x400) & 0xFFFFFF83;
    }
    else {
      return 0;
    }
  }
  if ((Status & 7) != 1) {
    return 0;
  }
  if (!(Status & Cause & 0xFF00)) {
    return 0;
  }
  return 1;
}

void gen_interupt()
{
  if(!(interrupt_unsafe_state & INTR_UNSAFE_RSP) && savestates_queued_load()) {
	return;
  }

  if (r4300.skip_jump) {
    if (q->count > Count || (Count - q->count) < 0x80000000) {
      r4300.next_interrupt = q->count;
    }
    else {
      r4300.next_interrupt = 0;
    }
    r4300.pc = r4300.skip_jump;
    r4300.last_pc = r4300.pc;
    r4300.skip_jump=0;
    return;
  } 

  switch(q->type) {
    case SPECIAL_INT:
      remove_interupt_event();
      add_interupt_event_count(SPECIAL_INT, (Count & 0x80000000) ^ 0x80000000);
      return;
    break;
    case VI_INT:
			// If the DP is still frozen with a completion signal pending, this frame isn't
			// really done yet. Defer presenting it until do_SP_task/update_DPC unfreezes
			// and fires it for us, instead of showing a broken framw now.
			if (dpc_register.do_on_unfreeze & DELAY_DP_INT)
				dpc_register.do_on_unfreeze |= DELAY_UPDATESCREEN;
			else
				updateScreen();
#ifdef PROFILE
      refresh_stat();
#endif
      new_vi();

      r4300.vi_field ^= (vi_register.vi_status >> 6) & 1;


      unsigned long *pnext_vi = get_event(VI_INT);
      unsigned long next_vi = (pnext_vi ? *pnext_vi : Count) + vi_register.vi_delay;
      remove_interupt_event();
      add_interupt_event_count(VI_INT, next_vi);
      r4300.next_vi = next_vi;
 
      MI_register.mi_intr_reg |= 0x08;
      if(!chk_status(1)) {
        return;
      }
    break;
  
    case COMPARE_INT:
      remove_interupt_event();
      Count += count_per_op;
      add_interupt_event_count(COMPARE_INT, Compare);
      Count -= count_per_op;
  
      Cause = (Cause | 0x8000) & 0xFFFFFF83;
      if(!chk_status(0)) {
        return;
      }
    break;
  
    case CHECK_INT:
      remove_interupt_event();
    break;
  
    case SI_INT:
      si_end_of_dma();
	  PIF_RAMb[0x3F] = 0x0;
      remove_interupt_event();
      MI_register.mi_intr_reg |= 0x02;
      si_register.si_status &= ~0x3;
      si_register.si_status |= 0x1000;
      if(!chk_status(1)) {
        return;
      }
    break;
  
    case PI_INT:
      remove_interupt_event();
      MI_register.mi_intr_reg |= 0x10;
      pi_register.read_pi_status_reg &= ~3;
      if(!chk_status(1)) {
        return;
      }
    break;
  
    case AI_INT:
      if (ai_register.ai_status & 0x80000000) { // full
        unsigned long *pai_event = get_event(AI_INT);
        unsigned long ai_event = pai_event ? *pai_event : 0;
        remove_interupt_event();
        ai_register.ai_status &= ~0x80000000;
        ai_register.current_delay = ai_register.next_delay;
        ai_register.current_len = ai_register.next_len;
        add_interupt_event_count(AI_INT, ai_event+ai_register.next_delay);
      }
      else {
        remove_interupt_event();
        ai_register.ai_status &= ~0x40000000;
        ai_delayed_carry = 0;
      }
      MI_register.mi_intr_reg |= 0x04;
      if(!chk_status(1)) {
        return;
      }
    break;

    case SP_INT:
      remove_interupt_event();
      if (!sp_register.sp_task_pending)
        sp_register.sp_status_reg |= 0x203; // halt | broke | taskdone
      if (!(sp_register.sp_status_reg & 0x40)) { // interrupt-on-break not enabled
        return;
      }
      MI_register.mi_intr_reg |= MI_INTR_SP;
      if(!chk_status(1)) {
        return;
      }
    break;

    case DP_INT:
      // Actually deliver the DP completion signal do_SP_task deferred
      remove_interupt_event();
      MI_register.mi_intr_reg |= MI_INTR_DP;
      if(!chk_status(1)) {
        return;
      }
    break;

    default:
      remove_interupt_event();
    break;
  }
  exception_general();
}
