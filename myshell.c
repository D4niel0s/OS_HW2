#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


int pipe(int fds[2]);

int runAppendComm(int,char **);
int runPipeComm(int,char**);
int runRedirectComm(int,char **);
int runCommInBG(int,char**);



// prepare and finalize calls for initialization and destruction of anything required
int prepare(void){
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    return 0;
}
int finalize(void){
    return 0;
}

// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should continue, 0 otherwise
int process_arglist(int count, char** arglist){
    int i, pid, waitRet, retVal;

    /*Check BG command*/
    if(strcmp(arglist[count-1], "&") == 0){
        return runCommInBG(count-1,arglist);
    }

    /*Check for modifiers and handle with corresponding function*/
    for(i=0;i<count-1;++i){
        if(strcmp(arglist[i], "|") == 0){
            return runPipeComm(i, arglist);
        }
        if(strcmp(arglist[i], "<") == 0){
            return runRedirectComm(i, arglist);
        }
        if(strcmp(arglist[i], ">>") == 0){
            return runAppendComm(i, arglist);
        }
    }
    
    /*If we got here, arglist represents a "regular" command we should execute*/
    pid = fork();
    if(pid < 0){
        perror("Error: forking failed\n");
        return 0;

    }else if(pid == 0){ /*Child*/
        signal(SIGINT, SIG_DFL);

        retVal = execvp(arglist[0], arglist);
        if(retVal == -1){
            perror("Error: couldn't execute command.\n");
            exit(1);
        }

    }else{ /*Parent*/
        waitRet = waitpid(pid, NULL, 0);
        if(waitRet == -1){
            switch(errno){
                case ECHILD:
                case EINTR:
                    break;
                default:
                    return 0;
            }
        }
    }
    return 1;
}


int runAppendComm(int appInd, char **args){
    int fd, pid, retVal,dupRet, clsRet, waitRet;

    args[appInd] = NULL;

    pid = fork();

    if(pid<0){
        perror("Error: could not fork\n");
        return 0;
    
    }else if(pid == 0){ /*Child*/
        signal(SIGINT, SIG_DFL);
        fd = open(args[appInd+1], O_WRONLY | O_APPEND | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        if(fd<0){
            perror("Error: could not open file\n");
            exit(1);
        }

        dupRet = dup2(fd, STDOUT_FILENO);
        if(dupRet < 0){
            perror("Error: could not redirect file descriptor\n");
            exit(1);
        }

        clsRet = close(fd);
        if(clsRet < 0){
            perror("Error: failed closing file\n");
            exit(1);
        }

        retVal = execvp(args[0],args);
        if(retVal < 0){
            perror("Error: could not execute command\n");
            exit(1);
        }

    }else{ /*Parent*/
        waitRet = waitpid(pid, NULL, 0);
        if(waitRet == -1){
            switch(errno){
                case ECHILD:
                case  EINTR:
                    break;
                default:
                    return 0;
            }
        }
    }

    return 1;
}

int runRedirectComm(int revInd, char **args){
    int fd, pid, retVal,dupRet, clsRet, waitRet;

    args[revInd] = NULL;

    pid = fork();

    if(pid<0){
        perror("Error: could not fork\n");
        return 0;
    
    }else if(pid == 0){ /*Child*/
        signal(SIGINT, SIG_DFL);
        fd = open(args[revInd+1], O_RDONLY);
        if(fd<0){
            perror("Error: could not open file\n");
            exit(1);
        }

        dupRet = dup2(fd, STDIN_FILENO);
        if(dupRet < 0){
            perror("Error: could not redirect file descriptor\n");
            exit(1);
        }

        clsRet = close(fd);
        if(clsRet < 0){
            perror("Error: failed closing file\n");
            exit(1);
        }

        retVal = execvp(args[0],args);
        if(retVal < 0){
            perror("Error: could not execute command\n");
            exit(1);
        }

    }else{ /*Parent*/
        waitRet = waitpid(pid, NULL, 0);
        if(waitRet == -1){
            switch(errno){
                case ECHILD:
                case  EINTR:
                    break;
                default:
                    return 0;
            }
        }
    }

    return 1;
}

int runPipeComm(int pipeInd, char**args){
    int pid1,pid2, retVal,dupRet, clsRet, waitRet1, waitRet2;
    int pfds[2];

    args[pipeInd] = NULL;

    if(pipe(pfds) < 0){
        perror("Error: piping failed.\n");
        return 0;
    }

    pid1 = fork();
    if(pid1 < 0){
        perror("Error: forking failed.\n");
        return 0;

    }else if(pid1 == 0){ /*Left side command*/
        signal(SIGINT, SIG_DFL);

        clsRet = close(pfds[0]);
        if(clsRet < 0){
            perror("Error: failed closing file\n");
            exit(1);
        }

        dupRet = dup2(pfds[1], STDOUT_FILENO);
        if(dupRet < 0){
            perror("Error: could not redirect file descriptor\n");
            exit(1);
        }

        clsRet = close(pfds[1]);
        if(clsRet < 0){
            perror("Error: failed closing file\n");
            exit(1);
        }

        retVal = execvp(args[0], args);
        if(retVal<0){
            perror("Error: couldn't execute command\n");
            exit(1);
        }
    
    }else{ /*Parent*/
        pid2 = fork();
        if(pid2 < 0){
            perror("Error: forking failed.\n");
            return 0;

        }else if(pid2 == 0){ /*Right side command*/
            signal(SIGINT, SIG_DFL);

            clsRet = close(pfds[1]);
            if(clsRet < 0){
                perror("Error: failed closing file\n");
                exit(1);
            }

            dupRet = dup2(pfds[0], STDIN_FILENO);
            if(dupRet < 0){
                perror("Error: could not redirect file descriptor\n");
                exit(1);
            }

            clsRet = close(pfds[0]);
            if(clsRet < 0){
                perror("Error: failed closing file\n");
                exit(1);
            }

            retVal = execvp(args[pipeInd+1], args+pipeInd+1);
            if(retVal<0){
                perror("Error: couldn't execute command\n");
                exit(1);
            }
        }else{ /*Parent*/
            clsRet = close(pfds[0]);
            if(clsRet < 0){
                perror("Error: failed closing file\n");
                exit(1);
            }

            clsRet = close(pfds[1]);
            if(clsRet < 0){
                perror("Error: failed closing file\n");
                exit(1);
            }
            
            waitRet1 = waitpid(pid1, NULL,0);
            if(waitRet1 < 0){
                switch(errno){
                    case ECHILD:
                    case EINTR:
                        break;
                    default:
                        return 0;
                }
            }

            waitRet2 = waitpid(pid2, NULL,0);
            if(waitRet2 < 0){
                switch(errno){
                    case ECHILD:
                    case EINTR:
                        break;
                    default:
                        return 0;
                }
            }

        }
    }
    
    return 1;
}



int runCommInBG(int count, char**args){
    int pid, retVal;

    args[count] = NULL;

    pid = fork();
    if(pid < 0){
        perror("Failed forking.\n");
        return 0;

    }else if(pid == 0){ /*Child*/
        retVal = execvp(args[0], args);
        if(retVal == -1){
            perror("Error: couldn't execute command in background.\n");
            exit(1);
        }
    }
    /*Parent does nothing here*/
    return 1;
}