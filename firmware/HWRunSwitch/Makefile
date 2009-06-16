CC := avr-gcc
CFLAGS := -Wall -Wextra -Os -mmcu=atmega128 -DF_CPU=16000000UL
OBJS := hwrunswitch.o
USBDEV := /dev/hwrunswitch

# Rule to build a HEX file from an ELF file.
hwrunswitch.hex : hwrunswitch.elf
	avr-objcopy -Oihex $< $@

# Rule to link an ELF file from a bunch of O files.
hwrunswitch.elf : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $+ -lm

# Rule to delete intermediate and output files.
.PHONY : clean
clean :
	-rm -f $(OBJS) hwrunswitch.hex hwrunswitch.elf

# Rule to upload the firmware.
.PHONY : burn
burn : hwrunswitch.hex
	avrdude -p atmega128 -c stk500v2 -P $(USBDEV) -U $<
