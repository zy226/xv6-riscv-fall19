#include <kernel/types.h>
#include <user/user.h>

int main(int argc,char *argv[]){
    int child_fd[2],parent_fd[2];
    char buf[64];
    long length = sizeof(buf);
    pipe(child_fd);
    pipe(parent_fd);
    //子进程
    if(fork() == 0){
        //写入parent_fd[1]
        if(write(parent_fd[1], buf, length) != length){
			exit(1);
		}
        //读取child_fd[0]接收字节
		if(read(child_fd[0], buf, length) != length){
			exit(1);
		}
		printf("%d: received ping\n", getpid());
        exit(0);
    }
    //父进程
    //写入child_fd[1]
	if(write(child_fd[1], buf, length) != length){
		exit(1);
	}
    //读取parent_fd[0]接收字节
	if(read(parent_fd[0], buf, length) != length){
		exit(1);
	}
	printf("%d: received pong\n", getpid());
    wait(0);
	exit(0);
}
