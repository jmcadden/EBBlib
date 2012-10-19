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

#include <stdint.h>

#include <lrt/io.h>
#include <l0/lrt/bare/stdio.h>
#include <l0/lrt/bare/arch/ppc32/bg_tree.h>
#include <l0/lrt/bare/arch/ppc32/bic.h>
#include <l0/lrt/bare/arch/ppc32/debug.h>
#include <l0/lrt/bare/arch/ppc32/fdt.h>
#include <l0/lrt/bare/arch/ppc32/mmu.h>
#include <lrt/string.h>
#include <lrt/assert.h>

//static int bgp_dcr_glob_att_write_set;
//static int bgp_dcr_glob_att_write_clear;
// not sure if we need these two above
static int node_id; // self
//static int curr_con; // self
static uint64_t dcr_base ;

static struct channel_t channel[BGP_NUM_CHANNEL];
static struct fdt_node *bg_tree, *reg, *node, *dcr_reg;

static inline uint32_t mfdcrx(int dcr)
{
  uint32_t ret;
  asm volatile (
      "mfdcrx %[val], %[dcrn]"
      : [val] "=r" (ret)
      : [dcrn] "r" (dcr)
      );
  return ret;
};

static inline void mtdcrx(int dcr, int val)
{
  asm volatile (
      "mtdcrx %[dcrn], %[val]"
      :
      : [dcrn] "r" (dcr),
       [val] "r" (val)
      );
  return ;
};

inline void 
bgtree_inject_packet (uint32_t *dest, void *lnkhdr, void *payload, uint32_t mioaddr)
{
  asm volatile(
      "lfpdx   0,0,%4;"	// F0=Q0 load *lnkhdr(%4)
      "stw     %3,%c5(%1);"	// store dest to HI offset from mioaddr
      "lfpdx   1,0,%0;"	// F1=Q1 load from *payload
      "lfpdux  2,%0,%6;"	// F2=Q2 load from *(payload+16)
      "lfpdux  3,%0,%6;"	// F3=Q3 load
      "stfpdx  0,0,%2;"	// Q0 store to *(mioaddr + TRx_DI)
      "lfpdux  4,%0,%6;"	// F4=Q4 load
      "lfpdux  5,%0,%6;"	// F5=Q5 load
      "lfpdux  6,%0,%6;"	// F6=Q6 load
      "stfpdx  1,0,%2;"	// Q1 store
      "stfpdx  2,0,%2;"	// Q2 store
      "stfpdx  3,0,%2;"	// Q3 store
      "lfpdux  7,%0,%6;"	// F7=Q7 load
      "lfpdux  8,%0,%6;"	// F8=Q8 load
      "lfpdux  9,%0,%6;"	// F9=Q9 load
      "stfpdx  4,0,%2;"	// Q4 store
      "stfpdx  5,0,%2;"	// Q5 store
      "stfpdx  6,0,%2;"	// Q6 store
      "lfpdux  0,%0,%6;"	// F0=Q10 load
      "lfpdux  1,%0,%6;"	// F1=Q11 load
      "lfpdux  2,%0,%6;"	// F2=Q12 load
      "stfpdx  7,0,%2;"	// Q7 store
      "stfpdx  8,0,%2;"	// Q8 store
      "stfpdx  9,0,%2;"	// Q9 store
      "lfpdux  3,%0,%6;"	// F3=Q13 load
      "lfpdux  4,%0,%6;"	// F4=Q14 load
      "lfpdux  5,%0,%6;"	// F5=Q15 load
      "stfpdx  0,0,%2;"	// Q10 store
      "stfpdx  1,0,%2;"	// Q11 store
      "stfpdx  2,0,%2;"	// Q12 store
      "stfpdx  3,0,%2;"	// Q13 store
      "stfpdx  4,0,%2;"	// Q14 store
      "stfpdx  5,0,%2;"	// Q15 store
      : "=b"  (payload)	// 0
      : "b"   (mioaddr),	// 1
      "b"   (mioaddr +
          _BGP_TRx_DI),	// 2 (data injection fifo address)
          "b"   (*dest),	// 3
          "b"   (lnkhdr),	// 4
          "i"   (_BGP_TRx_HI),	// 5
          "b"   (16),		// 6 (bytes per quad)
          "0"   (payload)	// "payload" is input and output
         : "fr0", "fr1", "fr2",
         "fr3", "fr4", "fr5",
         "fr6", "fr7", "fr8",
         "fr9", "memory"
            );
}


inline void 
bgtree_receive_240(void *payload, uint32_t mioaddr)
{
  /* read the payload via 16 consectative quadword (128bit) reads from the
   * payload reception register */
    asm volatile(
	"lfpdx   1,0,%1;"	// F1=Q1  load
	"lfpdx   2,0,%1;"	// F2=Q2  load
	"lfpdx   3,0,%1;"	// F3=Q3  load
	"lfpdx   4,0,%1;"	// F4=Q4  load
	"lfpdx   5,0,%1;"	// F5=Q5  load
	"stfpdx  1,0,%0;"	// Q1  store to *payload
	"stfpdux 2,%0,%2;"	// Q2  store to *(payload+16)
	"lfpdx   6,0,%1;"	// F6=Q6  load
	"lfpdx   7,0,%1;"	// F7=Q7  load
	"lfpdx   8,0,%1;"	// F8=Q8  load
	"stfpdux 3,%0,%2;"	// Q3  store
	"stfpdux 4,%0,%2;"	// Q4  store
	"stfpdux 5,%0,%2;"	// Q5  store
	"lfpdx   9,0,%1;"	// F9=Q9  load
	"lfpdx   0,0,%1;"	// F0=Q10 load
	"lfpdx   1,0,%1;"	// F1=Q11 load
	"stfpdux 6,%0,%2;"	// Q6  store
	"stfpdux 7,%0,%2;"	// Q7  store
	"stfpdux 8,%0,%2;"	// Q8  store
	"lfpdx   2,0,%1;"	// F2=Q12 load
	"lfpdx   3,0,%1;"	// F3=Q13 load
	"lfpdx   4,0,%1;"	// F4=Q14 load
	"stfpdux 9,%0,%2;"	// Q9  store
	"stfpdux 0,%0,%2;"	// Q10 store
	"stfpdux 1,%0,%2;"	// Q11 store
	"lfpdx   5,0,%1;"	// F5=Q15 load
	"stfpdux 2,%0,%2;"	// Q12 store
	"stfpdux 3,%0,%2;"	// Q13 store
	"stfpdux 4,%0,%2;"	// Q14 store
	"stfpdux 5,%0,%2;"	// Q15 store
	: "=b"   (payload)	// 0
	: "b"    (mioaddr +
		  _BGP_TRx_DR),	// 1 (data reception fifo address)
	  "b"    (16),		// 2 (bytes per quad)
	  "0"    (payload)	// "payload" is input and output
	: "fr0", "fr1", "fr2",
	  "fr3", "fr4", "fr5",
	  "fr6", "fr7", "fr8",
	  "fr9", "memory"
    );
}


void 
bgtree_clear_inj_exception_flags(void)
{
  mfdcrx(dcr_base + 0x48);
  return;
}

void 
bgtree_clear_recv_exception_flags(void)
{
  mfdcrx(dcr_base + 0x44);
  return;
}

//
FILE *
bgtree_init()
{
  uint64_t paddr, paddr_aligned;
  uint32_t size;
  uintptr_t vaddr;

  // lookup tree address in fdt
  bg_tree = fdt_get("/plb/tree");
  reg = fdt_get_in_node(bg_tree, "reg");
  node = fdt_get_in_node(bg_tree, "nodeid");
  dcr_reg = fdt_get_in_node(bg_tree, "dcr-reg");
  // get node_id
  node_id = fdt_read_prop_u32(node, 0); //8?
  // device control register base address
  dcr_base = fdt_read_prop_u32(dcr_reg, 0);

  // configure each channel of the tree
  for (int chnidx = 0; chnidx < BGP_NUM_CHANNEL; chnidx++)
  {
    paddr = fdt_read_prop_u64(reg, (chnidx*(12)));
    size = fdt_read_prop_u32(reg, (chnidx*(12)+8));
    // map channel into virtual memory
    paddr_aligned = paddr & ~((uint64_t)size - 1); //align
    vaddr = (uintptr_t)tlb_map(paddr_aligned, size,
        TLB_INHIBIT | TLB_GUARDED);
    channel[chnidx].base_phys = paddr_aligned;
    channel[chnidx].base = vaddr;
    channel[chnidx].size = size;
  }
  // disable send and recive IRQs on tree
  mtdcrx(dcr_base + 0x45, 0); // inject exception enable register
  mtdcrx(dcr_base + 0x49, 0); // recive exception enable register

  // setup reception watermarks
  mtdcrx(dcr_base + 0x42, 0x400); /* virtual channel 0 */
  mtdcrx(dcr_base + 0x43, 0x400); /* virtual channel 1 */

  // clear inject/recive flag registers
  bgtree_clear_recv_exception_flags();
  bgtree_clear_inj_exception_flags();

  // clear the BIC of any previous Tree IRQs
  for (int i = 0; i < 23; i++) { 
    bic_clear_irq(5, i);
  } 

  // enable watermark IRQs
  mtdcrx(dcr_base + 0x45, 0xF); // inject exception enable register

  return NULL;
}

void 
bgtree_debug(){
  lrt_printf("DEBUG: START\n");
  lrt_printf("DEBUG: node_id: %d \n", node_id);
  lrt_printf("DEBUG: dcr_base: %x \n", (unsigned int)dcr_base);
  for (int chnidx = 0; chnidx < BGP_NUM_CHANNEL; chnidx++){
    lrt_printf("DEBUG: chn %d base_phys: %llx \n", chnidx,(long long unsigned int)channel[chnidx].base_phys);
    lrt_printf("DEBUG: chn %d base: %x \n", chnidx, (unsigned int)channel[chnidx].base);
  }
  lrt_printf("BGTREE DEBUG: END\n");
}

void
bgtree_secondary_init()
{
  uint64_t paddr;
  uint32_t size;
  uintptr_t vaddr;
  for (int chnidx = 0; chnidx < BGP_NUM_CHANNEL; chnidx++){
    paddr = channel[chnidx].base_phys;
    vaddr = channel[chnidx].base;
    size = channel[chnidx].size;
    tlb_map_fixed(paddr, vaddr, size,
        TLB_INHIBIT | TLB_GUARDED);
  }
}

uintptr_t 
bgtree_get_channel(int c){
  LRT_Assert(c < BGP_NUM_CHANNEL);
  return channel[c].base; 
}
