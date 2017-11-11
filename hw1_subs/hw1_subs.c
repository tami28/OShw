/*
 * hw1_subs.c
 *
 *  Created on: 5 Nov 2017
 *      Author: root
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define EV_DIR "HW1DIR"
#define EV_FILE "HW1TF"
#define WORD_MAX 256
#define FILE_BUFF 4096
#define MAX(first,second) (((first)>(second))?(first) : (second))

int replaceWords(int fd, const char* find, const char* replace);
char* createPath( const char* dir, const char* file);

int main(int argc, const char* argv[]){
	char* hw1dir;
	char* hw1tf;
	char* fullPath = NULL;
	if(argc < 3){
		return 1;
	}
	//Get environment variables:
	hw1dir = getenv(EV_DIR);
	hw1tf = getenv(EV_FILE);
	if (!(hw1dir && hw1tf)){
		return 1;
	}

	//Create full path:
	fullPath =createPath(hw1dir,hw1tf);
	if (NULL == fullPath){
		free(fullPath);
		return 1;
	}
	//Get file descriptor
	int fd = open(fullPath, O_RDONLY);
	if (fd < 0){
		free(fullPath);
		close(fd);
		return 1;
	}
	//Replace and print words:
	int ret= replaceWords(fd, argv[1], argv[2]);
	//Clean after us:
	free(fullPath);
	close(fd);
	return ret;
}

/*
 * Creates a path to the file to be read according to environment variables.s
 */
char* createPath( const char* dir, const char* file){

	char* fullPath;
	if(dir == NULL || file == NULL ){
		return NULL;
	}

	fullPath = (char*)malloc(sizeof(char)*(strlen(dir)+strlen(file)+1));
	if(fullPath == NULL){
		return NULL;
	}
	strcpy(fullPath, dir);
	strcat(fullPath, "/\0");
	strcat(fullPath, file);
	return fullPath;
}

int replaceWords(int fd, const char* find, const char* replace){
	if(find == NULL || replace == NULL){
		return 1;
	}
	char fileBuff[FILE_BUFF] = {0};
	char* nextWord;
	int start;
	int findLen = strlen(find);
	ssize_t rfd = read(fd, fileBuff, FILE_BUFF );
	int temp;
	if (rfd < 0){
		printf("Error! : couldn't read from file");
		return 1;
	}
	//Found part of the loop in stack overflow, about how to read files in chunks:
	while(rfd >0){
		start =0;
		//For the current buffer, want to find all instance of the word to replace, this is the inner while:
		nextWord = strstr(&(fileBuff[start]), find);
		while(nextWord != NULL){
			//print the chars up until the word to replace:
			temp = fwrite(fileBuff+start, sizeof(char),nextWord-(fileBuff+start), stdout );
			if (temp <0){
				printf("Error! couldn't write to stdout");
				return 1;
			}
			//print the replaced word:
			temp = fwrite(replace, sizeof(char), strlen(replace), stdout);
			if (temp <0){
				printf("Error! couldn't write to stdout");
				return 1;
			}
			int length = nextWord-fileBuff - start; //how much printed till now
			//advance the starting point and look for next occurrence:
			start = start+ findLen + length; //TODO: +1??
			nextWord = strstr(&(fileBuff[start]), find);
		}
		//print the rest of the buffer until
		int left_to_print = MAX((MAX(start, (FILE_BUFF-WORD_MAX)) - start), rfd-start);
		temp = fwrite(fileBuff+start,  sizeof(char), left_to_print, stdout);
		if (temp <0){
			printf("Error! couldn't write to stdout");
			return 1;
		}
		//printf("%.*s", MAX(start, (FILE_BUFF-WORD_MAX)) - start, fileBuff+start);
		start = MAX(start, (FILE_BUFF-WORD_MAX));

		//Need to read more
		if (rfd > FILE_BUFF - WORD_MAX){ // if read a full buffer..

			for(int i=0; i<FILE_BUFF - start; i++){
				fileBuff[i] = fileBuff[start+i];
			}
			rfd = read(fd, &(fileBuff[FILE_BUFF - start]), start);
		}
		else{
			rfd = 0;
		}
		if (rfd < 0){
			printf("Error! : couldn't continue reading from file");
			return 1;
		}
	}
	//printf("\n");
	return 0;
}


//gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"hw1_subs.d" -MT"hw1_subs.o" -o "hw1_subs.o" "../hw1_subs.c"
