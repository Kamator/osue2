#
# @file Makefile
# @author Philipp Geisler <philpp.geisler@student.tuwien.ac.at> or <e11775812@student.tuwien.ac.at> 
#
# @date 18.12.2019
#
#

all: 3coloring.o
	gcc -o 3coloring  3coloring.o

3coloring.o:
	gcc -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c 3coloring.c

clean:
	$(info Deleting object files...)
	rm *.o
	$(info Deleting executables...)
	rm 3coloring
	$(info Deleting txt files...)
	
