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

struct channel_t
{
	uintptr_t base;		// virtual base address of tree
	uint64_t base_phys;	// phys location
  uint32_t size;
} __attribute__((packed));

FILE *bgtree_init(void);
void bgtree_secondary_init(void);
uintptr_t bgtree_get_channel(int c);
void bgtree_debug(void);
void bgtree_clear_inj_exception_flags(void);
void bgtree_clear_recv_exception_flags(void);

#endif

/* hardware header from L4 

// tree ifc memory offsets
#define BGP_TRx_DI		(0x00U)
#define BGP_TRx_HI		(0x10U)
#define BGP_TRx_DR		(0x20U)
#define BGP_TRx_HR		(0x30U)
#define BGP_TRx_Sx		(0x40U)

struct bgtree_header_t
{
    union {
	word_t raw;
	struct {
	    word_t pclass	: 4;
	    word_t p2p		: 1;
	    word_t irq		: 1;
	    word_t vector	: 24;
	    word_t csum_mode	: 2;
	} p2p;
	struct {
	    word_t pclass	: 4;
	    word_t p2p		: 1;
	    word_t irq		: 1;
	    word_t op		: 3;
	    word_t opsize	: 7;
	    word_t tag		: 14;
	    word_t csum_mode	: 2;
	} bcast;
    };
} __attribute__((packed));


struct bgtree_status_t 
{
    union {
	word_t raw;
	struct {
	    word_t inj_pkt	: 4;
	    word_t inj_qwords	: 4;
	    word_t		: 4;      
	    word_t inj_hdr	: 4;
	    word_t rcv_pkt	: 4;
	    word_t rcv_qwords	: 4;
	    word_t		: 3;
	    word_t irq		: 1;
	    word_t rcv_hdr	: 4;
	};
    };
} __attribute__((packed));
*/
/* link layer from L4 
struct bglink_hdr_t
{
    word_t dst_key; 
    word_t src_key; 
    u16_t conn_id; 
    u8_t this_pkt; 
    u8_t total_pkt;
    u16_t lnk_proto;	// 1 eth, 2 con, 3...
    u16_t optional;	// for encapsulated protocol use
} __attribute__((packed));
*/
#if 0

    struct channel_t 
    {
    public:
	addr_t base;		// virtual base address of tree
	paddr_t base_phys;	// phys location
	
	void send_header(bgtree_header_t *hdr)
	    { out_be32(addr_offset(base, BGP_TRx_HI), hdr->raw); }

	void send_payload_block(void *payload)
	    { out128(addr_offset(base, BGP_TRx_DI), payload); }

	void rcv_payload_block(void *payload)
	    { in128(addr_offset(base, BGP_TRx_DR), payload); }

	bgtree_header_t get_header()
	    { 
		bgtree_header_t hdr;
		hdr.raw = in_be32(addr_offset(base, BGP_TRx_HR)); 
		return hdr;
	    }
	
	bgtree_status_t get_status()
	    { 
		bgtree_status_t status;
		status.raw = in_be32(addr_offset(base, BGP_TRx_Sx)); 
		return status;
	    }

	bool init(int channel, paddr_t pbase, size_t size);
	bool send(bgtree_header_t hdr, bglink_hdr_t &lnkhdr, void *payload);
	bool poll(bglink_hdr_t *lnkhdr, void *payload);
    };

    channel_t channel[BGP_NUM_CHANNEL];

    word_t dcr_base;
    word_t curr_conn;
    word_t node_id;	// self
#endif 


