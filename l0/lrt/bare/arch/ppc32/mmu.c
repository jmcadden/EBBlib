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

#include <stddef.h>
#include <stdint.h>

#include <arch/powerpc/450/mmu.h>
#include <l0/lrt/bare/arch/ppc32/mmu.h>

static char *vmem_start = (char *)(1 << 31); //2 GB
static int tlb_entry = 2;

void *
tlb_map(uint64_t paddr, int size, int flags)
{
  if (size & ((1 << 10) - 1)) {
    return NULL;
  }
  int newsize = size >> 10;
  if (__builtin_popcount(newsize) != 1 ||
      !(SUPPORTED_TLB_PAGE_SIZE & newsize)) {
    return NULL;
  }
  
  uintptr_t vaddr = (uintptr_t)vmem_start;
  vaddr += size - 1;
  vaddr &= ~(size - 1);
  vmem_start = (char *)vaddr;

  tlb_word_0 t0;
  t0.val = 0;
  t0.epn = vaddr >> 10;
  t0.v = 1;
  int tlb_size = 31 - __builtin_clz(newsize);
  tlb_size /= 2;
  t0.size = tlb_size;

  tlb_word_1 t1;
  t1.val = 0;
  t1.rpn = (uint32_t)((paddr >> 10) & 0x3fffff);
  t1.erpn = (uint32_t)((paddr >> 32) & 0xf);

  tlb_word_2 t2;
  t2.val = 0;
  t2.wl1 = 1;
  t2.sx = 1;
  t2.sw = 1;
  t2.sr = 1;
  if (flags & TLB_INHIBIT) {
    t2.i = 1;
  }
  if (flags & TLB_GUARDED) {
    t2.g = 1;
  }
  asm volatile (
		"tlbwe %[word0],%[entry],0;"
		"tlbwe %[word1],%[entry],1;"
		"tlbwe %[word2],%[entry],2;"
		"isync;"
		:
		: [word0] "r" (t0.val),
		  [word1] "r" (t1.val),
		  [word2] "r" (t2.val),
		  [entry] "r" (tlb_entry) 
		);  
  tlb_entry++;

  return (void *)vaddr;
}
