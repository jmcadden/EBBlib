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
#include <l0/lrt/bare/arch/ppc32/debug.c>
#include <l0/lrt/event_irq_def.h>

//TODO: move these macros
/* mystical double-hummer optcode translation */
#define LFPDX(frt, ra, rb, addr) \
            asm volatile( ".long %[val]" \
                : \
                : [val] "i" ((31<<26)|((frt)<<21)|((ra)<<16)|((rb)<<11)|(462<<1)),  \
                  "r" (addr) \
              ); 
#define STFPDX(frt, ra, rb)     .long (31<<26)|((frt)<<21)|((ra)<<16)|((rb)<<11)|(974<<1)

COBJ_EBBType(TreeObj){
//  EVENTFUNC(Highwatermark); 
  EBBRC (*HighWatermark0)(TreeObjRef);
  EBBRC (*HighWatermark1)(TreeObjRef);
};

CObject(NullTreeObj) {
  CObjInterface(TreeObj) *ft;
};

EBBRC 
NullTreeObj_HighWatermark0(TreeObjRef _self)
{
  lrt_printf("Clearing Channel 0\n");
  uintptr_t tree;
  // clear the tree
  tree = bgtree_get_channel(0);
  uint32_t status = *(volatile uint32_t *)(tree + 0x40);
  uint8_t rcv_hdr = status & 0xf;
  while (rcv_hdr > 0) {
    *(volatile uint32_t *)(tree + 0x30); //read header
    register unsigned int addr asm ("r3") = (unsigned int)tree + 0x20;
    for (int j = 0; j < 256; j += 16) {
      LFPDX(0,0,3,addr);
    }
    rcv_hdr--;
  }
  return EBBRC_OK;
}

EBBRC 
NullTreeObj_HighWatermark1(TreeObjRef _self)
{
  lrt_printf("Clearing Channel 1\n");
  uintptr_t tree;
  // clear the tree
  tree = bgtree_get_channel(1);
  uint32_t status = *(volatile uint32_t *)(tree + 0x40);
  uint8_t rcv_hdr = status & 0xf;
  while (rcv_hdr > 0) {
    *(volatile uint32_t *)(tree + 0x30); //read header
    register unsigned int addr asm ("r3") = (unsigned int)tree + 0x20;
    for (int j = 0; j < 256; j += 16) {
      LFPDX(0,0,3,addr);
    }
    rcv_hdr--;
  }
  return EBBRC_OK;
}

CObjInterface(TreeObj) NullTreeObj_ftable = {
  .HighWatermark0 = NullTreeObj_HighWatermark0,
  .HighWatermark1 = NullTreeObj_HighWatermark1
};

TreeObjId theTreeId;
EventNo treeHighWaterMark0Event;
EventNo treeHighWaterMark1Event;

void
tree_setup(void)
{
  EBBRC rc = EBBAllocPrimId((EBBId *)&theTreeId);
  LRT_RCAssert(rc);

  rc = COBJ_EBBCALL(theEventMgrPrimId, allocEventNo, 
		    &treeHighWaterMark0Event);
  rc = COBJ_EBBCALL(theEventMgrPrimId, bindEvent, 
		    treeHighWaterMark0Event, (EBBId)theTreeId, 
		    COBJ_FUNCNUM_FROM_TYPE(CObjInterface(TreeObj), HighWatermark0));
  rc = COBJ_EBBCALL(theEventMgrPrimId, allocEventNo, 
		    &treeHighWaterMark1Event);
  rc = COBJ_EBBCALL(theEventMgrPrimId, bindEvent, 
		    treeHighWaterMark1Event, (EBBId)theTreeId, 
		    COBJ_FUNCNUM_FROM_TYPE(CObjInterface(TreeObj), HighWatermark1));
  IRQ HighWatermarkIRQ;
  bzero(&HighWatermarkIRQ, sizeof(IRQ));
  HighWatermarkIRQ.isNormalIRQ = true;
  HighWatermarkIRQ.group = 4;
  HighWatermarkIRQ.irq = 22;
  rc = COBJ_EBBCALL(theEventMgrPrimId, routeIRQ, &HighWatermarkIRQ, 
		    treeHighWaterMark0Event, EVENT_LOC_SINGLE, MyEventLoc());
  HighWatermarkIRQ.irq = 23;
  rc = COBJ_EBBCALL(theEventMgrPrimId, routeIRQ, &HighWatermarkIRQ, 
		    treeHighWaterMark1Event, EVENT_LOC_SINGLE, MyEventLoc());
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
  /* alloc and event and bind to our rep */
  rc = CObjEBBRootSharedCreate(&rootRef, (EBBRepRef)repRef);
  LRT_RCAssert(rc);

  rc = CObjEBBBind((EBBId)id, rootRef);
  LRT_RCAssert(rc);

  return EBBRC_OK;
}


/* TreeTst App Object */

CObject(TreeTst) {
  CObjInterface(App) *ft;
};

EBBRC
Treetst_start(AppRef _self){
  EBBRC rc;
  lrt_printf("TreeTst: test started\n");
  tree_setup();
  rc = NullTree_setup(theTreeId);
  LRT_RCAssert(rc);
  lrt_printf("TreeTst: TreeObj setup. We will now wait..\n");
  
  return EBBRC_OK;
}

CObjInterface(App) TreeTst_ftable = {
  .start = Treetst_start
};

APP_START_ONE(TreeTst);
