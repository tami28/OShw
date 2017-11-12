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
#define WORD_MAX 6 //256
#define FILE_BUFF 12 //4096
#define MAX(first,second) (((first)>(second))?(first) : (second))

int replaceWords(int fd, const char* find, const char* replace);
int replaceWords2(int fd, const char* find, const char* replace);
char* createPath( const char* dir, const char* file);

int main(int argc, const char* argv[]){
	char* hw1dir = NULL;
	char* hw1tf = NULL;
	char* fullPath = NULL;
	//make ssure enough args:
	if(argc < 3){
		printf("Not enough arguments");
		return 1;
	}
	//Get environment variables:
	hw1dir = getenv(EV_DIR);
	hw1tf = getenv(EV_FILE);
	if (!(hw1dir && hw1tf)){
		printf("enivronment variables are not set");
		return 1;
	}

	//Create full path:
	fullPath = createPath(hw1dir,hw1tf);
	if (NULL == fullPath){
		printf("%s", strerror (errno));
		return 1;
	}
	//Get file descriptor
	int fd = open(fullPath, O_RDONLY);
	if (fd < 0){
		printf("%s", strerror (errno));
		free(fullPath);
		return 1;
	}
	//Replace and print words:
	int ret = replaceWords2(fd, argv[1], argv[2]);
	if (ret != 0){
		printf("%s", strerror (errno));
	}
	//Clean after us:
	free(fullPath);
	close(fd);
	return ret;
}

/*
 * Creates a path to the file to be read according to environment variables.s
 */
char* createPath( const char* dir, const char* file){
	//sanity checks for vars:
	char* fullPath = NULL;
	if(dir == NULL || file == NULL ){
		return NULL;
	}

	fullPath = (char*)malloc(sizeof(char)*(strlen(dir)+strlen(file)+3));
	if(fullPath == NULL){
		return NULL;
	}
	//create the path by copying and concatenating:
	strcpy(fullPath, dir);
	strcat(fullPath, "/\0");
	strcat(fullPath, file);
	return fullPath;
}

int replaceWords(int fd, const char* find, const char* replace){
	//sanity check for cars:
	if(find == NULL || replace == NULL){
		return 1;
	}
	//initiialize vars:
	char fileBuff[FILE_BUFF+1] = {0}; //last one for \0 for strstr
	char* nextWord = NULL;
	int start =0;
	int temp = 0;
	int findLen = strlen(find);
	ssize_t rfd = read(fd, fileBuff, FILE_BUFF );
	//check if read:
	if (rfd < 0){
		printf("Error! : couldn't read from file");
		return 1;
	}
	fileBuff[rfd] = '\0';
	//Found part of the loop in stack overflow, about how to read files in chunks:
	while(rfd > 0){
		//TODO
		printf("\nbuffer:%s\nend buffer\n", fileBuff);
		rfd+= start;
		start =0;
		fileBuff[rfd+1] = '\0';//TODO
		//For the current buffer, want to find all instance of the word to replace, this is the inner while:
		nextWord = strstr(&(fileBuff[start]), find);
		while(nextWord != NULL){
			//print the chars up until the word to replace:
			temp = fwrite(fileBuff+start, sizeof(char),nextWord-(fileBuff+start), stdout );
			if (temp <0){
				return 1;
			}
			//print the replaced word:
			temp = fwrite(replace, sizeof(char), strlen(replace), stdout);
			if (temp <0){
				return 1;
			}
			int length = nextWord-fileBuff - start; //how much printed till now
			//advance the starting point and look for next occurrence:
			start = start+ findLen + length; //TODO: +1??
			nextWord = strstr(&(fileBuff[start]), find);
		}
		//print the rest of the buffer until
		int left_to_print = MAX((MAX(start, (rfd-WORD_MAX)) - start), rfd-start);
		printf("\n printing left buffer: %d\n", left_to_print);
		temp = fwrite(fileBuff+start,  sizeof(char), left_to_print, stdout);
		if (temp <0){
			return 1;
		}
		start = MAX(start, (rfd-WORD_MAX));

		//Need to read more
			for(int i=0; i<rfd - start; i++){
				fileBuff[i] = fileBuff[start+i];
			}
			rfd = read(fd, &(fileBuff[FILE_BUFF - start]), start);
	}
	//need?
	//temp = fwrite(fileBuff,  sizeof(char), start, stdout);
	if (temp <0){
		return 1;
	}

	if(rfd <0){
		return 1;
	}

	return 0;
}

int replaceWords2(int fd, const char* find, const char* replace){
	//sanity check for cars:
	if(find == NULL || replace == NULL){
		return 1;
	}
	//Format of getting fstat taken from http://codewiki.wikidot.com/c:system-calls:fstat

	//allocate place for the file according to it's size. Written in the forum it's allowed, along with the use of fstat.
	struct stat fileStat;
	if(fstat(fd,&fileStat) < 0){
		return 1;
	}
	char * buffer = (char*) calloc(fileStat.st_size +1, sizeof(char));
	if (buffer == NULL){
		return 1;
	}
	int total = 0;
	ssize_t rfd = 0;

	//read all the file:
	while (total < fileStat.st_size){
		rfd = read(fd, &(buffer[total]), fileStat.st_size );
		total+= rfd;
		if(rfd<0){
			free(buffer);
			return 1;
		}
		if(rfd == 0){
			free(buffer);
			printf("not the same size as expected!");
			return 1;
		}
	}
	if(total > fileStat.st_size){
		return 2;
	}
	//for strstr so no error:
	buffer[total + 1] = '\0';

	//find print & replace everything:

	ssize_t temp = 0;
	int start = 0;
	int findLen = strlen(find);
	char*	nextWord = strstr(&(buffer[start]), find);
	while(nextWord != NULL){
		//print the chars up until the word to replace:
		if (nextWord-(buffer+start) > 0){
			temp = fwrite(buffer+start, sizeof(char),nextWord-(buffer+start), stdout );
			if (temp <0){
				free(buffer);
				return 1;
			}
		}
		//print the replaced word:
		if (strlen(replace) > 0){
			temp = fwrite(replace, sizeof(char), strlen(replace), stdout);
			if (temp <0){
				free(buffer);
				return 1;
			}
		}
		int length = nextWord-buffer - start; //how much printed till now
		//advance the starting point and look for next occurrence:
		start = start+ findLen + length; //TODO: +1??
		nextWord = strstr(&(buffer[start]), find);
	}
	temp = fwrite(buffer+start, sizeof(char), fileStat.st_size - start, stdout);
	if (temp <0){
		free(buffer);
		return 1;
	}
	free(buffer);
	return 0;
}
