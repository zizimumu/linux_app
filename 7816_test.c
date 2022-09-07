#include <linux/serial.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>


#include <unistd.h>     /*Unix标准函数定义*/
#include <sys/types.h>  /**/
#include <sys/stat.h>   /**/
#include <termios.h>    /*PPSIX终端控制定义*/
#include <errno.h>      /*错误号定义*/
#include <getopt.h>
#include <string.h>


/* Include definition for ISO7816 ioctls: TIOCSISO7816 and TIOCGISO7816 */
#include <sys/ioctl.h>
struct serial_iso7816 {
        __u32   flags;                  /* ISO7816 feature flags */
#define SER_ISO7816_ENABLED             (1 << 0)
#define SER_ISO7816_T_PARAM             (0x0f << 4)
#define SER_ISO7816_T(t)                (((t) & 0x0f) << 4)
        __u32   tg;
        __u32   sc_fi;
        __u32   sc_di;
        __u32   clk;
        __u32   reserved[5];
};

#define TIOCGISO7816    _IOR('T', 0x42, struct serial_iso7816)
#define TIOCSISO7816    _IOWR('T', 0x43, struct serial_iso7816)




int set_Parity(int fd,int databits,int stopbits,int parity,int stall_len)
{
	struct termios options;
	if  ( tcgetattr( fd,&options)  !=  0) {
		perror("SetupSerial 1");
		return(1);
	}
	options.c_cflag &= ~CSIZE ;
	switch (databits) /*设置数据位数*/ {
	case 7:
		options.c_cflag |= CS7;
	break;
	case 8:
		options.c_cflag |= CS8;
	break;
	default:
		fprintf(stderr,"Unsupported data size\n");
		return (1);
	}
	
	switch (parity) {
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* Enable parity checking */
	break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);  /* 设置为奇效验*/
		options.c_iflag |= INPCK;             /* Disnable parity checking */
	break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;     /* Enable parity */
		options.c_cflag &= ~PARODD;   /* 转换为偶效验*/ 
		options.c_iflag |= INPCK;       /* Disnable parity checking */
	break;
	case 'S':	
	case 's':  /*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
	break;
	default:
		fprintf(stderr,"Unsupported parity\n");
		return (1);
	}
 	/* 设置停止位*/  
  	switch (stopbits) {
   	case 1:
    	options.c_cflag &= ~CSTOPB;
  	break;
 	case 2:
  		options.c_cflag |= CSTOPB;
  	break;
 	default:
  		fprintf(stderr,"Unsupported stop bits\n");
  		return (1);
 	}
  	/* Set input parity option */
  	if (parity != 'n')
    	options.c_iflag |= INPCK;
  	options.c_cc[VTIME] = 150; // 15 seconds
    options.c_cc[VMIN] = 0; //stall size 
    //options.c_cc[VMIN] = stall_len; 

	//options.c_lflag &= ~(ECHO | ICANON);
	options.c_cflag |= CLOCAL | CREAD;
	options.c_cflag &= ~CRTSCTS;
	
	
	// options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	//options.c_oflag &= ~OPOST;
	options.c_cflag &= ~(ICRNL | IXON);
    options.c_oflag = 0;
    options.c_lflag = 0;
	options.c_iflag &= ~(BRKINT | INPCK | ISTRIP | ICRNL | IXON);
	//options.c_cflag |= CRTSCTS; //  enable flow crtl


	

  	tcflush(fd,TCIFLUSH); /* Update the options and do it NOW */
  	if (tcsetattr(fd,TCSANOW,&options) != 0) {
    	perror("SetupSerial 3");
  		return (1);
 	}
	return (0);
}









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
unsigned char buf[128];
int main(char argc,char *argv[])  
{
		unsigned int count = 0;
		struct serial_iso7816 iso7816conf;
		char *arg;
		
		int ret;
        char *dev;
		
		if(argc != 3)
		{
			printf("param err\n");
			return 0;
		}
        
        dev = argv[1];
        arg = argv[2];
        /* Open your specific device (e.g., /dev/mydevice): */
        int fd = open (dev, O_RDWR);
        if (fd < 0) {
                /* Error handling. See errno. */
				printf("open err\n");
				return 0;
        }

		if (set_Parity(fd,8,2,'E',0)== 1) {
			printf("Set Parity Error\n");
			close(fd);
			return 0;
		}        

        /* Reserved fields as to be zeroed */
        memset(&iso7816conf, 0, sizeof(iso7816conf));

        /* Enable ISO7816 mode: */
        iso7816conf.flags |= SER_ISO7816_ENABLED;

        /* Select the protocol: */
        /* T=0 */
        iso7816conf.flags |= SER_ISO7816_T(0);
        /* or T=1 */
        //iso7816conf.flags |= SER_ISO7816_T(1);

        /* Set the guard time: */
        iso7816conf.tg = 2;

        /* Set the clock frequency*/
        iso7816conf.clk = 3571200;

        /* Set transmission factors: */
        iso7816conf.sc_fi = 372;
        iso7816conf.sc_di = 1;

        if (ioctl(fd, TIOCSISO7816, &iso7816conf) < 0) {
                /* Error handling. See errno. */
				printf("ioctl TIOCSISO7816 err\n");
				close (fd);
				return 1;
				
        }

		
        
        if(*arg == 'r'){
            printf("start reading\n");
            while(1){
                ret = read(fd,buf,10);
                if(ret > 0){
                    printf("receive %d bytes\n",ret);
                    print_hex(buf,ret);
                }
            }
        }
			
			
        if(*arg == 'w'){ 
            printf("start writing\n");
            while(1)
            {
                    //write(fd, "\x00\x84\x00\x00\x08", 5);
                write(fd, "123456", 6);
                usleep(1000000);
                count++;
                if((count % 10) == 0)
                    printf("send count %d\n",count);
            }
        }
			
        /* Close the device when finished: */
        if (close (fd) < 0) {
        }
}