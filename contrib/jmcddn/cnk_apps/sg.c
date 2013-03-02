/* Torus Scatter Gather */
#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <spi/bgp_SPI.h>
#include <common/bgp_personality.h>
#include <common/bgp_personality_inlines.h>
#include "sg.h"


/* Global FIFO / Counter groups for scatter-gather  */
DMA_InjFifoGroup_t  sg_inj_fifo_grp, sg_rec_fifo_grp;
DMA_CounterGroup_t  sg_inj_grp, sg_rec_grp;
static char inj_fifo_data[DMA_MIN_INJ_FIFO_SIZE_IN_BYTES] __attribute__ ((aligned(32)));

uint32_t rank;
  

/* Init DMA fifos, counters and buffers for scatter-gather */
unsigned int
DMA_sginit(uint32_t myrank)
{
  int i;
  // TODO: interrupt handler
  Kernel_CommThreadHandler  null_handle = NULL;

  // Injection Fifo Config
  // Note: fifo count fixed at 1 
  int fifo_ids[1] = {0}; 
  unsigned short priorities[1] = {0};
  unsigned short locals[1] = {0}; // 0 = non-local
  unsigned char inj_map[1] = {0xFF}; // all hw fifos

  rank = myrank; //FIXME, this is sloppy

  // Injection FIFO group
  if( DMA_InjFifoGroupAllocate( 
        SG_FIFO_GRP, 1, /* group num, fifo count */
        fifo_ids, priorities, locals, inj_map,                          
        NULL,NULL,                             
        (Kernel_InterruptGroup_t) 0,
        NULL,NULL,
        &sg_inj_fifo_grp) != 0)
    perror("DMA_InjFifoGroupAllocate  remote\n");

  // Initialize and activate fifo 
  if (DMA_InjFifoInitById(
        &sg_inj_fifo_grp, 0, inj_fifo_data, inj_fifo_data, 
        inj_fifo_data + DMA_MIN_INJ_FIFO_SIZE_IN_BYTES) != 0) {
    perror("DMA_InjFifoInitById  \n");
  }

  // Allocate Injection and Reception counters for scatter-gather
  int sg[SG_CTR_SGRP_COUNT];
  for( i=0; i<SG_CTR_SGRP_COUNT; i++)
    sg[i] = i; 

  if( DMA_CounterGroupAllocate(DMA_Type_Injection,
        SG_CTR_GRP, SG_CTR_SGRP_COUNT, sg,   /* group#, sgroups count, sgroup list */ 
        0, NULL, NULL,                       /* interrupts*/         
        (Kernel_InterruptGroup_t) 0,       
        &sg_inj_grp                        
        ) != 0)
    perror("DMA_CounterGroupAllocate");

  if( DMA_CounterGroupAllocate(DMA_Type_Reception,
        SG_CTR_GRP, 2, sg,                   /* group#, sgroups count, sgroup list */ 
        0, NULL, NULL,                       /* interrupts*/         
        (Kernel_InterruptGroup_t) 0,       
        &sg_rec_grp                       
        ) != 0)
    perror("DMA_CounterGroupAllocate");

  return 0; 
}


int
DMA_gettrack(void){

  int i;
  for(i=0; i<SG_TRACKS; i++)
    if( DMA_CounterGetEnabledById( &sg_rec_grp, i) == 0 && 
    (DMA_CounterGetEnabledById( &sg_inj_grp, i) == 0)){
      DMA_CounterSetEnableById( &sg_inj_grp, i);
      DMA_CounterSetEnableById( &sg_rec_grp, i);
      return i; 
    }
  return -1; // no free tracks
}

void
DMA_freetrack(int track){
  DMA_CounterSetDisableById( &sg_inj_grp, track);
  DMA_CounterSetDisableById( &sg_rec_grp, track);
}

/*  Set and enable counters for scatter-gather track buffer
 *  XXX: buf must already be 16-bit aligned 
 */
unsigned int
DMA_settrack(unsigned int track, int len, void* buff)
{
  //TODO: assert track >= 0 && track < SG_TRACKS - 1
  //TODO assert track enabled
  DMA_CounterSetDisableById( &sg_inj_grp, track);
  DMA_CounterSetDisableById( &sg_rec_grp, track);
  DMA_CounterSetBaseById(   &sg_inj_grp, track, buff);
  DMA_CounterSetMaxById(    &sg_inj_grp, track, buff+len);
  DMA_CounterSetValueById(  &sg_inj_grp, track, len);
  DMA_CounterSetBaseById(   &sg_rec_grp, track, buff);
  DMA_CounterSetMaxById(    &sg_rec_grp, track, buff+len);
  DMA_CounterSetValueById(  &sg_rec_grp, track, len);
 
      DMA_CounterSetEnableById( &sg_inj_grp, track);
      DMA_CounterSetEnableById( &sg_rec_grp, track);
  return 0; 
}

/* setuo IO vector structure */
unsigned int
DMA_sg_setvec( DMA_iovec *ivo, uint32_t rank,  uint32_t track, uint32_t len, uint32_t offset)
{
  ivo->dst_rank = rank; 
  ivo->dst_ctr_grp = SG_CTR_GRP;
  ivo->track = track; // 0-7
  ivo->offset = offset; 
  ivo->buf_len = len; // amount of data to send
  return 0;
}

uint32_t
DMA_config_dump(void){

  printf("SANITY DUMP\n");
  printf("inj_grp counter permission val: %x %x\n", sg_inj_grp.permissions[0], sg_inj_grp.permissions[1]);
  printf("inj_grp subgroup permission val: %x\n", sg_inj_grp.grp_permissions);
  printf("rec_grp counter permission val: %x %x\n", sg_rec_grp.permissions[0], sg_rec_grp.permissions[1]);
  printf("rec_grp subgroup permission val: %x\n", sg_rec_grp.grp_permissions);

  unsigned int val;
  void *addr, *max;
  int i;
  for(i=0; i<SG_TRACKS; i++){
    val = DMA_CounterGetValueById( &sg_inj_grp, i);
    addr = DMA_CounterGetBaseById( &sg_inj_grp, i);
    max = DMA_CounterGetMaxById( &sg_inj_grp, i);
    printf("counter inj_grp %d: %d, %lx, %lx\n",i, val, addr,max);
    val = DMA_CounterGetValueById( &sg_rec_grp, i);
    addr = DMA_CounterGetBaseById( &sg_rec_grp, i);
    max = DMA_CounterGetMaxById( &sg_rec_grp, i);
    printf("counter rec_grp %d: %d, %lx, %lx\n",i, val, addr,max);
  }

  return 0;
}


/* Blocking read across IO vector 
 *
 * return: amount read in
 *
 * */
unsigned int
DMA_readv(void* in, DMA_iovec *iov, int iovcnt)
{
  int orig, newv, i;
  uint32_t nextx, nexty, nextz, nextt;
  uint32_t selfx, selfy, selfz, selft;
  uint32_t dest_rank, read_len;
  DMA_InjDescriptor_t payload __attribute__ ((aligned(32)));
  DMA_InjDescriptor_t request;
  int offset, count;

  count = offset = 0;

  // get total amount to send 
  for( i=0; i<iovcnt; i++)
    count += iov[i].buf_len;
  
  printf("sanity: total send calculated  %d \n", count);

  if (Kernel_Rank2Coord(rank, &selfx, &selfy, &selfz, &selft) != 0)
    printf("Kernal_Ranks2Coords\n");

  // this should already exist
  // char rdma_read_data[count] __attribute__ ((aligned(32)));

 int freetrack =  DMA_gettrack();

  // new reception counter
  DMA_CounterSetBaseById(   &sg_rec_grp, freetrack, in);
  DMA_CounterSetMaxById(    &sg_rec_grp, freetrack, in+count);
  DMA_CounterSetValueById(  &sg_rec_grp, freetrack, count);
  DMA_CounterSetEnableById( &sg_rec_grp, freetrack);

  orig = newv = DMA_check_rec(freetrack);
  // for each iov, issue a direct put msg
  for( i=0; i<iovcnt; i++)
  {
    if (Kernel_Rank2Coord(iov[i].dst_rank, &nextx, &nexty, &nextz, &nextt) != 0)
      printf("Kernal_Ranks2Coords\n");
    printf("Attempting DMA read from rank %d \n", iov[i].dst_rank, nextx, nexty, nextz);

    if( DMA_TorusDirectPutDescriptor(&payload,
        selfx, selfy, selfz, 0, 0,  
        SG_CTR_GRP, iov[i].track, iov[i].offset,   /* dest injection counter*/
        SG_CTR_GRP, freetrack, offset,        /* my reception counter */
        iov[i].buf_len                /* dma len */
        ) != 0)
      perror("DMA_TorusDirectPutDescriptor\n");

      // payload counter
      DMA_CounterSetBaseById(   &sg_inj_grp, freetrack, &payload);
      DMA_CounterSetMaxById(    &sg_inj_grp, freetrack, &payload + sizeof(payload));
      DMA_CounterSetValueById(  &sg_inj_grp, freetrack, sizeof(payload));
      DMA_CounterSetEnableById( &sg_inj_grp, freetrack);

      //// sanity
      //unsigned int val;
      //void *addr, *max;
      //val = DMA_CounterGetValueById( &inj_grp, 0);
      //addr = DMA_CounterGetBaseById( &inj_grp, 0);
      //max = DMA_CounterGetMaxById( &inj_grp, 0);
      //printf("sanity: inj_grp %d, %lx, %lx, %lx\n", val, addr,max,data);
      
      // REMOTE GET
      if( DMA_TorusRemoteGetDescriptor(&request,
            nextx, nexty, nextz, 0, 0,
            SG_CTR_GRP, freetrack, 0,           /* my injection counter (for payload)*/
            SG_FIFO_GRP, 0              /* dest inj fifo */
            ) != 0)
        perror("DMA_TorusDirectPutDescriptor\n");

      if( DMA_InjFifoInjectDescriptorById(&sg_inj_fifo_grp, 0, &request) == 0)
        perror("DMA_InjFifoInjectDescriptorById");

      offset += iov[i].buf_len; // increase read offset
  }
  // **********************************************

  // spin until we've read all the data

  while( DMA_check_rec(freetrack) == count)
    ; 

  return count;
}

/* Async write out across IO vector 
 *
 * return: amount written out
 *
 * */
unsigned int
DMA_writev(void *fd, DMA_iovec *iov, int iovcnt)
{
  int i;
  int orig, newv;
  uint32_t nextx, nexty, nextz, nextt;
  DMA_InjDescriptor_t request;
  uint32_t dest_rank;

  // for each iov, issue a direct put msg
  for( i=0; i<iovcnt; i++)
  {
    dest_rank = iov[i].dst_rank;

    if (Kernel_Rank2Coord(dest_rank, &nextx, &nexty, &nextz, &nextt) != 0)
      printf("Kernal_Ranks2Coords\n");
    if( DMA_TorusDirectPutDescriptor(&request,
          nextx, nexty, nextz, 0, 0,                    
          SG_CTR_GRP, 0, 0,                 
          iov[i].dst_ctr_grp, iov[i].track, iov[i].offset,
          iov[i].buf_len                
          ) != 0)
      perror("DMA_TorusDirectPutDescriptor\n");

    orig = DMA_CounterGetValueById(&sg_inj_grp, 0);
    newv = orig;
    if( DMA_InjFifoInjectDescriptorById(&sg_inj_fifo_grp, 0, &request) == 0)
      perror("DMA_InjFifoInjectDescriptorById");

    while(orig == newv)
      newv = DMA_CounterGetValueById(&sg_inj_grp, 0);
  }
  
  return 0;
}


unsigned int 
DMA_check_rec(int track)
{
  return DMA_CounterGetValueById(&sg_rec_grp, track);
}

unsigned int 
DMA_check_inj(int track)
{
  return DMA_CounterGetValueById(&sg_rec_grp, track);
}

