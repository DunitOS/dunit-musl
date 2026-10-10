/* Dunit port: __unmapself(base, size) unmaps a detached thread's own stack and
 * then terminates the thread without touching the stack again. Upstream used
 * Linux munmap(11)+exit(60); Dunit uses the native Munmap(34) then
 * ThreadExit(31). The argument registers (base in %rdi, size in %rsi) already
 * match the native Munmap(addr,len).
 * Based on musl's __unmapself.s (MIT, Copyright 2011-2012 Nicholas J. Kain).
 */
.text
.global __unmapself
.type   __unmapself,@function
__unmapself:
	movl $34,%eax   /* Dunit Munmap(base, size) */
	syscall
	xor %rdi,%rdi   /* exit code 0 */
	movl $31,%eax   /* Dunit ThreadExit */
	syscall
	hlt
