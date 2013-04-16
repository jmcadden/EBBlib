// TESTS CONTROLS 
#define P2P_LOCAL   0
#define P2P_REMOTE  1
#define TEST_C      0
#define TEST_D      0

#define VERBOSE     0
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <common/bgp_personality.h>
#include <common/bgp_personality_inlines.h>
#include <spi/bgp_SPI.h>
#include <bpcore/ppc450_inlines.h>
#include <spi/kernel_interface.h>
/* project headers */
#include "sg.h" /* Scatter-Gather*/
#include "kludge.h"

#define QUAD 16
#define QUADWORD QUAD * 4
#define BUFFERLEN QUADWORD
#define PSIZE 1024

typedef uint32_t  val; 
typedef uint32_t  id;
typedef uint32_t  key;
typedef uint32_t  offset;

// GLOBALS
id myid;
uint32_t blocksize;
_BGP_Personality_t ps;
uint32_t dmem_track;

val dmem_base[PSIZE] __attribute__ ((aligned(QUAD)));
val input __attribute__ ((aligned(QUAD)));

key kstart;
key kend;

inline id
home(key k) { return k / PSIZE; }

inline offset
off(key k) { return k % PSIZE; }

inline int
local(id i) { return i == myid; }

void
dmem_put(key k, val a)
{
  DMA_iovec iovec;
  
  static val v __attribute__ ((aligned(QUAD)));
  v = a;

  if( local(home(k)))
    dmem_base[off(k)] = v;
  else
  { 
    if(VERBOSE)
      printf("remote write! %d -> %d (write to %d)  %d!! \n", v, k, home(k), local(home(k)));
    DMA_sg_setvec( &iovec, home(k), dmem_track, sizeof(val), off(k));
    DMA_writev(&v, &iovec, 1);
  }
  return;
}

val 
dmem_get(key k)
{
  static val v __attribute__ ((aligned(QUAD)));
  DMA_iovec iovec;
  
  if( local(home(k)))
    return  dmem_base[off(k)];
  else
  {

    if(VERBOSE)
      printf("key %d remote read! %d -> %d \n", k, myid, home(k));
    DMA_sg_setvec(&iovec, home(k), dmem_track, sizeof(val), off(k));
    DMA_readv(&v, &iovec, 1);
  }
  return v;
}

int dmem_init(void)
{
  int rc=-1;

  key k;
  Kernel_GetPersonality(&ps, sizeof(ps)); 
  myid = ps.Network_Config.Rank;
  blocksize = BGP_Personality_numComputeNodes(&ps);

  DMA_sginit(myid);
  dmem_track = DMA_gettrack();
  DMA_settrack(dmem_track, PSIZE*sizeof(val), dmem_base); 

  if(VERBOSE)
    printf("%s #%d/%d finished init\n",__func__, myid, blocksize);

  kstart = myid * PSIZE;
  kend = kstart + PSIZE; 

  for(k = kstart; k < kend; k++)
  {
    dmem_put(k, (val)myid);
    assert(dmem_get(k) == (val)myid);
  }

   rc = 1;
   return rc;
}

int
main(void){
  int nextloc;
  uint64_t stime, etime, ttime;
  val v;
  dmem_init();

  int i;

  nextloc = (myid + 1) % blocksize;
  GlobInt_Barrier(0, NULL, NULL);

/* Begin Tests */
  stime = _bgp_GetTimeBase();

/* Local Read */
if(P2P_LOCAL){
  if(VERBOSE)
    printf("%s #%d: running test 1 \n",__func__, myid);
  v = dmem_get(kstart);
}
  
/* Remote Read */
if(P2P_REMOTE){

  for( i=blocksize-1; i >= 0; i--)
  {
    
    if(myid == i)
    {
      printf("%s #%d: running test 2 \n",__func__, myid);
      v = dmem_get(nextloc * PSIZE);
      printf("%d: v=%d  %llu\n", myid, v);
    }
  
    GlobInt_Barrier(0, NULL, NULL);
  }

  if(VERBOSE)
    printf("%s #%d: running test 2 \n",__func__, myid);
  
}

  etime = _bgp_GetTimeBase();
  
  ttime = etime - stime;


  printf("%d: v=%d  %llu\n", myid, v, ttime);
  GlobInt_Barrier(0, NULL, NULL);


  if(myid == 0)
    printf("finish.\n");
  return 1;
}

#if 0
int
main (void){
 
  int i, orig, newv;
  int track;
  _BGP_Personality_t ps;
  uint32_t rank, blocksize;

  Kernel_GetPersonality(&ps, sizeof(ps)); 
  rank = ps.Network_Config.Rank;
  blocksize = BGP_Personality_numComputeNodes(&ps);

  uint32_t buf[blocksize] __attribute__ ((aligned(QUAD)));

  for(i=0; i<blocksize; i++)
    buf[i] = rank; // fill entire buffer with the node rank

  // init scatter-gather
  DMA_sginit(rank);
  track = DMA_gettrack();
  DMA_settrack(track, blocksize*sizeof(uint32_t), buf); 
  printf("#%d/%d tk:%d finished init\n",rank, blocksize, track);

  orig = newv = DMA_check_rec(0);

  GlobInt_Barrier(0, NULL, NULL);

  /* each node has created a s-g track, consisting of a buffer of integers
   * corrosponding to that nodes rank */ 

  if(rank != 0)
  {
    while(1){

      while(orig == newv)
      { newv = DMA_check_rec(0); }

      printf("%d:", rank); 
      for(i=0; i<blocksize; i++)
        printf("%d,",buf[i]); 
      printf("\n"); 

      orig = newv = DMA_check_rec(0);

      GlobInt_Barrier(0, NULL, NULL);
    }
  }
  else // rank == 0 
  {
    // _bgp_Delay(3000000);
    //DMA_config_dump();

    DMA_iovec iovec[blocksize-1];

    // Attempted to write to the track of each nodes, offset by node rank
    printf("SCATTER TEST \n");
  
    // create IO vector
    for(i=0; i<blocksize; i++)
      DMA_sg_setvec( &iovec[i], i+1, track, 4, (i+1)*4);

    DMA_writev(buf, iovec, blocksize-1);

    GlobInt_Barrier(0, NULL, NULL);
    
    printf("GATHER TEST \n");
    for(i=0; i<blocksize; i++)
      DMA_sg_setvec( &iovec[i], i+1, 0, 4, 0);


    DMA_readv(buf+1, iovec, blocksize-1);

    printf("gt:", rank); 
    for(i=0; i<blocksize; i++)
      printf("%d,",buf[i]); 
    printf("\n"); 

  }

  while(1)
    ;
  return 0;
}
#endif
