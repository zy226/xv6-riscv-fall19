#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
 
void execPipe(char*argv[],int argc);

 //摘自sh
#define MAXARGS 10
#define MAXWORD 30

int getcmd(char *buf, int nbuf)
{
    fprintf(2, "@ ");
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if (buf[0] == 0) // EOF
        return -1;
    return 0;
}
char whitespace[] = " \t\r\n\v";
char args[MAXARGS][MAXWORD];
//end

void setargs(char *cmd, char* argv[],int* argc)
{
    // 让argv的每一个元素都指向args的每一行
    for(int i=0;i<MAXARGS;i++){
        argv[i]=&args[i][0];
    }
    int i = 0; 
    int j = 0;
    while (cmd[j] != '\n' && cmd[j] != '\0')
    {
        //跳过空格
        while(strchr(whitespace,cmd[j])){
            j++;
        }
        argv[i++]=cmd+j;//地址
        while(!strchr(whitespace,cmd[j])){
            j++;
        }
        cmd[j]='\0';
        j++;
    }
    argv[i]=0;
    *argc=i;
}
 
void runcmd(char*argv[],int argc)
{
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"|")){
            // 遇到 | 即pipe,至少还有一个指令
            execPipe(argv,argc);
        }
        // 遇到 > ，执行输出重定向，关闭stdout
        if(!strcmp(argv[i],">")){
            close(1);
            open(argv[i+1],O_CREATE|O_WRONLY);
            argv[i]=0;
        }
        // 遇到< ,执行输入重定向，关闭stdin
        if(!strcmp(argv[i],"<")){
            close(0);
            open(argv[i+1],O_RDONLY);
            argv[i]=0;
        }
    }
    exec(argv[0], argv);
}
 
void execPipe(char*argv[],int argc){
    int i=0;
    // 找到命令中首个"|",替换成'\0'，后面递归调用
    for(;i<argc;i++){
        if(!strcmp(argv[i],"|")){
            argv[i]=0;
            break;
        }
    }
    int fd[2];
    pipe(fd);
    //执行命令 子进程左边，父进程右边
    if(fork() == 0){
        close(1);
        dup(fd[1]);
        close(fd[0]);
        close(fd[1]);
        runcmd(argv,i);
    }else{
        close(0);
        dup(fd[0]);
        close(fd[0]);
        close(fd[1]);
        runcmd(argv+i+1,argc-i-1);
    }
}

int main()
{
    char buf[100];

    // Read and run input commands.
    while (getcmd(buf, sizeof(buf)) >= 0)
    {
        if (fork() == 0)
        {
            char* argv[MAXARGS];
            int argc=-1;
            setargs(buf, argv,&argc);
            runcmd(argv,argc);
        }
        wait(0);
    }
    exit(0);
}