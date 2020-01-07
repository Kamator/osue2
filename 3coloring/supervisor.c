#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>


#define MAX_DATA (50)


struct myshm {
	unsigned int state;
	unsigned int data[MAX_DATA];
};


static void usage();
static void err_msg(char *msg);
static int createShm(const char *name, off_t size);

char *prog_name; 

int main (int argc, char *argv[]){
	prog_name = argv[0];
	
	if(argc > 1){
		usage();
	}

	struct myshm *useshm = createShm("/somename",2000);


	exit(EXIT_SUCCESS);
	



}

static void usage(){
	printf("%s: Synopsis = ./supervisor\n",prog_name);
	exit(EXIT_FAILURE);
}

static struct myshm *createShm(const char *name, off_t size){
	int fd = shm_open(name,O_RDWR | O_CREAT,0600);
	
	if(fd == -1){
		err_msg("Could not allocate memory.\n");
	}

	if(ftruncate(fd,size) < 0){
		err_msg("Problem with setting size of allocated memory.");
	}

	struct myshm *myshm;

	myshm = mmap(NULL, sizeof(*myshm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if(myshm == MAP_FAILED){
		err_msg("Could not map memory.");
	}

	if(close(fd) == -1){
		err_msg("Could not close file descriptor!");
	}

	return myshm;

}

static void err_msg(char *msg){
	fprintf(stderr,"%s: %s\n",prog_name,msg);
	exit(EXIT_FAILURE);
}
