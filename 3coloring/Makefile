#
# @file Makefile
# @author Philipp Geisler <philpp.geisler@student.tuwien.ac.at> or <e11775812@student.tuwien.ac.at> 
#
# @date 07.01.2020
#
#

all: generator supervisor

generator: generator.o
	gcc -o generator  generator.o -pthread -lrt

supervisor: supervisor.o
	gcc -o supervisor supervisor.o -pthread -lrt

generator.o:
	gcc -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c generator.c

supervisor.o: 
	gcc -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c supervisor.c



clean:
	$(info Deleting object files...)
	rm *.o
	$(info Deleting executables...)
	rm generator
	rm supervisor
	$(info Deleting existing semaphores...)
	rm /dev/shm/sem.sem_*
	rm /dev/shm/sharedspace
