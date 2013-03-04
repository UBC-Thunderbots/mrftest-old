ABI := avr
CC := $(ABI)-gcc
LD := $(ABI)-ld
PROJ := main
override CFLAGS := $(CFLAGS) -Wall -Wextra -std=gnu99 -Os -mcall-prologues -mmcu=avr3 -mno-interrupts -g -ffunction-sections -fdata-sections
override LDFLAGS := $(LDFLAGS) -Wl,-Tnavre.ld -Wl,--gc-sections -Wl,-u,vfprintf -lprintf_min
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
