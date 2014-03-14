PROJECT := flashburner
USE_STM32LIB := 1
USE_FREERTOS := 0
USE_USB := 0

include ../stm32lib/Makefile.common

.PHONY : dfu
dfu : $(PROJECT).bin
	@echo " DFU-UTIL  $<"
	@dfu-util -d 0483 -c 1 -i 0 -a 0 -D $< -s 0x08000000:leave
