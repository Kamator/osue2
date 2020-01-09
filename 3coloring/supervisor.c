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


#define CIRC_BUF_LEN 60
#define SEM_1 "/sem_1"
#define SEM_2 "/sem_2"
#define SHM_NAME "/sharedspace"


struct circ{
	unsigned int edgeCount;
	unsigned int write_pos;
	unsigned int read_pos;
	char *solutions[];
};


static void usage();
static void err_msg(char *msg);
//static *createShm(const char *name, off_t size);

char *prog_name; 

int main (int argc, char *argv[]){
	prog_name = argv[0];
	
	if(argc > 1){
		usage();
	}

	/*Create Shared Memory*/
	int fd = shm_open(SHM_NAME,O_RDWR | O_CREAT,0600);
	
	if(fd == -1){
		close(fd);
		shm_unlink(SHM_NAME);
		err_msg("Could not allocate memory.\n");
	}

	if(ftruncate(fd,CIRC_BUF_LEN) < 0){
		close(fd);
		shm_unlink(SHM_NAME);
		err_msg("Problem with setting size of allocated memory.");
	}


	struct circ *circ_buf;

	circ_buf = mmap(NULL, sizeof(*circ_buf), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);	

	if(circ_buf == MAP_FAILED){
		close(fd);
		shm_unlink(SHM_NAME);
		err_msg("Could not map memory.");
	}

	circ_buf->write_pos = 0;
	circ_buf->read_pos = 0;

	if(close(fd) == -1){
		munmap(circ_buf,sizeof(*circ_buf));
		shm_unlink(SHM_NAME);
		err_msg("Could not close file descriptor!");
	}

	/*Create Semaphore*/
	sem_t *free_sem = sem_open(SEM_1, O_CREAT | O_EXCL, 0600, 1);
	sem_t *used_sem = sem_open(SEM_2, O_CREAT | O_EXCL, 0600, 0);
	
	if(free_sem == SEM_FAILED || used_sem == SEM_FAILED){
		err_msg("Could not create Semaphores.");
	}

	printf("waiting for used_sem\n");
	/*Read from Circular Buffer*/
	sem_wait(used_sem);
	printf("cur sol = %s\n",circ_buf->solutions[circ_buf->read_pos]);
	//char *cur_sol = malloc(sizeof(char) * strlen(circ_buf->solutions[circ_buf->read_pos]));
	//cur_sol = '\0';
	//strcpy(cur_sol,circ_buf->solutions[circ_buf->read_pos]);
	sem_post(free_sem);
	circ_buf->read_pos = (circ_buf->read_pos + 1) % CIRC_BUF_LEN;

	//printf("solution = %s\n",cur_sol);
	
	sem_close(free_sem);
	sem_close(used_sem);

	munmap(circ_buf,sizeof(circ_buf));
	shm_unlink(SHM_NAME);


	exit(EXIT_SUCCESS);
	



}

static void usage(){
	printf("%s: Synopsis = ./supervisor\n",prog_name);
	exit(EXIT_FAILURE);
}


static void err_msg(char *msg){
	fprintf(stderr,"%s: %s\n",prog_name,msg);
	exit(EXIT_FAILURE);
}
