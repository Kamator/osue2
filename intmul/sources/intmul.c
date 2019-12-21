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
static char *addNumbers(char *num1, int shift1, char *num2);

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
		fprintf(stdout,"%X",res);
		exit(EXIT_SUCCESS);
	}

	
	/*Check if the numbers' size is even.*/
	//+1!
	if((((strlen(numa)+1)%2) != 0) || (((strlen(numb)+1)%2) != 0)){
		free(numa);
		free(numb);
		err_msg("The numbers' size must be even!");
	}

	/*Split up numbers.*/
	int len = strlen(numa)-1;  //because of newline
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

	char *resultOfComputation = calloc(2*len+1,sizeof(char));
	resultOfComputation = combineResults(res_1,res_2,res_3,res_4,n); 

	fprintf(stdout,"%s\n",resultOfComputation); 
	
	free(res_1);
	free(res_2);
	free(res_3);
	free(res_4);

	free(numa);
	free(numb);

	free(ah);
	free(bh);
	free(al);
	free(bl);


	exit(EXIT_SUCCESS);	
	

	
}

static char *combineResults(char *ah_bh, char *ah_bl, char *al_bh, char *al_bl, int len){

	char *res1 = addNumbers(al_bh,len/2,al_bl);
		
	char *res2 = addNumbers(ah_bl,len/2,res1);

	char *res3 = addNumbers(ah_bh,len,res2);

	return res3;


}

static char *addNumbers(char *num1, int shift1, char *num2){

	if(num1[strlen(num1)-1] == '\n'){
		num1[strlen(num1)-1] = '\0';
	}

	//shift number1 to the left first	
	char number_shifted[strlen(num1)+shift1];
	sprintf(number_shifted,"%s",num1);
	
	if(number_shifted[strlen(number_shifted)-1] == '\n'){
		number_shifted[strlen(number_shifted)-1] = '\0';
	}
	
	char zero[shift1];
	
	for(int i = 0; i < shift1; i++){
		zero[i] = '0';
	}
	strcat(number_shifted,zero);
	
	number_shifted[strlen(num1)+shift1] = '\0';

	char newNumber2[strlen(num2)];
	sprintf(newNumber2,"%s",num2);

	if(newNumber2[strlen(newNumber2)-1] == '\n'){
		newNumber2[strlen(newNumber2)-1] = '\0';
	}

	newNumber2[strlen(newNumber2)] = '\0';

	char *number2 = malloc(strlen(newNumber2)+strlen(number_shifted));
	char *num_shifted = malloc(strlen(newNumber2)+strlen(number_shifted));

	//numbers must have same length
	if(strlen(newNumber2) != strlen(number_shifted)){
		if(strlen(newNumber2) < strlen(number_shifted)){
			int a = strlen(number_shifted);
			int b = strlen(newNumber2);
			int diff = a - b;
			char help[strlen(number_shifted)];
			for(int k = 0; k < diff; k++){
				help[k] = '0';
			}
			help[diff] = '\0';
			strcat(help,newNumber2);
			strcpy(number2,help);
			strncpy(num_shifted,number_shifted,strlen(number_shifted));
		} else {
			int diff = strlen(newNumber2)-strlen(number_shifted);
			char help[strlen(newNumber2)];
			for(int k = 0; k < diff; k++){
				help[k] = '0';
			}
			help[diff] = '\0';
			strcat(help,number_shifted);
			strcpy(num_shifted,help);
			strcpy(number2,newNumber2);
		}
	} else {
		strcpy(number2,newNumber2);
		strcpy(num_shifted,number_shifted);
	}
	
	//begin to Add

	char *response = malloc(strlen(num_shifted)+strlen(number2));
	if(response == NULL){
		err_msg("malloc_1");
	}
	for(int k = 0; k < strlen(num_shifted); k++){
		response[k] = '0';
	}

	int carry = 0;
	int digit1;
	int digit2;
	int temp_res;
	char temp_hex[1];
	char temp_str[2];
	int res_digit;
	int res_idx = strlen(response)-1;

	int mem_len = strlen(num_shifted);

	for(int k = 0; k < mem_len;k++){
		digit1 = strtol(num_shifted+strlen(num_shifted)-1,NULL,16);
		digit2 = strtol(number2+strlen(number2)-1,NULL,16);

		temp_res = digit1 + digit2 + carry;
		carry = temp_res/(0x10);
		res_digit = temp_res%(0x10);
		
		sprintf(temp_str,"%X",res_digit);
		strncpy(temp_hex,temp_str,1);
	
		response[res_idx] = temp_hex[0];
		res_idx--;

		num_shifted[strlen(num_shifted)-1] = '\0';
		number2[strlen(number2)-1] = '\0';
	}

	if(carry != 0){
		sprintf(temp_str,"%X",carry);
		strncpy(temp_hex,temp_str,1);

		char *fullResp = malloc((strlen(response)*strlen(temp_hex))*sizeof(char));
		if(fullResp == NULL){
			err_msg("malloc");
		}


		fullResp[0] = temp_hex[0];
		strcat(fullResp,response);
		return fullResp;


	}
	
	return response;
	


}

static int child(char *numa, char *numb, char *resp){

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
			pid_t pid;
			int status;
			pid = wait(&status);
			if(WEXITSTATUS(status) != EXIT_SUCCESS){
				char *resp = malloc(sizeof(char)*2000);
				sprintf(resp,"waiting error .. pid = %d ... status =  %d",pid,WEXITSTATUS(status));
				err_msg("waiting error");
			}
			
			char *response = NULL;
			readFrom = fdopen(readpipe[0], "r");
			size_t length = 0;
			getline(&response,&length,readFrom);
			close(readpipe[0]);

			strcpy(resp,response);
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
