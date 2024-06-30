#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


int pipe(int fds[2]);

int runPipeComm(int pipeInd, char**args);
int runCommInBG(int count, char**args);



// prepare and finalize calls for initialization and destruction of anything required
int prepare(void){
    return 0;
}
int finalize(void){
    return 0;
}

// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should continue, 0 otherwise
int process_arglist(int count, char** arglist){
    
    int i, pid, status, retVal;

    if(strcmp(arglist[count-1], "&") == 0){
        arglist[count-1] = NULL;
        return runCommInBG(count-1,arglist);
    }
    for(i=0;i<count-1;++i){
        if(strcmp(arglist[i], "|") == 0){
            return runPipeComm(i, arglist);
        }
        if(strcmp(arglist[i], "<") == 0){
            
        }
        if(strcmp(arglist[i], ">>") == 0){
            
        }
    }
    
    pid = fork();
    switch(pid){
        case 0: /*Child*/
            retVal = execvp(arglist[0], arglist);
            if(retVal == -1){
                perror("Error: couldn't execute command.\n");
                exit(0);
            }
            break;
        case -1: /*Error*/
            return 0;
        default: /*Parent*/
            if(waitpid(pid, &status, 0) == -1){
                switch(errno){
                    case ECHILD:
                    case EINTR:
                        break;
                    default:
                        return 0;
                }
            }
            break;
    }

    return 1;
}


int runPipeComm(int pipeInd, char**args){
    int pid1,pid2, status;
    int pfds[2];

    args[pipeInd] = NULL;

    if(pipe(pfds) < 0){
        perror("Error: piping failed.\n");
        return 0;
    }


    if((pid1 = fork()) < 0){
        perror("Error: forking failed.\n");
        return 0;

    }else if(pid1 == 0){ /*Left side command*/
        if(dup2(pfds[1], STDOUT_FILENO) < 0){
            perror("Error: failed duping\n");
            exit(0);
        }
        close(pfds[0]);

        if(execvp(args[0], args) < 0){
            perror("Error: couldn't execute command.\n");
            exit(0);
        }

    }else{ /*Parent*/
        if((pid2 = fork()) < 0){
            perror("Error: forking failed.\n");
            return 0;

        }else if(pid2 == 0){ /*Right side command*/
            if(dup2(pfds[0], STDIN_FILENO) < 0){
                perror("Error: failed duping\n");
                exit(0);
            }
            close(pfds[1]);

            if(execvp(args[pipeInd+1], args+pipeInd+1) < 0){
                perror("Error: couldn't execute command.\n");
                exit(0);
            }
        }else{ /*Parent*/
            close(pfds[0]);
            close(pfds[1]);
            while(wait(&status) != pid1);
        }
    }
    
    return 1;
}



int runCommInBG(int count, char**args){
    int pid, retVal;
    
    if((pid = fork()) < 0){
        perror("Failed forking.\n");
        exit(0);
    }

    switch(pid){
        case 0: /*Child*/
            retVal = execvp(args[0], args);
            if(retVal == -1){
                perror("Error: couldn't execute command in background.\n");
                exit(0);
            }
            break;
        default: /*Parent*/
            return 1;
    }
    return 1;
}




