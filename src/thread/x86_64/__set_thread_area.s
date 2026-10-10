/* Dunit OS port: set the x86_64 thread pointer via the native Dunit syscall.
 *
 * Upstream musl issues arch_prctl(ARCH_SET_FS, p). The Dunit kernel exposes a
 * dedicated SetThreadPointer syscall (number 39, see dunit-os abi/syscalls.abi)
 * that installs the pointer into IA32_FS_BASE immediately and returns 0, so
 * %fs:0 reads back the thread pointer as soon as this returns — exactly what
 * __init_tp()/__pthread_self() need. The argument is already in %rdi.
 *
 * Based on musl's __set_thread_area.s (MIT, Copyright 2011-2012 Nicholas J. Kain).
 */
.text
.global __set_thread_area
.hidden __set_thread_area
.type __set_thread_area,@function
__set_thread_area:
	movl $39,%eax           /* Dunit SetThreadPointer */
	syscall                 /* tp in %rdi; sets %fs base, returns 0 */
	ret
