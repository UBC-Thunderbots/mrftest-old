.text
	/*
	 * Sleeps for a short period of time.
	 * Clobbers register l0 and ICC.
	 */
.global sleep_short
.type sleep_short, function
sleep_short:
	set 50000, %l0
	b,a sleep_impl



	/*
	 * Sleeps for approximately a second.
	 * Clobbers register l0 and ICC.
	 */
.global sleep_second
.type sleep_second, function
sleep_second:
	set 14000000, %l0
	/* Fall through. */



	/*
	 * Sleeps for the number of loop iterations specified in l0.
	 * Clobbers register l0 and ICC.
	 */
sleep_impl:
	subcc %l0, 1, %l0
	bne sleep_impl
	nop /* Delay slot */
	b,a retl_and_nop
