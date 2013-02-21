ABI := arm-linux-gnueabi
CC := $(ABI)-gcc
LD := $(ABI)-ld
OBJCOPY := $(ABI)-objcopy
PROJ := dongle
override CFLAGS := $(CFLAGS) -Wall -Wextra -std=gnu99 -O2 -mlittle-endian -mthumb -mcpu=cortex-m3 -nostdinc -nostdlib -g -fno-common -ffunction-sections -fdata-sections -isystem ../stm32lib/include
override LDFLAGS := $(LDFLAGS) -Tstm32f405.ld --gc-sections
override LDLIBS := $(LDLIBS) ../stm32lib/stm32lib.a

FW_SOURCES := $(wildcard *.c)
FW_HEADERS := $(wildcard *.h ../stm32lib/include/*.h)
FW_OBJS := $(patsubst %.c,%.o,$(FW_SOURCES))

$(PROJ).bin : $(PROJ).elf
	@echo " OBJCOPY   $@"
	@$(OBJCOPY) -O binary $+ $@

$(PROJ).elf : $(FW_OBJS) ../stm32lib/stm32lib.a stm32f405.ld
	@echo " LD        $@"
	@$(LD) $(LDFLAGS) -o$@ -Map=$(PROJ).map $(FW_OBJS) $(LDLIBS)

$(FW_OBJS) : $(FW_HEADERS) Makefile

$(FW_OBJS) : %.o : %.c
	@echo " CC        $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

../stm32lib/stm32lib.a : $(wildcard ../stm32lib/*.o ../stm32lib/*.c ../stm32lib/*.h ../stm32lib/include/*.h)
	$(MAKE) -C ../stm32lib

.PHONY : clean
clean :
	$(RM) $(PROJ).bin $(PROJ).elf $(PROJ).map *.o
	$(RM) -r html

.PHONY : dfu
dfu : $(PROJ).bin
	@echo " DFU-UTIL  $<"
	@dfu-util -d 0483 -c 1 -i 0 -a 0 -D $< -s 0x08000000:leave

.PHONY : doc
doc :
	doxygen
