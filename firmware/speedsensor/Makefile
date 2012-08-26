HOSTCC := $(CC)
ABI := arm-linux-gnueabi
CC := $(ABI)-gcc
LD := $(ABI)-ld
OBJCOPY := $(ABI)-objcopy
PROJ := dongle
override HOSTCFLAGS := $(CFLAGS) -Wall -Wextra -std=gnu99 -O2 -march=native -g
override CFLAGS := $(CFLAGS) -Wall -Wextra -std=gnu99 -O2 -mlittle-endian -mthumb -mcpu=cortex-m3 -nostdinc -nostdlib -g
override LDFLAGS := $(LDFLAGS) -Tstm32f405.ld --gc-sections

FW_SOURCES := $(filter-out enter-dfu-mode.c exit-dfu-mode.c,$(wildcard *.c))
FW_HEADERS := $(wildcard *.h)
FW_OBJS := $(patsubst %.c,%.o,$(FW_SOURCES))

$(PROJ).bin : $(PROJ).elf
	@echo " OBJCOPY   $@"
	@$(OBJCOPY) -O binary $+ $@

$(PROJ).elf : $(FW_OBJS) stm32f405.ld
	@echo " LD        $@"
	@$(LD) $(LDFLAGS) -o$@ -Map=$(PROJ).map $(FW_OBJS) $(LDLIBS)

$(FW_OBJS) : $(FW_HEADERS) Makefile

$(FW_OBJS) : %.o : %.c
	@echo " CC        $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

enter-dfu-mode exit-dfu-mode : % : %.c
	@echo " HOSTCC    $@"
	@$(HOSTCC) $(CPPFLAGS) $(HOSTCFLAGS) $(shell pkg-config --cflags --libs-only-other --libs-only-L libusb-1.0) -o $@ $< $(shell pkg-config --libs-only-l libusb-1.0)

.PHONY : clean
clean :
	$(RM) $(PROJ).bin $(PROJ).elf $(PROJ).map enter-dfu-mode exit-dfu-mode *.o

.PHONY : dfu
dfu : $(PROJ).bin enter-dfu-mode exit-dfu-mode
	@echo " ENTER-DFU-MODE"
	-@./enter-dfu-mode C057 2579
	@sleep 1
	@echo " DFU-UTIL"
	@dfu-util -d 0483:DF11 -c 1 -i 0 -a 0 -D $< -s 0x08000000
	@echo " EXIT-DFU-MODE"
	@./exit-dfu-mode 0483 DF11
