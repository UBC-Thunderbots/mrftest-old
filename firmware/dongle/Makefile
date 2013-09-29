PROJECT := dongle

include ../stm32lib/Makefile.common

.PHONY : dfu
dfu : $(PROJ).bin
	@echo " DFU-UTIL  $<"
	@dfu-util -d 0483 -c 1 -i 0 -a 0 -D $< -s 0x08000000:leave
