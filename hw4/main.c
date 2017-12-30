#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <linux/unistd.h>

#define BUFFER_SIZE 1024
void writeToOutputFileAndCleanBuffer();
void noThreadAllXor(int argc, char** argv);
void workerMain(char *filePath);
int readNextBlock(int fd, char* buff);
void xorWithOutput(char* inputBuffer, int length, int isFirst);
void xorAndWrite(char*, int, int);


int numThreads =1, workersDone =0, outputFD=0, lengthToWrite = 0;
int level = 0;

pthread_mutex_t bufferLock;
pthread_cond_t  allDoneCond;

char outputBuffer[BUFFER_SIZE] = {0};

pthread_t* threads;


//
//void noThreadAllXor(int argc, char** argv){
//    //OPEN ALL FILES:
//    int fd = open(argv[2], O_RDONLY);
//    char buff[BUFFER_SIZE] = {0};
//    if(fd < 0){
//        exit(-1);
//    }
//    firstFile = (Node*)malloc(sizeof(Node));
//    if (firstFile == NULL){
//        exit(-1);
//    }
//    firstFile->fd = fd;
//    firstFile->next = NULL;
//    firstFile->prev = NULL;
//
//    Node* curr = firstFile, *prev = firstFile->prev;
//    for (int i=3; i< argc; i++){
//        fd = open(argv[i], O_RDONLY);
//        if(fd < 0){
//            exit(-1);
//        }
//        curr = (Node*)malloc(sizeof(Node));
//        if (curr == NULL){
//            exit(-1);
//        }
//        curr->fd = fd;
//        curr->next = NULL;
//        curr->prev = prev;
//        if (prev != NULL){prev->next = curr;}
//        prev = curr;
//        curr = curr->next;
//    }
//    printf("finished creating nodes\n");
//    int numStillOpen = argc-2;
//
//    while (numStillOpen > 0){
//        curr = firstFile;
//        for (int i=0; i<numStillOpen; i++){
//            int ret = readNextBlock(curr->fd, buff);
//            printf("still open: %d\n", numStillOpen);
//            xorWithOutput(buff, ret);
//            //File finished:
//            if (ret<BUFFER_SIZE){
//                close(curr->fd);
//                curr->next->prev = curr->prev;
//                if(curr->prev == NULL){
//                    firstFile = curr->next;
//                } else{
//                    curr->prev->next = curr->next;
//                }
//                curr = curr->next;
//                free(curr);
//
//                numStillOpen--;
//            }
//            curr = curr->next;
//
//        }
//        writeToOutputFileFileAndCleanBuffer();
//    }
//}

void printTid(){
    pthread_t pt = pthread_self();
    unsigned int * x = (unsigned int*)(&pt);
    printf("%u",*x);
//    for (size_t i=0; i<sizeof(pt); i++) {
//        printf("%02x", (unsigned)(ptc[i]));
//    }
}

void workerMain(char *filePath){
    char buff[BUFFER_SIZE];
    int isFirst = 1;
    int readBytes = 0; //bytes read in reading
    int inputFd = open(filePath, O_RDONLY);
    int current_level =0;
    if (inputFd < 0){
        printf("couldn't open file %s. reason: %d\n", filePath, errno);
        exit(2);
    }

    while((readBytes = readNextBlock(inputFd, buff))>0){
        xorAndWrite(buff, readBytes, current_level);
        current_level++;
    }
    //finished, handle finished details:
    pthread_mutex_lock(&bufferLock);
    printTid();
    printf(" done reading\n");
    if (current_level!=level){
        printTid();
        printf(" waiting for level to finish before done..%d,%d.\n", current_level, level);
        pthread_cond_wait(&allDoneCond, &bufferLock);
    }
    //I'm not last, safely decrement:

    if(workersDone != numThreads-1){
        printTid();
        printf(" level not done. bye bye\n");
        numThreads--;
        pthread_mutex_unlock(&bufferLock);
    } else{
        printTid();
        printf(" I am fucking last\n");
        numThreads--;
        writeToOutputFileAndCleanBuffer();
        pthread_mutex_unlock(&bufferLock);
    }
    printTid();
    printf(" dead\n");
    pthread_exit(NULL);


}

int readNextBlock(int fd, char* buff){
    int ret= read(fd,buff, BUFFER_SIZE);
    int bytesRead= ret;
    while(bytesRead < BUFFER_SIZE && ret >0){
        ret = read(fd, &(buff[bytesRead]), BUFFER_SIZE- bytesRead);
        bytesRead+=ret;
    }
    if (ret < 0){
        printf("couldn't read from a input file\n");
        exit(2);
    }
    return bytesRead;
}

/*
 * 1. xor's input buffer with global buffer
 * 2. increments finished counter
 * 3. if last - write to file, intialize, and release xor lock
 */
void xorAndWrite(char* inputBuffer, int length, int current_level){
    xorWithOutput(inputBuffer, length, current_level);
    pthread_mutex_lock(&bufferLock);
    workersDone ++;
    if(length > lengthToWrite){
        lengthToWrite = length;
    }

    if(workersDone == numThreads){
        writeToOutputFileAndCleanBuffer();
    }
    pthread_mutex_unlock(&bufferLock);


}

void xorWithOutput(char* inputBuffer, int length, int current_level){
    pthread_mutex_lock(&bufferLock);

    if (current_level != level){
        pthread_cond_wait(&allDoneCond, &bufferLock);
    }
//    if (! isFirst){
//        printf("Not first write. wait on cond\n");
//
//    }
    printTid();
    printf(" Xoring...\n");
    for (int i =0; i<length; i++){
        outputBuffer[i] ^= inputBuffer[i];
    }
    pthread_mutex_unlock(&bufferLock);

}


void writeToOutputFileAndCleanBuffer(){
    printTid();
    printf(" Writing to file: %d\n", lengthToWrite);
    int ret =write(outputFD, outputBuffer,lengthToWrite);
    if (ret < 0 ){
        printf("Error writing %d\n", errno);
        exit(2);
    }
    for (int i = 0; i< BUFFER_SIZE; i++){
        outputBuffer[i] = 0;
    }
    workersDone = 0;
    lengthToWrite = 0;
    level++;
    pthread_cond_broadcast(&allDoneCond);
}

//numThreadsd should be updated before calling this:
void initAndcreateThreads(char **in_file_path){
    pthread_mutex_init(&bufferLock, NULL); //TODO everywhere: check return vals.
    pthread_cond_init(&allDoneCond, NULL);
    threads = (pthread_t*)malloc(numThreads*sizeof(pthread_t));
    for (int i=0; i<numThreads; i++){
        if (pthread_create(&(threads[i]), NULL, (void *(*)(void *))&workerMain, in_file_path[i]) != 0){
            printf("error creating thread, error: %d\n", errno);
            exit(2);
        }
    }
}

void waitForThreads(){
    for (int i = 0; i < numThreads; i++){
        pthread_join(threads[i],NULL);
    }
}

int main(int argc, char** argv) {
    if (argc<3){
        if (printf("Not enough input args. Usage: <output_file> <input_file1> ..., with any number of another input files.\n")<0){
            exit(-1);
        } exit(0);
    }
    if(printf("Hello, creating %s from %d input files\n", argv[1],argc-2 )<0){
        exit(-1);
    }
    outputFD = open(argv[1], O_CREAT| O_TRUNC | O_WRONLY);
    if(outputFD < 0){
        printf("failed opening \\ creating output file %s beacuse %d\n", argv[1], errno);
        exit(-1);
    }
    numThreads = argc - 2;

    //do here something
    //noThreadAllXor(argc, argv);
    initAndcreateThreads(&(argv[2]));
    waitForThreads();
    close(outputFD);
    printf("finished\n");
    free(threads);
    return 0;
}
