ABI := sparc-elf
AS := $(ABI)-as
LD := $(ABI)-ld
PROJ := bios
override ASFLAGS := $(ASFLAGS) -g -Av8
override LDFLAGS := $(LDFLAGS) -T leon3.ld --gc-sections -Map $(PROJ).map --no-undefined --warn-common
override LDLIBS :=

FW_SOURCES := $(wildcard *.s)
FW_HEADERS := $(wildcard *.inc)
FW_OBJS := $(patsubst %.s,%.o,$(FW_SOURCES))

$(PROJ).elf : $(FW_OBJS) leon3.ld
	@echo " LD        $@"
	@$(LD) $(LDFLAGS) -o$@ $(FW_OBJS) $(LDLIBS)

$(FW_OBJS) : $(FW_HEADERS) Makefile

$(FW_OBJS) : %.o : %.s
	@echo " AS        $@"
	@$(AS) $(ASFLAGS) -o$@ $<

.PHONY : clean
clean :
	$(RM) $(PROJ).elf $(PROJ).map *.o
