#ifndef SYNC_ATOMIC_AMD64_H
#define SYNC_ATOMIC_AMD64_H

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

inline uint32_t 
atomic_fetch_and_add (volatile uint32_t *addr, uint32_t val)
{
  return __sync_fetch_and_add(addr, val);
}

inline uint32_t 
atomic_fetch_and_or  (volatile uint32_t *addr, uint32_t val)
{
  return __sync_fetch_and_or(addr, val);
}

inline uint32_t 
atomic_fetch_and_and (volatile uint32_t *addr, uint32_t val)
{
  return __sync_fetch_and_and(addr, val);
}

inline uint32_t 
atomic_add_and_fetch (volatile uint32_t *addr, uint32_t val)
{
  return __sync_add_and_fetch(addr, val);
}

inline uint32_t 
atomic_sub_and_fetch (volatile uint32_t *addr, uint32_t val)
{
  return __sync_sub_and_fetch(addr, val);
}


inline uint32_t 
atomic_or_and_fetch  (volatile uint32_t *addr, uint32_t val)
{
  return __sync_or_and_fetch( addr, val);
}

inline uint32_t 
atomic_and_and_fetch (volatile uint32_t *addr, uint32_t val)
{
  return __sync_and_and_fetch(addr, val);
}


inline uint32_t 
atomic_bool_compare_and_swap (volatile uint32_t *addr, 
    uint32_t oldval, uint32_t newval)
{
  return __sync_bool_compare_and_swap(addr, oldval, newval);
}

inline uint32_t 
atomic_synchronize (void)
{
  __sync_syncronize();
}

#endif
