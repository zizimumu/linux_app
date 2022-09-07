#include <linux/serial.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>


void print_hex(unsigned char *buf,int len)
{
	int i;
	for(i=0;i<len;i++){
		if((i%16) == 0)
			printf("\n");
		printf("  0x%02x",buf[i]);
	}
    printf("\n");
	
}

char buf[1024*1024];
char buf_rd[1024];

int compare_buf(char *buf1, char *buf2, int len)
{
	int i;
	for(i=0;i<len;i++){
		if(*buf1 != *buf2)
			return -1;	
		buf1++;
		buf2++;
	}
	return 0;
}
int main(char argc,char *argv[])  
{
	int read_f = 0;
	int write_f = 0;
	char *arg ;
	int fd,ret;
	int len = 0,i;
	char *buf_tp;
	unsigned int count = 0,reveived = 0;


	//memset(buf,0x55,1024);
	for(i=0;i<1024*1024;i++)
		buf[i] = i;
	
	fd = open("/dev/usb/lp0",O_RDWR);
	if(fd < 0){
		printf("open err\n");
		return 0;
	}
	while(1){
		ret = write(fd,buf,512*16);
		if(ret <= 0)
			printf("send long data err\n");
		else
			reveived += ret;
		
		//usleep(200000);
		
		
		ret = write(fd,buf,302);
		if(ret <= 0)
			printf("send short data err\n");
		else
			reveived += ret;
		
		usleep(20000);
		count++;
		
		if((count % 100) == 0)
			printf("send %d bytes\n",reveived);
		
		
	}
	
	close(fd);


	return 0;
}
