MEMORY {
	flash_region (RXAI) : ORIGIN = 0x08000000, LENGTH = 1M
	sram_region (RWXA) : ORIGIN = 0x20000000, LENGTH = 128K
	ccm_region (RWXA) : ORIGIN = 0x10000000, LENGTH = 64K
}

SECTIONS {
	.exception_vectors : {
		KEEP(*(.exception_vectors))
	} >flash_region

	.interrupt_vectors : {
		KEEP(*(.interrupt_vectors))
	} >flash_region

	.text : {
		*(.text)
	} >flash_region

	.rodata : {
		*(.rodata)
	} >flash_region

	.data : {
		*(.data)
	} >sram_region AT>flash_region

	.bss : {
		*(.bss)
	} >sram_region AT>sram_region

	.stack (NOLOAD) : {
		*(.stack)
	} >ccm_region AT>ccm_region
}

linker_data_vma_start = ADDR(.data);
linker_data_vma_end = ADDR(.data) + SIZEOF(.data);
linker_data_lma_start = LOADADDR(.data);
linker_bss_vma_start = ADDR(.bss);
linker_bss_vma_end = ADDR(.bss) + SIZEOF(.bss);
