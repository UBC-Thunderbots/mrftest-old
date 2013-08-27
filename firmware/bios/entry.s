.include "io.inc"

.section .entry, "ax", @progbits
	/* Save processor state register into %g7. */
	rd %psr, %g7

	/* We get here due to either a reset (i.e. “trap zero”) or an actual trap.
	 * Registers l1 and l2 are used up by the hardware saving return PC and nPC.
	 * We use single vector trap mode, so all traps end up right here.
	 * First order of business is to figure out which trap happened and dispatch it appropriately.
	 * Do this in descending order of probability, so the more likely traps are handled faster. */
	rd %tbr, %l0
	and %l0, 0xFF0, %l0
	subcc %l0, 0x05 << 4, %g0
	be trap_entry_window_overflow
	subcc %l0, 0x06 << 4, %g0
	be trap_entry_window_underflow
	subcc %l0, 0x80 << 4, %g0
	be trap_entry_debug_puts
	subcc %l0, 0x81 << 4, %g0
	be trap_entry_flash_vm_execute
	subcc %l0, 0x00 << 4, %g0
	be trap_entry_reset
	nop /* Delay slot */
	b,a trap_entry_unhandled
