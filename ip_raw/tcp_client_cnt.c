

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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <limits.h>

#define portnumber 3333 //定义端口号：（0-1024为保留端口号，最好不要用）
unsigned char test_buf[1024*1024];
unsigned int send_len = 0;

void  handle_send(void )
{
    static  unsigned long last = 0;
    
    printf("send %d,speed %ld B = %ld Mb/s\n",send_len,(send_len - last)/2,(send_len - last)*8/2/1048576);
    last = send_len;
    
}


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


int main(int argc, char *argv[])
{
    int sockfd;
    int ret;
    struct sockaddr_in server_addr; //描述服务器的地址
    struct hostent *host;
    int nbytes;
    struct sigevent evp;
    struct itimerspec ts;
    timer_t timer;
    int size;
    if(argc!=3)
    {
        fprintf(stderr,"Usage:%s server_ip size\n",argv[0]);
        exit(1);
    }

	size    = (unsigned int)strtoul(argv[2], NULL, 0);
	memset(test_buf,0x78,size);
	
    if((host=gethostbyname(argv[1]))==NULL)
    {
        fprintf(stderr,"Gethostname error\n");
        exit(1);
    }

    /* 客户程序开始建立 sockfd描述符 */
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) // AF_INET:Internet;SOCK_STREAM:TCP
    {
        fprintf(stderr,"Socket Error:%s\a\n",strerror(errno));
        exit(1);
    }
    
    
    

    /* 客户程序填充服务端的资料 */
    bzero(&server_addr,sizeof(server_addr)); // 初始化,置0
    server_addr.sin_family=AF_INET; // IPV4
    server_addr.sin_port=htons(portnumber); // (将本机器上的short数据转化为网络上的short数据)端口号
    server_addr.sin_addr=*((struct in_addr *)host->h_addr); // IP地址
    
    /* 客户程序发起连接请求 */
    if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
    {
        fprintf(stderr,"Connect Error:%s\a\n",strerror(errno));
        exit(1);
    }

    
    printf("connected\n");
    


    ret = write(sockfd,test_buf,size);
    if(ret <= 0)
    {
        fprintf(stderr,"Write Error:%s\n",strerror(errno));
        //exit(1);
    }  

    /* 结束通讯 */
    close(sockfd);
    
    printf("done\n");
    exit(0);
} 

