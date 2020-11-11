#include <kernel/types.h>
#include <user/user.h>

int main(int argc,char *argv[]){
    int child_fd[2],parent_fd[2];
    char buf[10];
    long length = sizeof(buf);
    pipe(child_fd);
    pipe(parent_fd);
    //子进程
    if(fork() == 0){
        //读取child_fd[0]接收字节
        read(child_fd[0], buf, length);
        printf("%d: received %s\n", getpid(),buf);
        //写入parent_fd[1]  
        write(parent_fd[1], "pong", 4);
        exit();
    }
    //父进程
    //写入child_fd[1]
	write(child_fd[1], "ping", 4);
    //读取parent_fd[0]接收字节
    read(parent_fd[0], buf, length);
	printf("%d: received %s\n", getpid(),buf);
	exit();
}
