#ifndef L0_LRT_BARE_ARCH_PPC32_LINK
#define L0_LRT_BARE_ARCH_PPC32_LINK

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

/* link layer protocol IDs */
#define BGLINK_P_NET	0x01
#define BGLINK_P_CON	0x10

#if 0
union link_proto_opt {
    u16 raw;
    struct {
  u16 option	: 4;
  u16 pad_head	: 4;
  u16 pad_tail	: 8;
    } opt_net;
    struct {
  u16 len;
    } opt_con;
} __attribute__((packed));

struct bglink_hdr_tree {
    u32 dst_key;
    u32 src_key;
    u16 conn_id;
    u8 this_pkt;
    u8 total_pkt;
    u16 lnk_proto;  // net, con, ...
    union link_proto_opt opt;
} __attribute__((packed));

struct bglink_hdr_torus {
    u32 dst_key;
    u32 len;
    u16 lnk_proto;  // net, con, ...
    union link_proto_opt opt;
} __attribute__((packed));

/* link protocol callbacks
 * rcv is called when new packet arrives
 * flush is called when the device was busy and becomes idle
 *     again (flow control)
 */
struct bglink_proto {
    u16 lnk_proto;
    int receive_from_self;
    int (*tree_rcv)(struct sk_buff*, struct bglink_hdr_tree *);
    int (*tree_flush)(int chn);
    int (*torus_rcv)(struct sk_buff*, struct bglink_hdr_torus *);
    struct list_head list;
};

extern struct list_head linkproto_list;

/* both RCU functions */
void bglink_register_proto(struct bglink_proto *proto);
void bglink_unregister_proto(struct bglink_proto *proto);
struct bglink_proto* bglink_find_proto(u16 proto);
#endif

#if 0
/*
 * Here are some thoughts on how we might better consolidate link headers
 * for the tree and torus.  The idea is that there's an 8-byte packet header
 * that must be sent (at least) once per packet, and an 8-byte fragment header
 * that has to be included with every fragment.  For the tree we can include
 * both headers in every fragment.  For the torus, there's not room to send
 * the packet header in every fragment, so we'd have to send it once as part
 * of the payload in the first fragment (as we're doing now anyway).
 * The various structures might look something like:
 */

struct pkt_hdr {
    u32 lnk_proto    : 8;
    u32 dst_key      : 24;
    u16 len;
    u16 private;
} __attribute__((packed));

struct frag_hdr {
    u32 offset;
    u32 conn_id      : 8;
    u32 src_key      : 24;
} __attribute__((packed));

struct frag_hdr_tree {
    struct pkt_hdr pkt;
    struct frag_hdr frag;
} __attribute__((packed));

struct frag_hdr_torus {
    union torus_fifo_hw_header fifo;
    struct frag_hdr frag;
} __attribute__((packed));
#endif

#endif
