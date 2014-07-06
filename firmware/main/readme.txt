How to program to Brazil boards

1. Once finish modifying vhdl files, do "make synth" in vhdl/ directory
2. Go to firmware/main/ directory, do "touch bitstream.c". This modifies the last time the files is modified and thus prompt the make script to update the fpga bitstream
3. In the same directory, do "make". This compiles C code
4. Program the board through the STM dfu port. 
	To do this, the stm32f4 microcontroller needs to be booted in bootloader mode. Follow these steps: 
	a. place the power switch to OFF position 
	b. press down the piano DIP switch that is labelled BL 
	c. hold the power switch to START position (the temporary position) 
	d. "make dfu" in the same directory while holding the switch in START position. 
	e. turn off the board, undo the piano DIP switch and power on the board
	f. the udev rules needs to be set properly for this to work, if it doesn't ask some one for help
