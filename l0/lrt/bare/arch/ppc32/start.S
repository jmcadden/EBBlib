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

#include <arch/powerpc/asdef.h>
#include <arch/powerpc/regs.h>
	
	.globl _start
	.section .init.entry, "ax"
	.balign 8
_start:
	// setup MSR
	mfmsr r21
	/* diable external interupts*/
	andi. r21, r21, 0x7fff
	/* enable fpu & machine check */
	ori   r21, r21, 0x3000
	mtmsr r21
	
	//set small data areas to 0
	li	r13, 0
	li	r2, 0

	//load stack into r1
	lis	r1, _mini_stack@h
	ori	r1, r1, _mini_stack@l
	subi	r1, r1, 112 //ask about this

	mfspr	r21, SPRN_PIR
	lis	r22, entry_secondary@h
	ori	r22, r22, entry_secondary@l
1:
	dcbi	0, r22
	lwz	r23, 0(r22)
	cmpw	r23, r21
	bne	1b

	bl init_mapping
	bl init
spin:
	b	spin
  
	.globl entry_secondary
entry_secondary: .long 0
	
	.section .init.data
	.align 4
	.space 0x1000
_mini_stack:
