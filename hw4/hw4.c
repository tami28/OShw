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

#define BUFFER_SIZE 1048576
/*
 * Writes the xored buffer into to output file according to maxLength
 * initializes all needed vars
 * broadcasts to threads to continue.
 */
void writeToOutputFileAndCleanBuffer();
/*
 * Main of each thread, handles file ops, reads & xors & quits.
 * Knows it can die safely when it finished reading and the thread's level is the same as the global level.
 */
void workerMain(char *filePath);
/*
 * reads next block (or as much there is) for given fd.
 */
int readNextBlock(int fd, char* buff);
/*
 * Incharge of actually xoring given buff with the output.
 */
void xorWithOutput(char* inputBuffer, int length, int level);
/*
 * What each thread calls. Handles xoring & when to write.
 */
void xorAndWrite(char*, int , int);
/*
 * incharge of joining threads & destroying mutex etc..
 */
void waitForThreads(int totalNumberOfThreads);
/*
 * Initialize mutex etc and create threads.
 */
void initAndcreateThreads(char **in_file_path);
//#define DEBUG

#ifdef DEBUG
#define LOG(x) printf x
#else
#define LOG(x)

#endif


int numThreads =1, workersDone =0, outputFD=0, lengthToWrite = 0;
int level = 0, total_size=0;
pthread_mutex_t bufferLock;
pthread_cond_t  allDoneCond;
pthread_attr_t attr;
char outputBuffer[BUFFER_SIZE] = {0};
pthread_t* threads;

//Used for debugging:
#ifdef DEBUG
void printTid(){
    pthread_t pt = pthread_self();
    unsigned int * x = (unsigned int*)(&pt);
    printf("%u",*x);
}
#else
    void printTid(){}
#endif

/*
 * This is the "main" of each of the threads.
 */
void workerMain(char *filePath){
    char buff[BUFFER_SIZE];
    int readBytes = 0; //bytes read in reading
    int inputFd = open(filePath, O_RDONLY);
    int current_level =0;
    int ret;
    //Try open file path:
    if (inputFd < 0){
        printf("couldn't open file %s. reason: %d\n", filePath, errno);
        exit(2);
    }

    //Read one block at a time:
    while((readBytes = readNextBlock(inputFd, buff))>0){
        xorAndWrite(buff, readBytes, current_level);
        current_level++;
    }
    //finished, handle finished details:
    if ((ret = pthread_mutex_lock(&bufferLock)) != 0){
        printTid();
        LOG(("Error locking 1, beacuse of %d\n", ret));
        exit(2);
    }
    //The level hasn'tt finished yet, we don't want to touch the numThreads:
    if (current_level!=level){
        if ((ret = pthread_cond_wait(&allDoneCond, &bufferLock)) != 0){
            printf("Error waiting for condition variable, %d\n", ret);
            exit(2);
        }
    }
    //This thread isn't the last one in the round after(!) he finished, safely decrement:
    if(workersDone != numThreads-1){
        numThreads--;
        if ((ret = pthread_mutex_unlock(&bufferLock)) != 0){
            printf("Error unlocking, %d\n", ret);
            exit(2);
        }
    } else{ //he is the last one, it needs to right annd safely decrement:
        numThreads--;
        writeToOutputFileAndCleanBuffer();
        if ((ret = pthread_mutex_unlock(&bufferLock)) != 0){
            printf("Error unlocking\n");
            exit(2);
        }
    }
    close(inputFd);
    printTid();
    LOG(( " is quitting\n"));
    pthread_exit(NULL);
}

int readNextBlock(int fd, char* buff){
    int ret= read(fd,buff, BUFFER_SIZE);
    int bytesRead= ret;
    while(bytesRead < BUFFER_SIZE && ret >0){
        ret = read(fd, &(buff[bytesRead]), BUFFER_SIZE- bytesRead);
        bytesRead+=ret;
    }
    //Failed reading, kill everyone:
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
    if (pthread_mutex_lock(&bufferLock) != 0){
        printf("Error locking 2\n");
        exit(2);
    }
    workersDone ++;
    if(length > lengthToWrite){
        lengthToWrite = length;
    }
    //Last one: write to file & inintialuze everything:
    if(workersDone == numThreads){
        writeToOutputFileAndCleanBuffer();
    }
    if (pthread_mutex_unlock(&bufferLock) != 0){
        printf("Error unlocking\n");
        exit(2);
    }
}

/*
 * Xor safely with output:
 */
void xorWithOutput(char* inputBuffer, int length, int current_level){
    if (pthread_mutex_lock(&bufferLock) != 0){
        printf("Error locking 3\n");
        exit(2);
    }
    //Wait until the previous level was done
    if (current_level != level){
        if (pthread_cond_wait(&allDoneCond, &bufferLock) != 0){
            printf("Error waiting for condition variable\n");
            exit(2);
        }
    }
    //Xor:
    printTid();
    LOG((" is xoring\n"));
    for (int i =0; i<length; i++){
        outputBuffer[i] ^= inputBuffer[i];
    }
    if (pthread_mutex_unlock(&bufferLock) != 0){
        printf("Error unlocking\n");
        exit(2);
    }

}


void writeToOutputFileAndCleanBuffer(){
    //write to file:
    printTid();
    LOG(("writing to file\n"));

    int ret = write(outputFD, outputBuffer,lengthToWrite);
    int bytesWritten = ret;
    while (bytesWritten<lengthToWrite && ret>0){
        ret = write(outputFD, &(outputBuffer[bytesWritten]), lengthToWrite-bytesWritten);
        bytesWritten+= ret;
    }
    if (ret < 0 ){
        printTid();
        printf("Error in write, errno %d\n", errno);
        exit(2);
    }
    //initialize buffer (Xor with 0):
    for (int i = 0; i< BUFFER_SIZE; i++){
        outputBuffer[i] = 0;
    }
    //initialize params:
    workersDone = 0;
    total_size+=lengthToWrite;
    lengthToWrite = 0;
    level++;
    //free all those who are waiting:
    ret = pthread_cond_broadcast(&allDoneCond);
    if (ret != 0){
        printTid();
        printf("Error in pthread_cond_broadcast, errno %d\n", errno);
        exit(2);
    }
}

/*
 * Create all threads & initialize locks etc:
 */
void initAndcreateThreads(char **in_file_path){
    if (pthread_attr_init(&attr) != 0){
        printf("error initializing attribute, errno %d\n", errno);
        exit(2);
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0){
        printf("error setting detach state, %d\n", errno);
        exit(2);
    }
    if (pthread_mutex_init(&bufferLock, NULL) != 0){
        printf("error initializing mutex, %d\n", errno);
        exit(2);
    }
    if (pthread_cond_init(&allDoneCond, NULL) != 0){
        printf("error initializing condition, %d\n", errno);
        exit(2);
    }
    threads = (pthread_t*)malloc(numThreads*sizeof(pthread_t));
    if (threads == NULL){
        printf("error allocating memory, %d\n", errno);
        exit(2);
    }
    for (int i=0; i<numThreads; i++){
        if (pthread_create(&(threads[i]), &attr, (void *(*)(void *))&workerMain, in_file_path[i]) != 0){
            printf("error creating thread, error: %d\n", errno);
            exit(2);
        }
    }
}

void waitForThreads(int totalNumberOfThreads){

    for (int i = 0; i < totalNumberOfThreads; i++){
        pthread_join(threads[i],NULL);
    }
    if (pthread_attr_destroy(&attr) != 0){
        printf("error destroying attribute, %d", errno);
        exit(2);
    }
    if(pthread_mutex_destroy(&bufferLock) != 0){
        printf("error destroying mutex, %d", errno);
        exit(2);
    }
    if (pthread_cond_destroy(&allDoneCond) != 0){
        printf("error destroying condition, %d", errno);
        exit(2);
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
    outputFD = open(argv[1], O_CREAT| O_TRUNC | O_WRONLY, 0644);
    if(outputFD < 0){
        printf("failed opening \\ creating output file %s beacuse %d\n", argv[1], errno);
        exit(-1);
    }
    numThreads = argc - 2;
    //call threads & destrooy them:
    initAndcreateThreads(&(argv[2]));
    waitForThreads(argc-2);
    ///cleanup:
    close(outputFD);
    free(threads);
    //bye bye
    printf("Created %s with size %d bytes\n", argv[1],total_size );
    return 0;
}
