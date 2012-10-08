#include "../EBBKludge.H"
#include "SSACSimplePartitionedArray.H"
#include <l0/lrt/exit.h>
/*
#include <tornado/TAssert.H>
 */  
void
SSACSimplePartitionedArray :: HashQueues :: init(const int &numentries)
{
    tassert((!entries),
	  ass_printf("\nSSACSimplePartitionedArray::HashQueues:initq:entries!=0\n"));

    /*
    trace( MISC, TR_INFO,
	   tr_printf(
	       ">>> SSACSimplePartitionedArray::HashQueues::init: on que=%lx\n",
	       this ));
    // TODO: EBBPrimMalloc
    //entries=new CacheEntrySimple[numentries];
*/
}

CacheEntrySimple *
SSACSimplePartitionedArray :: HashQueues :: search(CacheObjectIdSimple &id,
						  const int &numentries)
{
    register int i=0;
    while (i<numentries)
    {
	if (id == entries[i].id)
	    return &(entries[i]);
	i++;
    }
    return (CacheEntrySimple *)0;
}

CacheEntrySimple *
SSACSimplePartitionedArray :: HashQueues :: lruentry(const int &numentries)
{
    // find least recently used
    // Note this could have been combined with first search but then
    // would have add expense to a hit.  this search should benifit
    // from a warmer cache.
    register int i=0;
    register CacheEntrySimple *ep=0;
    
    while (i<numentries)
    {
	if ( !( entries[i].flags & CacheEntrySimple::BUSY ) ){
	    if (!ep)
	    {
		ep=&(entries[i]);
		if (!ep->id.valid())
		    break;
	    }
	    else
		if ( entries[i].lastused < ep->lastused )
		    ep=&(entries[i]);
        }   	
      i++;
    }
    return ep;
}

void
SSACSimplePartitionedArray :: HashQueues :: rollover()
{
    tassert(0,
	    ass_printf("\nSSACSimplePartitionedArray::HashQueues:rollover() called\n"));
}

SSACSimplePartitionedArray :: HashQueues :: HashQueues()
{
    count=0;
    entries=0;
}

SSACSimplePartitionedArray :: HashQueues :: ~HashQueues()
{
  // TODO: delete!
  //  if (entries) delete[] entries;
}
  
SSACSimplePartitionedArray :: SSACSimplePartitionedArray( const int &numhashqs,
						const int &associativity)
{
    _associativity=associativity;
    _numhashqs = numhashqs;

    // TODO: add MYVP to 
    if (MYVP < _numhashqs % NUMPROC)
    {
      //TODO: Malloc
      //	_hashqs = new HashQueues[_numhashqs/NUMPROC + 1];
    }
    else
    {
      //TODO: Malloc
      //	_hashqs = new HashQueues[_numhashqs/NUMPROC];
    }
}

void
SSACSimplePartitionedArray :: partition(const int &index, int &vp, int &offset)
{
    int overflow = _numhashqs % NUMPROC;
    int quesperproc=_numhashqs / NUMPROC;
    
    if (index < overflow*(quesperproc + 1))
    {
	vp=index/(quesperproc+1);
	offset=index & (quesperproc );
    } else {
	vp=index / quesperproc;
	offset=index & (quesperproc -1);
    }
}

EBBRC 
SSACSimplePartitionedArray :: get( CacheObjectId &id, CacheEntry* &ce,
			      const gettype &type )
{
    EBBRC rtn=0;
       
    CacheObjectIdSimple& theid=((CacheObjectIdSimple &)id);    
    register struct HashQueues *hashq;
    // the assignment of ce to ep is here just to avoid compliation warning
    CacheEntrySimple *ep=(CacheEntrySimple *)ce;

    tassert(theid.valid(),
	    ass_printf("\nSSACSimplePartitionedArray::get() target id=0x%x is invalid\n"
		       , theid.id()));

    int vp=0;
    int index=0;

    partition(theid.index(_numhashqs),vp,index);

    if (vp!=MYVP)
    {
      // TODO: ebbcalls
      // line below used to avoid uninitilize warnings
	hashq = &(_hashqs[index]);
      /*
	SSACSimplePartitionedArray *rep=(SSACSimplePartitionedArray *)
	    ((SSACSimplePartitionedMH *)MYMHO)->findRepOn(vp);
	if (!rep)
	    rep=(SSACSimplePartitionedArray *)
		(((SSACSimplePartitionedMH *)MYMHO)->createRepOn(vp));
	hashq = &(rep->_hashqs[index]);
      */
    }
    else
    {
	hashq = &(_hashqs[index]);
    }
    
    
    if (!hashq->entries) {
	hashq->lock.acquireLock();
	hashq->init(_associativity);
	hashq->lock.releaseLock();
    }

 again:    
    hashq->lock.acquireLock();

    ep=hashq->search(theid,_associativity);

    if (ep)
    {
	// hit
	trace( MISC, TR_INFO,
	       tr_printf(
	   ">>> SSACSimplePartitionedArray::get: Hit: id=%d index=%d ep=%llx:\n",
		   theid.id(), theid.index(_numhashqs),(long long int)ep );
	       ep->print()
	    );
#ifndef NOLOCKSNOFLAGSNOCOUNTERS
	if ( ep->flags & CacheEntrySimple::BUSY )
	{
	    hashq->lock.releaseLock();
	    ep->sleep();
	    goto again;
	}
#endif
#ifndef NOLOCKSNOFLAGSNOCOUNTERS
	hashq->count++;
	if (!hashq->count) hashq->rollover();
	ep->lastused=hashq->count;
	if ( type == SSAC::GETFORWRITE )
	    ep->flags |= CacheEntrySimple::BUSY;
	hashq->lock.releaseLock();
#endif 
	ce=ep;
	SET_CLSCD(rtn,1);
	return rtn;
    } 
    else
    {
	// miss
	trace( MISC, TR_INFO,
	       tr_printf(
		   ">>> SSACSimplePartitionedArray::get: Miss: id=%d index=%d\n",
		   theid.id(),theid.index(_numhashqs)));

	ep=hashq->lruentry(_associativity);
	
	if (ep)
	{
#ifndef NOLOCKSNOFLAGSNOCOUNTERS
	    if (ep->flags & CacheEntrySimple::DIRTY)
#endif
	    {
		//clean the entry
		trace( MISC, TR_INFO,
		       tr_printf(
		   ">>> SSACSimplePartitionedArray::get:Miss: Cleaning entry:\n"
			   );
		           ep->print());
		ep->id.save(ep->data);
#ifndef NOLOCKSNOFLAGSNOCOUNTERS 
		ep->flags &= ~CacheEntrySimple::DIRTY;
#endif 
	    }
	    ep->id=theid;
	    ep->data=ep->id.load();
	}
	else  
	{
	    // no free elements take the simple way out for the moment
	    // just pass back an error code
	    trace( MISC, TR_INFO,
		   tr_printf(
		    ">>> SSACSimplePartitionedArray::get: Miss:*NOFREE ENTRIES*\n"
		       ));
	    SET_CLSCD(rtn,0);
	    hashq->lock.releaseLock();
	    return rtn;
	}
	hashq->count++;
	if (!hashq->count) hashq->rollover();
	ep->lastused=hashq->count;
	if ( type == SSAC::GETFORWRITE )
	    ep->flags |= CacheEntrySimple::BUSY;
	hashq->lock.releaseLock();
	ce=ep;
	SET_CLSCD(rtn,1);
	return rtn;
    }
}

EBBRC
SSACSimplePartitionedArray :: putback( CacheEntry* &ce, const putflag &flag )
{

    register CacheEntrySimple *entry=(CacheEntrySimple *)ce;
    register struct HashQueues *hashq=0;
    int vp=0;
    int index=entry->id.index(_numhashqs);

    partition(index,vp,index);

    if (vp!=MYVP)
    {
      //TODO: EBBCALL here
      /*
	SSACSimplePartitionedArray *rep=(SSACSimplePartitionedArray *)
	    ((SSACSimplePartitionedMH *)MYMHO)->findRepOn(vp);
	tassert(rep, ass_printf("\n **** ERROR: putback:  could not findRepOn %d\n",
				vp));
        hashq = &(rep->_hashqs[index]);
    
      */
    }
    else
    {
	hashq = &(_hashqs[index]);
    }
    

    if (!entry) return 0;
    
    tassert((hashq->entries),
	 ass_printf("\n\nSSACSimplePartitionedArray::putback: bad queue entry:\n");
	    entry->print());
    
    hashq->lock.acquireLock();
    if (flag == KEEP)
    {
	trace( MISC, TR_INFO,
	       tr_printf(">>> SSACSimplePartitionedArray::putback: KEEP entry:");
	       entry->print()
	    );
    	entry->lastused=hashq->count;
    }
    else
    {
	trace( MISC, TR_INFO,
	     tr_printf(">>> SSACSimplePartitionedArray::putback: DISCARD entry:");
	       entry->print()
	    );
	entry->lastused=0;
    }
    entry->flags &= ~CacheEntrySimple::BUSY;
    hashq->lock.releaseLock();
    entry->wakeup();
    return 0;
}

EBBRC
SSACSimplePartitionedArray :: flush()
{
    register struct HashQueues *hashq;
    register CacheEntrySimple *ce;
    register int i;
    
    for (i=0; i<_numhashqs; i++)
    {
	hashq = &(_hashqs[i]);
	if (!hashq->entries) continue;
    again:	
	hashq->lock.acquireLock();
	for ( register int j=0; j<_associativity; j++ )
	{
	    ce=&(hashq->entries[j]);
	    if ( !(ce->flags & CacheEntrySimple::BUSY)
               && (ce->id.valid())	    
		 && ( ce->flags & CacheEntrySimple::DIRTY ))
	    {
		trace( MISC, TR_INFO,
		       tr_printf(">>> SSACSimplePartitionedArray::flush: entry:");
		       ce->print()
		       );
		ce->flags |= CacheEntrySimple::BUSY;
		hashq->lock.releaseLock();
		ce->id.save(ce->data);
		hashq->lock.acquireLock();
		ce->flags &= ~CacheEntrySimple::DIRTY &
		             ~CacheEntrySimple::BUSY;
		hashq->lock.releaseLock();
		ce->wakeup();
		goto again;
	    }
	}
	hashq->lock.releaseLock();
    }
    return 0;
}

SSACSimplePartitionedArray :: ~SSACSimplePartitionedArray()
{
  /*
    trace( MISC, TR_INFO,
	  tr_printf("**** ~SSACSimplePartitionedArray ref=%lx\n",_ref));
    delete[] _hashqs; 
  */
}

EBBRC
SSACSimplePartitionedArray :: snapshot()
{
    struct HashQueues *hashq;
    int i,j,flags,vp,index=0;
    int numqsonvp,numqsonevenvps=_numhashqs/NUMPROC,unevenvps=_numhashqs % NUMPROC;
    SSACSimplePartitionedArray *rep;
	
    for (vp=0; vp<NUMPROC; vp++)
    {
	numqsonvp=numqsonevenvps;
	if (vp < unevenvps)
	{
	    numqsonvp++;
	}
        //EBBCALLS
        /*
	rep=(SSACSimplePartitionedArray *)
	    ((SSACSimplePartitionedMH *)MYMHO)->findRepOn(vp);
            tassert(rep, ass_printf("\n **** ERROR: snapshot:  could not findRepOn %d\n",
				vp));
	*/
	tr_printf("Hash Queues on vp=%d\n",vp);
	for (i=0; i<numqsonvp; i++)
	{
	    hashq = &(rep->_hashqs[i]);
            rep = NULL;
	    hashq->lock.acquireLock();
	    tr_printf("_hashq[%d].count=%lu\n_hashqs[%d].entries: ",index,hashq->count,index);
	    if (hashq->entries)
		for ( j=0; j<_associativity; j++)
		{
		    tr_printf("%d:%d:%lu:%lx:",j,
			      hashq->entries[j].id.id(),
			      hashq->entries[j].lastused,
			      (long unsigned int)hashq->entries[j].data);
		    flags=(hashq->entries[j]).flags;
		    if (flags & CacheEntrySimple::BUSY)
			printf("B|");
		    else printf("F|");
		    if (flags & CacheEntrySimple::DIRTY)
			printf("D ");
		    else printf("C ");
		}
	    else
		tr_printf("NULL");
	    tr_printf("\n");
	    hashq->lock.releaseLock();
	    index++;
	}
    }
    return 0;
}
  
 
 
