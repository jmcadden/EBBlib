#ifndef SYNC_RWLOCK_H
#define SYNC_RWLOCK_H

/*
 * Copyright (C) 2011 by Project SESA, Boston University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <arch/cpu.h>
#include <arch/atomic.h>

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

/* Simple Reader-Writer Lock 
 *
 * A simple reader-writer lock thats allows for concurrent reads and mutually
 * exclusive writes. Writer starvation is circomvented by prevening readers
 * from prempting a pending write. 
 *
 * All writers race for the lock. We do not currently prioritize by arrival time.   
 *
 * */
typedef union rwlock
{
  uint32_t raw;
  /* writers */
  struct {
    union {
      uint16_t raw;
      struct {
        uint8_t flag; /* active flag */
        uint8_t wait;  /* writer wait counter - try to avoid write starvation */
      };
    }writers;
  /* readers */
    union {
      uint16_t raw;
      struct {
        uint8_t flag; /* active flag */
        /* XXX: unpredicted behaviour with >256 simultaneous readers */
        uint8_t count; /* reader count */
      };
    }readers;
  };
} rwlock;

/* Initialize Lock */
static inline void
rwlock_init(rwlock *l)
{
  l->raw = 0; /* clear flags & count */
  atomic_synchronize();
}

/* Writer Acquire */
static inline void
rwlock_wrlock(rwlock *l)
{
  rwlock lsnap, lnew;
  atomic_synchronize();
  do{
    cpu_relax();
    lsnap = lnew = *l; /* transient snapshots of lock*/
    /* acquire lock if there are no current reader or writers.
     * disregard waiting flag... snooze you loose.
     */
    if (lnew.readers.raw == 0 && lnew.writers.raw == lnew.writers.wait)
      lnew.writers.flag = 1;
    else if (lnew.writers.wait == 0) /* set wait count */
      lnew.writers.wait += 1; 

    /* If we made a write, try and commit it. Else, we try again. */
  }while(!(lnew.raw != lsnap.raw && atomic_bool_compare_and_swap32(&l->raw, lsnap.raw, lnew.raw)));
}

/* Writer Release */ 
static inline void
rwlock_wrunlock(rwlock *l)
{
  rwlock lsnap, lnew;
  atomic_synchronize();
  do{
    cpu_relax();
    lsnap = lnew = *l; /* transient snapshots of lock*/
    /* acquire lock if there are no writers (include those waiting.) */
    lnew.writers.flag = 0;
    lnew.writers.wait += -1; 
    /* If we made a write, try and commit it. Else, we try again. */
  }while(!atomic_bool_compare_and_swap32(&l->raw, lsnap.raw, lnew.raw));
}

/* Reader Acquire */
static inline void
rwlock_rdlock(rwlock *l)
{
  rwlock lsnap, lnew;
  atomic_synchronize();
  do{
    cpu_relax();
    lsnap = lnew = *l; /* transient snapshots of lock*/
    /* acquire lock if there are no writers (include those waiting.) */
    if (lnew.writers.raw == 0)
      lnew.readers.flag = 1; lnew.readers.count += 1;
    /* If we made a write, try and commit it. Else, we try again. */
  }while(!(lnew.raw != lsnap.raw && atomic_bool_compare_and_swap32(&l->raw, lsnap.raw, lnew.raw)));
}

/* Reader Release */
static inline void
rwlock_rdunlock(rwlock *l)
{
  rwlock lsnap, lnew;
  atomic_synchronize();
  do{
    cpu_relax();
    lsnap = lnew = *l; /* transient snapshots of lock*/
    /* acquire lock if there are no writers (include those waiting.) */
    if(lnew.readers.count == 1)
      lnew.readers.raw = 0;
    else
      lnew.readers.count += -1; 
    /* If we made a write, try and commit it. Else, we try again. */
  }while(!(lnew.raw != lsnap.raw && atomic_bool_compare_and_swap32(&l->raw, lsnap.raw, lnew.raw)));
}
#endif
