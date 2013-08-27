/* Define the memory areas implemented on the FPGA. */
MEMORY {
	spi_region (RXAI) : ORIGIN = 0x60100000, LENGTH = 1M
	ram_region (RWXA) : ORIGIN = 0x40000000, LENGTH = 16K
}

/* Lay out the data from the input object files in memory. */
SECTIONS {
	/* Put the vector table where the BIOS expects to find it. */
	.vectors 0x60100000 : {
		KEEP(*(.vectors))
	} >spi_region

	/* Put the rest of the code immediately afterwards. */
	.text : {
		*(.text .text.*)
	} >spi_region

	/* Put read-only data in ROM along with the program code. */
	.rodata : {
		*(.rodata .rodata.*)
	} >spi_region

	/* Add the initialized data, storing a physical copy in ROM but allocating it in RAM. */
	.data : ALIGN(8) {
		*(.data .data.*)
	} >ram_region AT>spi_region

	/* Put the zero-initialized data in RAM only. */
	.bss : {
		*(.bss .bss.*)
	} >ram_region AT>ram_region
}

/* Provide symbols that the vector table can use to find things needed to initialize .data and wipe .bss. */
linker_data_vma_start = ADDR(.data);
linker_data_vma_end = ADDR(.data) + SIZEOF(.data);
linker_data_lma_start = LOADADDR(.data);
linker_bss_vma_start = ADDR(.bss);
linker_bss_vma_end = ADDR(.bss) + SIZEOF(.bss);
linker_heap_end = ORIGIN(ram_region) + LENGTH(ram_region) - 4K; /* Reserve the last 4K for stack. */
