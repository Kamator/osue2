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




void usage();
void err_msg(char *msg);


char *prog_name;


int main (int argc, char *argv[]){
	prog_name = argv[0];
	
	if(argc > 1){
		usage();
	}


	char *numa;
	char *numb; 
	size_t lena; 
	size_t lenb; 

	getline(&numa, &lena, stdin);
	getline(&numb, &lenb, stdin);

	if(strlen(numa) != strlen(numb)){
		free(numa);
		free(numb);
		err_msg("The numbers do not have the exact same length!");
	}

	if((((strlen(numa)+1)%2) != 0) || (((strlen(numb)+1)%2) != 0)){
		free(numa);
		free(numb);
		err_mgs("The numbers' size must be even!");
	}







	exit(EXIT_SUCCESS);	
	

	
}


void usage(){
	printf("%s - Synopsis:\n intmul\n",prog_name);
	exit(EXIT_FAILURE);
}

void err_msg(char *msg){
	fprintf(stderr,"%s: %s\n",prog_name,msg);
	exit(EXIT_FAILURE);
}
