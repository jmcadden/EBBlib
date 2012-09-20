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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

extern "C" {
#include <lrt/assert.h>
#include <lrt/io.h>
#include <l0/lrt/exit.h>
#include <l0/lrt/trans.h>
#include <l0/lrt/types.h>
#include <l0/cobj/cobj.h>
#include <l0/cobj/CObjEBB.h>
#include <l0/cobj/CObjEBBRoot.h>
#include <l0/cobj/CObjEBBRootMulti.h>
#include <l0/cobj/CObjEBBRootMultiImp.h>
#include <l0/EBBMgrPrim.h>
#include <l0/EventMgrPrim.h>
#include <l0/EventMgrPrimImp.h>
#include <l0/MemMgr.h>
#include <l0/MemMgrPrim.h>
#include <l1/App.h>
}
#include <l0/cplus/CPlusEBB.H>
#include <l0/cplus/CPlusEBBRoot.H>
#include <l0/cplus/CPlusEBBRootShared.H>

//XXX: remove kludge!!
#include "EBBKludge.H"
#include "ebb/Test.H"
#include "lib/SSACSimpleSharedArray.H"

#define DEFAULT_HASHQ_COUNT 8192

class SSACTest : public Test {
protected:
  SSACId ssac;
  int numEvents;  
  double writePct; 
  enum {HASHTABLESIZE=DEFAULT_HASHQ_COUNT};//this is the size of hashqs, each with an 'associative' ammount of cache entries.
  virtual EBBRC setup_test(int m, int c, double wpct);
  virtual EBBRC setup_iteration();
  virtual EBBRC work(int id);
  virtual EBBRC end();
public:
  EBBRC Create(TestId &rep);
};

EBBRC 
SSACTest::setup_test(int m, int n, double wpct) 
{
  iterations= m; 
  num_locations = n; 
  writePct = wpct; 
  /* allocate our basic results structure */
  wargs = (struct Test::WArgs *)
    // not_locations because we're shared
  EBBPrimMalloc(sizeof(struct Test::WArgs) * (num_locations * iterations), wargs, EBB_MEM_DEFAULT);
  tassert((wargs != NULL), ass_printf("malloc failed\n"));
  /* init the barrier */
  //init_barrier(&bar, num_locations);
//  TRACE("BEGIN");
  CacheObjectIdSimple id(0);
  CacheEntrySimple *entry=0;
  EBBRC rc;

  // CREATE TABLE

  // run through each entry of the hashqs, clear value & h
  DREF(ssac)->flush();
  for (unsigned long i=0; i<HASHTABLESIZE; i++) {
    id = i;
    rc=DREF(ssac)->get((CacheObjectId &)id,(CacheEntry * &)entry,
		       SSAC::GETFORWRITE);
    entry->data = (void *)i; // set data pointer to i TODO: verify
    rc=DREF(ssac)->putback((CacheEntry * &)entry, SSAC::KEEP);
  }
//  TRACE("END");
  return rc;
}

EBBRC
SSACTest::setup_iteration() { return 1; }

EBBRC
SSACTest::work(int myid)
{
  //  TRACE("BEGIN");
  CacheObjectIdSimple id(0);
  CacheEntrySimple *entry=0;
  int readCount, writeCount;
  EBBRC rc;
  intptr_t v;

  readCount = (1-writePct) * numEvents;
  writeCount = writePct * numEvents;
  rc = 0;

  for (int j=0; j<1;j++) {
    for (int i=0; i<writeCount; i++) {
      id = i;
      rc = DREF(ssac)->get((CacheObjectId &)id,(CacheEntry * &)entry,
			   SSAC::GETFORWRITE);
      v=(intptr_t)entry->data; v++; entry->data=(void *)v;
      entry->dirty();
      rc =DREF(ssac)->putback((CacheEntry * &)entry, SSAC::KEEP);
    }
    for (int k=0; k<readCount; k++){
      id = k;
      rc = DREF(ssac)->get((CacheObjectId &)id,(CacheEntry * &)entry,
			   SSAC::GETFORREAD);
      v=(intptr_t)entry->data;
    }
  }
  //  TRACE("END");
  return rc;
}

EBBRC
SSACTest::end()
{
  // TODO: take snapshot 
/* DUMP OUT TEST DATA
  int index;
  printf("Test, Process, Start, End, Dif\n");
  for (int i=0; i<iterations; i++)
    for (int j=0; j<numWorkers; j++){
      index = (i*numWorkers)+j;
      printf("%d, %d, %llu, %llu, %llu\n", i, j, wargs[index].start, wargs[index].end, wargs[index].end - wargs[index].start);
    }
*/
  Test::end();
  return 0;
}


/* *************************************************** */


class SimpleSharedTest : public SSACTest {
public:
  EBBRC setup_test();
  EBBRC Create(TestId &rep);
  virtual ~SimpleSharedTest();
};


EBBRC
SimpleSharedTest::Create(TestId &ssa)
{
  EBBRC rc;
  SimpleSharedTest *rep;
  CPlusEBBRootShared *root;

  rep = new SimpleSharedTest();
  root = new CPlusEBBRootShared();
  lrt_printf("c++ counter test using dynamic memory\n");

#if 0
  static CtrCPlus repObj;
  static CPlusEBBRootShared rootObj;
  rep = &repObj;
  root = &rootObj;
#endif

  // setup representative and root
 // rep->init(); TODO: init test
  // shared root knows about only one rep so we 
  root->init(rep);

  rc = EBBAllocPrimId((EBBId *)&ssa);
  LRT_RCAssert(rc);

  rc = CPlusEBBRoot::EBBBind((EBBId)ssa, root); 
  LRT_RCAssert(rc);
  return EBBRC_OK;
}

EBBRC
SimpleSharedTest::setup_test() 
{
  SSACSimpleSharedArray::Create(ssac, HASHTABLESIZE);
//  SSACTest::setup_test(1,1,1);
  return 0;
}

SimpleSharedTest::~SimpleSharedTest()
{
  // DREF(ssac)->destroy();
}





/* *************************************** */




// This function needs to create EBB rep, bind ID to event 
void
Setup_SSA_Test(int numIterations, int numEvents, double wpct)
{
  // here we create the ebb, bind the events
  SimpleSharedTest test();
  //init & run test
}


CObject(SSACTST) {
  CObjInterface(App) *ft;
};

EBBRC
SSACTST_start(AppRef _self )
{
  lrt_printf("SSACTST LOADED\n");

  int m=1; // no. of iterations
  double w=0.5; // test read/write percentag
  int c=1000; // test event noo

#if 0
  FIXME: get start_infok
  if (argc>1) n=atoi(argv[1]);
  if (argc>2) m=atoi(argv[2]);
  if (argc>3) c=atoi(argv[3]);
  if (argc>4) w=atof(argv[4]);
  if (argc>5) p=atoi(argv[5]);
#endif 

  Setup_SSA_Test(m,c,w);
  lrt_printf("Compleated SSAC Test App!\n");
  return EBBRC_OK;
}

CObjInterface(App) SSACTST_ftable = {SSACTST_start};

extern "C" {
APP_START_ONE(SSACTST);
}
