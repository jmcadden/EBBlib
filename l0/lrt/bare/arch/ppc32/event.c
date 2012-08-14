/*
 * Copyright (C) 2011 by Project SESA, Boston University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <config.h>
#include <arch/cpu.h>
#include <l0/EventMgrPrim.h>
#include <l0/lrt/event.h>
#include <l0/lrt/bare/arch/ppc32/bic.h>
#include <l0/lrt/mem.h>
#include <lrt/assert.h>
#include <sync/misc.h>

static lrt_event_loc num_loc;

lrt_event_loc
lrt_num_event_loc()
{
  return num_loc;
}

lrt_event_loc
lrt_next_event_loc(lrt_event_loc l)
{
  return (l + 1) % num_loc;
}

static void __attribute__((noreturn))
lrt_event_loop(void)
{
  //wait for eventmgr to be initialized
  EventMgrPrimId id;
  do {
    id = ACCESS_ONCE(theEventMgrPrimId);
    cpu_relax();
  } while (id == 0 || id == (EventMgrPrimId)-1);
  
  while(1) {
    COBJ_EBBCALL(theEventMgrPrimId, enableInterrupts);
  }
}

static uintptr_t **altstacks;

void *lrt_event_init(void *myloc)
{ 
  if (lrt_my_event_loc() != 0) {
    bic_secondary_init();
  }
  //disable timers
  tcr tcr;
  tcr.val = 0;
  set_spr(SPRN_TCR, tcr.val);

  tsr tsr;
  tsr.val = 0;
  set_spr(SPRN_TSR, tsr.val);

#define STACK_SIZE (1 << 14)
  char *myStack = lrt_mem_alloc(STACK_SIZE, 16, lrt_my_event_loc());

  altstacks[lrt_my_event_loc()] = 
    lrt_mem_alloc(4096, 16, lrt_my_event_loc());

  asm volatile (
		"mr 1, %[stack];"
		:
		: [stack] "r" (&myStack[STACK_SIZE-112])
		: "memory");

  extern long entry_secondary;
  entry_secondary = lrt_next_event_loc(lrt_my_event_loc());
  
  extern void lrt_start(void);
  lrt_start();
  lrt_event_loop();
}

void lrt_event_preinit(int cores)
{ 

  num_loc = cores;

  //disable and clear all IRQs on BIC
  bic_init();
  bic_disable_and_clear_all();
  // map ipis
  // IRQ 0-31 are no mapped to devicies, can be used for IPIs
  bic_enable_irq(BIC_IPI_GROUP, 0, NONCRIT, 0);
  bic_enable_irq(BIC_IPI_GROUP, 1, NONCRIT, 1);
  bic_enable_irq(BIC_IPI_GROUP, 2, NONCRIT, 2);
  bic_enable_irq(BIC_IPI_GROUP, 3, NONCRIT, 3);
  altstacks = lrt_mem_alloc(sizeof(char *) * cores, 8, 0);
}

void lrt_event_trigger_event(lrt_event_num num,
    enum lrt_event_loc_desc desc,
    lrt_event_loc loc)
{ 
  LRT_Assert(0); 
}
void lrt_event_route_irq(struct IRQ_t *isrc,
    lrt_event_num num,
    enum lrt_event_loc_desc desc,
    lrt_event_loc loc)
{ 
  LRT_Assert(0); 
}
void lrt_event_altstack_push(uintptr_t val)
{ 
  LRT_Assert(0); 
}

uintptr_t lrt_event_altstack_pop(void)
{ 
  LRT_Assert(0); 
  return 0;
}

void __attribute__((noreturn))
exception_common(int interrupt)
{
   lrt_printf("exception: %d\n",interrupt);
   while(1)
     ;
}

void
event_common(int interrupt)
{
  COBJ_EBBCALL(theEventMgrPrimId, dispatchIRQ);
}
