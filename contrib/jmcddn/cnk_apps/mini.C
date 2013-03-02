#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <bpcore/bgp_atomic_ops.h>        
#include <spi/bgp_SPI.h>
#include <common/bgp_personality.h>
#include <common/bgp_personality_inlines.h>

#define NUM_NODES 16
#define SRC_SPACE 256
#define DST_SPACE SRC_SPACE * (NUM_NODES - 1)


/* Counter depleation handeler template */
void 
null_func(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
  printf("null func was called!\n");
  return;
}


int
main(int argc, char **argv)
{
  uint32_t i,r,nn; 
  _BGP_Personality_t ps;
  int ps_psetnum;
  int ps_psetsize; /* Note: nodes allocate in sets of 16 */
  int ps_rank;
  char loc[BGPPERSONALITY_MAX_LOCATION];
  char* mymem;       
  char* sharedmem;  
  posix_memalign(&mymem, 16, SRC_SPACE); 
  posix_memalign(&sharedmem, 16, DST_SPACE); 
  bzero( &sharedmem,DST_SPACE);
  //
  kernel_coords_t self, dest; 
  DMA_InjDescriptor_t payload, remote;
  DMA_CounterGroup_t inj_desc, rec_desc;

  Kernel_GetPersonality(&ps, sizeof(ps)); 
  ps_psetsize = BGP_Personality_psetSize(&ps);
  ps_rank = BGP_Personality_rankInPset(&ps); /* note: rank begins count at 1 */
  BGP_Personality_getLocationString(&ps, loc);
  /* self coordinates */
  printf("#%d spacs: %d %s\n", ps_rank, ps_psetsize, loc);
  if( Kernel_Ranks2Coords(&self, ps_rank) != 0)
    perror("Kernal_Ranks2Coords\n");


  /*KLUDGE*/
  Kernel_InterruptGroup_t itrpt = NULL; /* interrupt group number ?  */
  int arr = 0; // array repesenting the group numbers to be allocated
  short unsigned arr2 = 0;
  unsigned char arr3 = 1;
  /* END KLUDGE */


  /*
   * Allocate & activate injection fifo
   */
  DMA_InjFifoGroup_t inj_fifo; 

  if( DMA_InjFifoGroupAllocate( 0,        /* fifo ID */
        1,                                /* fifo allocate count*/
        &arr,                             /* pointer to int arry of fifo ids*/
        &arr2,                            /* pointer to short arry of priorites 0=norm*/
        &arr2,                            /* locals, 0=non-local*/
        &arr3,                            /* bit set = torus fifo mapped */
        NULL,                             /* remote-get fifo-full handler. NULL=unhandled*/
        NULL,                             /* remote-get fifo-full handeler parameters*/
        NULL,                             /* remote-get fifo-full interrupt def*/
        NULL,                             /* remote-get fifo-full interrupt barrier func*/
        NULL,                             /* remote-get fifo-full interrupt barr func params */
        &inj_fifo                         /* Inject fifo struct - full upon sucessful return */
        ) != 0)
    perror("DMA_InjFifoGroupAllocate\n");

  // Assuming the defined value is in bytes..quads?
  // TODO: mult by quadword size..
  // FIXME: malloc & align in quadword size
  char myfifo[DMA_FIFO_DESCRIPTOR_SIZE_IN_QUADS];

  // Init & activate... 
  if( DMA_InjFifoInitById( &inj_fifo,
        0,              /* fifo_id */
        &myfifo,           /* start addr */
        &myfifo,           /* head addr */
        &myfifo+DMA_FIFO_DESCRIPTOR_SIZE_IN_QUADS      /* tail addr */
        ) != 0)
    perror("DMA_InjFifoInitById\n");


  /*
   * Allocate Counters
   *   A direct-put descriptor (our payload) requires both inject and receipt counters
   *   our (payload) descriptor need but one inject counter (the
   *   location of mymem[] is same on all nodes) Howevever, we will need 
   *   receive counters (and payload descriptors) for each
   *   direct-put DMA request
   *
   * Counter subgroup are the unit of allocation: 
   * 256 counters total 
   * 4 counter groups / 64 counters per 
   * 8 subgroup per / 8 counters per 
   *
   * 8 counters is the minimum unit of allocation 
   */

  /* Allocate Injection Counters */ 
  if( DMA_CounterGroupAllocate(DMA_Type_Injection,
        0, 1,                              /* group#, num_subgroups */
        &arr,                              /* subgroups list */ 
        0,                                 /* target core for interrupt */
        &null_func,                        /* handler func when counter hits zero */
        NULL,                              /* handler parameters */
        0,                                 /* interrupt group */
        &inj_desc                          /* injection counter descriptor */
        ) != 0)
    perror("DMA_CounterGroupAllocate");


  /* Allocate Reception Counters */ 
  if( DMA_CounterGroupAllocate(DMA_Type_Reception,
        0, 1,                              /* group#, num_subgroups */
        &arr,                              /* subgroups list */ 
        0,                                 /* target core */
        &null_func,                        /* handler func receive  counter hits zero */
        NULL,                              /* handler parameter */
        0,                                 /* interrupt group */
        &rec_desc                          /* injection counter descriptor */
        ) != 0)
    perror("DMA_CounterGroupAllocate");

  /* Set base for reception counter */
  if( DMA_CounterSetValueBase(
        &rec_desc.counter[0],    
        DST_SPACE,              /* value of counter */
        // FIXEME quad word aligned
        &sharedmem              /* virtual base address */
        ) != 0)
    perror("DMA_CounterGroupAllocate");

  /* Set base of an injection counter */
  if( DMA_CounterSetValueBase(
        &inj_desc.counter[0],      /* counter to update */
        SRC_SPACE,                 /* value of counter */
        &mymem                     /* virtual base address */
        ) != 0)
    perror("DMA_CounterGroupAllocate");

  /* fill local mem with rank id*/
  for(i=0; i<SRC_SPACE; i++)
    mymem[i] = ps_rank;

  /* Rank=1 continues, all other nodes spin */
  while(ps_rank > 1)
    ;
  for(i=0; i<9999999; i++) // stall a bit
    ;
  printf("node 1 continuing..\n");


  /* Torus Remote-Get DMA Messages
   */

  /*
  *  Allocate Descriptors
  *  allocate a direct-put descriptor for remote-get request payload.
  *  
  */

  /* Set local injection counter for DMA request */
  if( DMA_CounterSetValueBase(
        &inj_desc.counter[1],   /* counter to update */
        // VERIFY
        32,                      /* value of counter */
        &payload                /* virtual base address */
        ) != 0)
    perror("DMA_CounterGroupAllocate");

  /* Loop through each node and submit a Remote-Get request */
  for( i=2; i<ps_psetsize; i++){ //node rank begins at 1
    r=nn=0;

    // print coord rank and torus loc
    if( Kernel_Ranks2Coords(&dest, i) != 0)
      perror("Kernal_Rank2Coord\n");
    if( Kernel_Coord2Rank(dest.x, dest.y, dest.z, dest.t, &r, &nn) != 0)
      perror("Kernal_Coord2Rank\n");
    printf("#%d's location (%d, %d, %d, %d) r=%d, nodes=%d\n", i,dest.x,dest.y,dest.z,dest.t,r,nn);

    if( DMA_TorusDirectPutDescriptor(&payload,
          self.x, self.y, self.z,                 /* self location */
          0, 0,                                   /* hints, virt chan */
          0, 0, 0,                                /* inject counter group id, counter id, offset */
          0, 0, (sizeof(char)*SRC_SPACE*(i-1)),   /* reception counter group id, counter id, offset */
          SRC_SPACE                               /* dma len */
          ) != 0)
      perror("DMA_TorusDirectPutDescriptor\n");


    /* allocate descriptor for remote-get request */
    if( DMA_TorusRemoteGetDescriptor(&remote, 
          dest.x, dest.y, dest.z,                 /*  destination location */
          0, 0,                                   /* hints, virt chan */
          // TODO set id to corresponding counter
          0, 1, 0,                                /* inject counter group id, counter id, offset */
          0, 0                                    /* recv inject fifo group, fifo id */
          ) != 0)
      perror("DMA_TorusRemoteGetDescriptor\n");

    // TODO: add descriptor to FIFO and pray
  }



  return 0;
}
