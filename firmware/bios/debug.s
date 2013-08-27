.include "io.inc"

.text
	/*
	 * Enables the debug port.
	 * Assumes register g5 contains APB_IO_BASE.
	 * Clobbers register l0.
	 */
.global debug_enable
.type debug_enable, function
debug_enable:
	/* Set bit 0 (ENABLE) of DEBUG_R_CTL. */
	set 1, %l0
	retl
	st %l0, [%g5 + DEBUG_APB_OFFSET + DEBUG_R_CTL] /* Delay slot */



	/*
	 * Disables the debug port.
	 * Assumes register g5 contains APB_IO_BASE.
	 * Clobbers register l0 and ICC.
	 */
.global debug_disable
.type debug_disable, function
debug_disable:
	/* Wait until debug port is not busy. */
	ld [%g5 + DEBUG_APB_OFFSET + DEBUG_R_CTL], %l0
	andcc %l0, 2, %g0
	bne debug_disable
	nop /* Delay slot */

	/* Clear bit 0 (ENABLE) of DEBUG_R_CTL. */
	retl
	st %g0, [%g5 + DEBUG_APB_OFFSET + DEBUG_R_CTL] /* Delay slot */



	/*
	 * Writes the ASCIZ string pointed to by l0 to the debug port.
	 * Assumes register g5 contains APB_IO_BASE.
	 * Returns with l0 pointinng one byte past the terminating NUL of the printed string.
	 * Clobbers l3 and ICC.
	 */
.global debug_puts
.type debug_puts, function
debug_puts:
	/* Wait until the debug port is not busy. */
	ld [%g5 + DEBUG_APB_OFFSET + DEBUG_R_CTL], %l3
	andcc %l3, 2, %g0
	bne debug_puts

	/* Load the next byte and check if it’s zero. */
	ldub [%l0], %l3 /* Delay slot, useless but harmless if branch taken */
	orcc %l3, %l3, %g0
	be retl_and_nop
	add %l0, 1, %l0 /* Delay slot */

	/* Send it to the serial port and go back and do everything again. */
	st %l3, [%g5 + DEBUG_APB_OFFSET + DEBUG_R_DATA]
	b,a debug_puts
.global retl_and_nop
.type retl_and_nop, function
retl_and_nop:
	retl
	nop /* Delay slot */



	/*
	 * Writes the 8-bit value in l0 to the debug port in hex.
	 * Assumes register g5 contains APB_IO_BASE.
	 * Clobbers l0, l3, and l4 and ICC.
	 */
.global debug_putx8
.type debug_putx8, function
debug_putx8:
	/* Number of nybbles left to print. */
	set 2, %l3
	b debug_putx
	sll %l0, 24, %l0 /* Delay slot */



	/*
	 * Writes the 16-bit value in l0 to the debug port in hex.
	 * Assumes register g5 contains APB_IO_BASE.
	 * Clobbers l0, l3, and l4 and ICC.
	 */
.global debug_putx16
.type debug_putx16, function
debug_putx16:
	/* Number of nybbles left to print. */
	set 4, %l3
	b debug_putx
	sll %l0, 16, %l0 /* Delay slot */



	/*
	 * Writes the 32-bit value in l0 to the debug port in hex.
	 * Assumes register g5 contains APB_IO_BASE.
	 * Clobbers l0, l3, and l4 and ICC.
	 */
.global debug_putx32
.type debug_putx32, function
debug_putx32:
	/* Number of nybbles left to print. */
	set 8, %l3
	/* Fall through. */



	/*
	 * Writes the l3-nybble value in l0 to the debug port in hex.
	 * Assumes register g5 contains APB_IO_BASE.
	 * Clobbers l0, l3, and l4 and ICC.
	 */
debug_putx:
1:
	/* Wait until the debug port is not busy. */
	ld [%g5 + DEBUG_APB_OFFSET + DEBUG_R_CTL], %l4
	andcc %l4, 2, %g0
	bne 1b

	/* Shift next nybble into l4. */
	srl %l0, 28, %l4 /* Delay slot (useless but harmless if branch taken, as it overwrites %l4 not %l0) */

	/* Shift l0 up 4 bits. */
	sll %l0, 4, %l0

	/* Turn the nybble in l4 into an ASCII code. */
	subcc %l4, 10, %g0
	bcs,a 2f
	add %l4, 48, %l4 /* Delay slot, branch annulling, this instruction is executed only if the branch is taken, i.e. if l4 was less than 10. */
	add %l4, 65 - 10, %l4
2:

	/* Print the character, and loop back if any nybbles left to print. */
	subcc %l3, 1, %l3
	bne 1b
	st %l4, [%g5 + DEBUG_APB_OFFSET + DEBUG_R_DATA] /* Delay slot, branch not annulling, this instruction is always executed. */

	/* Done! */
	b,a retl_and_nop



.global trap_entry_debug_puts
.type trap_entry_debug_puts, function
trap_entry_debug_puts:
	/* Set up pointers to peripherals, save %o7 in %l7 so we can use the call instruction, and wait for the memory controller to be idle. */
	set APB_IO_BASE, %g5
	sethi %hi(AHB_IO_BASE), %g6
	mov %o7, %l7
	call flash_wait_idle
	or %g6, %lo(AHB_IO_BASE), %g6 /* Delay slot */

	/* Shut down the SPI memory controller and enable the debug port. */
	call debug_enable
	st %g0, [%g6 + SPIFLASH_AHB_OFFSET + SPIFLASH_R_CSR] /* Delay slot */

	/* Let the port settle. */
	call sleep_short
	nop /* Delay slot */

	/* Print the message. */
	call debug_puts
	mov %i0, %l0 /* Delay slot */

	/* Switch back to the SPI memory controller. */
	call debug_disable
	set 0x18, %l3 /* Delay slot */
	st %l3, [%g6 + SPIFLASH_AHB_OFFSET + SPIFLASH_R_CSR]

	/* Restore saved %o7. */
	mov %l7, %o7

	/* This is a syscall trap, so the caller expects the condition codes to be obliterated; we can therefore not bother fixing them before returning.
	 * Resume program execution using a standard resume-after-trap instruction sequence. */
	jmp %l2
	rett %l2 + 4
