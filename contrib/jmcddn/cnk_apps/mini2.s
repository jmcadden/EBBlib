	.file	"mini2.c"
	.section	.rodata
	.align 2
.LC0:
	.string	"&data=%p\n"
	.section	".text"
	.align 2
	.globl main
	.type	main, @function
main:
	stwu 1,-32(1)
	mflr 0
	stw 31,28(1)
	stw 0,36(1)
	mr 31,1
	lis 9,.LC0@ha
	la 3,.LC0@l(9)
	lis 9,data@ha
	la 4,data@l(9)
	crxor 6,6,6
	bl printf
	li 0,0
	stw 0,8(31)
	b .L2
.L3:
	lwz 0,8(31)
	mr 11,0
	lis 9,data@ha
	la 0,data@l(9)
	add 9,11,0
	li 0,1
	stb 0,0(9)
	lwz 9,8(31)
	addi 0,9,1
	stw 0,8(31)
.L2:
	lwz 0,8(31)
	cmpwi 7,0,63
	ble 7,.L3
	bl __sync__synchronize
	li 0,0
	mr 3,0
	lwz 11,0(1)
	lwz 0,4(11)
	mtlr 0
	lwz 31,-4(11)
	mr 1,11
	blr
	.size	main,.-main
	.comm	data,64,16
	.ident	"GCC: (GNU) 4.1.2 20070115 (SUSE Linux)"
	.section	.note.GNU-stack,"",@progbits
