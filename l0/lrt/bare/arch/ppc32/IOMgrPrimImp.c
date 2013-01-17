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

#include <l0/lrt/bare/arch/ppc32/bg_tree.h>
#include <l0/lrt/bare/arch/ppc32/debug.h>
#include <l0/lrt/event_irq_def.h>

/* event definitions */
COBJ_EBBType(TreeObj){
  /* tree device IRQs */
  EBBRC (*InjectionWatermark)(TreeObjRef);
  EBBRC (*ReceiveWatermark)(TreeObjRef);
  EBBRC (*ReceiveVC0)(TreeObjRef);
  EBBRC (*ReceiveVC1)(TreeObjRef);
};

CObject(NullTreeObj) {
  CObjInterface(TreeObj) *ft;
};

TreeObjId theTreeId;
EventNo TreeInjectEvent; /* injection high/low watermark */
EventNo TreeReceiveEvent; /* receive high watermark */
EventNo TreeReceiveEventVC0; /* receive packet interrupt on VC0 */
EventNo TreeReceiveEventVC1; /* receive packet interrupt on VC1 */

EBBRC 
NullTreeObj_Assert(TreeObjRef _self)
{
  LRT_Assert(0); 
  return EBBRC_OK;
}

  EBBRC 
NullTreeObj_Receive(TreeObjRef _self)
{
  /* The null handlered of receive high watermark interrupt */
  /* clear the both channels of the tree.*/
  uintptr_t tree;
  for( int i = 0 ; i < 2; i++){
    tree = bgtree_get_channel(i);
    uint32_t status = *(volatile uint32_t *)(tree + 0x40);
    uint8_t rcv_hdr = status & 0xf;
    while (rcv_hdr > 0) {
      *(volatile uint32_t *)(tree + 0x30); //read header
      // register unsigned int addr asm ("r3") = (unsigned int)tree + 0x20;
      for (int j = 0; j < TREE_PAYLOAD; j += 16) {
        // LFPDX(0,0,3,addr);
        asm("lfpdx 0,0,%0" :: "b" ((unsigned int)tree + 0x20));
      }
      rcv_hdr--;
    }
  }
  bgtree_clear_recv_exception_flags();
  return EBBRC_OK;
}


CObjInterface(TreeObj) NullTreeObj_ftable = {
  /* these handlered tied to tree IRQs */
  .InjectionWatermark = NullTreeObj_Assert,
  .ReceiveWatermark = NullTreeObj_Receive,
  .ReceiveVC0 = NullTreeObj_Assert,
  .ReceiveVC1 = NullTreeObj_Assert,
};

EBBRC
tree_setup(void)
{
  EBBRC rc = EBBAllocPrimId((EBBId *)&theTreeId);
  LRT_RCAssert(rc);

  // alloc the event ids
  rc = COBJ_EBBCALL(theEventMgrPrimId, allocEventNo, &TreeInjectEvent);
  rc = COBJ_EBBCALL(theEventMgrPrimId, allocEventNo, &TreeReceiveEvent);
  rc = COBJ_EBBCALL(theEventMgrPrimId, allocEventNo, &TreeReceiveEventVC0);
  rc = COBJ_EBBCALL(theEventMgrPrimId, allocEventNo, &TreeReceiveEventVC1);
  // 
  rc = COBJ_EBBCALL(theEventMgrPrimId, bindEvent, 
		    TreeInjectEvent, (EBBId)theTreeId, 
		    COBJ_FUNCNUM_FROM_TYPE(CObjInterface(TreeObj), InjectionWatermark));
  //
  rc = COBJ_EBBCALL(theEventMgrPrimId, bindEvent, 
		    TreeReceiveEvent, (EBBId)theTreeId, 
		    COBJ_FUNCNUM_FROM_TYPE(CObjInterface(TreeObj), ReceiveWatermark));
  //
  rc = COBJ_EBBCALL(theEventMgrPrimId, bindEvent, 
		    TreeReceiveEventVC0, (EBBId)theTreeId, 
		    COBJ_FUNCNUM_FROM_TYPE(CObjInterface(TreeObj), ReceiveVC0));
  //
  rc = COBJ_EBBCALL(theEventMgrPrimId, bindEvent, 
		    TreeReceiveEventVC1, (EBBId)theTreeId, 
		    COBJ_FUNCNUM_FROM_TYPE(CObjInterface(TreeObj), ReceiveVC1));

  // route IRQs
  IRQ TreeIRQ;
  bzero(&TreeIRQ, sizeof(IRQ));
  TreeIRQ.isNormalIRQ = true;
  TreeIRQ.group = 5;

  TreeIRQ.irq = 20;

  rc = COBJ_EBBCALL(theEventMgrPrimId, routeIRQ, &TreeIRQ, 
		    TreeReceiveEvent, EVENT_LOC_SINGLE, MyEventLoc());

  TreeIRQ.irq = 21;

  rc = COBJ_EBBCALL(theEventMgrPrimId, routeIRQ, &TreeIRQ, 
		    TreeReceiveEvent, EVENT_LOC_SINGLE, MyEventLoc());

  TreeIRQ.irq = 22;

  rc = COBJ_EBBCALL(theEventMgrPrimId, routeIRQ, &TreeIRQ, 
		    TreeReceiveEvent, EVENT_LOC_SINGLE, MyEventLoc());

  TreeIRQ.irq = 23;

  rc = COBJ_EBBCALL(theEventMgrPrimId, routeIRQ, &TreeIRQ, 
		    TreeReceiveEvent, EVENT_LOC_SINGLE, MyEventLoc());

  return EBBRC_OK;
}

EBBRC
NullTree_setup(TreeObjId id)
{
  EBBRC rc; 
  NullTreeObjRef repRef;
  CObjEBBRootSharedRef rootRef;

  /* alloc a rep */
  rc = EBBPrimMalloc(sizeof(NullTreeObj), &repRef, EBB_MEM_DEFAULT);
  LRT_RCAssert(rc);

  //setup the rep
  repRef->ft = &NullTreeObj_ftable;

  rc = CObjEBBRootSharedCreate(&rootRef, (EBBRepRef)repRef);
  LRT_RCAssert(rc);

  rc = CObjEBBBind((EBBId)id, rootRef);
  LRT_RCAssert(rc);

  return EBBRC_OK;
}


EBBRC
IOMgrPrimImpInit(void)
{
  EBBRC rc;
  rc = tree_setup();
  LRT_RCAssert(rc);
  rc = NullTree_setup(theTreeId);
  LRT_RCAssert(rc);
  lrt_printf("Tree initialized\n");
  
  return EBBRC_OK;
}

