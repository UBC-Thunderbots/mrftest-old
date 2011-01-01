HEX := chickertest.hex
PROC := PIC18F4550
CS := $(wildcard *.c)
OBJS := $(CS:.c=.o)
SDCCFLAGS := --std-sdcc99 -mpic16 -p18f4550 --fommit-frame-pointer

$(HEX) : $(OBJS)
	sdcc $(SDCCFLAGS) -o $@ $(OBJS)

%.o : %.c *.h
	sdcc $(SDCCFLAGS) -c -o $@ $<

.PHONY: clean
clean :
	$(RM) *.asm *.cod *.cof *.err *.hex *.lst *.map *.o

.PHONY: burn
burn : $(HEX)
	 pk2cmd -P$(PROC) -W -F $< -E -M -I -J

