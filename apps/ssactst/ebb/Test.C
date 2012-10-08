#include "../EBBKludge.H"
#include "Test.H"
#include <l0/lrt/exit.h>
#include <sync/barrier.h>
#include <l0/MemMgrPrim.h>
#include <l0/EventMgrPrim.h>

void * 
Test::operator new(size_t size)
{
  void *val;
  EBBRC rc;
  rc = EBBPrimMalloc(size, &val, EBB_MEM_DEFAULT);
  LRT_RCAssert(rc);
  return val;
}

void 
Test::operator delete(void * p, size_t size)
{
  // FIXME:
  LRT_Assert(0);
}

/* The Null Setup - */
EBBRC
Test::setup(void){ return EBBRC_OK; }

/* The Null Init - */
EBBRC
Test::init(void){ return EBBRC_OK; }

/* The Null Work - */
EBBRC
Test::work(int i){ return EBBRC_OK; }

/* The Null End - */
EBBRC
Test::end(void){ return EBBRC_OK; }

/* The NULL Test - run through the functions, but record no data. */
EBBRC
Test::run() {
  if (setup()<0) return -1;
  for(int j=0; j<iterations; j++){
    if (init()<0) return -1;
      work(j);
  }
  end();
  return 0;
}

/* The NULL Create - Setup Test object and bind to shared root EBB */
EBBRC
Test::create(TestId &test)
{
  EBBRC rc;
  Test *rep;
  CPlusEBBRootShared *root;
  rep = new Test();
  root = new CPlusEBBRootShared();
  // setup representative and root
  root->init(rep);
  rc = EBBAllocPrimId((EBBId *)&test);
  LRT_RCAssert(rc);
  rc = CPlusEBBRoot::EBBBind((EBBId)test, root); 
  LRT_RCAssert(rc);
  return EBBRC_OK;
}
