#ifndef SYNC_ATOMIC_H
#define SYNC_ATOMIC_H

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

#if 0

// GCC standard atomic calls
type __sync_fetch_and_add (type *ptr, type value, ...)
type __sync_fetch_and_sub (type *ptr, type value, ...)
type __sync_fetch_and_or (type *ptr, type value, ...)
type __sync_fetch_and_and (type *ptr, type value, ...)
type __sync_fetch_and_xor (type *ptr, type value, ...)
type __sync_fetch_and_nand (type *ptr, type value, ...)
    These builtins perform the operation suggested by the name, and returns the value that had previously been in memory. That is,
              { tmp = *ptr; *ptr op= value; return tmp; }
              { tmp = *ptr; *ptr = ~tmp & value; return tmp; }   // nand
         
type __sync_add_and_fetch (type *ptr, type value, ...)
type __sync_sub_and_fetch (type *ptr, type value, ...)
type __sync_or_and_fetch (type *ptr, type value, ...)
type __sync_and_and_fetch (type *ptr, type value, ...)
type __sync_xor_and_fetch (type *ptr, type value, ...)
type __sync_nand_and_fetch (type *ptr, type value, ...)
    These builtins perform the operation suggested by the name, and return the new value. That is,
              { *ptr op= value; return *ptr; }
              { *ptr = ~*ptr & value; return *ptr; }   // nand
         
bool __sync_bool_compare_and_swap (type *ptr, type oldval type newval, ...)
type __sync_val_compare_and_swap (type *ptr, type oldval type newval, ...)
    These builtins perform an atomic compare and swap. That is, if the current value of *ptr is oldval, then write newval into *ptr.
    The “bool” version returns true if the comparison is successful and newval was written. The “val” version returns the contents of *ptr before the operation.
__sync_synchronize (...)
    This builtin issues a full memory barrier.


K42
compare and store
fetch and stoe
fetch and add
fetch and and
fetch and store

L4
lock
unlock
load-reserve
store-reserve
atomic or (fetch and or)
atomic and (fetch and and)

An isync instruction causes the processor to complete execution of all
previous instructions and then to discard instructions (which may have begun
execution) following the isync. After the isync is executed, the following
instructions then begin execution. 

#endif 

/* These builtins perform the operation suggested by the name, and returns the
 * value that had previously been in memory. */
inline uint32_t atomic_fetch_and_add (volatile uint32_t *addr, uint32_t val);
inline uint32_t atomic_fetch_and_or  (volatile uint32_t *addr, uint32_t val);
inline uint32_t atomic_fetch_and_and (volatile uint32_t *addr, uint32_t val);

/* These builtins perform the operation suggested by the name, and return
 * the new value. */
inline uint32_t atomic_add_and_fetch (volatile uint32_t *addr, uint32_t value);
inline uint32_t atomic_sub_and_fetch (volatile uint32_t *addr, uint32_t value);
inline uint32_t atomic_or_and_fetch  (volatile uint32_t *addr, uint32_t value);
inline uint32_t atomic_and_and_fetch (volatile uint32_t *addr, uint32_t value);

/* These builtins perform an atomic compare and swap. That is, if the current
 * value of *ptr is oldval, then write newval into *ptr.  The “bool” version
 * returns true if the comparison is successful and newval was written. The
 * “val” version returns the contents of *ptr before the operation.  */
inline uint32_t atomic_bool_compare_and_swap volatile uint32_t *addr, uint32_t oldval, uint32_t newval);

/* This builtin issues a full memory barrier. */
inline uint32_t atomic_synchronize (void)

#endif
