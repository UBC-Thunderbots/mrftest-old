.include "config.inc"
.include "flash.inc"
.include "io.inc"

.text
	/* The trap entry point for system reset traps. */
.global trap_entry_reset
.type trap_entry_reset, function
trap_entry_reset:
	/* Set up pointers to peripherals, enable the debug port, and let the port settle. */
	set APB_IO_BASE, %g5
	call debug_enable
	sethi %hi(AHB_IO_BASE), %g6 /* Delay slot */
	call sleep_short
	or %g6, %lo(AHB_IO_BASE), %g6 /* Delay slot */

	/* Print the startup message. */
	sethi %hi(init_message), %l0
	call debug_puts
	or %l0, %lo(init_message), %l0 /* Delay slot */

	/* Try to enable single vector trap mode. */
	set 1 << 13, %l3
	wr %l3, %asr17
	nop /* For up to three instructions following a write special register, the effects may or may not apply. */
	nop
	nop
	rd %asr17, %l0
	andcc %l0, %l3, %g0
	sethi %hi(svt_missing_message), %l4
	be boot_error
	or %l4, %lo(svt_missing_message), %l4 /* Delay slot */

	/* Check that the CPU has the expected number of register windows. */
	wr %g0, -1, %wim
	nop /* For up to three instructions following a write special register, the effects may or may not apply. */
	nop
	nop
	rd %wim, %l0
	subcc %l0, (1 << NUM_WINDOWS) - 1, %g0
	sethi %hi(wrong_num_windows_message), %l4
	bne boot_error
	or %l4, %lo(wrong_num_windows_message), %l4 /* Delay slot */

	/* Flush and enable the instruction cache. */
	flush
	set 0x00010003, %l0 /* DS=0, FD=0, FI=0, ST=0, IB=1, IP=0, DP=0, DF=0, IF=0, DCS=00, ICS=11 */
	sta %l0, [%g0] 2

	/* CPU looks OK. */
	sethi %hi(cpu_ok_message), %l0
	call debug_puts
	or %l0, %lo(cpu_ok_message), %l0 /* Delay slot */

	/* Turn off the serial port. */
	call debug_disable

	/* Issue the SPI commands to reset out of continuous mode and read the chip’s JEDEC ID. */
	sethi %hi(reset_and_read_jedec_id_vm), %l3 /* Delay slot */
	call flash_vm_execute
	or %l3, %lo(reset_and_read_jedec_id_vm), %l3 /* Delay slot */
	set 0xFFFFFF, %l3
	and %l0, %l3, %l0

	/* Check if the Flash ID is right. */
	set 0xEF4015, %l3
	subcc %l0, %l3, %g0
	sethi %hi(wrong_flash_id_message), %l4
	bne boot_error
	or %l4, %lo(wrong_flash_id_message), %l4 /* Delay slot */

	/* Issue the SPI commands to start a continuous mode dual I/O read and enable ROM mode in the controller. */
	call flash_set_up_rom_mode

	/* Check for a proper signature on the vector table.
	 * Use a global register (%g1) for the vector table pointer as we will be messing with CWP below and thus destroying all our locals. */
	set SPI_FLASH_BASE + 0x100000, %g1 /* Delay slot */
	ld [%g1], %l0
	set 0x55AA9966, %l2
	subcc %l0, %l2, %g0
	sethi %hi(vector_table_signature_missing_message), %l4
	bne boot_error
	or %l4, %lo(vector_table_signature_missing_message), %l4 /* Delay slot */

	/* Layout of the vector table is as follows:
	 * [0]  signature
	 * [4]  entry point
	 * [8]  data VMA start
	 * [12] data VMA end
	 * [16] data LMA start
	 * [20] bss VMA start
	 * [24] bss VMA end
	 *
	 * Now copy the data section from its LMA to its VMA.
	 * The number of bytes copied is rounded up to a multiple of eight, but this doesn’t matter as we will overwrite extra data in the BSS clearing stage below. */
	ldd [%g1 + 8], %l2
	subcc %l3, %l2, %l3
	be 2f
	ld [%g1 + 16], %l4 /* Delay slot */
	mov %g0, %l0
1:
	ldd [%l4 + %l0], %l6
	std %l6, [%l2 + %l0]
	subcc %l3, 8, %l3
	bgu 1b
	add %l0, 8, %l0 /* Delay slot */
2:

	/* Scrub the bss section with zeroes. */
	ld [%g1 + 20], %l2
	ld [%g1 + 24], %l3
1:
	subcc %l3, %l2, %g0
	bleu 2f
	nop /* Delay slot */
	st %g0, [%l2]
	b 1b
	add %l2, 4, %l2 /* Delay slot */
2:

	/* Initialize the window invalid mask to have window zero be invalid. */
	wr %g0, 1, %wim

	/* Initialize the processor state register.
	 * We will set CWP=0 even though window 0 is invalid.
	 * This is acceptable as traps are not issued due to *having* and invalid window, only due to *entering* an invalid window by SAVE or RESTORE.
	 * The first thing the compiler does at the top of main() is a SAVE, which brings us into the first valid window.
	 * Sure, if main() ever returns, then the RESTORE at the end will take a window underflow trap.
	 * Nobody cares, because main() should never return. */
	/* EC=0, EF=0, PIL=0, S=0, PS=0, ET=1, CWP=0 equals 1 << 5 */
	wr %g0, 1 << 5, %psr
	nop /* For up to three instructions following a write special register, the effects may or may not apply. */
	nop

	/* Initialize the stack and frame pointers. */
	sethi %hi(SYSTEM_RAM_BASE + RAM_SIZE), %g2 /* Still within the three instructions suffering from indeterminate PSR, but only affecting a global %g2 so OK. */
	or %g2, %lo(SYSTEM_RAM_BASE + RAM_SIZE), %sp /* No longer within the three instructions, so writing to %sp (which is CWP-dependent) is OK. */
	mov %g0, %fp

	/* Zero all the OUT registers, which will become the IN registers in main. */
	clr %o0
	clr %o1
	clr %o2
	clr %o3
	clr %o4
	clr %o5
	/* Not %o6; that’s the stack pointer. */
	/* %o7 is done below to fill a delay slot. */

	/* Jump to the entry point. */
	ld [%g1 + 4], %l0
	jmp %l0
	clr %o7 /* Delay slot */



boot_error:
	/* Disable the SPI Flash and enable the debug port. */
	call debug_enable
	st %g0, [%g6 + SPIFLASH_AHB_OFFSET + SPIFLASH_R_CSR] /* Delay slot */

	/* Print the failure message once per second forever. */
1:
	call sleep_second
	nop /* Delay slot */
	call debug_puts
	mov %l4, %l0 /* Delay slot, restore l0 for printing */
	b,a 1b



.section .rodata, "a", @progbits
reset_and_read_jedec_id_vm:
	.byte FLASH_OP_WCSR, 0x03 /* ROM=0, DUAL=0, MISOOE=0, MOSIOE=1, CS=1 */
	.byte FLASH_OP_XC_LIT, 2, 0xFF, 0xFF /* 0xFFFF = reset continuous mode */
	.byte FLASH_OP_WCSR, 0x00 /* ROM=0, DUAL=0, MISOOE=0, MOSIOE=0, CS=0 */
	.byte FLASH_OP_WCSR, 0x03 /* ROM=0, DUAL=0, MISOOE=0, MOSIOE=1, CS=1 */
	.byte FLASH_OP_XC_LIT, 4, 0x9F, 0x00, 0x00, 0x00 /* 0x9F = read JEDEC ID */
	.byte FLASH_OP_WCSR, 0x00 /* ROM=0, DUAL=0, MISOOE=0, MOSIOE=0, CS=0 */
	.byte FLASH_OP_END
init_message:
	.ascii "BIOS INIT"
	.byte 13,10,0
svt_missing_message:
	.ascii "SVT missing"
	.byte 13,10,0
wrong_num_windows_message:
	.ascii "Wrong window count"
	.byte 13,10,0
cpu_ok_message:
	.ascii "CPU OK" /* Fall through */
.global eol
.type eol, object
eol:
	.byte 13,10,0
wrong_flash_id_message:
	.ascii "Wrong JEDEC ID"
	.byte 13,10,0
vector_table_signature_missing_message:
	.ascii "Wrong vector table signature"
	.byte 13,10,0
