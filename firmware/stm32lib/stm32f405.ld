/* Define the three memory areas implemented by the STM32F4 series */
MEMORY {
	flash_region (RXAI) : ORIGIN = 0x08000000, LENGTH = 1M
	sram_region (RWXA) : ORIGIN = 0x20000000, LENGTH = 128K
	ccm_region (RWXA) : ORIGIN = 0x10000000, LENGTH = 64K
}

/* Lay out the data from the input object files in memory */
SECTIONS {
	/* The CPU always expects the exception vectors to be at the bottom of memory */
	.exception_vectors 0x08000000 : {
		KEEP(*(.exception_vectors))
	} >flash_region

	/* The CPU expects the interrupt vectors to be positioned directly after the exception vectors */
	.interrupt_vectors 0x08000040 : {
		KEEP(*(.interrupt_vectors))
	} >flash_region

	/* Add program code to the Flash following the vectors */
	.text : {
		*(.text .text.*)
	} >flash_region

	/* Read-only data can also live entirely in Flash */
	.rodata : {
		*(.rodata .rodata.*)
	} >flash_region

	/* Read-write non-zero-initialized data must live in RAM but a copy be stored in Flash for initialization */
	.data : {
		*(.data .data.*)
	} >sram_region AT>flash_region

	/* Allocate the last 256 kiB (two sectors) of Flash for a core dump */
	.coredump 0x080C0000 (NOLOAD) : {
		*(.coredump)
	} >flash_region

	/* Zero-initialized data can live entirely in SRAM and be wiped on boot */
	.bss : {
		*(.bss .bss.*)
	} >sram_region AT>sram_region

	/* The stack can go in core-coupled memory for speed */
	.stack (NOLOAD) : {
		*(.pstack)
		*(.mstack)
	} >ccm_region AT>ccm_region
}

/* Provide symbols that the startup code uses to find the various sections in memory to initialize the data and wipe the bss */
linker_data_vma_start = ADDR(.data);
linker_data_vma_end = ADDR(.data) + SIZEOF(.data);
linker_data_lma_start = LOADADDR(.data);
linker_bss_vma_start = ADDR(.bss);
linker_bss_vma_end = ADDR(.bss) + SIZEOF(.bss);
