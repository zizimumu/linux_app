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

char buf[1024];
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
    char *dev;
	unsigned int count = 0;
	if(argc != 4){
		printf("param err \n");
		return 0;	
	}

    dev = argv[1];
	arg = argv[2];
	len = atoi(argv[3]);
	memset(buf,0x55,1024);
	fd = open(dev,O_RDWR);
	if(fd < 0){
		printf("open err\n");
		return 0;
	}

	if(*arg == 'r'){
		printf("start read %d bytes\n",len);
		buf_tp = buf;
		for(i=0;i<len;i++){
			ret = read(fd,buf_tp,1);
			//usleep(1000);
			buf_tp++;
		}
		print_hex(buf,len);
	}
	if(*arg == 'w'){
        
        printf("sending \n");
		while(1){
            ret = write(fd,buf,len);
            usleep(1000);
        }
		printf("start write %d bytes,ret %d\n",len,ret);
		
	}
	if(*arg == 'b'){
		write(fd,buf,len);
		usleep(1);
		ret = read(fd,buf_rd,len);
		if(ret != len || compare_buf(buf,buf_rd,ret)){
			printf("data check err,rev data %d:\n",ret);
			print_hex(buf_rd,ret);		
		}
		else{
			count++;
			if(count % 500)
				printf("ckeck ok %d\n",count);
		}
		
	}
	close(fd);


	return 0;
}
