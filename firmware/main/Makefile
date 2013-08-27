ABI := sparc-elf
CC := $(ABI)-gcc
LD := $(ABI)-ld
OBJCOPY := $(ABI)-objcopy
PROJ := main
C_AND_LD_FLAGS := -mcpu=leon -msoft-float -nostdlib -static
override CFLAGS := $(CFLAGS) $(C_AND_LD_FLAGS) -Wall -Wextra -std=gnu99 -Os -g -fomit-frame-pointer -ffunction-sections -fdata-sections -DF_CPU=48000000UL
override LDFLAGS := $(LDFLAGS) $(C_AND_LD_FLAGS) -Wl,-Tleon3.ld -Wl,--gc-sections
override LDLIBS := -lc -lm -lgcc

FW_SOURCES := $(wildcard *.c)
FW_HEADERS := $(wildcard *.h)
FW_OBJS := $(patsubst %.c,%.o,$(FW_SOURCES))

$(PROJ).mcs : $(PROJ).elf
	@echo " OBJCOPY   $@"
	@$(OBJCOPY) -O ihex $< $@

$(PROJ).elf : $(FW_OBJS) leon3.ld
	@echo " LD        $@"
	@$(CC) $(LDFLAGS) -o$@ -Wl,-Map=$(PROJ).map $(FW_OBJS) $(LDLIBS)

$(FW_OBJS) : $(FW_HEADERS) Makefile

$(FW_OBJS) : %.o : %.c
	@echo " CC        $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

.PHONY : clean
clean :
	$(RM) $(PROJ).elf $(PROJ).map $(PROJ).mcs *.o
