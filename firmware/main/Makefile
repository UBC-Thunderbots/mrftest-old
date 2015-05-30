PROJECT := main
LIBRARIES := cdcacm freertos stm32lib usb

include ../stm32lib/Makefile.common

$(PROJECT).dfu : $(PROJECT).bin
	@echo "DFU-SUFFIX $@"
	@cp $< $@
	@dfu-suffix -v 0483 -p 497f -a $@

.PHONY : dfu
dfu : $(PROJECT).dfu
	@echo " DFU-UTIL  $<"
	@dfu-util -d 0483 -c 1 -i 0 -a 0 -D $<

.PHONY : dfuse
dfuse : $(PROJECT).bin
	@echo " DFU-UTIL  $<"
	@dfu-util -d 0483 -c 1 -i 0 -a 0 -D $< -s 0x08000000:leave
