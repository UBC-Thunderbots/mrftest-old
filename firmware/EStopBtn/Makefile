CC := avr-gcc
CFLAGS := -Wall -Wextra -Os -mmcu=atmega128 -DF_CPU=16000000UL
OBJS := estopbtn.o
USBDEV := /dev/ttyUSB0

# Rule to build a HEX file from an ELF file.
estopbtn.hex : estopbtn.elf
	avr-objcopy -Oihex $< $@

# Rule to link an ELF file from a bunch of O files.
estopbtn.elf : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $+ -lm

# Rule to delete intermediate and output files.
.PHONY : clean
clean :
	-rm -f $(OBJS) estopbtn.hex estopbtn.elf

# Rule to upload the firmware.
.PHONY : burn
burn : estopbtn.hex
	avrdude -p atmega128 -c stk500v2 -P $(USBDEV) -U $<
