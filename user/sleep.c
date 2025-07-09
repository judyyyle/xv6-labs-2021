#include "kernel/types.h"
#include "user/user.h"

int 
main(int argc,char *argv[]){
    // incorrect usage
    if(argc < 2){
    	fprintf(2, "Usage: sleep <ticks>\n");
        exit(1);
	}

    // invalid time
    int ticks = atoi(argv[1]);
    if(ticks <= 0){
        fprintf(2, "invalid number of ticks.\n");
        exit(1);
    }

    sleep(ticks);
    exit(0);
}