

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
#include <signal.h>
#include <time.h>


#define SERVER_PORT 8888 //定义端口号：（0-1024为保留端口号，最好不要用）
#define MAX_MSG_SIZE (1024*1024)
char msg[MAX_MSG_SIZE];


unsigned long send_len = 0;
unsigned long rev_len = 0;
void  handle_rev(void )
{
    static  unsigned long last = 0;
    
    printf("send speed %ld B = %ld Mb/s\n",(send_len - last)/2,(send_len - last)*8/2/1048576);
    last = send_len;
    
}


void udps_respon(int sockfd)
{
    struct sockaddr_in addr;
    int addrlen,ret;
    
        bzero(msg,sizeof(msg)); // 初始化,清零
        addrlen = sizeof(struct sockaddr);
        
    while(1)
    {    /* 从网络上读,并写到网络上 */

        ret=recvfrom(sockfd,msg,MAX_MSG_SIZE,0,(struct sockaddr*)&addr,&addrlen); // 从客户端接收消息
        if(ret <=0){
            printf("send data failed, ret %d\n",ret );
            break;
        }
        else{
            send_len += ret;
        }
    }
}

int main(void)
{
    int sockfd; //socket描述符
    struct sockaddr_in addr; //定义服务器起地址
     struct sigevent evp;
    struct itimerspec ts;
    timer_t timer;
    int ret;
     int disable;
    
    /* 服务器端开始建立socket描述符 */
    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd<0)
    {
        fprintf(stderr,"Socket Error:%s\n",strerror(errno));
        exit(1);
    }

    
	evp.sigev_value.sival_ptr = &timer;
	evp.sigev_notify = SIGEV_SIGNAL;
	evp.sigev_signo = SIGUSR1;
	
	signal(SIGUSR1, handle_rev);
	
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
    
    
    
    /* 服务器端填充 sockaddr结构 */
    bzero(&addr,sizeof(struct sockaddr_in)); // 初始化,置0
    addr.sin_family=AF_INET; // Internet
    addr.sin_addr.s_addr=htonl(INADDR_ANY); // (将本机器上的long数据转化为网络上的long数据)和任何主机通信 //INADDR_ANY 表示可以接收任意IP地址的数据，即绑定到所有的IP
    //addr.sin_addr.s_addr=inet_addr("192.168.1.1"); //用于绑定到一个固定IP,inet_addr用于把数字加格式的ip转化为整形ip
    addr.sin_port=htons(SERVER_PORT); // (将本机器上的short数据转化为网络上的short数据)端口号

    /* 捆绑sockfd描述符 */
    if(bind(sockfd,(struct sockaddr *)&addr,sizeof(struct sockaddr_in))<0)
    {
        fprintf(stderr,"Bind Error:%s\n",strerror(errno));
        exit(1);
    }


    
    if(setsockopt(sockfd,SOL_SOCKET,SO_NO_CHECK,(void *)&disable,sizeof(disable)) < 0){
        printf("setsockopt error\n");
        return 0;
    }
    
    
    udps_respon(sockfd); // 进行读写操作
    close(sockfd);
} 

