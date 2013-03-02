/* Torus Scatter Gather */
#ifndef _SG_H_
#define _SG_H_

// FIFOS
#define SG_FIFO_GRP         3

// COUNTERS
#define SG_CTR_GRP          3
#define SG_CTR_SGRP_COUNT   1
#define SG_TRACKS           SG_CTR_SGRP_COUNT * 8

typedef struct DMA_iovec_s {
  unsigned int dst_rank;
  unsigned int dst_ctr_grp;
  unsigned int track;
  unsigned int buf_len;
  unsigned int offset;
} DMA_iovec;

unsigned int
DMA_settrack(unsigned int track, int len, void* buff);

unsigned int
DMA_sginit(uint32_t myrank);

unsigned int
DMA_readv(void* in, DMA_iovec *iov, int iovcnt);

unsigned int
DMA_writev(void* in, DMA_iovec *iov, int iovcnt);


unsigned int
DMA_check_rec(int track);

unsigned int
DMA_check_inj(int track);
#endif
