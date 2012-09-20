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
  // NYI
  LRT_Assert(0);
}


EBBRC
Test::run_test() {
  if (setup_test()<0) return -1;
  for(int j=0; j<iterations; j++){
    struct WArgs *args = &(wargs[j]);
    if (setup_iteration()<0) return -1;
    //TODO: barrier here //barrier(&bar, &bar_sense);
    args->start = rdtsc();
    work(j);
    args->end = rdtsc();
  }
  end();
  return 0;
  //TODO: proper return values
}

/* virtual */ EBBRC
Test::end()
{
//TODO: simple "end of test" output. Move dump child
/*
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

// lookup virtual function table
