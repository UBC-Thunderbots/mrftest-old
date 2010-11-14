SOURCES := $(filter-out pingen.c,$(wildcard *.c))
PROJ := sj2usb-main
SDCC_FLAGS := --std-sdcc99 -mpic16 -p18f65j50 --fomit-frame-pointer --obanksel=1 --optimize-cmp --optimize-df --use-non-free
PK2CMD_FLAGS := -I -J -E -M -PPIC18F65J50 -W
LKR := 18f65j50.lkr

$(PROJ).hex : $(SOURCES:.c=.o) $(LKR)
	echo "  LD $@"
	sdcc $(SDCC_FLAGS) -Wl,-m -Wl,-s -Wl,$(LKR) -o $@ $(filter-out $(LKR),$+) libc18f.lib

%.o : %.c pins.h $(wildcard *.h)
	echo "  CC $@"
	sdcc $(SDCC_FLAGS) -c $<

pins.h pins.inc : pins.txt pingen
	echo "  PINGEN $@"
	./pingen

pingen : pingen.c
	echo "  HOSTCC $@"
	$(CC) -Wall -Wextra -ansi -pedantic -o$@ $+

.PHONY : clean
clean :
	$(RM) $(SOURCES:.c=.asm) $(SOURCES:.c=.lst) $(SOURCES:.c=.o) $(PROJ).cod $(PROJ).hex $(PROJ).lst $(PROJ).map pins.h pins.inc pingen
	$(RM) -r html

.PHONY : burn
burn : $(PROJ).hex
	pk2cmd $(PK2CMD_FLAGS) -F $<

.PHONY : doc
doc :
	doxygen

.PHONY : uart
uart :
	pk2uart -b38462

.SILENT :
