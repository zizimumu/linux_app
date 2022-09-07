#include <stdio.h>      /*标准输入输出定义*/
#include <stdlib.h>
#include <unistd.h>     /*Unix标准函数定义*/
#include <sys/types.h>  /**/
#include <sys/stat.h>   /**/
#include <fcntl.h>      /*文件控制定义*/
#include <termios.h>    /*PPSIX终端控制定义*/
#include <errno.h>      /*错误号定义*/
#include <getopt.h>
#include <string.h>

#define FALSE 1
#define TRUE 0

char *recchr="We received:\"";

void print_usage();

int speed_arr[] = { 
	B921600, B460800, B230400, B115200, B57600, B38400, B19200, 
	B9600, B4800, B2400, B1200, B300, 
};

int name_arr[] = {
	921600, 460800, 230400, 115200, 57600, 38400,  19200,  
	9600,  4800,  2400,  1200,  300,  
};

void set_speed(int fd, int speed)
{
	int   i;
	int   status;
	struct termios   Opt;
	tcgetattr(fd, &Opt);

	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) {
		if  (speed == name_arr[i])	{
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status = tcsetattr(fd, TCSANOW, &Opt);
			if  (status != 0)
				perror("tcsetattr fd1");
				return;
		}
		tcflush(fd,TCIOFLUSH);
  	 }

	if (i == 12){
		printf("\tSorry, please set the correct baud rate!\n\n");
		print_usage(stderr, 1);
	}
}
/*
	*@brief   设置串口数据位，停止位和效验位
	*@param  fd     类型  int  打开的串口文件句柄*
	*@param  databits 类型  int 数据位   取值 为 7 或者8*
	*@param  stopbits 类型  int 停止位   取值为 1 或者2*
	*@param  parity  类型  int  效验类型 取值为N,E,O,,S
*/
int set_Parity(int fd,int databits,int stopbits,int parity)
{
	struct termios options;
	if  ( tcgetattr( fd,&options)  !=  0) {
		perror("SetupSerial 1");
		return(FALSE);
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
		return (FALSE);
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
		return (FALSE);
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
  		return (FALSE);
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
  		return (FALSE);
 	}
	return (TRUE);
}

/**
	*@breif 打开串口
*/
int OpenDev(char *Dev)
{
	int fd = open( Dev, O_RDWR );         //| O_NOCTTY | O_NDELAY
 	if (-1 == fd) { /*设置数据位数*/
   		perror("Can't Open Serial Port");
   		return -1;
	} else
		return fd;
}


/* The name of this program */
const char * program_name;

/* Prints usage information for this program to STREAM (typically
 * stdout or stderr), and exit the program with EXIT_CODE. Does not
 * return.
 */

void print_usage (FILE *stream, int exit_code)
{
    fprintf(stream, "Usage: %s option [ dev... ] \n", program_name);
    fprintf(stream,
            "\t-h  --help     Display this usage information.\n"
            "\t-d  --device   The device ttyS[0-3] or ttySCMA[0-1]\n"
	    "\t-b  --baudrate Set the baud rate you can select\n" 
	    "\t               [230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300]\n"
            "\t-s  --string   Write the device data\n");
    exit(exit_code);
}



/*
	*@breif  main()
 */
int main(int argc, char *argv[])
{
    
    int fd1,fd2,fd3,fd4,fd5,fd6,fd7,fd8,fd9;
    
    fd1 = open("/dev/ttyS1", O_RDWR );
    fd1 = open("/dev/ttyS2", O_RDWR );
    fd1 = open("/dev/ttyS3", O_RDWR );
    fd1 = open("/dev/ttyS4", O_RDWR );
    fd1 = open("/dev/ttyS5", O_RDWR );
    fd1 = open("/dev/ttyS6", O_RDWR );
    fd1 = open("/dev/ttyS7", O_RDWR );
    fd1 = open("/dev/ttyS8", O_RDWR );
    fd1 = open("/dev/ttyS9", O_RDWR );

	while(1);
	exit(0);
}

