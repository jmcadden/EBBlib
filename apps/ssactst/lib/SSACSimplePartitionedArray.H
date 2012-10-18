#ifndef _SSACSIMPLEPARTITIONEDARRAY_H_
#define _SSACSIMPLEPARTITIONEDARRAY_H_
#include "SSAC.H"
#include "CacheSimple.H"


class SSACSimplePartitionedArray : public SSAC {
  /*
   *
   { 
   class SSACSimplePartitionedMH : public MHReplicate {
   int  _numhashqsperrep;
   int  _associativity;
   public:
   SSACSimplePartitionedMH(const int &numhashqs,
   const int &associativity ) {
   _numhashqsperrep=numhashqs;
   _associativity=associativity;
   }

   ClusteredObject * createRepOn(const int vp) {
   RFUNC0(vp,(SSACSimplePartitionedArray **)_ref,
   SSACSimplePartitionedArray::init);
   return findRepOn(vp);
   }

   virtual ClusteredObject * createFirstRep() {
   return (ClusteredObject *)
   new SSACSimplePartitionedArray(_numhashqsperrep,
   _associativity);
   }
   virtual ClusteredObject * createRep() {
   return (ClusteredObject *)
   new SSACSimplePartitionedArray(_numhashqsperrep,
   _associativity);
   }
   };
   friend class SSACSimplePartitionedArray::SSACSimplePartitionedMH;
   */
  class SimpleLock {
    int l;
    public:
    void releaseLock() {}
    void acquireLock() {}
  };
  enum {DEFAULT_ASSOCIATIVITY=4};
  struct HashQueues {
    SimpleLock lock;
    unsigned long count;
    CacheEntrySimple *entries;
    void init(const int &numentries);
    inline CacheEntrySimple *search(CacheObjectIdSimple &id,
        const int &numentries);
    //	inline CacheEntrySimple *lruentry(const int &numentries);
    CacheEntrySimple *lruentry(const int &numentries);
    void rollover();
    HashQueues(); 
    ~HashQueues();
  } *_hashqs;

  int      _numhashqs;
  int      _associativity;

  void partition(const int &index, int &vp, int &offset);
//  SSACRef ref() { return (SSACRef)_ref;}
  SSACSimplePartitionedArray( const int &numhashqs,
      const int &associativity );
  public:
#ifdef EBBLIB
  void * operator new(size_t size);
  void operator delete(void * p, size_t size);
#endif
  /*
  static SSACRef create( const int &numhashqs,
      const int &associativity=DEFAULT_ASSOCIATIVITY) {
    return (SSACRef)((new SSACSimplePartitionedMH(numhashqs,
            associativity))->ref());
  }
  */
  static EBBRC Create( SSACId &id, const int &numhashqs,
		       const int &associativity=DEFAULT_ASSOCIATIVITY);
  virtual EBBRC init() { return 0; }
  virtual EBBRC get( CacheObjectId &id, CacheEntry* &ce,
      const gettype &type );
  virtual EBBRC putback( CacheEntry* &ce, const putflag &flag );
  virtual EBBRC flush();
  virtual EBBRC snapshot();
  ~SSACSimplePartitionedArray();
};
#endif
