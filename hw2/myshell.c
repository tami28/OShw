#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

/**
 * Helper Functions:
 *      isPiped - is the command piped annd where
 *      isBackground - ...
 *      execChild - incharge of ececuting a child, with a proper pipe if needed.
 * Main Function:
 *      Execute the command according to BG\FG\PIPED. Waitpid for non BG children.
 */

static const char ERR_DUP[] = "Error in dup, couldn't create pipe correctly.\n";
static const char ERR_EXEC[] =  "Error in execvp, couldn't run command.\n";
static const char ERR_COUNT[] = "Error in count, not enough args.\n";
static const char ERR_PIPE[] = "Error in pipe, couldn't create pipe.\n";
static const char ERR_SIGACTION[] = "Error in sigaction, couldn't define new.\n";
//Check if the command is a piped command:
int isPiped(int length, char ** arglist){
    for (int i=0; i<length; i++){
        if (strcmp(arglist[i], "|") == 0){
            return i;
        }
    }
    return -1;
}

//checks if there is an ampercent in the commands
bool isBackground(int length, char** arglist){
    return (0 == strcmp(arglist[length-1], "&"));
}


void execChild(char** arglist, int* pipeFd, int in){
    //define properly the pipe for the child if it's a | command:
    if (pipeFd != NULL){
        //The second command in the pipe:
        if (in){
            close(pipeFd[1]);
            if (-1 ==dup2(pipeFd[0], 0)){
                //In all prints, according to https://stackoverflow.com/questions/39002052/how-i-can-print-to-stderr-in-c
                //the best way to do this is fprintf..
                fprintf(stderr,ERR_DUP);
                exit(1);
            }
            close(pipeFd[0]);
        } else{
            //The first child:
            close(pipeFd[0]);
            if (-1 ==dup2(pipeFd[1], 1)){
                fprintf(stderr,ERR_DUP);
                exit(1);
            }
            close(pipeFd[1]);

        }
    }
    //Execute the command:
    if (-1 == execvp(arglist[0], arglist)){
        fprintf(stderr, ERR_EXEC);
        exit(1);
    }
}

int process_arglist(int count, char** arglist){
    //Again - taken from stack overflow..
    //Used for children: foreground need to change this.
    struct sigaction dontIgnore;
    memset(&dontIgnore, 0, sizeof(dontIgnore));
    dontIgnore.sa_handler = SIG_DFL;

    // Is legal?
    if (0>= count){
        fprintf(stderr, ERR_COUNT);
        return 1;
    }

    //Set helper vars to know which case we are:
    bool bg  = isBackground(count, arglist);
    int pipeInd = isPiped(count, arglist);

    //Declared ahead of time so that we can use it inside all ifs\else
    int pipeFd[2] = {0};
    //Open pipe if in
    if (-1 != pipeInd){
        if (pipe(pipeFd)){
            fprintf(stderr, ERR_PIPE);
            exit(1);
        }
    }

    pid_t pid2, pid1 = fork();

    // we are in child prtocess
    if (pid1 == 0) {
        if (pipeInd == -1) {
            // First child process id background process:
            if (bg == true) {
                //execvp runs until null, and we don't want it to run on the &:
                arglist[count - 1] = NULL;
                execChild(arglist, NULL, 0);
            }
             //First & only child is normal:
            else {
                sigaction(SIGINT, &dontIgnore, NULL);
                execChild(arglist, NULL, 0);
            }
        }
        //is pipe command, running the first one:
        else{
            sigaction(SIGINT, &dontIgnore, NULL);
            //Change so it will work:
            arglist[pipeInd] = NULL;
            //Execute for the out side of the pipe, the writer
            execChild(arglist, pipeFd, 0);

        }
    }
    //No need to wait for anything:
    if (bg){
        return 1;
    }

    //Fork another child if needed:
    if (pipeInd != -1) {
        pid2 = fork();
        if (pid2 == 0) {
            sigaction(SIGINT, &dontIgnore, NULL);
            //Execute as the reader:
            execChild(&(arglist[pipeInd + 1]), pipeFd, 1);

        }
        //Close refrence to the pipe file descriptor:
        close(pipeFd[0]);
        close(pipeFd[1]);
    }

    int tmp = 0, tmp2 = 0;
    //Wait for one foreground child:
    if (-1 == pipeInd ) {
        if (waitpid(pid1, &tmp, 0) == pid1) {
            return 1;
        }
    }
    //Wait for two foreground children:
    else{
        if ((waitpid(pid1, &tmp, 0) == pid1) && (waitpid(pid2, &tmp2, 0) == pid2)){
            return 1;
        }
    }
    return 1;
}

// prepare and finalize calls for initialization and destruction of anything required

int prepare(void){
    //Found in stack overflow:
    //according to wiki, https://en.wikipedia.org/wiki/Child_process, If we specificlly set
    //SIGCHILD to SIG_IGN , it reaps dead children automatically, leaving no zombies.
    struct sigaction ignore;
    memset(&ignore, 0, sizeof(ignore));
    ignore.sa_handler = SIG_IGN;
    if (0 != sigaction(SIGCHLD, &ignore, NULL)){
        fprintf(stderr, ERR_SIGACTION);
        exit(1);
    }
    if(0 != sigaction(SIGINT, &ignore, NULL)){
        fprintf(stderr, ERR_SIGACTION);
        exit(1);
    }
    return 0;
}


int finalize(void){
    return 0;
}
