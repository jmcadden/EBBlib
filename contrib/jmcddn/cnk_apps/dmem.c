// TESTS CONTROLS 
#define P2P_LOCAL   0
#define P2P_REMOTE  0
#define P2P_DSM     1
#define VERBOSE     1
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
#define MB  1048576

typedef uint32_t  val; 
typedef uint32_t  id;
typedef uint32_t  key;
typedef uint32_t  offset;

// GLOBALS
id myid;
uint32_t blocksize;
_BGP_Personality_t ps;
uint32_t dmem_track, dmem_full;
void *va_min, *va_max;
val *vps, *vpr; 

val dmem_base[PSIZE] __attribute__ ((aligned(QUAD)));
val input __attribute__ ((aligned(QUAD)));
key kstart;
key kend;

/***************************************************/

inline id
home(key k) { return k / PSIZE; }

inline offset
off(key k) { return k % PSIZE; }

inline int
local(id i) { return i == myid; }

/***************************************************/

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

    // fill in read location struct
    DMA_sg_setvec(&iovec, home(k), dmem_track, sizeof(val), off(k));
    // make request (blocking)
    DMA_readv(&v, &iovec, 1);
  }
  return v;
}

/***************************************************/

int dmem_init(void)
{
  int rc=-1;

  key k;
  DMA_AddressingInitPhase1();
  Kernel_GetPersonality(&ps, sizeof(ps)); 
  myid = ps.Network_Config.Rank;
  blocksize = BGP_Personality_numComputeNodes(&ps);
  
  // DSM pointers
  int i,j;
  vps = malloc(MB*6);
  vpr = malloc(MB*6);
  for(i=0; i<6; i++)
    for(j=0; j<MB; j++)
      vps[(i*MB)+j] = i+1;
      vpr[(i*MB)+j] = 0;


  printf("vps=%p\n", vps);

  /// Address Space Alloc
  DMA_AddressingGetMinMaxVa(&va_min, &va_max);
  if(myid == 0 && VERBOSE){
    printf("addr range min->max: %p -> %p \n",va_min, va_max);
    printf("addr diff: %p \n",va_max-va_min);
    printf("an addr: %p -  %p = %p \n", &myid, va_min, (int)&myid-(int)va_min);
  }

  DMA_sginit(myid);
  dmem_track = DMA_gettrack();
  DMA_settrack(dmem_track, PSIZE*sizeof(val), dmem_base); 
  dmem_full = DMA_gettrack();
  DMA_settrack(dmem_full, (uint32_t)(va_max-va_min-1), va_min); 

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
  val v = 0;
  int i;
  //
  dmem_init();
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
      if(VERBOSE) 
        printf("%s #%d: remote test \n",__func__, myid);
      v = dmem_get(nextloc * PSIZE);
      if(VERBOSE)
        printf("%d: v=%d  %llu\n", myid, v);
    }
    
    GlobInt_Barrier(0, NULL, NULL);
  }

  if(VERBOSE)
    printf("%s #%d: remote test finished\n",__func__, myid);
  
}
//
if(P2P_DSM && myid == 0){


      if(VERBOSE) 
        printf("%s #%d: dsm  test \n",__func__, myid);
      // lets read the ID of the next node

      DMA_iovec iovec;
      //((int)&myid-(int)va_min));
      DMA_sg_setvec(&iovec, (myid+1)%16, dmem_full, MB*6, (uint32_t)vps - (uint32_t)va_min);
      // make request (blocking)
      DMA_readv(vpr, &iovec, 1);
      //
      if(VERBOSE)
        printf("%d: v=%d \n", myid, v);
  
    GlobInt_Barrier(0, NULL, NULL);

  if(VERBOSE)
    printf("%s #%d: dsm test finished\n",__func__, myid);
  
}

  etime = _bgp_GetTimeBase();
  /*** TEST OVER *****/
  ttime = etime - stime;
  printf("%d: v=%d  %llu\n", myid, v, ttime);
  GlobInt_Barrier(0, NULL, NULL);

  if(myid == 0 && VERBOSE)
    printf("Test finish sucessfully.\n");

  return 1;
}

