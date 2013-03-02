#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <spi/bgp_SPI.h>
#include <common/bgp_personality.h>
#include <common/bgp_personality_inlines.h>

#define QUAD 16

char data[4*QUAD] __attribute__ ((aligned(QUAD)));
char myfifo[DMA_FIFO_DESCRIPTOR_SIZE_IN_QUADS * QUAD] __attribute__ ((aligned(QUAD)));
  static char inj_fifo_data[DMA_MIN_INJ_FIFO_SIZE_IN_BYTES] __attribute__ ((aligned(32)));

_BGP_Personality_t ps;
DMA_InjFifoGroup_t inj_fifo, inj_fifo_local;
DMA_RecFifoMap_t rec_fifo_map;
DMA_RecFifoGroup_t *rec_fifo; 
DMA_InjDescriptor_t dput_s, dput_r;
DMA_CounterGroup_t inj_grp, rec_grp;
uint32_t rank;
///


/* Counter empty interrupt handler */
  void 
null_func(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
  printf("null func was called!\n");
  return;
}

void
local_barrier(void)
{
  return; 
}

void
global_barrier(void)
{
  //global barrier
  GlobInt_Barrier(0, NULL, NULL);
  return;
}

/* dump of counter status - groups free */
void
print_global_counter_status(DMA_Type_t type){
  int t1;
  int t2[DMA_NUM_COUNTERS_PER_SUBGROUP];
  printf("Counter Type %d Status\n", type);
  for(int i=0; i<DMA_NUM_COUNTER_GROUPS; i++){
    t1=0;
    if( DMA_CounterGroupQueryFree(type, i, &t1, &t2[0]) != 0)
      perror("DMA_CounterGroupQueryFree");
    printf("Group #%d has %d subgroups free:", i, t1);
    for(int j=0; j<t1; j++)
      printf("%d,",t2[j]);
    printf("\n");
  }
}


  void
dma_init(void)
{
  Kernel_CommThreadHandler  null_handle = NULL;

  //
  // INIT PHASE 1
  // 
  //  if( DMA_AddressingInitPhase1() != 0)
  //    perror("DMA_AddressingInitPhase1\n");

  //
  // FIFOS 
  //
  // reception
 // rec_fifo_map.save_headers = 0;
 // for (int i = 0; i < DMA_NUM_NORMAL_REC_FIFOS; i++) {
 //   rec_fifo_map.fifo_types[i] = 0;
 // }
 // for (int i = 0; i < DMA_NUM_HEADER_REC_FIFOS; i++) {
 //   rec_fifo_map.hdr_fifo_types[i] = 0;
 // }
 // rec_fifo_map.threshold[0] = 0;
 // rec_fifo_map.threshold[1] = 0;
 // for (int i = 0; i < 4; i++) {
 //   for (int j = 0; j < 8; j++) {
 //     rec_fifo_map.ts_rec_map[i][j] = 0;
 //   }
 // }
 // if(DMA_RecFifoSetMap(&rec_fifo_map) != 0)
 //   perror("DMA_RecFifoSetMap\n");
 // if(!(rec_fifo = DMA_RecFifoGetFifoGroup(0,0,NULL,NULL,NULL,NULL,NULL)))
 //   perror("DMA_RecFifoGetFifoGroup\n");


/*KLUDGE*/
int fifo_ids[2] = {0,1}; 
unsigned short priorities[2] = {0,0};
unsigned short locals[2] = {0, 1}; // 0 = non-local
unsigned char inj_map[2] = {0xFF, 0};
int sg = 0;

  // injection
  if( DMA_InjFifoGroupAllocate( 0,        
        2,                                
        fifo_ids,                         
        priorities,                       
        locals,                           
        inj_map,                          
        NULL,                             
        NULL,                             
			  (Kernel_InterruptGroup_t) 0,
        NULL,                             
        NULL,                             
        &inj_fifo                         
        ) != 0)
    perror("DMA_InjFifoGroupAllocate  remote\n");

  // Initialize and activate fifo 
  if (DMA_InjFifoInitById(&inj_fifo, 0, inj_fifo_data, 
			  inj_fifo_data, 
			  inj_fifo_data + 
			  DMA_MIN_INJ_FIFO_SIZE_IN_BYTES) != 0) {
    perror("DMA_InjFifoInitById  \n");
  }

  //
  // COUNTERS 
  //
  if( DMA_CounterGroupAllocate(DMA_Type_Injection,
        0, 1,                              /* group#, num_subgroups */
        &sg,                               /* subgroups list */ 
        0,                                 /* target core for interrupt */
        NULL,                              /* handler func when counter hits zero */
        NULL,                              /* handler parameters */
        (Kernel_InterruptGroup_t) 0,       /* interrupt group */
        &inj_grp                           /* injection counter descriptor */
        ) != 0)
    perror("DMA_CounterGroupAllocate");

  if( DMA_CounterGroupAllocate(DMA_Type_Reception,
        0, 1,                              /* group#, num_subgroups */
        &sg,                               /* subgroups list */ 
        0,                                 /* target core for interrupt */
        NULL,                              /* handler func when counter hits zero */
        NULL,                              /* handler parameters */
        (Kernel_InterruptGroup_t) 0,       /* interrupt group */
        &rec_grp                          /* injection counter descriptor */
        ) != 0)
    perror("DMA_CounterGroupAllocate");

  DMA_CounterSetBaseById( &inj_grp, 0, data);
  DMA_CounterSetMaxById( &inj_grp, 0,  data+sizeof(data));
  DMA_CounterSetValueById( &inj_grp, 0, 4*QUAD);
  DMA_CounterSetEnableById( &inj_grp, 0);
  
  // sanity
  unsigned int val;
  void *addr, *max;
  val = DMA_CounterGetValueById( &inj_grp, 0);
  addr = DMA_CounterGetBaseById( &inj_grp, 0);
  max = DMA_CounterGetMaxById( &inj_grp, 0);
  printf("sanity: inj_grp %d, %lx, %lx, %lx\n", val, addr,max,data);

  
  DMA_CounterSetBaseById( &rec_grp, 0, data);
  DMA_CounterSetMaxById( &rec_grp, 0,  data+sizeof(data));
  DMA_CounterSetValueById( &rec_grp, 0, 4*QUAD);
  DMA_CounterSetEnableById( &rec_grp, 0);

  // sanity
  val = DMA_CounterGetValueById( &rec_grp, 0);
  addr = DMA_CounterGetBaseById( &rec_grp, 0);
  max = DMA_CounterGetMaxById( &rec_grp, 0);
  printf("sanity: rec_grp %d, %lx, %lx, %lx\n", val, addr,max,data);

  return;
}

void
dma_read(int dest_rank){
  int orig, newv;
  uint32_t nextx, nexty, nextz, nextt;
  uint32_t selfx, selfy, selfz, selft;
  DMA_InjDescriptor_t payload __attribute__ ((aligned(32)));
  DMA_InjDescriptor_t request;

  if (Kernel_Rank2Coord(rank, &selfx, &selfy, &selfz, &selft) != 0)
    printf("Kernal_Ranks2Coords\n");
  if (Kernel_Rank2Coord(dest_rank, &nextx, &nexty, &nextz, &nextt) != 0)
    printf("Kernal_Ranks2Coords\n");

  printf("Attempting DMA read from rank %d (%d,%d,%d)\n", dest_rank, nextx, nexty, nextz);

  if( DMA_TorusDirectPutDescriptor(&payload,
        selfx, selfy, selfz,
        0, 0,                    /* hints, virt chan */
        0, 0, 0,                 /* dest injection counter*/
        0, 0, 0,                 /* my reception counter */
        (4*QUAD)                 /* dma len */
        ) != 0)
    perror("DMA_TorusDirectPutDescriptor\n");

  DMA_CounterSetBaseById( &inj_grp, 1, &payload);
  DMA_CounterSetMaxById( &inj_grp, 1,  &payload + sizeof(payload));
  DMA_CounterSetValueById( &inj_grp, 1, sizeof(payload));
  DMA_CounterSetEnableById( &inj_grp, 1);
  
  // sanity
  unsigned int val;
  void *addr, *max;
  val = DMA_CounterGetValueById( &inj_grp, 0);
  addr = DMA_CounterGetBaseById( &inj_grp, 0);
  max = DMA_CounterGetMaxById( &inj_grp, 0);
  printf("sanity: inj_grp %d, %lx, %lx, %lx\n", val, addr,max,data);

  // REMOTE GET
  if( DMA_TorusRemoteGetDescriptor(&request,
        nextx, nexty, nextz,
        0, 0,                    /* hints, virt chan */
        0, 1, 0,                 /* my injection counter (for payload)*/
        0, 0                 /* dest inj fifo */
        ) != 0)
    perror("DMA_TorusDirectPutDescriptor\n");

  orig = DMA_CounterGetValueById(&inj_grp, 1);
  newv = orig;

  if( DMA_InjFifoInjectDescriptorById(&inj_fifo, 0, &request) == 0)
    perror("DMA_InjFifoInjectDescriptorById");
  
  while(orig == newv)
  {
    newv = DMA_CounterGetValueById(&inj_grp, 1);
  }

  printf("Finished DMA read request \n");
  return;
}

void
dma_send(int dest_rank){

  int orig, newv;
  uint32_t nextx, nexty, nextz, nextt;
  DMA_InjDescriptor_t request;

  if (Kernel_Rank2Coord(dest_rank, &nextx, &nexty, &nextz, &nextt) != 0)
    printf("Kernal_Ranks2Coords\n");

  printf("Attempting send to rank %d (%d,%d,%d)\n", dest_rank, nextx, nexty,nextz);

  if( DMA_TorusDirectPutDescriptor(&request,
        nextx, nexty, nextz,
        0, 0,                    /* hints, virt chan */
        0, 0, 0,                 /* inject counter group id, counter id, offset */
        0, 0, 0,                 /* reception counter group id, counter id, offset */
        (4*QUAD)                 /* dma len */
        ) != 0)
    perror("DMA_TorusDirectPutDescriptor\n");

  //FREESPACEE SANITY
//  int fs = DMA_FifoGetFreeSpace( &inj_fifo.fifos[0].dma_fifo, 1,0);
//  if ( fs < DMA_MIN_INJECT_SIZE_IN_QUADS + DMA_FIFO_DESCRIPTOR_SIZE_IN_QUADS )
//    printf("Freespace Sanity Failure: %d \n", fs);
//  printf("Freespace Sanity Success: %d \n", fs);

  orig = DMA_CounterGetValueById(&inj_grp, 0);
  newv = orig;

  if( DMA_InjFifoInjectDescriptorById(&inj_fifo, 0, &request) == 0)
    perror("DMA_InjFifoInjectDescriptorById");
  
  while(orig == newv)
  {
    newv = DMA_CounterGetValueById(&inj_grp, 0);
  }

  printf("Finished send!\n");
  return;
}

int
main()
{
  int i,count;
  unsigned int orig, newv;
  uint32_t fakerank;
  // identify self
  Kernel_GetPersonality(&ps, sizeof(ps)); 
  fakerank = BGP_Personality_rankInPset(&ps); /* note: rank begins count at 1 */
  rank = ps.Network_Config.Rank;
  printf("#%d,%d &data=%p\n",rank, fakerank,&data);

  // fill data
  for( i=0; i<4*QUAD; i++)
    data[i]=rank;
  _bgp_mbar();    /* Make sure these writes have been accepted by the memory */

  // initialize DMA
  dma_init();
  printf("#%d finished init\n",rank);

  orig = DMA_CounterGetValueById(&rec_grp, 0);
  newv = orig;
  GlobInt_Barrier(0, NULL, NULL);
  /* other nodes spin */
  if( rank > 1)
    while(1)
      ;
 // print_global_counter_status(DMA_Type_Injection);
 // printf("inj_grp counter permission val: %x %x\n", inj_grp.permissions[0], inj_grp.permissions[1]);
 // printf("inj_grp subgroup permission val: %x\n", inj_grp.grp_permissions);
 // print_global_counter_status(DMA_Type_Reception);
 // printf("rec_grp counter permission val: %x %x\n", rec_grp.permissions[0], rec_grp.permissions[1]);
 // printf("rec_grp subgroup permission val: %x\n", rec_grp.grp_permissions);

  /* simple dma test*/
  if(rank == 1) {
    printf("sanity rec: %d %d\n", orig, newv);
    for(int i=0; i < 4*QUAD; i++)
      printf("%d",data[i]); 
    printf("\n"); 

    while(orig == newv)
    { newv = DMA_CounterGetValueById(&rec_grp, 0); }

    printf("receive complete!\n");
    for(int i=0; i < 4*QUAD; i++)
      printf("%d",data[i]); 

    printf("\n"); 
    
  } else {
    dma_send(1); // send to rank=1
    _bgp_Delay(3000000);
    // send complete, let's do a pull from another node
    dma_read(2);

    while(orig == newv)
    { newv = DMA_CounterGetValueById(&rec_grp, 0); }

    printf("RDMA read complete!\n");
    for(int i=0; i < 4*QUAD; i++)
      printf("%d",data[i]); 

    printf("\n"); 


  }
  //_bgp_Delay(3000000);
  //
  while(1)
    ;
  return 0;
}


