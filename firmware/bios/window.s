.include "config.inc"
.include "io.inc"

.text
.global trap_entry_window_overflow
.type trap_entry_window_overflow, function
trap_entry_window_overflow:
	/* Point to the window *after* (one less than) the trap (current) window; this window was previously the oldest valid window in the application. */
	save

	/* Check that the process’s stack pointer is within bounds.
	 * We must do this without touching any of the local registers.
	 * Use g5 for our calculations. */
	set SYSTEM_RAM_BASE + RAM_SIZE - STACK_SIZE, %g5
	subcc %sp, %g5, %g5
	bcs trap_entry_unhandled
	subcc %g5, STACK_SIZE - 64, %g0 /* Delay slot (useless but harmless if branch taken) */
	bgu trap_entry_unhandled

	/* In (%g5 XOR %g6), compute a new window invalid mask (the original mask rotated 1 bit right). */
	rd %wim, %g6 /* Delay slot (useless but harmless if branch taken) */
	srl %g6, 1, %g5
	sll %g6, NUM_WINDOWS - 1, %g6

	/* Store the local and input registers in this window to the stack at this window’s stack pointer. */
	std %l0, [%sp]
	std %l2, [%sp + 8]
	std %l4, [%sp + 16]
	std %l6, [%sp + 24]
	std %i0, [%sp + 32]
	std %i2, [%sp + 40]
	std %i4, [%sp + 48]
	std %i6, [%sp + 56]

	/* Done, except for final fixups and trap return. */
	b,a common_fix_psr_and_wim_and_return



.global trap_entry_window_underflow
.type trap_entry_window_underflow, function
trap_entry_window_underflow:
	/* Copy the current value of WIM to %g6. */
	rd %wim, %g6

	/* Make all windows valid so we can screw around with them. */
	wr %g0, %wim

	/* For up to three instructions following a write special register instruction, the effects may or may not apply.
	 * The next three instructions are arithmetic/logic instructions which are not affected by WIM, so there’s no need for NOPs. */

	/* In %g6, compute a new window invalid mask (the original mask rotated 1 bit left). */
	sll %g6, 1, %g5
	srl %g6, NUM_WINDOWS - 1, %g6
	or %g6, %g5, %g6

	/* Point to the window before the window that was active in the application when the trap was taken, which is itself one window before the trap (current) window. */
	restore
	restore

	/* Check that the process’s stack pointer is within bounds.
	 * We must do this without touching any of the local registers.
	 * Use g5 for our calculations. */
	set SYSTEM_RAM_BASE + RAM_SIZE - STACK_SIZE, %g5
	subcc %sp, %g5, %g5
	bcs trap_entry_unhandled
	subcc %g5, STACK_SIZE - 64, %g0 /* Delay slot (useless but harmless if branch taken) */
	bgu trap_entry_unhandled

	/* Set %g5 to zero so that (%g5 XOR %g6) is the proper new WIM value. */
	mov %g0, %g5 /* Delay slot (useless but harmless if branch taken) */

	/* Load the local and input registers in this window from the stack at this window’s stack pointer. */
	ldd [%sp], %l0
	ldd [%sp + 8], %l2
	ldd [%sp + 16], %l4
	ldd [%sp + 24], %l6
	ldd [%sp + 32], %i0
	ldd [%sp + 40], %i2
	ldd [%sp + 48], %i4
	ldd [%sp + 56], %i6

	/* Fall through. */



common_fix_psr_and_wim_and_return:
	/* Restore the old PSR, as the trap entry code destroyed the ICCs, and also go back to the trap window. */
	wr %g7, %psr

	/* Apply the new WIM value we calculated earlier, in (%g5 XOR %g6). */
	wr %g5, %g6, %wim

	/* For up to three instructions following a write special register instruction, the effects may or may not apply.
	 * The writes to PSR and WIM are completely independent, so no nops are needed between them.
	 * The JMP and RETT both depend on the write to PSR, because they read local registers and thus read CWP.
	 * The RETT also reads WIM and writes CWP.
	 * Thus, the JMP needs to be three instructions after the write to PSR, and the RETT needs to be three instructions after the write to WIM. */
	nop
	nop

	/* Resume program execution using a standard resume-from-trap instruction sequence. */
	jmp %l1
	rett %l2
