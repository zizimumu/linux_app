#include <stdio.h>      /*标准输入输出定义*/
#include <pthread.h>

#include <stdlib.h>
#include <unistd.h>     /*Unix标准函数定义*/
#include <sys/types.h>  /**/
#include <sys/stat.h>   /**/
#include <fcntl.h>      /*文件控制定义*/
#include <sys/types.h>
#include <unistd.h>

int a = 0x11223344;
int b = 0x55667788;
pthread_t ntid;


#define TIME_CNT 30000000
void *thr_fn(void *arg)
{
	unsigned int cnt = 0;
	int fd;
	char buf = '0';

	printf("new thread running\n");

	while(1){

		fd = open("/sys/devices/virtual/misc/sys_test/uart_gpio",O_RDWR);
		write(fd,&buf,1);
		close(fd);
		cnt++;
		if((cnt % TIME_CNT) == 0)
			printf("thread running\n");
	}	
	return((void *)0);
}


int main(void )
{
	unsigned int cnt = 0, cnt1 = 0;

	int	err; 
char buf = '0';
int fd;

	printf("\nmain pid=%lu\n", getpid());


	err = pthread_create(&ntid, NULL, thr_fn, NULL); 
	if (err != 0) 
		printf("can't create thread\n");




	while(1){
		cnt++;
		if((cnt % TIME_CNT) == 0){
			printf("main running\n");
			cnt1++;
			if(cnt1 == 3){
				printf("main write\n");
				fd = open("/sys/devices/virtual/misc/sys_test/uart_gpio",O_RDWR);
				write(fd,&buf,1);
				close(fd);
			}
		}
	}
	
	return 0;
}
