CC := arm-elf-gcc
LD := arm-elf-ld
OBJCOPY := arm-elf-objcopy
PROJ := dongle
override CFLAGS := $(CFLAGS) -Wall -Wextra -std=gnu99 -O2 -mlittle-endian -mthumb -mcpu=cortex-m3 -nostdinc -nostdlib -g
override LDFLAGS := $(LDFLAGS) -Tstm32f405.ld --gc-sections

$(PROJ).bin : $(PROJ).elf
	@echo " OBJCOPY   $@"
	@$(OBJCOPY) -O binary $+ $@

$(PROJ).elf : $(patsubst %.c,%.o,$(wildcard *.c)) stm32f405.ld
	@echo " LD        $@"
	@$(LD) $(LDFLAGS) -o$@ -Map=$(patsubst %.elf,%.map,$@) $(filter %.o,$+) $(LDLIBS)

$(patsubst %.c,%.o,$(wildcard *.c)) : $(wildcard *.h) Makefile

%.o : %.c
	@echo " CC        $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

.PHONY : clean
clean :
	$(RM) $(PROJ).bin $(PROJ).elf $(PROJ).map *.o

.PHONY : dfu
dfu : $(PROJ).bin
	dfu-util -d 0483:DF11 -c 1 -i 0 -a 0 -D $< -s 0x08000000
