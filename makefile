all: shm.pd_darwin

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch -arch i386

.c.pd_darwin:
	cc $(DARWINCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	cc -bundle -arch i386 -undefined suppress -flat_namespace -o $*.pd_darwin $*.o 
	rm -f $*.o

clean: ; rm -f *.pd_darwin *.o
