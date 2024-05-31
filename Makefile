CFLAGS=-Wall
CFLAGS=-O0 -g

all: cclean

cclean: cclean.o

clean:
	rm -f *.o cclean

install: cclean
	install cclean /usr/local/bin/
