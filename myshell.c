#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


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

    int i;
    if(strcmp(arglist[count], "&") == 0){

    }
    for(i=0;i<count;++i){
        if(strcmp(arglist[count], "|") == 0){
            
        }
        if(strcmp(arglist[count], "<") == 0){
            
        }
        if(strcmp(arglist[count], ">>") == 0){
            
        }
    }


    
}




