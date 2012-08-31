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
#include <inttypes.h>

#include <l0/lrt/types.h>
#include <l0/cobj/cobj.h>
#include <lrt/io.h>
#include <l0/lrt/trans.h>
#include <lrt/assert.h>
#include <l0/cobj/CObjEBB.h>
#include <l0/EBBMgrPrim.h>
#include <l0/cobj/CObjEBBUtils.h>
#include <l0/cobj/CObjEBBRoot.h>
#include <l0/cobj/CObjEBBRootMulti.h>
#include <l0/cobj/CObjEBBRootMultiImp.h>
#include <l0/EventMgrPrim.h>
#include <l0/EventMgrPrimImp.h>
#include <l0/MemMgr.h>
#include <l0/MemMgrPrim.h>
#include <l0/lrt/event.h>
#include <l0/lrt/event_irq_def.h>
#include <l0/lrt/bare/arch/ppc32/bic.h>
#include <lrt/string.h>

STATIC_ASSERT(LRT_EVENT_NUM_EVENTS % 8 == 0,
              "num allocatable events isn't divisible by 8");
static uint8_t alloc_table[LRT_EVENT_NUM_EVENTS / 8];
static EventNo irq_table[BIC_NUM_GROUPS][BIC_NUM_IRQS];

struct event_bvs {
  uint32_t vec[8];
  char packing[CACHE_LINE_ALIGNMENT - (sizeof(uint32_t) * 8)];
} _ALIGN_CACHE_;

static inline void
event_set_bit_bv(struct event_bvs *bv, int bit)
{
  int word = bit / 32;
  uint32_t mask = (uint32_t)1 << (bit % 32);
  __sync_or_and_fetch(&bv->vec[word], mask);
}

static inline int
event_get_unset_bit_bv(struct event_bvs *bv)
{
  for (int word = 7; word >= 0; word--) {
    uint32_t val = ACCESS_ONCE(bv->vec[word]);
    if (val) {
      int bit = 31 - __builtin_clz(val);
      __sync_and_and_fetch(&bv->vec[word], ~(1 << bit));
      return word * 32 + bit;
    }
  }
  return -1;
}

CObject(EventMgrPrimImp){
  CObjInterface(EventMgrPrim) *ft;

  // for now, make this share descriptor tables, may replicate
  // later
  struct lrt_event_descriptor *lrt_event_table_ptr;
  EventMgrPrimImpRef *reps;
  CObjEBBRootMultiRef theRoot;
  EventLoc eventLoc;
  struct event_bvs bv;
};

static EBBRC
EventMgrPrimImp_allocEventNo(EventMgrPrimRef _self, EventNo *eventNoPtr)
{
  int i;
  //we start from the beginning and just find the first
  // unallocated event
  for (i = 0; i < LRT_EVENT_NUM_EVENTS; i++) {
    uint8_t res = __sync_fetch_and_or(&alloc_table[i / 8], 1 << (i % 8));
    if (!(res & (1 << (i % 8)))) {
      break;
    }
  }
  if (i >= LRT_EVENT_NUM_EVENTS) {
    return EBBRC_OUTOFRESOURCES;
  }
  *eventNoPtr = i;
  return EBBRC_OK;
}

static EBBRC
EventMgrPrimImp_freeEventNo(EventMgrPrimRef _self, EventNo eventNo)
{
  __sync_fetch_and_and(&alloc_table[eventNo / 8], ~(1 << (eventNo % 8)));
  return EBBRC_OK;
}

static EBBRC
EventMgrPrimImp_bindEvent(EventMgrPrimRef _self, EventNo eventNo,
          EBBId handler, EBBFuncNum fn)
{
  EventMgrPrimImpRef self = (EventMgrPrimImpRef)_self;
  self->lrt_event_table_ptr[eventNo].id = handler;
  self->lrt_event_table_ptr[eventNo].fnum = fn;
  return EBBRC_OK;
}

static EBBRC
EventMgrPrimImp_routeIRQ(EventMgrPrimRef _self, IRQ *isrc, EventNo eventNo,
                      enum EventLocDesc desc, EventLoc el)
{
  LRT_Assert(desc == EVENT_LOC_SINGLE);
  LRT_Assert(isrc->isNormalIRQ);
  irq_table[isrc->group][isrc->irq] = eventNo;
  bic_enable_irq(isrc->group, isrc->irq, NONCRIT, el);
  return EBBRC_OK;
}

static inline EventMgrPrimImpRef
findTarget(EventMgrPrimImpRef self, EventLoc loc)
{
  RepListNode *node;
  EBBRep * rep = NULL;

  while (1) {
    rep = (EBBRep *)self->reps[loc];
    if (rep == NULL) {
      for (node = self->theRoot->ft->nextRep(self->theRoot, 0, &rep);
           node != NULL;
           node = self->theRoot->ft->nextRep(self->theRoot, node, &rep)) {
        LRT_Assert(rep != NULL);
        if (((EventMgrPrimImpRef)rep)->eventLoc == loc) break;
      }
      self->reps[loc] = (EventMgrPrimImpRef)rep;
    }
    // FIXME: handle case that rep doesn't yet exist
    if (rep != NULL) {
      return (EventMgrPrimImpRef)rep;
    }
    lrt_printf("x");
  }
  LRT_Assert(0);/* can't get here */
}

static EBBRC
EventMgrPrimImp_triggerEvent(EventMgrPrimRef _self, EventNo eventNo,
                          enum EventLocDesc desc, EventLoc el)
{
  EventMgrPrimImpRef self = (EventMgrPrimImpRef)_self;
  LRT_Assert(el < lrt_num_event_loc());
  LRT_Assert(desc == EVENT_LOC_SINGLE);

  EventMgrPrimImpRef rep = self;

  if (el != lrt_my_event_loc()) {
    rep = findTarget(self, el);
  }

  event_set_bit_bv(&rep->bv, eventNo);
  
  if (el != lrt_my_event_loc()) {
    bic_raise_irq(BIC_IPI_GROUP, el);
  }

  return EBBRC_OK;
}

static EBBRC
EventMgrPrimImp_dispatchEvent(EventMgrPrimRef _self, EventNo eventNo)
{ 
  EventMgrPrimImpRef self = (EventMgrPrimImpRef)_self;
  struct lrt_event_descriptor *desc = &self->lrt_event_table_ptr[eventNo];
  lrt_trans_id id = desc->id;
  lrt_trans_func_num fnum = desc->fnum;

  //this infrastructure should be pulled out of this file
  lrt_trans_rep_ref ref = lrt_trans_id_dref(id);
  ref->ft[fnum](ref);
  return EBBRC_OK;
}

static EBBRC
EventMgrPrimImp_dispatchIRQ(EventMgrPrimRef _self)
{ 
  EventMgrPrimImpRef self = (EventMgrPrimImpRef)_self;
  // check which group raised an irq
  unsigned int group = bic_get_core_noncrit(lrt_my_event_loc());
  int event;
  // check if IPI occured
  if (group & (1 << (31 - BIC_IPI_GROUP))) {
    // We know the only IRQ that could have fired on our core
    // in this group is a specific IRQ
    bic_clear_irq(BIC_IPI_GROUP, MyEventLoc());
    //FIXME this could run unboundedly
    while (1) {
      event = event_get_unset_bit_bv(&self->bv);
      if (event == -1)
        break;
      EventMgrPrimImp_dispatchEvent(_self, event);
    }
    // else, not an IPI 
  } else {
    int group_num = __builtin_clz(group);
    int status = bic_get_status(group_num);
    // check each IRQ on the bitvector
    while (status != 0) {
      int irq_num = __builtin_clz(status);
      if (bic_targeted_to(group_num, irq_num, NONCRIT, MyEventLoc())) {
        EventMgrPrimImp_dispatchEvent(_self, irq_table[group_num][irq_num]);
        bic_clear_irq(group_num, irq_num);
        return EBBRC_OK;
      }
      status &= ~(1 << (31 - irq_num));
    }
  } 
  return EBBRC_OK;
}

static EBBRC
EventMgrPrimImp_enableInterrupts(EventMgrPrimRef _self)
{
  EventMgrPrimImpRef self = (EventMgrPrimImpRef)_self;
  int bit = event_get_unset_bit_bv(&self->bv);
  if (bit != -1) {
    EventMgrPrimImp_dispatchEvent(_self, bit);
    return EBBRC_OK;
 }
 
  msr msr;
  asm volatile (
		"mfmsr %[msr]"
		: [msr] "=r" (msr)
		);
  msr.we = 1;
  msr.ee = 1;
  asm volatile (
		"mtmsr %[msr]"
		:
		: [msr] "r" (msr)
		: "r0", "r3", "r4", "r5", "r6", "r7", "r8",
		  "r9", "r10", "r11", "r12"
		);
  /* FIXME: set clobber */
  return EBBRC_OK;
}


CObjInterface(EventMgrPrim) EventMgrPrimImp_ftable = {
  .allocEventNo = EventMgrPrimImp_allocEventNo,
  .freeEventNo = EventMgrPrimImp_freeEventNo,
  .bindEvent = EventMgrPrimImp_bindEvent,
  .routeIRQ = EventMgrPrimImp_routeIRQ,
  .triggerEvent = EventMgrPrimImp_triggerEvent,
  .enableInterrupts = EventMgrPrimImp_enableInterrupts,
  .dispatchEvent = EventMgrPrimImp_dispatchEvent,
  .dispatchIRQ = EventMgrPrimImp_dispatchIRQ
};

static void
EventMgrPrimSetFT(EventMgrPrimImpRef o)
{
  o->ft = &EventMgrPrimImp_ftable;
}

static EBBRep *
EventMgrPrimImp_createRep(CObjEBBRootMultiRef root)
{
  EventMgrPrimImpRef repRef;
  EBBRC rc;
  rc = EBBPrimMalloc(sizeof(EventMgrPrimImp), &repRef, EBB_MEM_DEFAULT);
  LRT_RCAssert(rc);

  EventMgrPrimSetFT(repRef);
  repRef->theRoot = root;
  repRef->eventLoc = lrt_my_event_loc();
  
  bzero(&repRef->bv, sizeof(repRef->bv));

  rc = EBBPrimMalloc(sizeof(repRef->reps)*lrt_num_event_loc(), &repRef->reps,
		     EBB_MEM_DEFAULT);
  LRT_RCAssert(rc);

  bzero(repRef->reps, sizeof(repRef->reps) * lrt_num_event_loc());

  // note we get here with the root object locked, and we are assuming tht
  // in searching for/allocating the event_table.  When we parallelize
  // rep creation this will fail
  EBBRep *rep;
  root->ft->nextRep(root, 0, &rep);
  if (rep != NULL) {
    repRef->lrt_event_table_ptr = ((EventMgrPrimImpRef)rep)->lrt_event_table_ptr;
  } else {
    // allocate the table; reminder this is locked at root
    rc = EBBPrimMalloc(sizeof(struct lrt_event_descriptor)*LRT_EVENT_NUM_EVENTS,
                       &repRef->lrt_event_table_ptr, EBB_MEM_DEFAULT);
  }

  return (EBBRep *)repRef;
}

EBBRC
EventMgrPrimImpInit(void)
{
  EBBRC rc;
  static CObjEBBRootMultiImpRef rootRef;

  if (__sync_bool_compare_and_swap(&theEventMgrPrimId, (EventMgrPrimId)0,
                                   (EventMgrPrimId)-1)) {
    EBBId id;
     rc = CObjEBBRootMultiImpCreate(&rootRef, EventMgrPrimImp_createRep);
    LRT_RCAssert(rc);
    rc = EBBAllocPrimId(&id);
    LRT_RCAssert(rc);
    rc = EBBBindPrimId(id, CObjEBBMissFunc, (EBBArg)rootRef);
    LRT_RCAssert(rc);
    theEventMgrPrimId = (EventMgrPrimId)id;
  } else {
    while ((*(volatile uintptr_t *)&theEventMgrPrimId)==-1);
  }
  return EBBRC_OK;
}
