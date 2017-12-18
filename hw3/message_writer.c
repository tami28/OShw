//
// Created by root on 09/12/2017.
//
#include "message_slot.h"
#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Missing arguments. Should call with <message slot file path> <target message channel> <message to pass>");
    }
    int fd = open(argv[1], O_RDWR);
    if (fd < 0){
        printf("Couldn't open the file, errno %d\n", errno);
        return -1;
    }
    int channel = atoi( argv[2] );
    int ret_val;
    ret_val = ioctl( fd, MSG_SLOT_CHANNEL,channel );
    if (ret_val < 0){
        printf("Error in ioctl: %d", errno);
    }
    ret_val = write( fd, argv[3], strlen(argv[3]));
    if (ret_val != SUCCESS){
        printf("Failed writing");
    }
    close(fd);
    return 0;

}
