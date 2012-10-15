#ifndef ARCH_POWERPC_ATOMIC_H
#define ARCH_POWERPC_ATOMIC_H 
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
atomic_fetch_and_add (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndAdd32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	add	%2,%1,%3	# tmp = oval + val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndAdd32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return oval;
}

  inline uint32_t 
atomic_fetch_and_add32 (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndAdd32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	add	%2,%1,%3	# tmp = oval + val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndAdd32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return oval;
}

  inline uint32_t 
atomic_fetch_and_or  (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndOr32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	or	%2,%1,%3	# tmp = oval OR val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndOr32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return oval;
}

  inline uint32_t 
atomic_fetch_and_or32  (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndOr32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	or	%2,%1,%3	# tmp = oval OR val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndOr32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return oval;
}

  inline uint32_t 
atomic_fetch_and_and (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndAnd32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	and	%2,%1,%3	# tmp = oval AND val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndAnd32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return oval;
}

  inline uint32_t 
atomic_fetch_and_and32 (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndAnd32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	and	%2,%1,%3	# tmp = oval AND val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndAnd32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return oval;
}

  inline uint32_t 
atomic_add_and_fetch (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndAdd32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	add	%2,%1,%3	# tmp = oval + val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndAdd32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return tmp;
}

  inline uint32_t 
atomic_add_and_fetch32 (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndAdd32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	add	%2,%1,%3	# tmp = oval + val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndAdd32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return tmp;
}

  inline uint32_t 
atomic_sub_and_fetch (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndAdd32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	subf	%2,%1,%3	# tmp = oval - val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndAdd32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return tmp;
}

  inline uint32_t 
atomic_sub_and_fetch32 (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndAdd32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	subf	%2,%1,%3	# tmp = oval - val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndAdd32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return tmp;
}

  inline uint32_t 
atomic_or_and_fetch  (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndAdd32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	or	%2,%1,%3	# tmp = oval OR val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndAdd32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return tmp;
}

  inline uint32_t 
atomic_or_and_fetch32  (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndAdd32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	or	%2,%1,%3	# tmp = oval OR val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndAdd32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return tmp;
}

  inline uint32_t 
atomic_and_and_fetch (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndAdd32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	and	%2,%1,%3	# tmp = oval AND val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndAdd32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return tmp;
}

  inline uint32_t 
atomic_and_and_fetch32 (volatile uint32_t *ptr, uint32_t val)
{
  uint32_t oval;
  uint32_t tmp;

  __asm__ ("\n"
      "# _FetchAndAdd32						\n"
      "	lwarx	%1,0,%4		# oval = (*ptr)	[linked]	\n"
      "	and	%2,%1,%3	# tmp = oval AND val		\n"
      "	stwcx.	%2,0,%4		# (*ptr) = tmp	[conditional]	\n"
      "	bne-	$-12		# if (store failed) retry	\n"
      "# end _FetchAndAdd32						\n"
      : "=m" (*(char*)ptr), "=&r" (oval), "=&r" (tmp)
      : "r" (val), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );

  return tmp;
}


  inline uint32_t 
atomic_bool_compare_and_swap (volatile uint32_t *ptr, uint32_t oval, uint32_t nval)
{
  uint32_t tmp;

  __asm__ ("\n"
      "# _CompareAndStore32						\n"
      "	lwarx	%1,0,%4		# tmp = (*ptr)	[linked]	\n"
      "	cmplw	%1,%2		# if (tmp != oval)		\n"
      "	bne-	$+20		#     goto failure		\n"
      "	stwcx.	%3,0,%4		# (*ptr) = nval	[conditional]	\n"
      "	bne-	$-16		# if (store failed) retry	\n"
      "	li	%1,1		# tmp = SUCCESS			\n"
      "	b	$+8		# goto end			\n"
      "	li	%1,0		# tmp = FAILURE			\n"
      "# end _CompareAndStore32					\n"
      : "=m" (*(char*)ptr), "=&r" (tmp)
      : "r" (oval), "r" (nval), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );
  return tmp;
}

  inline uint32_t 
atomic_bool_compare_and_swap32 (volatile uint32_t *ptr, uint32_t oval, uint32_t nval)
{
  uint32_t tmp;

  __asm__ ("\n"
      "# _CompareAndStore32						\n"
      "	lwarx	%1,0,%4		# tmp = (*ptr)	[linked]	\n"
      "	cmplw	%1,%2		# if (tmp != oval)		\n"
      "	bne-	$+20		#     goto failure		\n"
      "	stwcx.	%3,0,%4		# (*ptr) = nval	[conditional]	\n"
      "	bne-	$-16		# if (store failed) retry	\n"
      "	li	%1,1		# tmp = SUCCESS			\n"
      "	b	$+8		# goto end			\n"
      "	li	%1,0		# tmp = FAILURE			\n"
      "# end _CompareAndStore32					\n"
      : "=m" (*(char*)ptr), "=&r" (tmp)
      : "r" (oval), "r" (nval), "r" (ptr), "m" (*(char*)ptr)
      : "cc"
      );
  return tmp;
}

  inline void 
atomic_synchronize (void)
{
  //__asm__ __volatile__ ("eieio" : : : "memory");
  __asm__ __volatile__ ("isync" : : : "memory");
}

#endif
