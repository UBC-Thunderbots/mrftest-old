.include "flash.inc"
.include "io.inc"

.text
	/*
	 * Sets up the Flash to run in ROM mode.
	 * Assumes register %g6 contains AHB_IO_BASE.
	 * Clobbers %l0, %l3, %l4, %l5, %l6, and ICC.
	 */
.global flash_set_up_rom_mode
.type flash_set_up_rom_mode, function
flash_set_up_rom_mode:
	set set_up_rom_mode_vm, %l3
	/* Fall through */



	/*
	 * Executes the sequence of Flash VM operations pointed to by %l3.
	 * Assumes register %g6 contains AHB_IO_BASE.
	 * Returns the most recently received four bytes in %l0, with the newest being in the LSB.
	 * Clobbers %l3, %l4, %l5, %l6, and ICC.
	 */
.global flash_vm_execute
.type flash_vm_execute, function
flash_vm_execute:
	/* Clear the return value accumulator. */
	mov %g0, %l0

.L_opcode_top:
	/* Load the next opcode. */
	ldub [%l3], %l4

	/* Check it for validity. */
	subcc %l4, FLASH_OP_BRANCH, %g0
	bgu trap_entry_unhandled

	/* Dispatch it. */
	sll %l4, 2, %l4 /* Delay slot (useless but harmless if branch taken) */
	set opcode_dispatch_table, %l6
	add %l6, %l4, %l4
	ld [%l4], %l4
	jmp %l4
	nop /* Delay slot */

.L_opcode_end:
	/* Return to the caller, flushing the instruction cache in case anything from Flash was in it. */
	retl
	flush

.L_opcode_wcsr:
	/* Write the next byte to the CSR. */
	ldub [%l3 + 1], %l4
	st %l4, [%g6 + SPIFLASH_AHB_OFFSET + SPIFLASH_R_CSR]

	/* Consume the two bytes and proceed with the next opcode. */
	b .L_opcode_top
	add %l3, 2, %l3 /* Delay slot */

.L_opcode_xc_lit:
	/* Byte [%l3 + 1] is length, while [%l3 + 2] through [%l3 + length + 1] inclusive are data. Grab the length and a pointer to the data. */
	ldub [%l3 + 1], %l4
	add %l3, 2, %l5

	/* Consume the bytes and go to the common transceive handling code. */
	b .L_xc_common
	add %l5, %l4, %l3 /* Delay slot */

.L_opcode_xc_buf:
	/* Byte [%l3 + 1] is length, while [%l3 + 2] through [%l3 + 5] inclusive are pointer to the data. Grab both. Do not use ld as the pointer may not be aligned. */
	ldub [%l3 + 2], %l5
	sll %l5, 24, %l5
	ldub [%l3 + 3], %l4
	sll %l4, 16, %l4
	or %l4, %l5, %l5
	ldub [%l3 + 4], %l4
	sll %l4, 8, %l4
	or %l4, %l5, %l5
	ldub [%l3 + 5], %l4
	or %l4, %l5, %l5
	ldub [%l3 + 1], %l4

	/* Consume the bytes and go to the common transceive handling code. */
	add %l3, 6, %l3
	/* Fall through. */

.L_xc_common:
	/* At this point, %l3 points to the next opcode, %l4 is the number of bytes left to transceive, and %l5 points to the next byte to send.
	 * Decrement the byte count, getting out of here if we have no more bytes left. */
	subcc %l4, 1, %l4
	bcs .L_opcode_top

	/* Fetch the next byte, advance the pointer over it, and write it to the data register. */
	ldub [%l5], %l6
	st %l6, [%g6 + SPIFLASH_AHB_OFFSET + SPIFLASH_R_DATA]
	add %l5, 1, %l5

	/* Wait until the transceiver is idle. */
1:
	ld [%g6 + SPIFLASH_AHB_OFFSET + SPIFLASH_R_CSR], %l6
	andcc %l6, 1 << 5, %g0
	bne 1b

	/* Read and accumulate the byte received during that operation, then go to next byte if any. */
	ld [%g6 + SPIFLASH_AHB_OFFSET + SPIFLASH_R_DATA], %l6 /* Delay slot (useless but harmless if branch taken) */
	sll %l0, 8, %l0
	b .L_xc_common
	or %l0, %l6, %l0 /* Delay slot */

.L_opcode_branch:
	/* Compute the conditional. */
	ldub [%l3 + 1], %l4
	ldub [%l3 + 2], %l5
	and %l0, %l4, %l4
	xorcc %l4, %l5, %g0
	bne,a 1f
	ldub [%l3 + 3], %l4 /* Delay slot, branch annulling, load first byte of target address into %l4 only if branch taken */

	/* The branch was not taken, so consume the instruction and go to top. */
	b .L_opcode_top
	add %l3, 7, %l3 /* Delay slot */

1:
	/* The branch was taken, the delay slot has copied the first byte of the target address into %l4, and we need to build the rest of the target address and execute the jump. */
	sll %l4, 24, %l4
	ldub [%l3 + 4], %l5
	sll %l5, 16, %l5
	or %l4, %l5, %l4
	ldub [%l3 + 5], %l5
	sll %l5, 8, %l5
	or %l4, %l5, %l4
	ldub [%l3 + 6], %l5
	b .L_opcode_top
	or %l4, %l5, %l3 /* Delay slot */



	/*
	 * Waits until the SPI Flash bus is idle.
	 * Assumes register %g6 contains AHB_IO_BASE.
	 * Clobbers %l0 and ICC.
	 */
.global flash_wait_idle
.type flash_wait_idle, function
flash_wait_idle:
	ld [%g6 + SPIFLASH_AHB_OFFSET + SPIFLASH_R_CSR], %l0
	andcc %l0, 0x20, %g0
	bne flash_wait_idle
	nop /* Delay slot */
	b,a retl_and_nop



.global trap_entry_flash_vm_execute
.type trap_entry_flash_vm_execute, function
trap_entry_flash_vm_execute:
	/* Enable traps for the sake of debugging. */
	rd %psr, %l0
	wr %l0, 1 << 5, %psr

	/* Set up AHB I/O pointer. */
	set AHB_IO_BASE, %g6

	/* Save %o7 in %l7 so we can use the call instruction. */
	mov %o7, %l7

	/* Wait until the SPI memory controller is idle. */
	call flash_wait_idle

	/* Copy the syscall parameter (pointer to VMOPS block) from i0 to l3. */
	mov %i0, %l3 /* Delay slot */

	/* Shut down the SPI memory controller and execute the operation. */
	call flash_vm_execute
	st %g0, [%g6 + SPIFLASH_AHB_OFFSET + SPIFLASH_R_CSR] /* Delay slot */

	/* Copy the return value into the proper place for the application to receive it, then re-enable ROM mode. */
	call flash_set_up_rom_mode
	mov %l0, %i0 /* Delay slot */

	/* Disable traps, as RETT fails if they are enabled. */
	rd %psr, %l0
	wr %l0, 1 << 5, %psr
	nop

	/* Restore saved %o7. */
	mov %l7, %o7

	/* This is a syscall trap, so the caller expects the condition codes to be obliterated; we can therefore not bother fixing them before returning.
	 * Resume program execution using a standard resume-after-trap instruction sequence. */
	jmp %l2
	rett %l2 + 4



.section .rodata, "a", @progbits
.align 4
opcode_dispatch_table:
	.word .L_opcode_end
	.word .L_opcode_wcsr
	.word .L_opcode_xc_lit
	.word .L_opcode_xc_buf
	.word .L_opcode_branch
set_up_rom_mode_vm:
	.byte FLASH_OP_WCSR, 0x03 /* ROM=0, DUAL=0, MISOOE=0, MOSIOE=1, CS=1 */
	.byte FLASH_OP_XC_LIT, 1, 0xBB /* 0xBB = fast read dual I/O */
	.byte FLASH_OP_WCSR, 0x0F /* ROM=0, DUAL=1, MISOOE=1, MOSIOE=1, CS=1 */
	.byte FLASH_OP_XC_LIT, 4, 0x00, 0x00, 0x00, 0xA0 /* address and mode byte */
	.byte FLASH_OP_WCSR, 0x00 /* ROM=0, DUAL=0, MISOOE=0, MOSIOE=0, CS=0 */
	.byte FLASH_OP_WCSR, 0x18 /* ROM=1, DUAL=1, MISOOE=0, MOSIOE=0, CS=0 */
	.byte FLASH_OP_END
