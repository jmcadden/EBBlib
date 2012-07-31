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
#include <l0/lrt/bare/arch/ppc32/fdt.h>
#include <arch/powerpc/mmu.h>
#include <arch/powerpc/regs.h>
#include <l0/lrt/bare/arch/ppc32/lrt_start.h>
#include <l0/lrt/bare/arch/ppc32/mailbox.h>
#include <l0/lrt/bare/arch/ppc32/pic.h>
#include <lrt/io.h>

FILE *stdout;

static inline void
clear_bss(void)
{
  extern uint8_t sbss[];
  extern uint8_t ebss[];
  for (uint8_t *i = sbss; i < ebss; i++) {
    *i = 0;
  }
}

int
ipow(int base, int exp)
{
  int result = 1;
  while (exp) {
    if (exp & 1) {
      result *= base;
    }
    exp >>= 1;
    base *= base;
  }
  return result;
}


void __attribute__((section(".init.text"), noreturn))
init(struct fdt *fdt) 
{
  extern uint8_t _vec_start[];
  set_ivpr(_vec_start);

  //IVORS in the order that fixed registers
  // are set
  set_spr(SPRN_IVOR0, 0x000);
  set_spr(SPRN_IVOR1, 0x100);
  set_spr(SPRN_IVOR2, 0x200);
  set_spr(SPRN_IVOR3, 0x300);
  set_spr(SPRN_IVOR4, 0x400);
  set_spr(SPRN_IVOR5, 0x500);
  set_spr(SPRN_IVOR6, 0x600);
  set_spr(SPRN_IVOR7, 0x700);
  set_spr(SPRN_IVOR8, 0x800);
  set_spr(SPRN_IVOR9, 0x900);
  set_spr(SPRN_IVOR10, 0xA00);
  set_spr(SPRN_IVOR11, 0xB00);
  set_spr(SPRN_IVOR12, 0xC00);
  set_spr(SPRN_IVOR13, 0xD00);
  set_spr(SPRN_IVOR14, 0xE00);
  set_spr(SPRN_IVOR15, 0xF00);
  
  //setup MSR
  msr msr = get_msr();
  //enable machine check
  msr.me = 1;
  //make sure external interrupts are off
  msr.ee = 0;
  //enable fpu
  msr.fp = 1;
  set_msr(msr);

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
  asm volatile ("isync"); 

  if (fdt->magic != fdt_validmagic) {
    while (1)
      ;
  }
  
  //check if where we copy the fdt is overlapping with the fdt
  extern char kend[];
  if (((uintptr_t)kend + fdt->size) >= (uintptr_t)fdt) {
    while (1)
      ;
  }

  uintptr_t *newfdt = (uintptr_t *)kend;
  uintptr_t *oldfdt = (uintptr_t *)fdt;
  for (int i = 0; i < (fdt->size / sizeof(uintptr_t)); i++) {
    newfdt[i] = oldfdt[i];
  }

  //Map in mailbox
  //TODO: TLB map function
  uint64_t mb_phys = 0x7ffffc000LL; //FIXME: get from FDT
  uint32_t mb_virt = 0xffffc000; //allocate somehow
  t0.val = 0;
  t0.epn = (uint32_t)(mb_virt >> 10);
  t0.v = 1;
  t0.size = 2;//ilog2( >> 10) / 2;
  t1.val = 0;
  t1.rpn = (uint32_t)((mb_phys >> 10) & 0x3fffff);
  t1.erpn = (uint32_t)((mb_phys >> 32) & 0xf);
  t2.val = 0;
  t2.wl1 = 1;
  t2.sx = 1;
  t2.sw = 1;
  t2.sr = 1;

  asm volatile (
		"tlbwe %[word0],%[entry],0;"
		"tlbwe %[word1],%[entry],1;"
		"tlbwe %[word2],%[entry],2;"
		"isync;"
		:
		: [word0] "r" (t0.val),
		  [word1] "r" (t1.val),
		  [word2] "r" (t2.val),
		  [entry] "r" (3) //FIXME
		);  

  // Final setup
  clear_bss();
  stdout = mailbox_init();
  printf("Mailbox initialized\n");

  // Print TLB
  for (int i = 0; i < 64; i++) {
    tlb_word_0 w0;
    tlb_word_1 w1;
    tlb_word_2 w2;
    asm volatile (
        "tlbre %[word0],%[entry],0;"
        "tlbre %[word1],%[entry],1;"
        "tlbre %[word2],%[entry],2"
        : [word0] "=&r" (w0.val),
        [word1] "=&r" (w1.val),
        [word2] "=&r" (w2.val)
        : [entry] "r" (i)
        );
    mmucr mmucr;
    mmucr.val = get_spr(SPRN_MMUCR);
    if (w0.v) {
      printf("TLB Entry %d:\n", i);
      printf("t0: %x, t1: %x, t2: %x\n", w0.val, w1.val, w2.val);
      uint32_t size = ipow(4,w0.size); // in Kb
      uint32_t vaddr = w0.epn << (10);
      uint32_t vaddr_start = vaddr & (~((1 << (__builtin_ffs(size) + 9)) - 1));
      uint32_t vaddr_end = vaddr | ((1 << (__builtin_ffs(size) + 9)) - 1);
      uint64_t paddr = (((uint64_t)w1.erpn) << 32) |
        (((uint64_t)w1.rpn) << 10);
      uint64_t paddr_start = paddr &
        (~((1 << (((uint64_t)__builtin_ffs(size)) + 9)) - 1));
      uint64_t paddr_end = paddr |
        ((1 << (((uint64_t)__builtin_ffs(size)) + 9)) - 1);

      printf("Virt: %x - %x -> Phys: %llx - %llx\n",
          vaddr_start, vaddr_end,
          paddr_start, paddr_end);
      printf("TID: %d, TS: %d, Size: %dkb, FAR: %d\n",
          mmucr.stid, w0.ts, size, w2.far);
      printf("WL1: %d, IL1I: %d, IL1D: %d, IL2I: %d, IL2D: %d\n",
          w2.wl1, w2.il1i, w2.il1d, w2.il2i, w2.il2d);
      printf("W: %d, I: %d, M: %d, G: %d, E: %d, UX: %d, UW: %d\n",
          w2.w, w2.i, w2.m, w2.g, w2.e, w2.ux, w2.uw);
      printf("UR: %d, SX: %d, SW: %d, SR: %d\n",
          w2.ur, w2.sx, w2.sw, w2.sr);
      printf("\n\n");
    }
  }// end print TLB
  printf("Success!\n");
  //printf("FDT: %x\n", fdt);
  while(1);
  
//  lrt_pic_init(lrt_start_isr);
}
