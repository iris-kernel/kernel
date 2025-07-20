.section .init

.option norvc

.type start, @function
.global start
start:
	.cfi_startproc

.option push
.option norelax
	la gp, global_pointer
.option pop

	/* Reset satp */
	csrw satp, zero

	/* Setup stack */
	la sp, stack_top

	/* Clear the BSS section */
	la t5, bss_start
	la t6, bss_end
bss_clear:
	sd zero, (t5)
	addi t5, t5, 8
	bltu t5, t6, bss_clear

	/* Move DTB pointer from a1 to a0 for boot_cmain */
	mv a0, a1

	/* Jump to C */
	la t0, boot_cmain
	csrw mepc, t0
	tail boot_cmain

	.cfi_endproc

.end
