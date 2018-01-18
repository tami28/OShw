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



#define HELP_MSG "Not enough args. Please enter <hostname\\IP> <port> <Length> excatly."
#define ERROR_MSG "Error: %s\n"

//#define INPUT_FILE "/dev/urandom"
#define INPUT_FILE "/home/tami/src/ex5/peter_pan.txt"
#define TRUE 1
#define FALSE 0
#define BUFF_LEN 16384
#define UINT_LEN 4

#define PRINT_ERR(ret) if ((ret) == -1){printf(ERROR_MSG, strerror(errno));return -1;}

int isPossibleIP(char* host){
    char* curr = host;
    while (*curr !='\0'){
        if (!((*curr <= '9' && *curr >= '0') || (*curr == '.'))){
            return FALSE;
        }
        ++curr;
    }
    return TRUE;
}

int openConnection(char* host, char* port, int socket_fd){
    //first try host as ip:
    int is_ip = isPossibleIP(host);
    int ret;
    //for hostname, used example from http://man7.org/linux/man-pages/man3/getaddrinfo.3.html
    //possible ip, use the socket struct:
    if (is_ip){
        struct sockaddr_in addr;
        int prt_num = atoi(port);
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(prt_num);
        if (inet_aton(host, &addr.sin_addr) == 0) {
            printf(ERROR_MSG, strerror(errno));
            exit(-1);
        }
        ret = connect(socket_fd,(struct sockaddr*) &addr,sizeof(addr));
        //Maybe isn't ip after all:
        if (ret <0){
            is_ip = FALSE;
        }
    }
    //wasn't an ip, try converting from host name:
    if (!is_ip){
        struct addrinfo hints;
        struct addrinfo *result;
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_flags = 0;
        hints.ai_protocol = 0;          /* Any protocol */
        ret = getaddrinfo(host, port, &hints, &result);
        //also this didn't work! no proper host.
        PRINT_ERR(ret)
        ret = connect(socket_fd, result->ai_addr, result->ai_addrlen); //todo : loop here
        PRINT_ERR(ret)
        freeaddrinfo(result);
    }
    return 0;
}

int sendData(int socket_fd,int N ){
    int input_fd = open(INPUT_FILE, O_RDONLY);
    char buff [BUFF_LEN];
    int ret = 0, toRead = 0, bytesRead = 0;
    int written = 0;
    //open file:
    if (!input_fd){
        printf(ERROR_MSG, strerror(errno));
        return 1;
    }
    //read & write loop:
    while (written < N){
        //Read:
        toRead  = BUFF_LEN;
        if (N-written < BUFF_LEN){
            toRead = N-written;
        }
        ret =read(input_fd,buff,toRead);
        bytesRead = ret;
        while(bytesRead < toRead && ret >0){
            ret = read(input_fd, &(buff[bytesRead]), BUFF_LEN- bytesRead);
            bytesRead+=ret;
        }
        if (ret < 0){
            printf(ERROR_MSG, strerror(errno));
            return -1;
        }
        //Write:
        ret = write(socket_fd, buff,toRead);
        int bytesWritten = ret;
        while (bytesWritten<toRead && ret>0){
            ret = write(socket_fd, &(buff[bytesWritten]), toRead-bytesWritten);
            bytesWritten+= ret;
        }
        if (ret < 0 ){
            printf(ERROR_MSG, strerror(errno));
            return -1;
        }
        written+= toRead;
    }
    close(input_fd);
    return 0;
}

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

int readUint(int socket_fd){
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
    unsigned int C = *(unsigned int *)charC;
    printf("# of printable characters: %u\n", C);
    return 0;
}

int main(int argc, char** argv) {
    int socket_fd;
    if (argc != 4){
        printf(HELP_MSG);
        return 1;
    }

    //open socket:
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if( socket_fd < 0)
    {
        printf(ERROR_MSG, strerror(errno));
        return 1;
    }
    if (openConnection(argv[1], argv[2], socket_fd)){
        return -1;
    }
    unsigned int N= atoi(argv[3]);
    if(sendUint(socket_fd,N)){
        return -1;
    }
    if (sendData(socket_fd, N)){
        return -1;
    }
    if(readUint(socket_fd)){
        return -1;
    }
    close(socket_fd);
    return 0;
}


//TODO: start by sending N, then random, then read until