/**
 * @file intmul.c
 * @author Philipp Geisler <philipp.geisler@student.tuwien.ac.at>
 * @date 18.12.2019
 *
 * @brief
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

static void usage();
static void err_msg(char *msg);
static int child(char *numa, char *numb,char *resp); 
static char *combineResults(char *ah_bh, char *ah_bl, char *al_bh, char *al_bl, int len);
static int calcExp(int a, int flag);

char *prog_name;


int main (int argc, char *argv[]){
	prog_name = argv[0];
	
	if(argc > 1){
		usage();
	}


	char *numa = NULL;
	char *numb = NULL;
	size_t lena = 0;
	size_t lenb = 0;

	getline(&numa, &lena, stdin);
	getline(&numb, &lenb, stdin);

	/*Check if numbers have same Length.*/
	if(strlen(numa) != strlen(numb)){
		free(numa);
		free(numb);
		err_msg("The numbers do not have the exact same length!");
	}

	/*Check if number is only one digit long.*/
	if(strlen(numa) <= 2 && strlen(numb) <= 2){

		int fla = 0;
		int flb = 0; 

		/*Check if valid number.*/
		if(numa[0] >= '0' && numa[0] <= '9'){
			fla = 1; 
		} else if (numa[0] >= 'A' && numa[0] <= 'F'){
			fla = 1;
		} else if (numa[0] >= 'a' && numa[0] <= 'f'){
			fla = 1;
		}

		if(numb[0] >= '0' && numb[0] <= '9'){
			flb = 1;
		} else if (numb[0] >= 'A' && numb[0] <= 'F'){
			flb = 1;
		} else if (numb[0] >= 'a' && numb[0] <= 'f'){
			flb = 1;
		}
		
		if(fla != 1 || flb != 1){
			free(numa);
			free(numb);
			err_msg("Not a valid number!");
		}

		int vala = strtol(numa, NULL, 16);
		int valb = strtol(numb, NULL, 16);
			
		free(numa);
		free(numb);

		int res = vala*valb;
		fprintf(stdout,"%X\n",res);
		exit(EXIT_SUCCESS);
	}

	
	/*Check if the numbers' size is even.*/
	if((((strlen(numa)+1)%2) != 0) || (((strlen(numb)+1)%2) != 0)){
		free(numa);
		free(numb);
		err_msg("The numbers' size must be even!");
	}

	/*Split up numbers.*/
	int len = strlen(numa)-1;
	int halflen = len/2;
	
	char *ah = calloc(halflen,sizeof(char));
	char *al = calloc(halflen,sizeof(char));
	char *bh = calloc(halflen,sizeof(char));
	char *bl = calloc(halflen,sizeof(char));

	strncpy(ah,numa,halflen);
	strncpy(al,numa+halflen,halflen);
	strncpy(bh,numb,halflen);
	strncpy(bl,numb+halflen,halflen);

	char *res_1 = calloc(2*halflen+1,sizeof(char)); //ah*bh
	char *res_2 = calloc(2*halflen+1,sizeof(char)); //ah*bl
	char *res_3 = calloc(2*halflen+1,sizeof(char)); //al*bh
	char *res_4 = calloc(2*halflen+1,sizeof(char)); //al*bl

	int co1 = child(ah,bh,res_1);
	int co2 = child(ah,bl,res_2);
	int co3 = child(al,bh,res_3);
	int co4 = child(al,bl,res_4);

	if(co1 == EXIT_FAILURE || co2 == EXIT_FAILURE || co3 == EXIT_FAILURE || co4 == EXIT_FAILURE){
		err_msg("Something happened during forking!");
	}
	
	//now you can perform formula and print result of multiplication to stdout! 
	//A*B = res_1 * 16^n + res_2 * 16^(n/2) + res_3 * 16^(n/2)
	int n = strlen(numa)-1;

	char *resultOfComputation = combineResults(res_1,res_2,res_3,res_4,n); 

	fprintf(stdout,"%s\n",resultOfComputation); 

	exit(EXIT_SUCCESS);	
	

	
}

static char *combineResults(char *ah_bh, char *ah_bl, char *al_bh, char *al_bl, int len){

	char *response = calloc(2*len+1, sizeof(char));

	for(int i = 0; i <= 2*len; i++){
		response[i] = '0';
	}

	int len_ahbh = strlen(ah_bh);
	int len_ahbl = strlen(ah_bl);
	int len_albh = strlen(al_bh);
	int len_albl = strlen(al_bl);

	//set al_bl directly (kind of like a offset)
	
	for(int i = 0; i < len_albl;i++){
		response[(2*n)-i] = al_bl[len_albl-i];
		al_bl[len_albl-i] = '\0';
	}





	return response;


}

static int calcExp(int n, int flag){
	int ret = 1;
	int half = n/2;
	if(n % 2 != 0){
		err_msg("Length of number not even!");
	}
	//16^n
	if(flag == 0){
		for(;n > 0; n--){
			ret *= 16;
		}
	} else if (flag == 1){ //16^n/2
		for(;n > half; n--){
			ret *= 16;
		}	
	} else {
		err_msg("Flag not passed correctly!");
	}

	return ret;

}

 
static int child(char *numa, char *numb, char *resp){
	int status = 0;
	int readpipe[2];
	int writepipe[2]; 

	FILE *writeTo;
	FILE *readFrom; 

	if(pipe(readpipe) == -1 || pipe(writepipe) == -1){
		err_msg("Could not create pipes!");
	}
	
	pid_t cpid = fork();
	switch (cpid) {
		case -1:
			err_msg("Cannot fork!");
		case 0:
			//Child Instructions --> reads on writepipe, writes on readpipe
			close(writepipe[1]);
			close(readpipe[0]);

			dup2(writepipe[0],STDIN_FILENO);
			dup2(readpipe[1],STDOUT_FILENO);
			
			close(writepipe[0]);
			close(readpipe[1]);
			
			execlp("./intmul","./intmul",NULL);	
			return EXIT_FAILURE;

		break;
		default:
			//parent Instructions
			close(writepipe[0]);
			close(readpipe[1]);
			
			writeTo = fdopen(writepipe[1], "w");
			fprintf(writeTo,"%s\n%s\n",numa,numb);
			fclose(writeTo);
			close(writepipe[1]);

			//wait on child
			if(waitpid(cpid,&status,0) == -1){
				return EXIT_FAILURE;
			}
			
			readFrom = fdopen(readpipe[0], "r");
			char *response;
			size_t length;
			getline(&response,&length,readFrom);
			strcpy(resp,response);
			fclose(readFrom);
			close(readpipe[0]);
			free(response);

			return EXIT_SUCCESS; 
	
		
		break;
	}
}



static void usage(){
	printf("%s - Synopsis:\n intmul\n",prog_name);
	exit(EXIT_FAILURE);
}

static void err_msg(char *msg){
	fprintf(stderr,"%s: %s\n",prog_name,msg);
	exit(EXIT_FAILURE);
}
