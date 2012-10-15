#ifndef ATOMIC_AMD64_H
#define ATOMIC_AMD64_H

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

#define atomic_fetch_and_sub            __sync_fetch_and_sub
#define atomic_fetch_and_sub32          __sync_fetch_and_sub

#define atomic_fetch_and_add            __sync_fetch_and_add
#define atomic_fetch_and_add32          __sync_fetch_and_add

#define atomic_fetch_and_or             __sync_fetch_and_or
#define atomic_fetch_and_or32           __sync_fetch_and_or

#define atomic_fetch_and_and            __sync_fetch_and_and
#define atomic_fetch_and_and32          __sync_fetch_and_and

#define atomic_add_and_fetch            __sync_add_and_fetch
#define atomic_add_and_fetch32          __sync_add_and_fetch

#define atomic_sub_and_fetch            __sync_sub_and_fetch
#define atomic_sub_and_fetce32          __sync_sub_and_fetch

#define atomic_or_and_fetch             __sync_or_and_fetch
#define atomic_or_and_fetch32           __sync_or_and_fetch

#define atomic_and_and_fetch            __sync_and_and_fetch
#define atomic_and_and_fetch32          __sync_and_and_fetch

#define atomic_bool_compare_and_swap    __sync_bool_compare_and_swap
#define atomic_bool_compare_and_swap32  __sync_bool_compare_and_swap

#define atomic_synchronize              __sync_synchronize

#endif
