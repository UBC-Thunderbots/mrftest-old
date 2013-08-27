.include "config.inc"
.include "io.inc"

/* Allocate space for all 8 locals and all 8 ins for each window. */
.equiv WINDOW_STASH_BUFFER, (SYSTEM_RAM_BASE + RAM_SIZE - 16 * 4 * NUM_WINDOWS)
/* Allocate space for PSR, WIM, TBR, Y, PC, nPC, %g1, %g2, %g3, %g4. */
.equiv GLOBAL_STASH_BUFFER, (WINDOW_STASH_BUFFER - 10 * 4)

.text
.global trap_entry_unhandled
.type trap_entry_unhandled, function
trap_entry_unhandled:
	/* Set up pointers to peripherals. */
	set APB_IO_BASE, %g5
	set AHB_IO_BASE, %g6

	/* Set SYSCTL: Test LEDs = 111, radio LED = 0, software interlocks = 1, motor power = 0, logic power = 1 */
	set (7 << 6) | (1 << 3) | (1 << 0), %l0
	st %l0, [%g5 + SYSCTL_APB_OFFSET + SYSCTL_R_SYSCTL]

	/* Discharge capacitors. */
	set 1 << 1, %l0
	st %l0, [%g5 + CHICKER_APB_OFFSET + CHICKER_R_CSR]

	/* Turn off SPI ROM mode. */
	st %g0, [%g6 + SPIFLASH_AHB_OFFSET + SPIFLASH_R_CSR]

	/* Enable debug output. */
	call debug_enable

	/* Save globals into the global stash buffer. */
	set GLOBAL_STASH_BUFFER, %l3 /* Delay slot */
	st %g7, [%l3]
	rd %wim, %l0
	st %l0, [%l3 + 4]
	rd %tbr, %l0
	st %l0, [%l3 + 8]
	rd %y, %l0
	st %l0, [%l3 + 12]
	st %l1, [%l3 + 16]
	st %l2, [%l3 + 20]
	st %g1, [%l3 + 24]
	st %g2, [%l3 + 28]
	st %g3, [%l3 + 32]
	st %g4, [%l3 + 36]

	/* Iterate the windows and save window registers into window stash buffer. */
	wr %g0, 1 << 7, %psr
	wr %g0, %wim
	set NUM_WINDOWS, %g1
	set WINDOW_STASH_BUFFER, %g2
1:
	std %l0, [%g2]
	std %l2, [%g2 + 8]
	std %l4, [%g2 + 16]
	std %l6, [%g2 + 24]
	std %i0, [%g2 + 32]
	std %i2, [%g2 + 40]
	std %i4, [%g2 + 48]
	std %i6, [%g2 + 56]
	subcc %g1, 1, %g1
	add %g2, 64, %g2
	bne,a 1b
	restore /* Delay slot, branch annulling, this instruction is executed only if the branch is taken. */

.L_print_loop:
	/* Wait a second. */
	call sleep_second

	/* Print the top message. */
	sethi %hi(top_message), %l1 /* Delay slot, note using %l1 temporarily here as %l0 is destroyed by sleep_second */
	call debug_puts
	or %l1, %lo(top_message), %l0 /* Delay slot */

	/* Blink the LEDs. */
	ld [%g5 + SYSCTL_APB_OFFSET + SYSCTL_R_SYSCTL], %l0
	xor %l0, (7 << 6), %l0
	st %l0, [%g5 + SYSCTL_APB_OFFSET + SYSCTL_R_SYSCTL]

	/* Print the globals. */
	mov 10, %o0 /* Delay slot */
	set global_names, %o1
	set GLOBAL_STASH_BUFFER, %o2
	call print_registers
	mov 5, %o3 /* Delay slot */

	/* Print the windows. */
	mov %g0, %g4 /* Window number */
	/* %o0 and %o1 will be set for each window. */
	/* %o2 is already pointing in the right place, as the window stash buffer immediately follows the global stash buffer. */
	mov 8, %o3
1:
	sethi %hi(window), %l0
	call debug_puts
	or %l0, %lo(window), %l0 /* Delay slot */

	call debug_putx8
	mov %g4, %l0 /* Delay slot */

	sethi %hi(eol), %l0
	call debug_puts
	or %l0, %lo(eol), %l0 /* Delay slot */

	set local_names, %o1
	call print_registers
	mov 16, %o0 /* Delay slot */

	add %g4, 1, %g4
	subcc %g4, NUM_WINDOWS, %g0
	bne 1b
	nop /* Delay slot */

	/* Do it forever, in case a serial port isn’t initially attached to the system. */
	b,a .L_print_loop



	/*
	 * Prints a set of registers.
	 * Parameters:
	 *  %o0 - number of registers to print
	 *  %o1 - pointer to names of registers, 4 bytes each
	 *  %o2 - pointer to values of registers, 4 bytes each
	 *  %o3 - number of registers after which to print a newline
	 *  %o7 - return address
	 *
	 * On return:
	 *  %o0 - zero
	 *  %o1 - points past the last register name
	 *  %o2 - points past the last register value
	 */
print_registers:
	/* Save context (we will call subfunctions, and this is the easiest way to protect registers). */
	save

	/* %l1 will count registers until a newline is needed. */
	mov %g0, %l1

2:
	/* Print the name of the register. */
	call debug_puts
	mov %i1, %l0 /* Delay slot */

	/* Print a colon and a space. */
	sethi %hi(colon_space), %l0
	call debug_puts
	or %l0, %lo(colon_space), %l0 /* Delay slot */

	/* Print the value of the register. */
	call debug_putx32
	ld [%i2], %l0 /* Delay slot */

	/* Advance the register-count-mod-newline-count and print either three spaces or a newline appropriately. */
	add %l1, 1, %l1
	subcc %l1, %i3, %g0
	sethi %hi(eol_or_three_spaces + 3), %l0
	bne 1f
	or %l0, %lo(eol_or_three_spaces + 3), %l0 /* Delay slot */
	sub %l0, 3, %l0 /* Move from the three spaces to the EOL */
	mov %g0, %l1 /* Reset the counter */
1:
	call debug_puts
	add %i2, 4, %i2 /* Advance the value pointer, delay slot */

	/* Decrement the register count and go back to print another register if appropriate. */
	subcc %i0, 1, %i0
	bne,a 2b
	add %i1, 4, %i1 /* Advance the register name pointer, delay slot, branch annulling, instruction executed if branch taken. */

	/* Done. */
	ret
	restore



.section .rodata, "a", @progbits
top_message:
	.byte 13, 10, 13, 10, 13, 10
	.ascii "TRAP" /* Fall through */
eol_or_three_spaces:
	.byte 13, 10, 0
	.ascii "   "
	.byte 0

colon_space:
	.ascii ": "
	.byte 0

window:
	.byte 13, 10
	.ascii "W "
	.byte 0

global_names:
	.ascii "PSR"
	.byte 0
	.ascii "WIM"
	.byte 0
	.ascii "TBR"
	.byte 0
	.ascii "  Y"
	.byte 0
	.ascii " PC"
	.byte 0
	.ascii "nPC"
	.byte 0
	.ascii "%g1"
	.byte 0
	.ascii "%g2"
	.byte 0
	.ascii "%g3"
	.byte 0
	.ascii "%g4"
	.byte 0

local_names:
	.ascii "%l0"
	.byte 0
	.ascii "%l1"
	.byte 0
	.ascii "%l2"
	.byte 0
	.ascii "%l3"
	.byte 0
	.ascii "%l4"
	.byte 0
	.ascii "%l5"
	.byte 0
	.ascii "%l6"
	.byte 0
	.ascii "%l7"
	.byte 0
	.ascii "%i0"
	.byte 0
	.ascii "%i1"
	.byte 0
	.ascii "%i2"
	.byte 0
	.ascii "%i3"
	.byte 0
	.ascii "%i4"
	.byte 0
	.ascii "%i5"
	.byte 0
	.ascii "%i6"
	.byte 0
	.ascii "%i7"
	.byte 0
