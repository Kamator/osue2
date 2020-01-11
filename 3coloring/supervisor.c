/**
 * @file supervisor.c
 * @author Philipp Geisler <philipp.geisler@student.tuwien.ac.at>
 * @date 11.01.2020
 *
 * @brief
 * This program supervises multiple generators that produce sets of edges that need to be removed in
 * order to make a given graph 3-colorable. These solutions are stored in a circular buffer. The
 * supervisor remembers the best solution. If a solution without any edges has been sent to the
 * supervisor the supervisor shuts down the generators. 
 *
 */

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
#include <signal.h>

#define EDGE_MAX 8
#define CIRC_BUF_LEN 40
#define SEM_1 "/sem_1"
#define SEM_2 "/sem_2"
#define MTEX "/sem_3"
#define SHM_NAME "/sharedspace"


struct circ{
	unsigned int edgeCount;
	unsigned int write_pos;
	unsigned int read_pos;
	unsigned int termination;
	char solutions[CIRC_BUF_LEN][100];
};

struct circ *circ_buf;
sem_t *free_sem, *used_sem, *mtex_sem;

static void usage();
static void cleanup();
static void err_msg(char *msg);
static void signal_handler(int signal);

char *prog_name; 

/**
 * Program entry point. 
 * @brief The program starts here. The synopis is very simple. The supervisor must be called without
 * any arguments or options. 
 *
 * @details The shared memory is created directly here. 
 *
 * @param argv The argument vector.
 * @param argc The argument counter.
 *
 */

int main (int argc, char *argv[]){
	prog_name = argv[0];
	
	if(argc > 1){
		usage();
	}

	if(signal(SIGINT,signal_handler) == SIG_ERR){
		printf("\nProblem with SIGINT.\n");
	}
	if(signal(SIGTERM,signal_handler) == SIG_ERR){
		printf("\nProblem with SIGTERM.\n");
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

	circ_buf = mmap(NULL, sizeof(*circ_buf), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);	

	if(circ_buf == MAP_FAILED){
		close(fd);
		shm_unlink(SHM_NAME);
		err_msg("Could not map memory.");
	}

	circ_buf->write_pos = 0;
	circ_buf->read_pos = 0;
	circ_buf->edgeCount = EDGE_MAX; 
	circ_buf->termination = 0;

	if(close(fd) == -1){
		munmap(circ_buf,sizeof(*circ_buf));
		shm_unlink(SHM_NAME);
		err_msg("Could not close file descriptor!");
	}

	/*Create Semaphore*/
	free_sem = sem_open(SEM_1, O_CREAT | O_EXCL, 0600, 1);
	used_sem = sem_open(SEM_2, O_CREAT | O_EXCL, 0600, 0);
	mtex_sem = sem_open(MTEX, O_CREAT | O_EXCL, 0600,1);

	if(free_sem == SEM_FAILED || used_sem == SEM_FAILED){
		err_msg("Could not create Semaphores.");
	}

	char cur_sol[100];
	cur_sol[0] = '\0';
	unsigned int bestSoFar = EDGE_MAX;

	/*Supervisor Main Loop*/
	while (1){

	sem_wait(used_sem);
	
	/*Copy to Local*/
	if(circ_buf->edgeCount < bestSoFar ){
		if(circ_buf->edgeCount == 0){
			printf("Graph is 3-colorable!\n");
			circ_buf->termination = 1;
		} else {
			bestSoFar = circ_buf->edgeCount;
			strcpy(cur_sol,circ_buf->solutions[circ_buf->read_pos]);
	        	printf("Found Solution with %d edges: %s\n",circ_buf->edgeCount,cur_sol);
			cur_sol[0] = '\0';
		}
	}
	/*Free read area*/
	circ_buf->solutions[circ_buf->read_pos][0] = '\0';
	
	sem_post(free_sem);

	circ_buf->read_pos = (circ_buf->read_pos + 1) % CIRC_BUF_LEN;

	if(circ_buf->termination == 1){
		break;
	}

	}

	cleanup();

	exit(EXIT_SUCCESS);
	



}

/**
 * Handles interrupts. 
 * @brief This function handles interrupts and signals the generators to shut down.
 * 
 * @details In order to shut down the generators, a flag is set (circ_buf->termination).
 *
 */

static void signal_handler(int signal){
	if(signal == SIGINT || signal == SIGTERM){
		circ_buf->termination = 1;
		sem_post(free_sem);
		cleanup();
		printf("Supervisor shuts down.\n");
		exit(EXIT_SUCCESS);
	}
}

/**
 * Cleans up after success and/or failure.
 * 
 * @brief This function gets called if the supervisor is shut down via an interrupt or if the
 * supervisor got a solution that says that the graph is 3-colorable. 
 *
 * @details Cleans everything, also the shared memory and not only the semaphores.
 *
 */

static void cleanup(){	
	if(munmap(circ_buf,sizeof(*circ_buf)) == -1){
		err_msg("could not unmap memory.");
	}
	
	if(shm_unlink(SHM_NAME) == -1){
		err_msg("Could not unlink shared memory.\n");
	}

	printf("\nClosing Semaphores...\n");

	if(sem_close(free_sem) == -1 || sem_close(used_sem) == -1){
		err_msg("Could not close Semaphores!\n");
	}

	if(sem_unlink(SEM_1) == -1 || sem_unlink(SEM_2) == -1){
		err_msg("Could not unlink Semaphores!\n");
	}


}

/**
 * how to use the supervisor
 *
 * @brief Just explains how to start the supervisor.
 *
 */

static void usage(){
	printf("%s: Synopsis = ./supervisor\n",prog_name);
	exit(EXIT_FAILURE);
}

/**
 * error message
 * 
 * @brief Sends an error message, gets called on error and exits the program.
 *
 */

static void err_msg(char *msg){
	fprintf(stderr,"%s: %s\n",prog_name,msg);
	exit(EXIT_FAILURE);
}
