ABI := avr
CC := $(ABI)-gcc
LD := $(ABI)-ld
PROJ := main
override CFLAGS := $(CFLAGS) -Wall -Wextra -std=gnu99 -O2 -mmcu=avr2 -mno-interrupts -nostdinc -nostdlib -g
override LDFLAGS := $(LDFLAGS) -Tnavre.ld --gc-sections

FW_SOURCES := $(wildcard *.c)
FW_HEADERS := $(wildcard *.h)
FW_OBJS := $(patsubst %.c,%.o,$(FW_SOURCES))

$(PROJ).elf : $(FW_OBJS) navre.ld
	@echo " LD        $@"
	@$(LD) $(LDFLAGS) -o$@ -Map=$(PROJ).map $(FW_OBJS) $(LDLIBS)

$(FW_OBJS) : $(FW_HEADERS) Makefile

$(FW_OBJS) : %.o : %.c
	@echo " CC        $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

.PHONY : clean
clean :
	$(RM) $(PROJ).elf $(PROJ).map *.o
