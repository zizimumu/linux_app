#include <stdio.h>      /*标准输入输出定义*/
#include <pthread.h>

#include <stdlib.h>
#include <unistd.h>     /*Unix标准函数定义*/
#include <sys/types.h>  /**/
#include <sys/stat.h>   /**/
#include <fcntl.h>      /*文件控制定义*/
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
	unsigned int cnt = 0;
	int fd;
	char buf = '0';
	int	err; 
    char *pt;
    
    pt = argv[1];

	printf("\nmain pid=%lu\n", getpid());


    fd = open("/dev/sys_test",O_RDWR);
    if(fd <0){
        printf("open file error\n");
        return 0;
    }
    
    if(*pt == 'r'){
        read(fd,&buf,1);
    }else{
        write(fd,&buf,1);
    }   
    close(fd);
	
	return 0;
}
