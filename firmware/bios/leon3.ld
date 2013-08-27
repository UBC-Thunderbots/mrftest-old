/* Define the memory areas implemented on the FPGA */
MEMORY {
	bios_region (RXA)       : ORIGIN = 0x20000000, LENGTH = 2K
}

/* Lay out the data from the input object files in memory */
SECTIONS {
	/* The CPU always starts executing code from address 0x10000000, so put the entry point there */
	.entry 0x20000000 : {
		KEEP(*(.entry))
	} >bios_region

	/* Put the rest of the code immediately afterwards */
	.text : {
		*(.text .text.*)
	} >bios_region

	/* Add the read-only data */
	.rodata : {
		*(.rodata .rodata.*)
	} >bios_region
}

