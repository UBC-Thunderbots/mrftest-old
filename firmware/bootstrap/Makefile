SOURCES := $(wildcard *.c)
PROJ := bootstrap
SDCC_FLAGS := --std-sdcc99 -mpic16 -p18f67j50 --fomit-frame-pointer --obanksel=1 --optimize-cmp --optimize-df --use-non-free -I../shared
PK2CMD_FLAGS := -I -J -E -M -PPIC18F67J50 -W
LKR := 18f67j50.lkr

$(PROJ).hex : $(SOURCES:.c=.o) $(LKR)
	echo "  LD $@"
	sdcc $(SDCC_FLAGS) -Wl,-m -Wl,-s -Wl,$(LKR) -o $@ $(filter-out $(LKR),$+) libc18f.lib

%.o : %.c pins.h $(wildcard *.h ../shared/*.h)
	echo "  CC $@"
	sdcc $(SDCC_FLAGS) -c $<

pins.h pins.inc : ../main/pins.txt ../pingen/pingen
	echo "  PINGEN $@"
	../pingen/pingen ../main/pins.txt

../pingen/pingen : $(wildcard ../pingen/*.[ch])
	make -C ../pingen

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

.SILENT :
