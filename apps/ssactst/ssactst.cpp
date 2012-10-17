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
#include <arch/atomic.h>
}
#include <l0/cplus/CPlusEBB.H>
#include <l0/cplus/CPlusEBBRoot.H>
#include <l0/cplus/CPlusEBBRootShared.H>

//XXX: remove kludge!!
#include "EBBKludge.H"
#include "ebb/Test.H"
#include "lib/SSACSimpleSharedArray.H"

#define DEFAULT_HASHQ_COUNT 192

class SSACTest;
typedef SSACTest **SSACTestId;

class SSACTest : public Test {
protected:
  SSACId ssac;
  int numEvents;  
  double writePct; 
  //size of hashqs, each with an 'associative' ammount of cache entries.
  // TODO: set associativity
  enum { HASHTABLESIZE = DEFAULT_HASHQ_COUNT };
  virtual EBBRC init();
  virtual EBBRC end();
  virtual EBBRC setup();
  virtual EBBRC work(int id);
public:
  static EBBRC create(TestId &rep);
  virtual EBBRC set_vars(int m, int n, int w);
};

EBBRC 
SSACTest::setup(void) 
{
  TRACE("SSACTest::");
  return 1;
}

EBBRC
SSACTest::set_vars(int m, int n, int w)
{
  iterations = m; 
  numEvents = n; 
  writePct = w; 
  return 1;
}

EBBRC
SSACTest::init() { 
  return 1; 
}

EBBRC
SSACTest::work(int myid)
{
  TRACE("BEGIN");
  CacheObjectIdSimple id(0);
  CacheEntrySimple *entry=0;
 // int readCount, writeCount;
  EBBRC rc;
  intptr_t v;

  //readCount = (1-writePct) * numEvents;
  //writeCount = writePct * numEvents;
  rc = 0;

  for (int j=0; j<1;j++) {
    for (int i=0; i<numEvents; i++) {
    //for (int i=0; i<writeCount; i++) {
      id = i;
      rc = CPLUS_EBBCALL(ssac, get, (CacheObjectId &)id,(CacheEntry * &)entry, SSAC::GETFORWRITE);
      v=(intptr_t)entry->data; v++; entry->data=(void *)v;
      entry->dirty();
      rc = CPLUS_EBBCALL(ssac, putback,(CacheEntry * &)entry, SSAC::KEEP);
    }
   // for (int k=0; k<readCount; k++){
   //   id = k;
   //   rc = CPLUS_EBBCALL(ssac, get, (CacheObjectId &)id,(CacheEntry * &)entry, SSAC::GETFORREAD);
   //   v=(intptr_t)entry->data;
   // }
  }
  return rc;
}

EBBRC
SSACTest::end()
{
  EBBRC rc;
  rc = CPLUS_EBBCALL(ssac, snapshot);
/* DUMP OUT TEST DATA
  int index;
  printf("Test, Process, Start, End, Dif\n");
  for (int i=0; i<iterations; i++)
    for (int j=0; j<numWorkers; j++){
      index = (i*numWorkers)+j;
      printf("%d, %d, %llu, %llu, %llu\n", i, j, wargs[index].start, wargs[index].end, wargs[index].end - wargs[index].start);
    }
*/
  return 0;
}


/* *************************************************** */


class SimpleSharedTest : public SSACTest {
public:
  static EBBRC create(SSACTestId &rep);
  EBBRC setup(void); 
};

EBBRC
SimpleSharedTest::setup(void) 
{
  EBBRC rc;
  TRACE("SimpleSharedTest::");
  SSACSimpleSharedArray::Create(ssac, HASHTABLESIZE);
  CacheObjectIdSimple id(0);
  CacheEntrySimple *entry=0;
  // flush cache and full with garbage data
  rc = CPLUS_EBBCALL(ssac, flush);
  for (unsigned long i=0; i<HASHTABLESIZE; i++) {
    id = i;
  //  rc=DREF(ssac)->get((CacheObjectId &)id,(CacheEntry * &)entry, SSAC::GETFORWRITE);
    rc = CPLUS_EBBCALL(ssac, get, (CacheObjectId &)id,(CacheEntry * &)entry, SSAC::GETFORWRITE);
    entry->data = (void *)i; // set data pointer 
    rc = CPLUS_EBBCALL(ssac, putback,(CacheEntry * &)entry, SSAC::KEEP);
    //rc=DREF(ssac)->putback((CacheEntry * &)entry, SSAC::KEEP);
    
  }
  return rc;
}


EBBRC
SimpleSharedTest::create(SSACTestId &ssa)
{
  EBBRC rc;
  SimpleSharedTest *rep;
  CPlusEBBRootShared *root;
  rep = new SimpleSharedTest();
  root = new CPlusEBBRootShared();
  // setup representative and root
  root->init(rep);
  rc = EBBAllocPrimId((EBBId *)&ssa);
  LRT_RCAssert(rc);
  rc = CPlusEBBRoot::EBBBind((EBBId)ssa, root); 
  LRT_RCAssert(rc);

  return EBBRC_OK;
}

/* *************************************** */

CObject(SSACTST) {
  CObjInterface(App) *ft;
};

EBBRC
SSACTST_start(AppRef _self )
{

#if 0
  int m=1; // no. of iterations
  double w=0.5; // test read/write percentag
  int c=1000; // test event noo
  FIXME: get start_infok
  if (argc>1) n=atoi(argv[1]);
  if (argc>2) m=atoi(argv[2]);
  if (argc>3) c=atoi(argv[3]);
  if (argc>4) w=atof(argv[4]);
  if (argc>5) p=atoi(argv[5]);
#endif 
  
  atomic_synchronize();

  // create the ebb id
  SSACTestId ssa_test;
  SimpleSharedTest::create(ssa_test);
  CPLUS_EBBCALL(ssa_test, set_vars, 3, 5, 5);
  CPLUS_EBBCALL(ssa_test, run);

  lrt_printf("Compleated SSAC Test App!\n");
  return EBBRC_OK;
}

CObjInterface(App) SSACTST_ftable = {SSACTST_start};

extern "C" {
APP_START_ONE(SSACTST);
}
