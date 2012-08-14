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

struct bgp_mailbox_desc {
  uint16_t offset;	// offset from SRAM base
  uint16_t size;	// size including header, 0=not present
} __attribute__((packed));

typedef struct bgp_mailbox {
  volatile uint16_t command;	// comand; upper bit=ack
  uint16_t len;		// length (does not include header)
  uint16_t result;		// return code from reader
  uint16_t crc;		// 0=no CRC
  char data[0];
} bgp_mailbox;


static int bgp_dcr_glob_att_write_set;
static int bgp_dcr_glob_att_write_clear;
static int bgp_dcr_mask;

static bgp_mailbox *bgp_mbox;
static int bgp_mbox_size;
static char bgp_mbox_buffer[256];
static uintptr_t bgp_mbox_buffer_len = 0;

static int
mailbox_putc(int c)
{
  bgp_mbox_buffer[bgp_mbox_buffer_len++] = c;
  
  if (bgp_mbox_buffer_len >= bgp_mbox_size || c == '\n') {
    memcpy(&bgp_mbox->data, bgp_mbox_buffer, bgp_mbox_buffer_len);
    bgp_mbox->len = bgp_mbox_buffer_len;
    bgp_mbox->command = 2;
    asm volatile (
		  "mbar;"
		  "mtdcrx %[dcrn], %[val];"
		  :
		  : [dcrn] "r" (bgp_dcr_glob_att_write_set),
		    [val] "r" (bgp_dcr_mask),
		    "m" (*bgp_mbox)
		  );
    do {
      asm volatile (
		    "dcbi 0, %[addr]"
		    :
		    : [addr] "b" (&(bgp_mbox->command))
		    );
    } while (!(bgp_mbox->command & 0x8000));

    asm volatile (
    		  "mtdcrx %[dcrn], %[val];"
    		  :
    		  : [dcrn] "r" (bgp_dcr_glob_att_write_clear),
    		    [val] "r" (bgp_dcr_mask),
    		    "m" (*bgp_mbox)
    		  );
    bgp_mbox_buffer_len = 0;
  }
  return c;
}

#include <l0/lrt/event.h>

static int
mailbox_write(uintptr_t cookie, const char *str, int len) 
{
  for (int i = 0; i < len; i++) {
    mailbox_putc((int)str[i]);
  }
  return 0;
}

FILE mailbox = {
  .cookie = 0,
  .write = mailbox_write
};

static const int MB_MAP_SIZE = 1 << 10; //1K

static uint64_t paddr_aligned;
static uintptr_t vaddr;

FILE *
mailbox_init()
{
  struct fdt_node *console = fdt_get("/jtag/console0");
  struct fdt_node *reg = fdt_get_in_node(console, "reg");
  uint64_t paddr = fdt_read_prop_u64(reg, 0);
  paddr |= 0x700000000LL; //THIS IS BECAUSE UBOOT IS BROKEN  
  paddr_aligned = paddr & ~(MB_MAP_SIZE - 1); //align
  vaddr = (uintptr_t)tlb_map(paddr_aligned, MB_MAP_SIZE,
			     TLB_INHIBIT | TLB_GUARDED);
  bgp_mbox = (bgp_mailbox *)(vaddr + (uintptr_t)(paddr_aligned - paddr));
  
  bgp_mbox_size = fdt_read_prop_u32(reg, 8);

  struct fdt_node *dcr_reg = fdt_get_in_node(console, "dcr-reg");
  bgp_dcr_glob_att_write_set = fdt_read_prop_u32(dcr_reg, 0);
  bgp_dcr_glob_att_write_clear = fdt_read_prop_u32(dcr_reg, 4);

  struct fdt_node *dcr_mask = fdt_get_in_node(console, "dcr-mask");
  bgp_dcr_mask = fdt_read_prop_u32(dcr_mask, 0);

  return &mailbox;
}

void
mailbox_secondary_init()
{
  tlb_map_fixed(paddr_aligned, vaddr, MB_MAP_SIZE,
		TLB_INHIBIT | TLB_GUARDED);
}
