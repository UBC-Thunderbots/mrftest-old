counter4.hex : counter4.o 18f2550.lkr
	gplink -m -s 18f2550.lkr -o $@ counter4.o

counter4.o : counter4.asm
	gpasm -c -w2 $<

.PHONY : clean
clean :
	-rm -f counter4.cod counter4.hex counter4.lst counter4.map counter4.o
