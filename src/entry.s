.section .init

.option norvc

.type _start, @function
.global _start
_start:
	.cfi_startproc

.option push
.option norelax
	la gp, global_pointer
.option pop

	/* Setup stack */
	la sp, stack_top

	/* Clear the BSS section */
	la t5, bss_start
	la t6, bss_end
bss_clear:
	sd zero, (t5)
	addi t5, t5, 8
	bltu t5, t6, bss_clear

	/* OpenSBI passes DTB pointer in a1, hartid in a0 */
	/* Move DTB pointer from a1 to a0 for boot_cmain */
	mv a0, a1

	/* Jump to C */
	tail boot_cmain

	.cfi_endproc

.end