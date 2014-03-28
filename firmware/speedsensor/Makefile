PROJECT := speedsensor
LIBRARIES := stm32lib

include ../stm32lib/Makefile.common

.PHONY : dfu
dfu : $(PROJECT).bin
	@echo " DFU-UTIL  $<"
	@dfu-util -d 0483 -c 1 -i 0 -a 0 -D $< -s 0x08000000:leave
