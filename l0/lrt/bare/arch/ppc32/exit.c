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
#include <inttypes.h>
#include <l0/EventMgrPrim.h>
#include <l0/lrt/exit.h>
#include <l0/lrt/bare/arch/ppc32/bg_tree.h>
#include <l0/lrt/bare/arch/ppc32/bic.h>
#include <l0/lrt/event_irq_def.h>
#include <lrt/io.h>

void __attribute__((noreturn))
lrt_exit(int i)
{
  lrt_printf("Exit called: %d\n", i);

  // disable all other cores 
  for(EventNo num = NextEventLoc(MyEventLoc());
      num != MyEventLoc();
      num = NextEventLoc(num)) {
      bic_raise_irq(BIC_IPI_GROUP, BIC_CRIT_IPI_BASE+num);
  }

  // clear incoming packets off the tree
  uintptr_t tree;
  while (1)
  {
    for( int i = 0 ; i < 2; i++){
      tree = bgtree_get_channel(i);
      uint32_t status = *(volatile uint32_t *)(tree + 0x40);
      uint8_t rcv_hdr = status & 0xf;
      while (rcv_hdr > 0) {
        *(volatile uint32_t *)(tree + 0x30); //read header
        // register unsigned int addr asm ("r3") = (unsigned int)tree + 0x20;
        for (int j = 0; j < TREE_PAYLOAD; j += 16) {
          // LFPDX(0,0,3,addr);
          asm("lfpdx 0,0,%0" :: "b" ((unsigned int)tree + 0x20));
        }
        rcv_hdr--;
      }
    }
  };

}

