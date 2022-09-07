
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <limits.h>

#include <signal.h>
#include <time.h>

#define SERVER_PORT 8888 //定义端口号：（0-1024为保留端口号，最好不要用）
#define MAX_BUF_SIZE (43*1500)  //UDP 最大传输size不能超过65507
unsigned char test_buf[MAX_BUF_SIZE];


int
setnonblocking(int fd, int nonblocking)
{
    int flags, newflags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    if (nonblocking)
	newflags = flags | (int) O_NONBLOCK;
    else
	newflags = flags & ~((int) O_NONBLOCK);
    if (newflags != flags)
	if (fcntl(fd, F_SETFL, newflags) < 0) {
	    perror("fcntl(F_SETFL)");
	    return -1;
	}
    return 0;
}


unsigned long send_len = 0;
unsigned long rev_len = 0;
void  handle_send(void )
{
    static  unsigned long last = 0;
    
    printf("send speed %ld B = %ld Mb/s\n",(send_len - last)/2,(send_len - last)*8/2/1048576);
    last = send_len;
    
}


int main(int argc,char **argv)
{
    int sockfd; //socket描述符
    struct sockaddr_in addr; //定义服务器起地址
    int ret;
     struct sigevent evp;
    struct itimerspec ts;
    timer_t timer;
    
    
    if(argc!=2)
    {
        fprintf(stderr,"Usage:%s server_ip\n",argv[0]);
        exit(1);
    }
    
    

	
	evp.sigev_value.sival_ptr = &timer;
	evp.sigev_notify = SIGEV_SIGNAL;
	evp.sigev_signo = SIGUSR1;
	
	signal(SIGUSR1, handle_send);
	
    timer_create(CLOCK_REALTIME, &evp, &timer);

	
	ts.it_interval.tv_sec = 2;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 2;
	ts.it_value.tv_nsec = 0;   
    
     ret = timer_settime(timer, 0, &ts, NULL);
	if( ret ){
        printf("timer_settime error\n");
        return ;
    }   
    
    

    /* 建立 sockfd描述符 */
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd<0)
    {
        fprintf(stderr,"Socket Error:%s\n",strerror(errno));
        exit(1);
    }

    // setnonblocking(sockfd,1);
    
    /* 填充服务端的资料 */
    bzero(&addr,sizeof(struct sockaddr_in)); // 初始化,置0
    addr.sin_family=AF_INET; // Internet
    addr.sin_port=htons(SERVER_PORT);// (将本机器上的short数据转化为网络上的short数据)端口号
    if(inet_aton(argv[1],&addr.sin_addr)<0) /*inet_aton函数用于把字符串型的IP地址转化成网络2进制数字*/
    {
        fprintf(stderr,"Ip error:%s\n",strerror(errno));
        exit(1);
    }
    
    while(1){
        ret = sendto(sockfd,test_buf,sizeof(test_buf),0,(struct sockaddr*)&addr,sizeof(struct sockaddr_in));
        if(ret <=0){
            printf("send data failed, ret %d\n",ret );
            break;
        }
        else{
            send_len += ret;
        }
    }

    // udpc_requ(sockfd,&addr,sizeof(struct sockaddr_in)); // 进行读写操作
    close(sockfd);
} 

