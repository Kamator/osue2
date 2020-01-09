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



char *prog_name;
static void usage();
static void err_msg(char *msg);
static int getMax(char *arguments[], int len);

static void createSendSolution(int maxIdx, int edges[][maxIdx]);
static void sendToSupervisor(char *solution);

int main(int argc, char *argv[]){
	prog_name = argv[0];
	int len = argc-1;
	srand(time(NULL));
	if(argc < 2){
		usage();
	}
	
	
	/*Get Max and sanitize.*/
	int max = getMax(argv+1,len);
	
	int graph[max+1][max+1];

	/*set to zero */
	for(int k = 0; k <= max; k++){
		for(int l = 0; l <= max; l++){
			graph[k][l] = 0;	
		}
	}

	int i = 1;
	int first,second;
	while(i <= len && argv[i] != NULL){
		first = strtol(argv[i],NULL,10);
		second = strtol(strchr(argv[i],'-')+1,NULL,10);
		graph[first][second] = 1;
		i++;		
	}
	
	createSendSolution(max,graph);

	exit(EXIT_SUCCESS);
}

static void createSendSolution(int maxIdx, int edges[][maxIdx+1]){
	/*Generator Main Loop*/

	while(1){
	
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

	char *setEdges[8];
	
	for(int i = 0; i < 8; i++){
		setEdges[i] = NULL;	
	}
	int edgeCount = 0;
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
				sprintf(toAppend,"%d-%d ",i,j);

				setEdges[edgeCount] = toAppend;
				edgeCount++;
			}
			
			if(edgeCount >= 8){
				break;
			}
		}
		if(edgeCount >= 8){
			break;
		}
	}
	
	/*Check if graph is already 3-colorable*/
	if(setEdges[0] == NULL){
		setEdges[0] = "";
	}

	char *solution = malloc(sizeof(char)*strlen(setEdges[0])*8);
	solution[0] = '\0';

	for(int i = 0; i < 8 && setEdges[i] != NULL; i++){
		strncat(solution,setEdges[i],strlen(setEdges[i]));
	}

	
	/**
	 * TO-DO: 
	 * 1) Implement Supervisor with Circular Buffer DONE
	 * 2) setEdges = Set of Edges that is sent to Circular Buffer
	 * 3) edgeCount = Number of Edges in current solution
	 */

	sendToSupervisor(solution);


	}	
}


static void sendToSupervisor(char *solution){
	/*Open shared Memory*/
	int fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);

	if (fd == -1){
		close(fd);
		shm_unlink(SHM_NAME);
		err_msg("Could not open file descriptor.");
	}

	struct circ *circ_buf;

	circ_buf = mmap(NULL,sizeof(*circ_buf),PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	if (circ_buf == MAP_FAILED){
		close(fd);
		shm_unlink(SHM_NAME);
		err_msg("Could not map memory.");
	}

	/*Look for Semaphores*/
	sem_t *free_sem = sem_open(SEM_1,0);
	sem_t *used_sem = sem_open(SEM_2,0);

	printf("waiting for free_sem\n");
	sem_wait(free_sem);
	circ_buf->solutions[circ_buf->write_pos] = solution;
	sem_post(used_sem);
	circ_buf->write_pos = (circ_buf->write_pos + 1) % CIRC_BUF_LEN;

}

static void usage(){
	err_msg("Synopsis = ./generator 0-1 1-2 ...");
}

static void err_msg(char *msg){
	fprintf(stdout,"%s: %s\n",prog_name,msg);
	exit(EXIT_FAILURE);
}

static int getMax(char *args[], int len){
	int i = 0;
	int curr_max = 0;
	int first,second;

	while (i <= len && args[i] != NULL){
		char *checkStr;

		first = strtol(args[i],&checkStr,10);
		if(checkStr == args[i] || *checkStr == '\0'){
			err_msg("could not parse first vertex.");
		}
		second = strtol(strchr(args[i],'-')+1,&checkStr,10);
	
		if(checkStr == strchr(args[i],'-')+1){
			err_msg("problem with casting second number");
		}

		if(first == second){
			err_msg("no loops allowed");
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
