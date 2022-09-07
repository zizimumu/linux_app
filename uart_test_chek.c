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
int speed,g_len = 32,time = 20 ;
#define FIRST_CHAR 0x30
#define END_CHAR (FIRST_CHAR + g_len -1)


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
int set_Parity(int fd,int databits,int stopbits,int parity,int stall_len)
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

int check_start(char *buf,int len ,int *res)
{
    int i;
	*res = 1;
	
    for(i=0;i<len;i++){

		
        if(buf[i] == (char)END_CHAR)
            return i;
		
		if(buf[i] != (char)(0x30 + i))
			*res = 0;
    }
    
    return -1;
    
}
int check_rev(char *buf,int len)
{
    int i;
    for(i=0;i<len;i++){
        if(buf[i] != (char)(0x30 + i))
            return 1;
    }
    
    return 0;
    
}

void memcpy_s(unsigned char *des,unsigned char *src,int len)
{
	int i;
	for(i=0;i<len;i++){
		des[i] = src[i];
	}
}
/*
	*@breif  main()
 */
 char buff[1024];	
 char xmit[1024]; /* Default send data */ 
 
int main(int argc, char *argv[])
{
	int  fd, next_option, havearg = 0;
	char *device;
	int i=0,j=0,ret,valid,remain,res;
	int nread,count= 0,total;			/* Read the counts of data */
		/* Recvice data buffer */
	pid_t pid;
	unsigned int correct_cnt = 0,err_count = 0;
	
    char *read_p;
	
	
	const char *const short_options = "hd:l:t:s:b:";

	const struct option long_options[] = {
		{ "help",   0, NULL, 'h'},
		{ "device", 1, NULL, 'd'},
        { "len", 1, NULL, 'l'},
        { "timer", 1, NULL, 't'},
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
			case 'l':
				g_len = atoi(optarg);
				
				break;
			case 't':
				time = atoi(optarg);
				
				break;
			case 's':
				//xmit = optarg;
				//havearg = 1;
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
    printf("test len is %d,END_CHAR %d, sleep time %d\n",g_len,END_CHAR,time);
    
    if(g_len >= (255- 48)){
        printf("too max,len err\n");
        return -1;
    }
    for(i=0;i<g_len;i++){
        xmit[i] = '0'+i;
    }
	fd = OpenDev(device);

	if (fd > 0) {
		set_speed(fd, speed);
	} else {
		fprintf(stderr, "Error opening %s: %s\n", device, strerror(errno));
		exit(1);
	}

	if (set_Parity(fd,8,1,'N',g_len)== FALSE) {
		fprintf(stderr, "Set Parity Error\n");
		close(fd);
		exit(1);
	}

	pid = fork();	
	

	if (pid < 0) { 
		fprintf(stderr, "Error in fork!\n"); 
	} else if (pid == 0){
		while(1) {
			//printf("%s SEND: %s\n",device, xmit);
			write(fd, xmit, g_len);
			
			if(time > 0)
				usleep(time*1000);
			//i++;
		}
		exit(0);
	} else { 
        read_p = buff;
		total = 0;

		while(1) {
			nread = read(fd, read_p, g_len);
			
			
			//printf("s\n");
            //print_hex(read_p,nread);
			//printf("\n");
			
			
			if (nread > 0) {
                total += nread;
				read_p += nread;
				
				if(total >= g_len){
					ret = check_start(buff,total,&res);
					if(ret == (g_len - 1) && (res == 1)){
						if(((correct_cnt ) % 500) == 0)
							printf("%s :receive OK,right %d,err %d\n",device,correct_cnt,err_count);
						
						correct_cnt++;						
					}
					else{
						printf("err %d,ret %d,total %d\n",err_count,ret,total);
						err_count++;
						print_hex(buff,total);						
					}
					
					if(ret >= 0 ){
						remain = total - ret - 1;
						memcpy_s(buff,buff + ret + 1,remain);
						read_p = buff + remain ;
						total = remain;						
					}
					
				}
			}
			else{
				printf("%s,read file err,%d\n",device,nread);
			}
		}	
	}
	close(fd);
	exit(0);
}

