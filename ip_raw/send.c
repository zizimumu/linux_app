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
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/if_ether.h>




 /* ip首部长度 */
 #define IP_HEADER_LEN sizeof(struct ip)
 /* tcp首部长度 */
 #define TCP_HEADER_LEN sizeof(struct tcphdr)
 /* ip首部 + tcp首部长度 */
 #define IP_TCP_HEADER_LEN IP_HEADER_LEN + TCP_HEADER_LEN
 
 
 
 static int hex2num(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

void hexstr2mac(char *dst, char *src) {
	int i=0;
    while(i<6) {
        if(' ' == *src||':'== *src||'"'== *src||'\''== *src) {
            src++;
            continue;
        }
        *(dst+i) = ((hex2num(*src)<<4)|hex2num(*(src+1)));
	i++;
        src += 2;        
    }
}



 void err_exit(const char *err_msg)
 {
     perror(err_msg);
     exit(1);
 }
 
 /* 填充ip首部 */
 struct ip *fill_ip_header(const char *src_ip, const char *dst_ip, int ip_packet_len)
 {
     struct ip *ip_header;
 
     ip_header = (struct ip *)malloc(IP_HEADER_LEN);
     ip_header->ip_v = IPVERSION;
     ip_header->ip_hl = sizeof(struct ip) / 4;        /* 这里注意，ip首部长度是指占多个32位的数量，4字节=32位，所以除以4 */
     ip_header->ip_tos = 0;
     ip_header->ip_len = htons(ip_packet_len);        /* 整个IP数据报长度，包括普通数据 */
     ip_header->ip_id = 0;                            /* 让内核自己填充标识位 */
     ip_header->ip_off = 0;
     ip_header->ip_ttl = MAXTTL;
     ip_header->ip_p = IPPROTO_TCP;                   /* ip包封装的协议类型 */
     ip_header->ip_sum = 0;                           /* 让内核自己计算校验和 */
     ip_header->ip_src.s_addr = inet_addr(src_ip);    /* 源IP地址 */
     ip_header->ip_dst.s_addr = inet_addr(dst_ip);    /* 目标IP地址 */
 
     return ip_header;
 }
 
 
 
unsigned short check_sum(unsigned short *addr,int len){
    register int nleft=len;
    register int sum=0;
    register short *w=addr;
    short answer=0;

    while(nleft>1)
    {
            sum+=*w++;
            nleft-=2;
    }
    if(nleft==1)
    {
            *(unsigned char *)(&answer)=*(unsigned char *)w;
            sum+=answer;
    }

    sum=(sum>>16)+(sum&0xffff);
    sum+=(sum>>16);
    answer=~sum;
    return(answer);
}
    
    
 /* 填充tcp首部 */
 struct tcphdr *fill_tcp_header(int src_port, int dst_port)
 {
     struct tcphdr *tcp_header;
 
     tcp_header = (struct tcphdr *)malloc(TCP_HEADER_LEN);
     tcp_header->source = htons(src_port); 
     tcp_header->dest = htons(dst_port);
     /* 同IP首部一样，这里是占32位的字节多少个 */
     tcp_header->doff = sizeof(struct tcphdr) / 4;
     /* 发起连接 */
     tcp_header->syn = 1;
     tcp_header->window = 4096;
     tcp_header->check = 0;
     
     
     // tcp_header->check=check_sum((unsigned short*)tcp_header,sizeof(struct tcphdr));
 
     return tcp_header;
 }
 
 /* 发送ip_tcp报文 */
 void ip_tcp_send(const char *src_ip, int src_port, const char *dst_ip, int dst_port, const char *data)
 {
     struct ip *ip_header;
     struct tcphdr *tcp_header;
     struct sockaddr_in dst_addr;
     socklen_t sock_addrlen = sizeof(struct sockaddr_in);
 
     int data_len = strlen(data);
     int ip_packet_len = IP_TCP_HEADER_LEN + data_len;
     char buf[ip_packet_len];
     int sockfd, ret_len, on = 1;
 
     bzero(&dst_addr, sock_addrlen);
     dst_addr.sin_family = PF_INET;
     dst_addr.sin_addr.s_addr = inet_addr(dst_ip);
     dst_addr.sin_port = htons(dst_port);
 
     /* 创建tcp原始套接字 */
    if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP)) == -1)
    //if ((sockfd = socket(PF_PACKET,  SOCK_DGRAM, htons(ETH_P_IP)))== -1)
         err_exit("socket()");
 
     /* 开启IP_HDRINCL，自定义IP首部 */
     if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == -1)
         err_exit("setsockopt()");


     //setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)); 


     
     /* ip首部 */
     ip_header = fill_ip_header(src_ip, dst_ip, ip_packet_len);
     /* tcp首部 */
     tcp_header = fill_tcp_header(src_port, dst_port);
 
     bzero(buf, ip_packet_len);
     memcpy(buf, ip_header, IP_HEADER_LEN);
     memcpy(buf + IP_HEADER_LEN, tcp_header, TCP_HEADER_LEN);
     memcpy(buf + IP_TCP_HEADER_LEN, data, data_len);
 
     /* 发送报文 */
     ret_len = sendto(sockfd, buf, ip_packet_len, 0, (struct sockaddr *)&dst_addr, sock_addrlen);
     if (ret_len > 0)
         printf("sendto() ok!!!\n");
     else printf("sendto() failed\n");
 
     close(sockfd);
     free(ip_header);
     free(tcp_header);
 }
 
 unsigned char tcp_buf[1024*1024];
 #define TEST_LEN (1024*1)
  void ip_tcp_send_raw(const char *src_ip, int src_port, const char *dst_ip, int dst_port)
 {
     struct ip *ip_header;
     struct tcphdr *tcp_header;
     struct sockaddr_in dst_addr;
     socklen_t sock_addrlen = sizeof(struct sockaddr_in);
 
     int ip_packet_len = IP_TCP_HEADER_LEN + TEST_LEN;

     int sockfd, ret_len, on = 1;
 
     bzero(&dst_addr, sock_addrlen);
     dst_addr.sin_family = PF_INET;
     dst_addr.sin_addr.s_addr = inet_addr(dst_ip);
     dst_addr.sin_port = htons(dst_port);
 
     /* 创建tcp原始套接字 */
    if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP)) == -1)
    //if ((sockfd = socket(PF_PACKET,  SOCK_DGRAM, htons(ETH_P_IP)))== -1)
         err_exit("socket()");
 
     /* 开启IP_HDRINCL，自定义IP首部 */
     if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == -1)
         err_exit("setsockopt()");


     //setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)); 


     
     /* ip首部 */
     ip_header = fill_ip_header(src_ip, dst_ip, ip_packet_len);
     /* tcp首部 */
     tcp_header = fill_tcp_header(src_port, dst_port);
 
     bzero(tcp_buf, ip_packet_len);
     memcpy(tcp_buf, ip_header, IP_HEADER_LEN);
     memcpy(tcp_buf + IP_HEADER_LEN, tcp_header, TCP_HEADER_LEN);
 
     /* 发送报文 */
     ret_len = sendto(sockfd, tcp_buf, ip_packet_len, 0, (struct sockaddr *)&dst_addr, sock_addrlen);
     if (ret_len > 0)
         printf("sendto() ok!!!\n");
     else 
         printf("sendto() failed\n");
 
     close(sockfd);
     free(ip_header);
     free(tcp_header);
 }
 
 
#define ETH_P_DEAN 0x0855 //自定义的以太网协议type
 
 void raw_send_SOCK_DGRAM(const char *data)
 {
    int sd,ret;
    struct ifreq req;
    struct sockaddr_ll stTagAddr;

    //sd = socket(PF_INET,SOCK_DGRAM,0);//这个sd就是用来获取eth0的index，完了就关闭
    //if(sd < 0)
    //    printf("fist socket failed\n");
    //strncpy(req.ifr_name,"eth0",4);//通过设备名称获取index
    //ret=ioctl(sd,SIOCGIFINDEX,&req);
    //if(ret < 0)
    //    printf("ioctl error %d\n",ret);
    //   
    //close(sd);     
     
    

    memset(&stTagAddr, 0 , sizeof(stTagAddr));
    stTagAddr.sll_family    = AF_PACKET;//填写AF_PACKET,不再经协议层处理
    stTagAddr.sll_protocol  = htons(ETH_P_DEAN);
    
    
    stTagAddr.sll_ifindex   = 0; //req.ifr_ifindex;//网卡eth0的index，非常重要，系统把数据往哪张网卡上发，就靠这个标识
    stTagAddr.sll_pkttype   = PACKET_OUTGOING;//标识包的类型为发出去的包
    stTagAddr.sll_halen     = 6;    //目标MAC地址长度为6

    //填写目标MAC地址  54-E1-AD-E6-FC-6A
    stTagAddr.sll_addr[0]   = 0x54;
    stTagAddr.sll_addr[1]   = 0xE1;
    stTagAddr.sll_addr[2]   = 0xAD;
    stTagAddr.sll_addr[3]   = 0xE6;
    stTagAddr.sll_addr[4]   = 0xFC;
    stTagAddr.sll_addr[5]   = 0x6A;
    
    
    sd = socket(AF_PACKET,SOCK_DGRAM,htons(ETH_P_DEAN));
    if(sd < 0)
        printf("socket failed, %d\n",sd);

    //填充帧头和内容
    
    printf("send to %d ethernet\n",req.ifr_ifindex);

    ret = sendto(sd, (unsigned char *)data, strlen(data), 0, (const struct sockaddr *)&stTagAddr, sizeof(stTagAddr));
    if(ret <=0)
        printf("send data failed, len %d\n",strlen(data) );

     close(sd);

 }
 
 uint8_t ether_frame[4096];
 
 void raw_send(char *dev, char *mac_addr, const char *data)
 {
    int sd,ret;
    uint8_t src_mac[6];
    uint8_t dst_mac[6];;    
    struct ifreq req;
    struct sockaddr_ll stTagAddr;
    struct ifreq ifr;
    char *interface=dev;
    
    if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_DEAN))) < 0) {
        printf ("socket() failed to get socket descriptor for using ioctl() ");
        return;
    }
    memset (&ifr, 0, sizeof (ifr));
    snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
    
    if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0) {
        printf ("ioctl() failed to get source MAC address ");
        return;
    }
    close(sd);
    
    
    memcpy(src_mac, ifr.ifr_hwaddr.sa_data, 6);
   
    memset(&stTagAddr, 0 , sizeof(stTagAddr));
    
    
    if ((stTagAddr.sll_ifindex = if_nametoindex(interface)) == 0) {
        printf ("if_nametoindex() failed to obtain interface index ");
        return;
    }    
    
    
    hexstr2mac(dst_mac,mac_addr);
    // fc:c2:3d:0c:44:4c
    //dst_mac[0]   = 0xfc;
    //dst_mac[1]   = 0xc2;
    //dst_mac[2]   = 0x3d;
    //dst_mac[3]   = 0x0c;
    //dst_mac[4]   = 0x44;
    //dst_mac[5]   = 0x4c;    


    stTagAddr.sll_family = AF_PACKET;
    memcpy(stTagAddr.sll_addr, src_mac, 6);
    stTagAddr.sll_halen = htons (6);
    
    
    memset(ether_frame,0,sizeof(ether_frame));
    memcpy(ether_frame, dst_mac, 6);
    memcpy(ether_frame + 6, src_mac, 6);
    ether_frame[12] = ETH_P_DEAN / 256;
    ether_frame[13] = ETH_P_DEAN % 256;
    memcpy(ether_frame + 14 , data, strlen(data));
    
    
    printf("dest mac %02x:%02x:%02x:%02x:%02x:%02x\n",dst_mac[0],dst_mac[1],dst_mac[2],dst_mac[3],dst_mac[4],dst_mac[5]);
    printf("source mac %02x:%02x:%02x:%02x:%02x:%02x\n",src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5]);
    printf("ethnet device index %d, len %d\n",stTagAddr.sll_ifindex, strlen(data) );


    if ((sd = socket (PF_PACKET, SOCK_RAW, htons(ETH_P_DEAN))) < 0) {//创建正真发送的socket
        printf ("socket() failed ");
        return;
    }
    // 14 + strlen(data)
    ret = sendto(sd, (unsigned char *)ether_frame, 14 + strlen(data), 0, (const struct sockaddr *)&stTagAddr, sizeof(stTagAddr));
    if(ret <=0)
        printf("send data failed, len %d, ret %d\n",strlen(data),ret );

     close(sd);

    
 }
 
 
 int main(int argc, const char *argv[])
 {
     ip_tcp_send_raw("192.168.10.122",112,"192.168.10.101",1133);   // const char *src_ip, int src_port, const char *dst_ip, int dst_port);
     return 0;
 }