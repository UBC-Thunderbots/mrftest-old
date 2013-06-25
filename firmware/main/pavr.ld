/* Define the memory areas implemented on the FPGA */
MEMORY {
	rom_region (RXAI) : ORIGIN = 0x000000, LENGTH = 32K
	ram_region (RWXA) : ORIGIN = 0x800060, LENGTH = 7K
}

/* Lay out the data from the input object files in memory */
SECTIONS {
	/* The CPU always starts executing code from address zero, so put the entry point there */
	.entry 0x000000 : {
		KEEP(*(.entry))
	} >rom_region

	/* Put the rest of the code immediately afterwards in ROM */
	.text : {
		*(.trampolines)
		*(.text .text.*)
		*(.progmem .progmem.*)
	} >rom_region

	/* Data lives in RAM; no initialization is needed for initialized data because RAM is preloaded in the bitstream */
	.data : {
		*(.rodata .rodata.*)
		*(.data .data.*)
		*(.bss .bss.*)
	} >ram_region
}
