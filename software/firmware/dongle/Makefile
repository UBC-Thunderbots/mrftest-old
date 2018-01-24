PROJECT := dongle
LIBRARIES := freertos stm32lib usb

include ../stm32lib/Makefile.common

.PHONY : dfu
dfu : $(PROJECT).bin
	@echo " DFU-UTIL  $<"
	@dfu-util -d 0483 -a "@Internal Flash  /0x08000000/04*016Kg,01*064Kg,07*128Kg" -D $< -s 0x08000000:leave
