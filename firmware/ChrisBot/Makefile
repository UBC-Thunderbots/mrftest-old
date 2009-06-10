CC := avr-gcc
CFLAGS := -Wall -Wextra -Os -mmcu=atmega128 -DF_CPU=16000000UL
OBJS := main.o debug.o pwmpins.o adcpins.o rtc.o filter.o

# Rule to build a HEX file from an ELF file.
bot.hex : bot.elf
	avr-objcopy -Oihex $< $@

# Rule to link an ELF file from a bunch of O files.
bot.elf : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $+

# Dependencies of O files on headers.
main.o : constants.h debug.h iopins.h pwmpins.h adcpins.h rtc.h filter.h led.h
debug.o : constants.h debug.h
pwmpins.o : pwmpins.h
adcpins.o : adcpins.h
rtc.o : rtc.h
filter.o : filter.h

# Rule to delete intermediate and output files.
.PHONY : clean
clean :
	-rm -f $(OBJS) bot.hex bot.elf
