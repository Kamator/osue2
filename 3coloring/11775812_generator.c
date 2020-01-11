/**
 * @file generator.c
 * @author Philipp Geisler <philipp.geisler@student.tuwien.ac.at>
 * @date 11.01.2020
 *
 * @brief This Program generats random solutions for the 3-colorability problem. These solutions are 
 * generated automatically. If the solution is better than the current best stored in the circular
 * buffer provided by the supervisor then the solution is sent to the circular buffer and stored.
 *
 */


#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>

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
sem_t *free_sem;
sem_t *used_sem;
sem_t *mtex_sem;

char *prog_name;
static void usage();
static void err_msg(char *msg);
static int getMax(char *arguments[], int len);
static void setup_shm();
static void createSendSolution(int maxIdx, int edges[][maxIdx]);
static void sendToSupervisor(char solution[], unsigned int edgeCount);
static void cleanup();
static void exit_graceful();

/**
 * Program entry point.
 * @brief The program starts here. There generators must be invoked with the edges as positional
 * arguments. From the edges, the number of vertices is calculated and the adjacency matrix that 
 * corresponds with the graph is created.
 * 
 * @details Even though main creates the input set, the generator main-loop is stored in a different
 * function. 
 *
 * @param argc The argument counter. 
 * @param argv The argument vector.
 */

int main(int argc, char *argv[]){
	prog_name = argv[0];
	int len = argc-1;
	srand(time(NULL));
	if(argc < 2){
		usage();
	}
	
	
	/*Get Max and sanitize.*/
	int max = getMax(argv+1,len);
	
	/*Indexes go from 0 to max*/
	int graph[max+1][max+1];

	/*set to zero */
	for(int k = 0; k <= max; k++){
		for(int l = 0; l <= max; l++){
			graph[k][l] = 0;	
		}
	}

	int i = 1;
	int first,second;

	/*Adjacency Matrix Set-Up*/
	while(i <= len && argv[i] != NULL){
		first = strtol(argv[i],NULL,10);
		second = strtol(strchr(argv[i],'-')+1,NULL,10);
		
		if(errno == ERANGE){
			err_msg("Under- or overflow ocured.\n");
		}

		graph[first][second] = 1;
		i++;		
	}

	/*Create a set of edges for this input set.*/
	createSendSolution(max,graph);

	exit(EXIT_SUCCESS);
}

/**
 * Create a solution and send it to the supervisor.
 *
 * @brief This function calculates a set of edges that must be removed in order to make the
 * previously randomly colored graph 3-colorable. 
 *
 * @details The solution is only sent to the supervisor if it is better than the current best. This 
 * saves space in the buffer and causes less computation to do for the supervisor. This is handled
 * via a shared variable called circ_buf->edgeCount. 
 * 
 * @param maxIdx number of vertices
 * @param edges	 the adjacency matrix 
 *
 */

static void createSendSolution(int maxIdx, int edges[][maxIdx+1]){

	/*Create shared memory -> SHM needs to be created by supervisor before.*/
	setup_shm();

	
	/*Generator Main Loop*/
	while(1){
	
	if(circ_buf->termination == 1){
		exit_graceful();
	}


	int workEdges[maxIdx+1][maxIdx+1];

	for(int i = 0; i <= maxIdx; i++){
		for(int j = 0; j <= maxIdx; j++){
			workEdges[i][j] = edges[i][j];	
		}
	}

	int vertex[maxIdx+1];

	for(int i = 0; i <= maxIdx; i++){
		vertex[i] = (rand() % 3) + 1;
	}

	char setEdges[8][12];
	
	for(int i = 0; i < 8; i++){
		setEdges[i][0] = '\0';	
	}
	unsigned int edgeCount = 0;

	int notValid = 0;

	for(int i = 0; i <= maxIdx; i++){
		for(int j = 0; j <= maxIdx; j++){
			if(vertex[i] == vertex[j] && (workEdges[i][j] == 1 || workEdges[j][i] == 1)){
				if(workEdges[i][j] == 1){
					workEdges[i][j] = 0;
				}

				if(workEdges[j][i] == 1){
					workEdges[j][i] = 0;
				}
				
				char *toAppend = malloc(sizeof(char)*10);
				
				if(toAppend == NULL){
					cleanup();
					err_msg("Could not allocate any more memory!\n");
				}
				
				sprintf(toAppend,"%d-%d ",i,j);
				strcpy(setEdges[edgeCount],toAppend);
				
				if(setEdges == NULL){
					cleanup();
					err_msg("Error when pasting result of computation.\n");
				}
				

				edgeCount++;
			}
			
			if(edgeCount >= circ_buf->edgeCount){
				notValid = 1;
				break;
			}
		}
		if(edgeCount >= circ_buf->edgeCount){
			notValid = 1;
			break;
		}
	}
	
	//Edge Count <= 8

	/*Legal Solution*/
	if(notValid == 0){
		
		int size_arr = 0; 

		for(int i = 0; i < circ_buf->edgeCount && setEdges[i][0] != '\0'; i++){
			size_arr += strlen(setEdges[i]);
		}

		char solution[100];
		solution[0] = '\0';

		for(int i = 0; i < circ_buf->edgeCount && setEdges[i][0] != '\0'; i++){
			strncat(solution,setEdges[i],strlen(setEdges[i]));
			if(solution == NULL){
				cleanup();
				err_msg("Could not concatenate result.\n");
			}
		}		

		solution[strlen(solution)-1] = '\0';

		sem_wait(mtex_sem);
		if(edgeCount < circ_buf->edgeCount){
			sendToSupervisor(solution,edgeCount);
		}
		sem_post(mtex_sem);

		}

	}	
}

/**
 * Setup shared memory.
 * @brief This function sets up shared memory used by the generator.
 *
 * @details Resources must be created by the supervisor. The generator merely "connects" to the 
 * supervisors provided memory.
 * 
 */

static void setup_shm(){
	/*Open shared Memory*/
	int fd = shm_open(SHM_NAME, O_RDWR, 0600);

	if (fd == -1){
		close(fd);
		shm_unlink(SHM_NAME);
		err_msg("Could not open file descriptor.");
	}

	
	circ_buf = mmap(NULL,sizeof(*circ_buf),PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	if (circ_buf == MAP_FAILED){
		close(fd);
		shm_unlink(SHM_NAME);
		err_msg("Could not map memory.");
	}

	if(close(fd) < 0){
		munmap(circ_buf,sizeof(*circ_buf));
		shm_unlink(SHM_NAME);
		err_msg("Could not close File Descriptor.");
	}

	/*Look for Semaphores*/
	free_sem = sem_open(SEM_1,0);
	used_sem = sem_open(SEM_2,0);
	mtex_sem = sem_open(MTEX,0);

	if(free_sem == SEM_FAILED || used_sem == SEM_FAILED || mtex_sem == SEM_FAILED){
		cleanup();
		err_msg("Could not connect to semaphores!\n");
	}
}

/**
 * Send solution to supervisor. 
 *
 * @brief This function sends a solution to the supervisor. If the supervisor hasn't freed space
 * the program waits on the supervisor here.
 *
 * @details Here also the variable circ_buf->edgeCount is set. This also changes the behavior of
 * possible other generators. 
 *
 * @param solution the solution sent to the supervisor
 * @param edgeCount the number of edges that need to be removed
 *
 */

static void sendToSupervisor(char solution[], unsigned int edgeCount){
	if(circ_buf->termination == 1){
		exit_graceful();
	}
	sem_wait(free_sem);

	strncpy(circ_buf->solutions[circ_buf->write_pos],solution,strlen(solution));
	/*Set EdgeCount*/
	if(edgeCount < circ_buf->edgeCount){
		circ_buf->edgeCount = edgeCount;
	}
	circ_buf->write_pos = (circ_buf->write_pos + 1) % CIRC_BUF_LEN;

	sem_post(used_sem);

}

/**
 * Usage
 * @brief Returns an error if the program has been called wrongly.
 */

static void usage(){
	err_msg("Synopsis = ./generator 0-1 1-2 ...");
}

/**
 * Send an error message.
 * @brief This function returns an error message if called.
 */

static void err_msg(char *msg){
	fprintf(stderr,"%s: %s\n",prog_name,msg);
	exit(EXIT_FAILURE);
}

/**
 * Exit the program gracefully.
 * @brief If the program has worked correctly, use this function to exit. 
 * @details cleanup() cleans up resources before. 
 *
 */

static void exit_graceful(){
	cleanup();	
	exit(EXIT_SUCCESS);
}

/**
 * Clean up resources.
 * @brief If anything not in the ordinary happened use this function to clean up resources.
 * @details Most of the unusual stuff is handled by the supervisor - hence the shortness here.
 *
 */

static void cleanup(){
	if(munmap(circ_buf,sizeof(*circ_buf)) == -1){
		err_msg("Could not unmap Memory!\n");
	}

	if(sem_close(free_sem) == -1 || sem_close(used_sem) == -1){
		err_msg("Could not close Semaphores.\n");
	}
}

/** 
 * Get number of vertices.
 * @brief This function gets called from main and calculates the number of vertices. This has a
 * dependency towards the adjacency matrix that is needed later on for the generation of randomly 
 * colored graphs.
 *
 * @details The input is also sanitized here.
 *
 * @param args the input vector.
 * @param len argc-1 --> number of edges to be considered
 *
 */

static int getMax(char *args[], int len){
	int i = 0;
	int curr_max = 0;
	int first,second;

	while (i <= len && args[i] != NULL){
		char *checkStr;

		first = strtol(args[i],&checkStr,10);
		
		if(errno == ERANGE){
			err_msg("Under- or overflow ocured.\n");
		}

		if(checkStr == args[i] || *checkStr == '\0'){
			err_msg("could not parse first vertex.");
		}

		second = strtol(strchr(args[i],'-')+1,&checkStr,10);
	
		if(errno == ERANGE){
			err_msg("Under- or overflow ocured.\n");
		}

		if(checkStr == strchr(args[i],'-')+1){
			err_msg("problem with casting second number");
		}

		if(first == second){
			err_msg("No loops allowed");
		}

		if(first > curr_max){
			curr_max = first;
		}  	
		if(second > curr_max){
			curr_max = second;
		}
	
		i++;
	}
	return curr_max;
}
