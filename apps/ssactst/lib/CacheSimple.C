#include "../EBBKludge.H"
#include "CacheSimple.H"
 
CacheObjectDataSimple
CacheObjectIdSimple :: load()
{
    register unsigned int i=0;
    while(i<LOADSPIN) i++; 
    return (void *)long(_id);
}

EBBRC
CacheObjectIdSimple :: save(CacheObjectDataSimple data)
{
    // what is the point of this?
    register unsigned int i=0;
    while(i<SAVESPIN) i++;
    return 0;
}
 
#ifdef EBBLIB
void *
CacheObjectIdSimple::operator new(size_t size)
{
  void *val;
  EBBRC rc;
  rc = EBBPrimMalloc(size, &val, EBB_MEM_DEFAULT);
  LRT_RCAssert(rc);
  return val;
}

void
CacheObjectIdSimple::operator delete(void * p, size_t size)
{
  // NYI
  LRT_RCAssert(0);
}
#endif
// ---

CacheEntrySimple :: CacheEntrySimple()
{
    flags=ZERO;
    lastused=0;
    data=0;
}
  
void 
CacheEntrySimple :: sleep()
{
    // spin for a short period of time
    register int i=0;
    while(i<BACKOFF) i++;
}

void
CacheEntrySimple :: wakeup()
{
    return;
}

void
CacheEntrySimple :: dirty()
{
    flags |= DIRTY;
    return;
}

void
CacheEntrySimple :: print()
{
    printf("CacheEntrySimple: \n\tthis=%p\n\tid=%d\n\tflags=",this,id.id());
    if (flags & BUSY) printf("BUSY | "); else printf("FREE | ");
    if (flags & DIRTY) printf("DIRTY"); else printf("CLEAN ");
    printf("\n\tlastused=%ld\n\tdata=%p\n",lastused,data);
}

#ifdef EBBLIB
void *
CacheEntrySimple::operator new(size_t size)
{
  void *val;
  EBBRC rc;
  rc = EBBPrimMalloc(size, &val, EBB_MEM_DEFAULT);
  LRT_RCAssert(rc);
  return val;
}

void
CacheEntrySimple::operator delete(void * p, size_t size)
{
  // NYI
  LRT_RCAssert(0);
}
#endif
