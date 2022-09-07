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
#include <signal.h>
#include <time.h>
#include <poll.h>  

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
 
 struct itimerspec ts;
timer_t timer;
int rev_len[1024];
int rev_cnt = 0;
int pre_cnt = 0;

void  handle()
{
    int i;
    
    if(rev_cnt != pre_cnt ){
        printf("\n");
        for(i=0;i<rev_cnt;i++){
            printf("%d ",rev_len[i]);
        }
        printf("\n");
    }
    
    pre_cnt = rev_cnt;
    
}

char buff[4096];
int main(int argc, char *argv[])
{
	int  fd, next_option, havearg = 0;
	char *device;
	int i=0,j=0;
	int nread;			/* Read the counts of data */
			/* Recvice data buffer */
	pid_t pid;
	char *xmit = "1234567890"; /* Default send data */ 
	int speed ;
	const char *const short_options = "hd:s:b:";
	struct sigevent evp;
    struct pollfd fds;  

	int ret;
    
	const struct option long_options[] = {
		{ "help",   0, NULL, 'h'},
		{ "device", 1, NULL, 'd'},
		{ "string", 1, NULL, 's'},
		{ "baudrate", 1, NULL, 'b'},
		{ NULL,     0, NULL, 0  }
	};
	
	program_name = argv[0];

	do {
		next_option = getopt_long (argc, argv, short_options, long_options, NULL);
		switch (next_option) {
			case 'h':
				print_usage (stdout, 0);
			case 'd':
				device = optarg;
				havearg = 1;
				break;
			case 'b':
				speed = atoi(optarg);
				break;

			case 's':
				xmit = optarg;
				havearg = 1;
				break;
			case -1:
				if (havearg)  break;
			case '?':
				print_usage (stderr, 1);
			default:
				abort ();
		}
	}while(next_option != -1);

	sleep(1);
	fd = OpenDev(device);

	if (fd > 0) {
		set_speed(fd, speed);
	} else {
		fprintf(stderr, "Error opening %s: %s\n", device, strerror(errno));
		exit(1);
	}

	if (set_Parity(fd,8,1,'N')== FALSE) {
		fprintf(stderr, "Set Parity Error\n");
		close(fd);
		exit(1);
	}


    

	
	evp.sigev_value.sival_ptr = &timer;
	evp.sigev_notify = SIGEV_SIGNAL;
	evp.sigev_signo = SIGUSR1;
	
	signal(SIGUSR1, handle);
	
	ret = timer_create(CLOCK_REALTIME, &evp, &timer);
	if( ret )
		perror("timer_create");
	
	ts.it_interval.tv_sec = 3;  // 第一超时后以后每次超时时间
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 3;  // 第一次超时时间
	ts.it_value.tv_nsec = 0; //1000000;
	

	
	ret = timer_settime(timer, 0, &ts, NULL);
	if( ret )
		perror("timer_settime");
    
    
    
    fds.fd = fd;  
    fds.events = POLLIN;    
    
    printf("polling\n");
    while(1) {
#if 1
        nread = read(fd, buff, 128);
        if (nread > 0) {
            rev_len[rev_cnt] = nread;
            rev_cnt++;
        }
#else
        ret = poll(&fds,1,5000);
        if(ret ==  0 )
            printf("poll time out\n");
        else{
            nread = read(fd, buff, 128);
            if (nread > 0) {
                rev_len[rev_cnt] = nread;
                rev_cnt++;
            }           
            
        }
#endif
    }	
	
	close(fd);
	exit(0);
}

