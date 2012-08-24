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
//
FILE *
bgtree_init()
{
  uint64_t paddr, paddr_aligned;
  uint32_t size;
  uintptr_t vaddr;

  // lookup tree in fdt
  bg_tree = fdt_get("/plb/tree");
  reg = fdt_get_in_node(bg_tree, "reg");
  node = fdt_get_in_node(bg_tree, "nodeid");
  dcr_reg = fdt_get_in_node(bg_tree, "dcr-reg");
  // get node_id
  node_id = fdt_read_prop_u32(node, 0); //8?
  // device control register base address
  dcr_base = fdt_read_prop_u32(dcr_reg, 0);

  // map each tree channel into TLB 
  for (int chnidx = 0; chnidx < BGP_NUM_CHANNEL; chnidx++)
  {
    // *8 instead?
    paddr = fdt_read_prop_u64(reg, (chnidx*(12)));
    size = fdt_read_prop_u32(reg, (chnidx*(12)+8));
    // map into virtual memory
    paddr_aligned = paddr & ~((uint64_t)size - 1); //align
    vaddr = (uintptr_t)tlb_map(paddr_aligned, size,
        TLB_INHIBIT | TLB_GUARDED);
    channel[chnidx].base_phys = paddr_aligned;
    channel[chnidx].base = vaddr;
    channel[chnidx].size = size;
    // disable send and recive IRQs
    mtdcrx(dcr_base + 0x45, 0);
    mtdcrx(dcr_base + 0x49, 0);

    // clear pending IRQs
    mfdcrx(dcr_base + 0x44);
    mfdcrx(dcr_base + 0x48);

    for (int i = 0; i < 24; i++) {
      bic_clear_irq(4, i);
    }

    // setup reception watermarks
    // potential bug? We're clearing the other bits
    mtdcrx(dcr_base + 0x42, 0x40);
    mtdcrx(dcr_base + 0x43, 0x40);

    // enable both watermark interrupts
    mtdcrx(dcr_base + 0x45, 0x3);
  }
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
