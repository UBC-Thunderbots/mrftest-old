ABI := avr
CC := $(ABI)-gcc
LD := $(ABI)-ld
PROJ := main
C_AND_LD_FLAGS := -mcall-prologues -mno-interrupts -mmcu=avr3
override CFLAGS := $(CFLAGS) $(C_AND_LD_FLAGS) -Wall -Wextra -std=gnu99 -Os -g -ffunction-sections -fdata-sections
override LDFLAGS := $(LDFLAGS) $(C_AND_LD_FLAGS) -Wl,-Tnavre.ld -Wl,--gc-sections -Wl,-u,vfprintf -lprintf_min
override LDLIBS := -lm

FW_SOURCES := $(wildcard *.c)
FW_HEADERS := $(wildcard *.h)
FW_OBJS := $(patsubst %.c,%.o,$(FW_SOURCES))

$(PROJ).elf : $(FW_OBJS) navre.ld
	@echo " LD        $@"
	@$(CC) $(LDFLAGS) -o$@ -Wl,-Map=$(PROJ).map $(FW_OBJS) $(LDLIBS)

$(FW_OBJS) : $(FW_HEADERS) Makefile

$(FW_OBJS) : %.o : %.c
	@echo " CC        $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

.PHONY : clean
clean :
	$(RM) $(PROJ).elf $(PROJ).map *.o
