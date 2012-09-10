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

#include <stdint.h>

#include <arch/powerpc/cpu.h>
#include <arch/powerpc/mmu.h>
#include <arch/powerpc/regs.h>
#include <l0/lrt/bare/arch/ppc32/bg_tree.h>
#include <l0/lrt/bare/arch/ppc32/bic.h>
#include <l0/lrt/bare/arch/ppc32/debug.h>
#include <l0/lrt/bare/arch/ppc32/fdt.h>
#include <l0/lrt/bare/arch/ppc32/lrt_start.h>
#include <l0/lrt/bare/arch/ppc32/mailbox.h>
#include <l0/lrt/bare/arch/ppc32/pic.h>
#include <l0/lrt/event.h>
#include <l0/lrt/mem.h>
#include <l0/lrt/trans.h>
#include <lrt/io.h>

static inline void
clear_bss(void)
{
  extern uint8_t sbss[];
  extern uint8_t ebss[];
  for (uint8_t *i = sbss; i < ebss; i++) {
    *i = 0;
  }
}

void __attribute__((section(".init.text")))
init_mapping(void)
{
  //FIXME: something to do with mmucr
  //Map lower 2 gb
  tlb_word_0 t0;
  tlb_word_1 t1;
  tlb_word_2 t2;
  t0.val = 0;
  t0.epn = 0; //virtual address 0
  t0.v = 1;
  t0.size = 10; //1GB page
  t1.val = 0;
  t1.rpn = 0; //physical address 0
  t1.erpn = 0;
  t2.val = 0;
  t2.wl1 = 1;
  t2.m = 1;
  t2.sx = 1;
  t2.sw = 1;
  t2.sr = 1;
  asm volatile (
		"tlbwe %[word0],%[entry],0;"
		"tlbwe %[word1],%[entry],1;"
		"tlbwe %[word2],%[entry],2;"
		:
		: [word0] "r" (t0.val),
		  [word1] "r" (t1.val),
		  [word2] "r" (t2.val),
		  [entry] "r" (0)
		);
  t0.epn = 1 << 20; //virtual address 1GB
  t1.rpn = 1 << 20; //physical address 1GB
  t2.sx = 0;
  asm volatile (
		"tlbwe %[word0],%[entry],0;"
		"tlbwe %[word1],%[entry],1;"
		"tlbwe %[word2],%[entry],2;"
		:
		: [word0] "r" (t0.val),
		  [word1] "r" (t1.val),
		  [word2] "r" (t2.val),
		  [entry] "r" (1)
		);
  
  // Unmap all entries other than our init mapping
  for (int i = 2; i < 64; i++) {
    t0.val = 0;
    asm volatile (
        "tlbwe %[word0],%[entry],0;"
        "tlbwe %[word1],%[entry],1;"
        "tlbwe %[word2],%[entry],2;"
        :
        : [word0] "r" (t0.val),
        [word1] "r" (t1.val),
        [word2] "r" (t2.val),
        [entry] "r" (i)
        );
  }
  //this synchronizes, so the shadow TLB gets flushed and our new mappings are picked up
  // all future accesses will miss the TLB cache and pickup our new mappings
  asm volatile ("isync"); 
}

void __attribute__((noreturn))
init(struct fdt *fdt) 
{
  // core execute this function sequentially begining with core 0
  if (lrt_my_event_loc() == 0) {
    clear_bss();
    fdt_init(fdt);
    stdout = mailbox_init();

    //disable and clear all IRQs on BIC
    bic_init();
    bic_disable_and_clear_all();

    debug_init();
    bgtree_init();
    
    int cores = 4;
    lrt_mem_preinit(cores);
    lrt_event_preinit(cores);
    lrt_trans_preinit(cores);
  } else {
    mailbox_secondary_init();
    debug_secondary_init();
    bgtree_secondary_init();
  }
   
  lrt_event_init(NULL); //unused parameter
  
  LRT_Assert(0);

  while(1)
    ;
}
