#include "kernel/types.h"
#include "user/user.h"
/*
void func(int *input, int num){
	if(num == 1){
		printf("prime %d\n", *input);
		return;
	}
	int p[2],i;
	int prime = *input;
	int temp;
	printf("prime %d\n", prime);
	pipe(p);
    if(fork() == 0){
        for(i = 0; i < num; i++){
            temp = *(input + i);
			write(p[1], (char *)(&temp), 4);
		}
        exit(0);
    }
	close(p[1]);
	if(fork() == 0){
		int counter = 0;
		char buffer[4];
		while(read(p[0], buffer, 4) != 0){
			temp = *((int *)buffer);
			if(temp % prime != 0){
				*input = temp;
				input += 1;
				counter++;
			}
		}
		func(input - counter, counter);
		exit(0);
    }
	wait(0);
	wait(0);
}

int main(int argc, char *argv[]){
    int input[34];
	int i = 0;
    while(i<34){
        input[i] = i+2;
        i++;
    }
	func(input, 34);
    exit(0);
}
*/
void source() {
  int i;
  for (i = 2; i < 36; i++) {
    write(1, &i, sizeof(i));
  }
}

void cull(int p) {
  int n;
  while (read(0, &n, sizeof(n))) {
    if (n % p != 0) {
      write(1, &n, sizeof(n));
    }
  }
}

void redirect(int k, int pd[]) {
  close(k);
  dup(pd[k]);
  close(pd[0]);
  close(pd[1]);
}

void sink() {
  int pd[2];
  int p;
  if (read(0, &p, sizeof(p))) {
    printf("prime %d\n", p);
    pipe(pd);
    if (fork()) {
      redirect(0, pd);
      sink();
    } else {
      redirect(1, pd);
      cull(p);
    }
  }
}

int main(int argc, char *argv[]) {

  int pd[2];
  pipe(pd);
  if (fork() == 0) {
    redirect(1, pd);
    source();
  }
  else{
    redirect(0, pd);
    sink();
  }
  exit(0);
}