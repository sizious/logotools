CC	= gcc
CFLAGS	= -O3

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

all: logoinsert pngtomr

pngtomr: pngtomr.o
	$(CC) $^ -o $@ -lpng

logoinsert: logoinsert.o
	$(CC) $^ -o $@

.PHONY : clean
clean:
	rm -f *.o

.PHONY : distclean
distclean: clean 
	rm -f logoinsert pngtomr
