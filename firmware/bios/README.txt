This is the BIOS. It lives in a ROM attached to the AHB, and is both the entry point for system powerup and also the trap handler (in single vector trap mode) for hardware exceptions and system calls. The BIOS has three primary jobs: to initialize the system enough to launch the main application firmware, to catch fatal application errors and display debug output, and to provide a few services for the application firmware.

System initialization is fairly straightforward. The BIOS checks some registers to ensure the system looks like what it expects to see—for example, it checks that the number of register windows in the CPU agrees with the number expected by the window overflow and underflow trap handlers. Assuming everything checks out, it initializes the SPI Flash memory controller, copies the .data section’s image from SPI Flash to system RAM, sets up an initial register environment (in particular an initial stack pointer), and transfers control to the application firmware, which now finds itself in an environment conducive to running pure C code.

Simple fatal error handling is implemented. If the application encounters a fatal error (e.g. dereferences a null pointer, executes an illegal instruction, etc.), the BIOS will safe the robot hardware and then display some basic debugging information on the serial port. In case no serial receiver is attached when the error occurs, the BIOS will repeat the information every few seconds.

Finally, a few services are implemented in the BIOS, because they can’t be implemented in application firmware. One such service is the window overflow and underflow trap handlers: these must be implemented in supervisor-mode code as trap handlers; while the BIOS could arrange to let the application firmware handle these, there’s no real benefit and it would require a handoff mechanism to redirect trap handling. The other services provided by the BIOS are those that can’t be implemented in application firmware because they use the SPI Flash bus, which can’t be touched while application firmware is running from it; these services are writing to the Flash memory and sending data to the debug serial port.

The BIOS does not follow the same rules as normal application firmware with respect to register usage and function calling conventions. The BIOS is always entered either from system reset or from a trap, which means it must follow suitable rules for handling traps. Specifically:
- Registers i0 through i7 belong to the application firmware that was just running (as its o0 through o7 registers) and must be preserved for non-system-call traps. System calls may modify them.
- Registers l1 and l2 contain the return PC and nPC and must be preserved for all traps.
- Registers o0 through o7 belong to the next register window which, if the trap is being taken in the invalid window, belongs to application firmware NWINDOWS-1 up the call stack and must therefore be preserved.
- The PSR must be preserved.
- Functions must all be written as if leaf; no additional windows can be allocated. Return addresses must be saved into additional registers to prevent clobbering.
- Registers g5 through g7 are free to use.

Given these rules, only registers g5 through g7, l0, and l3 through l7 are freely usable by all BIOS code, with i0 through i7 additionally usable in system calls. Among other things, this means the CALL instruction cannot be used unless o7 is protected from clobbering.

On entry to the BIOS, the PSR is saved into g7 so the condition codes can be destroyed by the trap dispatcher and fixed later.
Registers g5 and g6 are available for individual trap handlers to use as needed.
Many utility routines assume g5 is set to APB_IO_BASE and g6 to AHB_IO_BASE.
Trap handlers that need to call other functions but expect to return to the application typically save o7 in l7 and restore it immediately before returning.
