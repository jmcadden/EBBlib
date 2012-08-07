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
#include <l0/lrt/event.h>
#include <l0/lrt/bare/arch/ppc32/bic.h>
#include <l0/lrt/mem.h>
#include <lrt/assert.h>

static lrt_event_loc num_loc;

lrt_event_loc
lrt_num_event_loc()
{
  //FIXME: for smp
  return 1;
}

lrt_event_loc
lrt_next_event_loc(lrt_event_loc l)
{
  //FIXME: for smp
  return l;
}

static void __attribute__((noreturn))
lrt_event_loop(void)
{
  LRT_Assert(0);
}

static uintptr_t **altstacks;

void *lrt_event_init(void *myloc)
{ 
#define STACK_SIZE (1 << 14)
  char *myStack = lrt_mem_alloc(STACK_SIZE, 16, lrt_my_event_loc());

  altstacks[lrt_my_event_loc()] = 
    lrt_mem_alloc(4096, 16, lrt_my_event_loc());

  asm volatile (
		"mr 1, %[stack];"
		"bl lrt_start"
		:
		: [stack] "r" (&myStack[STACK_SIZE]));
  lrt_event_loop();
}

void lrt_event_preinit(int cores)
{ 
  num_loc = cores;

  //disable timers
  tcr tcr;
  tcr.val = get_spr(SPRN_TCR);
  tcr.wie = 0;
  tcr.die = 0;
  tcr.fie = 0;
  set_spr(SPRN_TCR, tcr.val);

  //disable and clear all IRQs on BIC
  bic_init();
  bic_disable_and_clear_all();
  
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
}

