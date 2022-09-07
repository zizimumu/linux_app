 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <netinet/ip.h>
 #include <netinet/tcp.h>
 #include <linux/if_packet.h>
#include <netinet/if_ether.h>
#include <net/if.h>
 #include <signal.h>
#include <time.h>

 /* ip首部长度 */
 #define IP_HEADER_LEN sizeof(struct ip)
 /* tcp首部长度 */
 #define TCP_HEADER_LEN sizeof(struct tcphdr)
 /* ip首部 + tcp首部长度 */
 #define IP_TCP_HEADER_LEN IP_HEADER_LEN + TCP_HEADER_LEN
 /* 接收数据缓冲大小 */
 #define BUFFER_SIZE 1024
 /* ip首部 + tcp首部 + 数据缓冲区大小 */
 #define IP_TCP_BUFF_SIZE IP_TCP_HEADER_LEN + BUFFER_SIZE
 
 
struct itimerspec ts;
timer_t timer;	
unsigned int recv_len = 0;
unsigned int last_recv_len = 0;


 void err_exit(const char *err_msg)
 {
     perror(err_msg);
     exit(1);
 }
 
 void print_data(unsigned char *buf, int len)
 {
     int i;
     
     printf("\n");
     printf("receiver:\n");
     for(i=0;i<len;i++){
         printf(" %02x",buf[i]);
         
     }
     printf("\n");
 }
  char buf[4096];
 /* 原始套接字接收 */
 
 #define ETH_P_DEAN 0x0855 //自定义的以太网协议type
 
 void raw_socket_recv(char *dev)
 {
     struct ip *ip_header;
     struct tcphdr *tcp_header;
     int sock_raw_fd, ret_len;
      struct sockaddr_ll         m_AddrSll;  

    
     // if ((sock_raw_fd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP)) == -1)  ETH_P_DEAN
     // if ((sock_raw_fd = socket(PF_PACKET,  SOCK_RAW, htons(ETH_P_ALL)))== -1)
     if ((sock_raw_fd = socket(PF_PACKET,  SOCK_RAW, htons(ETH_P_DEAN)))== -1)    
     // if ((sock_raw_fd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP)) == -1)
         err_exit("socket()");


     
     
 	memset(&m_AddrSll, 0, sizeof(struct sockaddr_ll));
	m_AddrSll.sll_family = AF_PACKET;
	m_AddrSll.sll_ifindex = if_nametoindex(dev);
	m_AddrSll.sll_protocol = htons(ETH_P_DEAN);
    
	if(bind(sock_raw_fd, (struct sockaddr *) &m_AddrSll, sizeof(m_AddrSll)) == -1 )
	{
	   printf("bind socket error!\n");
	   return;
	}
         
         
         
     /* 接收数据 */
     while (1)
     {
         
         bzero(buf, IP_TCP_BUFF_SIZE);
         ret_len = recv(sock_raw_fd, buf, 4096, 0);
         //if (ret_len > 14)
         //{
         //   print_data(buf,ret_len);
         //}
         if(ret_len >0)
             recv_len += ret_len;
     }
     
     close(sock_raw_fd);
 }
 
 
 
 void  handle(void){
     
     if((last_recv_len == recv_len) && recv_len !=0){
         printf("\nreceived %d bytes\n",recv_len);
         last_recv_len = recv_len = 0;
     }
     last_recv_len = recv_len;
     
 }
 int main(int argc, const char *argv[])
 {
     struct sigevent evp;
     int ret;

    
    
     /* 原始套接字接收 */
     if(argc <2)
     {
         printf("parameter error\n");
         return 0;
     }
     
    evp.sigev_value.sival_ptr = &timer;
    evp.sigev_notify = SIGEV_SIGNAL;
    evp.sigev_signo = SIGUSR1;
	signal(SIGUSR1, handle);
	
	ret = timer_create(CLOCK_REALTIME, &evp, &timer);
	if( ret )
		perror("timer_create");
	
	ts.it_interval.tv_sec = 1;  // 第一超时后以后每次超时时间
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 1;  // 第一次超时时间
	ts.it_value.tv_nsec = 0; //1000000;
	
	ret = timer_settime(timer, 0, &ts, NULL);
	if( ret )
		perror("timer_settime");
    
    
     raw_socket_recv(argv[1]);
 
     return 0;
 }