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

#include <l0/lrt/types.h>
#include <l0/cobj/cobj.h>
#include <lrt/io.h>
#include <lrt/exit.h>
#include <lrt/string.h>
#include <l0/lrt/trans.h>
#include <lrt/assert.h>
#include <l0/cobj/CObjEBB.h>
#include <l0/EBBMgrPrim.h>
#include <l0/cobj/CObjEBBUtils.h>
#include <l0/cobj/CObjEBBRoot.h>
#include <l0/cobj/CObjEBBRootShared.h>
#include <l0/cobj/CObjEBBRootMulti.h>
#include <l0/cobj/CObjEBBRootMultiImp.h>
#include <l0/EventMgrPrim.h>
#include <l0/EventMgrPrimImp.h>
#include <l0/MemMgr.h>
#include <l0/MemMgrPrim.h>
#include <l1/App.h>
#include <l1/startinfo.h>

#include <l0/lrt/bare/arch/ppc32/bg_tree.h>
#include <l0/lrt/bare/arch/ppc32/link.h>
#include <l0/lrt/bare/arch/ppc32/debug.h>
#include <l0/lrt/event_irq_def.h>

struct package{
  int i;
};


/* TreeTst App Object */

CObject(TreeTst) {
  CObjInterface(App) *ft;
};

EBBRC
Treetst_rcv_print(union bgtree_header dst, struct bglink_hdr_tree lnkhdr, struct package *p){

  
  /*
	TRACE("bgnet: stat=%x, dst=%x, hdr: conn=%x, "
	      "this_pkt=%x, tot_pkt=%x, dst=%x, src=%x]\n",
	      status.raw, dst.raw, lnkhdr.conn_id,
	      lnkhdr.this_pkt, lnkhdr.total_pkt,
	      lnkhdr.dst_key, lnkhdr.src_key);
*/

  lrt_printf("Recieved: dst=%x, proto=%d, payload[%d] \n", dst.raw, lnkhdr.lnk_proto, p->i);
  return EBBRC_OK;
}

EBBRC
Treetst_rcv(void)
{
  lrt_printf("Receive\n");
  struct bglink_hdr_tree lnkhdr __attribute__((aligned(16)));
  unsigned i;
  void *payloadptr;
  union bgtree_status status;
  union bgtree_header dst;
  uintptr_t tree;
  EBBRC rc = 0;

  // receive on both channels
  for( i = 0 ; i < BGP_NUM_CHANNEL; i++){
    tree = bgtree_get_channel(i);
    status.raw = *(volatile uint32_t *)(tree + 0x40); /* channel status */
    while (status.x.rcv_hdr) {  /*while we have packets to read */
      dst.raw = *(volatile uint32_t *)(tree + 0x30); /* read header*/
      // XXX: disregarding packet header
      // let's grab the link header from the payload
      lnkhdr = *(volatile struct bglink_hdr_tree *)(tree + _BGP_TRx_DR);
      // XXX: disregarding protocol, size, pkt count
      // allocate and retrieve payload
      rc = EBBPrimMalloc((TREE_FRAGPAYLOAD), &payloadptr, EBB_MEM_DEFAULT);
      LRT_RCAssert(rc);
      bgtree_receive_240(payloadptr, tree); /* read payload */
      // so, lets print our data
      if(lnkhdr.lnk_proto == 9)
        Treetst_rcv_print(dst, lnkhdr, (struct package *)payloadptr);

      // pick up packets that arrived meanwhile...
      if ( (status.x.rcv_hdr--) == 0)
        status.raw = *(volatile uint32_t *)(tree + 0x40); /* channel status */
    }
  }
  return EBBRC_OK;
}

EBBRC
Treetst_snd(int ch, int i){

  struct bglink_hdr_tree lnkhdr __attribute__((aligned(16)));
  union bgtree_header dest;
  void *payloadptr = 0;
  uintptr_t tree;
  struct package p;

  // init datastructures
  EBBPrimMalloc((TREE_FRAGPAYLOAD), payloadptr, EBB_MEM_DEFAULT);
  bzero(payloadptr, TREE_FRAGPAYLOAD);
  bzero(&dest, sizeof(union bgtree_header)); 
  bzero(&lnkhdr, sizeof(struct bglink_hdr_tree)); 

  tree = bgtree_get_channel(ch);
  p.i = i; /* send value */

  /* configure link header */
  //TODO:
  /* uint32_t dst_key;
     uint32_t src_key;
     uint16_t conn_id;
     uint8_t this_pkt;
     uint8_t total_pkt;
     uint16_t lnk_proto;  // net, con, ...
     union link_proto_opt opt;
     */
   lnkhdr.lnk_proto  = 0x10;
   lnkhdr.dst_key  = 0x10;
  /* configure pkt header */
  /*
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
     } 
     */
   dest.bcast.pclass = 15; //tree_route
   dest.bcast.p2p = 0; // broadcast = true

  memcpy((uintptr_t *)(tree + _BGP_TRx_HI), (&dest.raw), sizeof(union bgtree_header));/*write pkt header */
  memcpy((uintptr_t *)(tree + _BGP_TRx_DI), &lnkhdr, sizeof(struct bglink_hdr_tree)); /*write lnk header */
  memcpy(payloadptr, &p, sizeof(struct package));

  bgtree_inject_packet(&dest.raw, &lnkhdr, payloadptr, tree);

  return EBBRC_OK;
}

EBBRC
Treetst_start(AppRef _self){
  lrt_printf("Run treetest\n");

  lrt_printf("send msg to con \n");
  /* CON WRITE TEST -- send msgs to the linux console */
  Treetst_snd(0,987);

  lrt_printf("msg sent\n");
  return EBBRC_OK;
}

CObjInterface(App) TreeTst_ftable = {
  .start = Treetst_start
};

APP_START_ONE(TreeTst);





