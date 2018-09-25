CFLAGS=-O0 -g -pedantic
PROG=midilatency
LDLIBS=-lasound

all: $(PROG)

midilatency: midilatency.o

clean:
	rm -f *~ *.bak *.o $(PROG)
