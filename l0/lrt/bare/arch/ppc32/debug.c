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

#include <l0/lrt/bare/stdio.h>
#include <l0/lrt/bare/arch/ppc32/fdt.h>
#include <l0/lrt/bare/arch/ppc32/mmu.h>
#include <lrt/string.h>
#include <sync/misc.h>

#define BREAKPOINT(COUNT)                          \
  if(get_debug_status()){ while(get_debug_val() == COUNT); }  \


#define BREAK_SET(VAL)                          \
  if(get_debug_status()){ set_debug_val(VAL); }  \

typedef struct bgp_mailbox {
  volatile uint16_t command;	// comand; upper bit=ack
  uint16_t len;		// length (does not include header)
  uint16_t result;		// return code from reader
  uint16_t crc;		// 0=no CRC
  char data[0];
} bgp_mailbox;

static const int MB_MAP_SIZE = 1 << 10; //1K
static uint64_t paddr_aligned;
static uintptr_t vaddr;
static int debug_status = 0;
static bgp_mailbox *debug;

void
debug_init(void)
{
  struct fdt_node *console = fdt_get("/jtag/console1");
  struct fdt_node *reg = fdt_get_in_node(console, "reg");
  uint64_t paddr = fdt_read_prop_u64(reg, 0);
  paddr |= 0x700000000LL; //THIS IS BECAUSE UBOOT IS BROKEN  
  paddr_aligned = paddr & ~(MB_MAP_SIZE - 1); //align
  vaddr = (uintptr_t)tlb_map(paddr_aligned, MB_MAP_SIZE,
			     TLB_INHIBIT | TLB_GUARDED);
  debug = (bgp_mailbox *)(vaddr + (uintptr_t)(paddr - paddr_aligned));
  debug->data[0] = 0;
  debug_status = 1;
  return ;
}
void
debug_secondary_init()
{
  tlb_map_fixed(paddr_aligned, vaddr, MB_MAP_SIZE,
		TLB_INHIBIT | TLB_GUARDED);
}

int
get_debug_status(void){
  int val = debug_status;
  return val;
}

void
set_debug_val(int val){
  debug->data[0] = val;
  return ;
}

int
get_debug_val(void){
 int val = ACCESS_ONCE(debug->data[0]);
 return val;
}


