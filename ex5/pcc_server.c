//
// Created by root on 15/01/2018.
//
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pthread.h>

#define HELP_MSG "Should be called with <port>\n"
#define ERROR_MSG "Error: %s\n"
#define UINT_LEN 4
#define PRINTABLES_LEN 95
#define PRINTABLES_MIN 32
#define PRINTABLES_MAX 126
#define BUFF_LEN 16384

pthread_mutex_t countersLock, activeThreadsLock;
pthread_cond_t  allDoneCond;
pthread_attr_t attr;
unsigned int counters[PRINTABLES_LEN];
int active_threads = 0;
bool sigint_happend = false;

int sendUint(int socket_fd,unsigned int N ){
    char * charN = (char*)&N;
    int ret = write(socket_fd, charN, UINT_LEN);
    int written = ret;
    while (written< UINT_LEN && ret > 0){
        ret = write(socket_fd, &(charN[written]), UINT_LEN-written);
        written+= ret;
    }
    if (ret < 0 ){
        printf(ERROR_MSG, strerror(errno));
        return -1;
    }
    return 0;
}

int readUint(int socket_fd, unsigned int * C){
    char charC[UINT_LEN];
    int bytesRead = read(socket_fd, charC, UINT_LEN);
    int ret = bytesRead;

    while(bytesRead < UINT_LEN && ret >0){
        ret = read(socket_fd, &(charC[bytesRead]), UINT_LEN- bytesRead);
        bytesRead+=ret;
    }
    if (ret <0){
        printf(ERROR_MSG, strerror(errno));
        return -1;
    }
    *C = *(unsigned int *)charC;
    return 0;
}


int bindAndListen(int listen_fd, unsigned int port){
    struct sockaddr_in serv_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );
    memset( &serv_addr, 0, addrsize );
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    if( 0 != bind( listen_fd,(struct sockaddr*) &serv_addr,addrsize ) )
    {
        printf(ERROR_MSG, strerror(errno));
        return -1;
    }
    if( 0 != listen( listen_fd, 10 ) )
    {
        printf(ERROR_MSG, strerror(errno));
        return -1;
    }


}


//TODO: should only be once ,do for a temp counters arr then add that to the global one
void parsPrintables(unsigned int localCounters[PRINTABLES_LEN]) {
    for (int i =0; i < PRINTABLES_LEN; i++){
        __sync_fetch_and_add(&(counters[i]), localCounters[i]);
    }
}

unsigned int parsBuff(unsigned int localCounters[PRINTABLES_LEN], char buff[BUFF_LEN], int size){
    unsigned int ret =0;
    for (int i=0; i<PRINTABLES_LEN; i++){
        if (buff[i] >= PRINTABLES_MIN && buff[i] <= PRINTABLES_MAX){
            localCounters[buff[i] - PRINTABLES_MIN]++;
            ret++;
        }
    }
    return ret;
}



void updateActiveThreads(int i){

    __sync_fetch_and_add(&active_threads, i);
}

void connectionMain(int connection_fd){
    unsigned int N;
    char buff[BUFF_LEN];
    int ret =0, totalRead=0;
    unsigned int totalPrintable = 0, temp;
    unsigned int localCounter[PRINTABLES_LEN];
    memset(localCounter,0, PRINTABLES_LEN*sizeof(unsigned int));
    //read N:
    if (readUint(connection_fd, &N)){
        exit(-1);
    }
    // read the stream:
    ret = read(connection_fd, buff, BUFF_LEN);
    totalRead+= ret;
    while (totalRead<N && ret >0){
        temp = parsBuff(localCounter, buff, ret);
        if (temp < 0){
            exit(-1);
        }
        totalPrintable+= temp;
        ret =read(connection_fd,buff,BUFF_LEN);
        totalRead+= ret;
    }
    if (ret < 0){
        printf(ERROR_MSG, strerror(errno));
        exit(-1);
    }
    //Send C:
    if (sendUint(connection_fd, totalPrintable)){
        exit(-1);
    }
    //clean up:
    close(connection_fd);
    updateActiveThreads(-1);
}



void initLocks(){
    if (pthread_attr_init(&attr) != 0){
        printf(ERROR_MSG, strerror(errno));
        exit(-1);
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0){
        printf(ERROR_MSG, strerror(errno));
        exit(-1);
    }
    if (pthread_mutex_init(&activeThreadsLock, NULL) != 0){
        printf(ERROR_MSG, strerror(errno));
        exit(-1);
    }

    if (pthread_mutex_init(&countersLock, NULL) != 0){
        printf(ERROR_MSG, strerror(errno));
        exit(-1);
    }
    if (pthread_cond_init(&allDoneCond, NULL) != 0){
        printf(ERROR_MSG, strerror(errno));
        exit(-1);
    }
}

void destroyLocks(){
    if (pthread_attr_destroy(&attr) != 0){
        printf(ERROR_MSG, strerror(errno));
        exit(-1);
    }
    if(pthread_mutex_destroy(&countersLock) != 0){
        printf(ERROR_MSG, strerror(errno));
        exit(-1);
    }
    if(pthread_mutex_destroy(&activeThreadsLock) != 0){
        printf(ERROR_MSG, strerror(errno));
        exit(-1);
    }
    if (pthread_cond_destroy(&allDoneCond) != 0){
        printf(ERROR_MSG, strerror(errno));
        exit(-1);
    }
}

void printCounters(){
    for (int i =0; i< PRINTABLES_LEN; i++){
        printf("char '%c' : %u times\n", i+ PRINTABLES_MIN, counters[i]);
    }
}


void cleanup(){

    //TODO: what if some thread was called just before sigint, and didn't update active threads before this?
    //TODO: need to think how to manage it properly.
    if (pthread_mutex_lock(&active_threads) != 0){
        printf("Error locking\n");
        exit(2);
    }
    //Wait until all threads are done:
    if (active_threads){
        if (pthread_cond_wait(&allDoneCond, &active_threads) != 0){
            printf("Error waiting for condition variable\n");
            exit(2);
        }
    }
    if (pthread_mutex_unlock(&active_threads) != 0){
        printf("Error unlocking\n");
        exit(2);
    }
    //if we got here all threads are done.
    printCounters();
    destroyLocks();
}


int main(int argc, char** argv) {
    int listen_fd = -1;
    int connect_fd = -1;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );
    if (argc != 2){
        printf(HELP_MSG);
        exit(-1);
    }
    //open socket:
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if( listen_fd < 0){
        printf(ERROR_MSG, strerror(errno));
        exit(-1);
    }
    unsigned int port= atoi(argv[1]);
    if (bindAndListen(listen_fd, port)){
        exit(-1);
    }
    initLocks();
    while( !sigint_happend  ){
        updateActiveThreads(1);
        connect_fd = accept( listen_fd,(struct sockaddr*) &peer_addr,&addrsize);
        if( connect_fd < 0 ) {
            printf(ERROR_MSG, strerror(errno));
            exit(-1);
        }
    }

    cleanup();
    //TODO: if sigint happend in the middle of accepting connection should accept it?
    //TODO change threads SIGINT handler to none
    //TODO write sigint handler for this
    //TODO setsockopt with option SO_REUSEADDR
    //TODO: each thread should ignore sigints, main thread should call specific function.
    close(listen_fd);
    return 0;
}