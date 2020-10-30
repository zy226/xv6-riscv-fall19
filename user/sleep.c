#include "kernel/types.h"
#include "user/user.h"

int main(int argn, char *argv[]){
	if(argn != 2){
		fprintf(2, "must 1 argument for sleep\n");
		exit(1);
	}
	int sleepNum = atoi(argv[1]);
	printf("Sleep %d\n",sleepNum);
	sleep(sleepNum);
	exit(0);
}
