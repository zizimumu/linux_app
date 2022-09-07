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
 
 void raw_socket_recv()
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


     
     
 	//memset(&m_AddrSll, 0, sizeof(struct sockaddr_ll));
	//m_AddrSll.sll_family = AF_PACKET;
	//m_AddrSll.sll_ifindex = if_nametoindex("eth0");
	//m_AddrSll.sll_protocol = htons(ETH_P_ALL);
    //
	//if(bind(sock_raw_fd, (struct sockaddr *) &m_AddrSll, sizeof(m_AddrSll)) == -1 )
	//{
	//   printf("bind socket error!\n");
	//   return;
	//}
         
         
         
     /* 接收数据 */
     while (1)
     {
         
         bzero(buf, IP_TCP_BUFF_SIZE);
         ret_len = recv(sock_raw_fd, buf, 4096, 0);
         if (ret_len > 14)
         {
            print_data(buf,ret_len);
         }
     }
     
     close(sock_raw_fd);
 }
 
 int main(void)
 {
     /* 原始套接字接收 */
     raw_socket_recv();
 
     return 0;
 }