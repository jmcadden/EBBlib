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

/*
#include <arch/amd64/apic.h>
#include <arch/amd64/idt.h>
#include <arch/amd64/pic.h>
#include <arch/amd64/pit.h>
#include <arch/amd64/rtc.h>
#include <arch/amd64/segmentation.h>
#include <l0/lrt/event.h>
#include <l0/lrt/mem.h>
#include <l0/lrt/bare/arch/amd64/acpi.h>
#include <l0/lrt/bare/arch/amd64/lrt_start.h>
#include <lrt/io.h>
*/

static int num_event_loc;

lrt_event_loc
lrt_num_event_loc()
{
  return num_event_loc;
}

lrt_event_loc
lrt_next_event_loc(lrt_event_loc l)
{
  return num_event_loc;
  // return (l + 1) % lrt_num_event_loc();
}

static lrt_event_loc bsp_loc;

void
lrt_event_set_bsp(lrt_event_loc loc)
{
  bsp_loc = loc;
}

lrt_event_loc
lrt_event_bsp_loc()
{
  return bsp_loc;
}
static struct lrt_event_descriptor lrt_event_table[LRT_EVENT_NUM_EVENTS];

static idtdesc *idt;


static inline void
idt_map_vec(uint8_t vec, void *addr) {
  /*
  idt[vec].offset_low = ((uintptr_t)addr) & 0xFFFF;
  idt[vec].offset_high = ((uintptr_t)addr) >> 16;
  idt[vec].selector = 0x8; //Our code segment
  idt[vec].ist = 0; //no stack switch
  idt[vec].type = 0xe;
  idt[vec].p = 1; //present
  */
  return;
}


static void *isrtbl[];

static inline void
init_idt(void)
{
  /*
  for (int i = 0; i < 256; i++) {
    idt_map_vec(i, isrtbl[i]);
  }
  */
  return;
}

void
lrt_event_preinit(int cores)
{
  /*
  num_event_loc = cores;
  idt = lrt_mem_alloc(sizeof(idtdesc) * 256, 8, lrt_event_bsp_loc());
  init_idt();
  LRT_Assert(has_lapic());
  disable_pic();
  //Disable the pit, irq 0 could have fired and therefore wouldn't
  //have been masked and then we enable interrupts so we must reset
  //the PIT (and we may as well prevent it from firing)
  disable_pit();

  //Disable the rtc, irq 8 could have fired and therefore wouldn't
  //have been masked and then we enable interrupts so we must disable
  //it
  disable_rtc();

  enable_lapic();
  */
  return;
}

void __attribute__ ((noreturn))
lrt_event_loop(void)
{
  //After we enable interrupts we just halt, an interrupt should wake
  //us up. Once we finish the interrupt, we halt again and repeat
#if 0
  __asm__ volatile("sti"); //enable interrupts
  while (1) {
    __asm__ volatile("hlt");
  }
#endif 
}

volatile int smp_lock;

void *
lrt_event_init(void *unused)
{ return; }
#if 0  
  load_idtr(idt, sizeof(idtdesc) * 256);

  enable_lapic();

  lrt_event_loc loc = get_lapic_id();

  lrt_event_loc *myloc = lrt_mem_alloc(sizeof(lrt_event_loc),
                                       sizeof(lrt_event_loc),
                                       loc);

  *myloc = loc;

  asm volatile (
                "wrmsr"
                :
                : "d" ((uintptr_t)myloc >> 32),
                  "a" ((uintptr_t)myloc),
                  "c" (0xc0000101));

  // call lrt_start before entering the loop
  // We switch stacks here to our own dynamically allocated stack
#define STACK_SIZE (1 << 14)
  char *myStack = lrt_mem_alloc(STACK_SIZE, 16, lrt_my_event_loc());

  asm volatile (
                "mov %[stack], %%rsp\n\t"
                "mfence\n\t"
                "movl $0x0, %[smp_lock]\n\t"
                "call lrt_start"
                :
                : [stack] "r" (&myStack[STACK_SIZE]),
                  [smp_lock] "m" (smp_lock)
                );
  lrt_event_loop();
}
#endif

void
lrt_event_bind_event(lrt_event_num num, lrt_trans_id handler,
                     lrt_trans_func_num fnum)
{
  // lrt_event_table[num].id = handler;
  // lrt_event_table[num].fnum = fnum;
}

void
lrt_event_trigger_event(lrt_event_num num, enum lrt_event_loc_desc desc,
                        lrt_event_loc loc)
{
  /*
  lapic_icr_low icr_low;
  icr_low.raw = 0;
  icr_low.vector = num + 32;
  icr_low.level = 1;

  lapic_icr_high icr_high;
  icr_high.raw = 0;
  if (desc == LRT_EVENT_LOC_SINGLE) {
    icr_low.destination_shorthand = 0;
    icr_high.destination = loc;
  } else {
    LRT_Assert(0);
  }

  send_ipi(icr_low, icr_high);
  */
}

void lrt_event_route_irq(struct IRQ_t *isrc, lrt_event_num num,
                         enum lrt_event_loc_desc desc, lrt_event_loc loc)
{
  LRT_Assert(0);
}

void
exception_common(uint8_t num) {
  lrt_printf("Received exception %d\n!", num);
  LRT_Assert(0);
}

void
event_common(uint8_t num) {
  /*
  send_eoi();
  uint8_t ev = num - 32; //first 32 interrupts are reserved
  struct lrt_event_descriptor *desc = &lrt_event_table[ev];
  lrt_trans_id id = desc->id;
  lrt_trans_func_num fnum = desc->fnum;

  //this infrastructure should be pulled out of this file
  lrt_trans_rep_ref ref = lrt_trans_id_dref(id);
  ref->ft[fnum](ref);
  */
}

extern void isr_0(void);
static void *isrtbl[] = { isr_0 } 
