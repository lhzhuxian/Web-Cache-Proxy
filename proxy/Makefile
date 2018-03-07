CC=g++
CFLAGS=-O3 -fPIC

all: proxy

proxy: *.cpp
	$(CC) $(CFLAGS)  -Wl,-rpath=$(WDIR) -o $@ *.cpp -lpthread

clean:
	rm -f *~ *.o proxy

clobber:
	rm -f *~ *.o
