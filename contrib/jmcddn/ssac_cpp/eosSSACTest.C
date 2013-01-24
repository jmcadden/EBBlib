#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include "EBBKludge.H"
#include "Test.H"
#include "SSACSimpleSharedArray.H"

#define NODE_COUNT        1
#define CORE_COUNT        3
#define BIND_THREADS      0
#define TEST_ITERATIONS   1
#define TEST_EVENTS       100
#define RW_RATIO          0.5
#define HASHTABLESIZE     8192 //this is the number of hash queues, each with a fixed associativity

class SSACTest : public Test {
  protected:
    SSACId ssac;
    EBBRC work(int id);
    EBBRC init();
    EBBRC end();
  public:
    SSACTest(int n, int m, int c, bool p, double wpct): Test(n,m,c,p,wpct) {}

};

/* Initialise array */
  EBBRC
SSACTest::init()
{
  //  TRACE("BEGIN");
  CacheObjectIdSimple id(0);
  CacheEntrySimple *entry=0;
  EBBRC rc;
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

/**
 * Test's Work function - here we pull and update an entry from each hashqueue
 * */
  EBBRC
SSACTest::work(int myid)
{
  CacheObjectIdSimple id(0);
  CacheEntrySimple *entry=0;
  int readCount, writeCount;
  EBBRC rc;
  intptr_t v;

  readCount = (int)floor((1-writePct) * numEvents);
  writeCount = (int)floor(writePct * numEvents);

  for (int j=0; j<1;j++) {
    for (int i=0; i<writeCount; i++) {
      id = i;
      rc = DREF(ssac)->get((CacheObjectId &)id,(CacheEntry * &)entry,
          SSAC::GETFORWRITE);
      // write to data object, mark dirty, put back
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
  return rc;
}

  EBBRC
SSACTest::end()
{
  // DREF(ssac)->snapshot();
  Test::end();
  return 0;
}

class SSATest : public SSACTest {
  public:
    SSATest(int n, int m, int c, bool p, double wpct);
    virtual ~SSATest();
};

SSATest::SSATest(int n, int m, int c, bool p, double wpct) : SSACTest(n,m,c,p,wpct)
{
  // init hash table
  SSACSimpleSharedArray::Create(ssac, HASHTABLESIZE);
}

SSATest::~SSATest()
{
  // DREF(ssac)->destroy();
}

  void
SSACSimpleSharedArrayTest(int numWorkers, int numIterations, int numEvents, bool bindThread, double wpct)
{
  SSATest test(numWorkers, numIterations, numEvents, bindThread, wpct);
  test.doTest();
}

  int
main(int argc, char **argv)
{
  // test defaults
  int   n = CORE_COUNT; 
  int   m = TEST_ITERATIONS;
  bool  p = BIND_THREADS;
  double w= RW_RATIO;
  int   c = TEST_EVENTS;

  //can be optionally set via cmd line input
  if (argc>1) n=atoi(argv[1]);
  if (argc>2) m=atoi(argv[2]);
  if (argc>3) c=atoi(argv[3]);
  if (argc>4) w=atof(argv[4]);
  if (argc>5) p=atoi(argv[5]);

#if 0 //TODO: move barrier inward
  BarrierTest(n);
#endif

  // run the test
  SSACSimpleSharedArrayTest(n,m,c,p,w);

  return 0;
}
