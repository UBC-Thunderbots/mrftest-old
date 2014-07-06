void fpga_bitstream_data(void) {
	asm volatile(".global fpga_bitstream_data_start\n\t.type fpga_bitstream_data_start, object\nfpga_bitstream_data_start:\n\t.incbin \"../../vhdl/thunderbots.bin\"\n\t.global fpga_bitstream_data_end\n\t.type fpga_bitstream_data_end, object\nfpga_bitstream_data_end:\n");
}

