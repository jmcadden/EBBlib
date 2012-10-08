#ifndef __EBB_KLUDGE_H__
#define __EBB_KLUDGE_H__

// this is tornado speak for setting the class code, we should strip this out everywhere
// after we decide what we want to do with errors with EBBLIB
#define SET_CLSCD(A,B) 
#define TRACE lrt_printf

#if 0
#define LNX_TRACE(fmt, ...)	lrt_printf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__)
#endif

#define MISC 		1
#define TR_INFO 	1
#define MYVP            0
#define NUMPROC         1

#define tassert(EX,STMT)			\
  if (!(EX)) {					\
    TRACE("%s", #EX);				\
    STMT;					\
    lrt_exit(0);					\
  }

#define trace(S,L,STMT)				\
  {						\
    TRACE("%s %s", #S, #L);	                \
    STMT;                                       \
  }

#define ass_printf TRACE 
#define tr_printf TRACE

#ifndef EBBLIB

#define DREF(id) id

#else
#include <config.h>
#include <stdint.h>

#include <l0/lrt/types.h>
#include <l0/cobj/cobj.h>
#include <lrt/io.h>
#include <l0/lrt/trans.h>
#include <l0/cobj/CObjEBB.h>
#include <l0/EBBMgrPrim.h>
#include <l0/cplus/CPlusEBB.H>
#include <l0/cplus/CPlusEBBRoot.H>
#include <l0/cplus/CPlusEBBRootShared.H>
#include <l0/MemMgr.h>
#include <l0/MemMgrPrim.h>

#define DREF EBBId_DREF
#endif

#endif

