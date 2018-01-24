
# Should be equivalent to your list of C files, if you don't build selectively
SRC=$(wildcard *.h *.c)

test: $(SRC)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)