#ifndef L0_LRT_BARE_ARCH_PPC32_TREE
#define L0_LRT_BARE_ARCH_PPC32_TREE

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

#include <l0/lrt/bare/stdio.h>

#define BGP_NUM_CHANNEL		2

#define TREE_IRQ_GROUP		5
#define TREE_IRQ_BASE		20
#define TREE_IRQ_NONCRIT_NUM	20
#define TREE_NONCRIT_BASE	0
#define TREE_FIFO_SIZE		8
#define TREE_PAYLOAD		256

#define _BGP_TRx_DI      (0x00)    // Offset from Tree VCx for Data   Injection   (WO,Quad)
#define _BGP_TRx_HI      (0x10)    // Offset from Tree VCx for Header Injection   (WO,Word)
#define _BGP_TRx_DR      (0x20)    // Offset from Tree VCx for Data   Reception   (RO,Quad)
#define _BGP_TRx_HR      (0x30)    // Offset from Tree VCx for Header Reception   (RO,Word)
#define _BGP_TRx_Sx      (0x40)    // Offset from Tree VCx for Status             (RO,Word)
#define _BGP_TRx_SO      (0x50)    // Offset from Tree VCx for Status of Other VC (RO,Word)

// Virtual Addresses for Tree VC0
#define _BGP_TR0_DI    (_BGP_VA_TREE0 | _BGP_TRx_DI)
#define _BGP_TR0_HI    (_BGP_VA_TREE0 | _BGP_TRx_HI)
#define _BGP_TR0_DR    (_BGP_VA_TREE0 | _BGP_TRx_DR)
#define _BGP_TR0_HR    (_BGP_VA_TREE0 | _BGP_TRx_HR)
#define _BGP_TR0_S0    (_BGP_VA_TREE0 | _BGP_TRx_Sx)
#define _BGP_TR0_S1    (_BGP_VA_TREE0 | _BGP_TRx_SO)

// Virtual Addresses for Tree VC1
#define _BGP_TR1_DI    (_BGP_VA_TREE1 | _BGP_TRx_DI)
#define _BGP_TR1_HI    (_BGP_VA_TREE1 | _BGP_TRx_HI)
#define _BGP_TR1_DR    (_BGP_VA_TREE1 | _BGP_TRx_DR)
#define _BGP_TR1_HR    (_BGP_VA_TREE1 | _BGP_TRx_HR)
#define _BGP_TR1_S1    (_BGP_VA_TREE1 | _BGP_TRx_Sx)
#define _BGP_TR1_S0    (_BGP_VA_TREE1 | _BGP_TRx_SO)

// Packet Payload: fixed size for all Tree packets
#define _BGP_TREE_PKT_MAX_BYTES    (256)       // bytes in a tree packet
#define _BGP_TREE_PKT_MAX_SHORT    (128)
#define _BGP_TREE_PKT_MAX_LONG      (64)
#define _BGP_TREE_PKT_MAX_LONGLONG  (32)
#define _BGP_TREE_PKT_MAX_QUADS     (16)       // quads in a tree packet

#if 0
/* mystical double-hummer optcode translation macros */
#define LFPDX(frt, ra, rb, addr) \
  asm volatile( ".long %[val]" \
      : \
      : [val] "i" ((31<<26)|((frt)<<21)|((ra)<<16)|((rb)<<11)|(462<<1)),  \
      "r" (addr) \
      ); 

#define STFPDX(frt, ra, rb, addr)  \
  asm volatile( ".long %[val]" \
      : \
      : [val] "i" ((31<<26)|((frt)<<21)|((ra)<<16)|((rb)<<11)|(974<<1)) \
      "r" (addr) \
      );
#endif

union bgtree_header {
  unsigned int raw;
  struct {
    unsigned int pclass	: 4;
    unsigned int p2p	: 1;
    unsigned int irq	: 1;
    unsigned vector		: 24;
    unsigned int csum_mode	: 2;
  } p2p;
  struct {
    unsigned int pclass	: 4;
    unsigned int p2p	: 1;
    unsigned int irq	: 1;
    unsigned int op		: 3;
    unsigned int opsize	: 7;
    unsigned int tag	: 14;
    unsigned int csum_mode	: 2;
  } bcast;
} __attribute__((packed));

union bgtree_status {
  unsigned int raw;
  struct {
    unsigned int inj_pkt	: 4;
    unsigned int inj_qwords	: 4;
    unsigned int __res0	: 4;
    unsigned int inj_hdr	: 4;
    unsigned int rcv_pkt	: 4;
    unsigned int rcv_qwords : 4;
    unsigned int __res1	: 3;
    unsigned int irq	: 1;
    unsigned int rcv_hdr	: 4;
  } x;
} __attribute__((packed));

struct channel_t
{
  uintptr_t base;		// virtual base address of tree
  uint64_t base_phys;	// phys location
  uint32_t size;
} __attribute__((packed));

union link_proto_opt {
  uint16_t raw;
  struct {
    uint16_t option	: 4;
    uint16_t pad_head	: 4;
    uint16_t pad_tail	: 8;
  } opt_net;
  struct {
    uint16_t len;
  } opt_con;
} __attribute__((packed));

struct bglink_hdr_tree {
    uint32_t dst_key;
    uint32_t src_key;
    uint16_t conn_id;
    uint8_t this_pkt;
    uint8_t total_pkt;
    uint16_t lnk_proto;  // net, con, ...
    union link_proto_opt opt;
} __attribute__((packed));


#define TREE_LNKHDRLEN		(sizeof(struct bglink_hdr_tree))
#define TREE_FRAGPAYLOAD	(TREE_PAYLOAD - TREE_LNKHDRLEN)

FILE *bgtree_init(void);
void bgtree_secondary_init(void);
uintptr_t bgtree_get_channel(int);
void bgtree_debug(void);
void bgtree_inject_packet(uint32_t *, void *, void *, uint32_t);
void bgtree_receive_240(void *, uint32_t);
void bgtree_clear_inj_exception_flags(void);
void bgtree_clear_recv_exception_flags(void);

#endif
