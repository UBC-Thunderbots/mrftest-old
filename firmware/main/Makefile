PROJECT := main
LIBRARIES := cdcacm freertos stm32lib usb

include ../stm32lib/Makefile.common

$(PROJECT).dfu : $(PROJECT).bin
	@echo "DFU-SUFFIX $@"
	@cp $< $@
	@dfu-suffix -v 0483 -p 497e -a $@

.PHONY : dfu
dfu : $(PROJECT).dfu
	@echo " DFU-UTIL  $<"
	@dfu-util -a Firmware -D $<

.PHONY : dfuse
dfuse : $(PROJECT).bin
	@echo " DFU-UTIL  $<"
	@dfu-util -d 0483 -a "@Internal Flash  /0x08000000/04*016Kg,01*064Kg,07*128Kg" -D $< -s 0x08000000:leave
