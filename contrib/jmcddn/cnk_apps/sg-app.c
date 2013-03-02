/* Torus Scatter Gather App */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <spi/bgp_SPI.h>
#include <common/bgp_personality.h>
#include <common/bgp_personality_inlines.h>
#include "sg.h" /* Scatter-Gather*/

#define QUAD 16
#define QUADWORD QUAD * 4
#define BUFFERLEN QUADWORD


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
