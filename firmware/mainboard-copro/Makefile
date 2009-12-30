HEX := mainboard-copro.hex
LKR := 18f4550.lkr
PROC := PIC18F4550
ASMS := $(wildcard *.asm)
OBJS := $(ASMS:.asm=.o)

$(HEX) : $(OBJS) $(LKR)
	gplink -o $@ -m -s $(LKR) $(subst $(LKR),,$+)

%.d %.o : %.asm
	{ gpasm -c -C -y -w2 -M $< && sed -e 's/^$(@:.d=.o) :/$@ $(@:.d=.o) :/' < $(@:.o=.d) > $(@:.o=.d).new && mv $(@:.o=.d).new $(@:.o=.d) ; } || { $(RM) $(@:.o=.d) $(@:.o=.d).new ; false ; }

ifneq ($(MAKECMDGOALS),clean)
include $(ASMS:.asm=.d)
endif

.PHONY: clean
clean :
	$(RM) *.cod *.cof *.d *.err *.hex *.lst *.map *.o

.PHONY: burn
burn : $(HEX)
	 pk2cmd -P$(PROC) -W -F $< -E -M -I -J -K

.PHONY : dep
dep : $(ASMS:.asm=.d)

