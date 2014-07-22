PROJECT := main
LIBRARIES := cdcacm freertos stm32lib usb

include ../stm32lib/Makefile.common


.PHONY : dfu
dfu : $(PROJECT).bin
	@echo " DFU-UTIL  $<"
	@dfu-util -d 0483 -c 1 -i 0 -a 0 -D $< -s 0x08000000:leave

bitstream.o: ../../vhdl/thunderbots.bin
