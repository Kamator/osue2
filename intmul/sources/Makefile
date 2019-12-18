#
# @file Makefile
# @author Philipp Geisler <philpp.geisler@student.tuwien.ac.at> or <e11775812@student.tuwien.ac.at> 
#
# @date 18.12.2019
#
#

all: intmul.o
	gcc -o intmul intmul.o

intmul.o:
	gcc -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c intmul.c

clean:
	$(info Deleting object files...)
	rm *.o
	$(info Deleting executables...)
	rm intmul
	$(info Deleting txt files...)
	rm *.txt
