ABI := arm-linux-gnueabi
CC := $(ABI)-gcc
AR := $(ABI)-ar
OBJCOPY := $(ABI)-objcopy
PROJ := stm32lib
override CFLAGS := $(CFLAGS) -Wall -Wextra -std=gnu99 -O2 -mlittle-endian -mthumb -mcpu=cortex-m3 -nostdinc -nostdlib -g -fno-common -ffunction-sections -fdata-sections -isystem include
override LDFLAGS := $(LDFLAGS) -Tstm32f405.ld --gc-sections

FW_SOURCES := $(wildcard *.c)
FW_HEADERS := $(wildcard *.h include/*.h)
FW_OBJS := $(patsubst %.c,%.o,$(FW_SOURCES))

$(PROJ).a : $(FW_OBJS)
	@echo " AR        $@"
	@$(RM) $@
	@$(AR) rsc $@ $+

$(FW_OBJS) : $(FW_HEADERS) Makefile

$(FW_OBJS) : %.o : %.c
	@echo " CC        $@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

.PHONY : clean
clean :
	$(RM) $(PROJ).a *.o
	$(RM) -r html

.PHONY : doc
doc :
	doxygen
