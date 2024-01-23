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
#include <signal.h>
#include <time.h>


#define MAX_PACKETS 32

#define MAX_MTU 1500
 /* ip首部长度 */
 #define IP_HEADER_LEN sizeof(struct ip)
 /* tcp首部长度 */
 #define TCP_HEADER_LEN sizeof(struct tcphdr)
 /* ip首部 + tcp首部长度 */
 #define IP_TCP_HEADER_LEN IP_HEADER_LEN + TCP_HEADER_LEN
 
unsigned long send_len = 0;
unsigned long rev_len = 0;
#define ETH_P_DEAN 0x0855 //自定义的以太网协议type
uint8_t ether_frame[1024*1024];


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
 
 
void  handle_send(void )
{
    static  unsigned long last = 0;
    
    printf("send speed %ld B = %ld Mb/s\n",(send_len - last)/2,(send_len - last)*8/2/1048576);
    last = send_len;
    
}

void  handle_receive(void )
{
    static  unsigned long last_rev = 0;
    
    printf("receive speed %ld B = %ld Mb/s\n",(rev_len - last_rev)/2,(rev_len - last_rev)*8/2/1048576);
    last_rev = rev_len;
    
}


void send_sig_handle(int signo)
{
    printf("send total %d\n",send_len);
    exit(0);
}

void rev_sig_handle(int signo)
{
    printf("received total %d\n",rev_len);
    exit(0);
}



 void raw_send(char *dev,char *mac_addr, char *sendStr)
 {
    int sd,ret,mtu,i;
    uint8_t src_mac[6];
    uint8_t dst_mac[6];;    
    struct ifreq req;
    struct sockaddr_ll stTagAddr;
    struct ifreq ifr;
    char *interface=dev;
    unsigned int left =0,frame_len;
    unsigned char *buf;
    
    struct sigevent evp;
    struct itimerspec ts;
    timer_t timer;
	

	

    
    
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
    
    
    if(ioctl(sd,SIOCGIFMTU,&ifr) < 0){
        printf("failed to get MTU\n");
        return;
    }
    mtu = ifr.ifr_mtu;
    printf("MTU %d\n",mtu);
    if(mtu + 14 > sizeof(ether_frame)){
        printf("buff size is not enough\n");
        return ;
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
    //dst_mac[0]   = 0xff;
    //dst_mac[1]   = 0xff;
    //dst_mac[2]   = 0xff;
    //dst_mac[3]   = 0xff;
    //dst_mac[4]   = 0xff;
    //dst_mac[5]   = 0xff;    


    stTagAddr.sll_family = AF_PACKET;
    memcpy(stTagAddr.sll_addr, src_mac, 6);
    stTagAddr.sll_halen = htons (6);
    
    
    buf = ether_frame;
    memset(ether_frame,0,sizeof(ether_frame));
    
	
	
	
	for(i=0;i<MAX_PACKETS;i++){
        
        memcpy(buf, dst_mac, 6);
        memcpy(buf + 6, src_mac, 6);
        buf[12] = ETH_P_DEAN / 256;
        buf[13] = ETH_P_DEAN % 256;
        memset(buf + 14,0xaa,mtu);
        
        buf += (mtu + 14);
    }
    
    
    printf("dest mac %02x:%02x:%02x:%02x:%02x:%02x\n",dst_mac[0],dst_mac[1],dst_mac[2],dst_mac[3],dst_mac[4],dst_mac[5]);
    printf("source mac %02x:%02x:%02x:%02x:%02x:%02x\n",src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5]);
    //printf("ethnet device index %d, len %d\n",stTagAddr.sll_ifindex, strlen(data) );


    if ((sd = socket (PF_PACKET, SOCK_RAW, htons(ETH_P_DEAN))) < 0) {//创建正真发送的socket
        printf ("socket() failed ");
        return;
    }
    
  
    
    //while(1){

        frame_len = strlen( sendStr)+14;
		memcpy(ether_frame + 14,sendStr , strlen( sendStr) );
		
		
        ret = sendto(sd, (unsigned char *)ether_frame,frame_len, 0, (const struct sockaddr *)&stTagAddr, sizeof(stTagAddr));
        if(ret <=0){
            printf("send data failed, ret %d\n",ret );

        }

    //}
    

    close(sd);

    
 }
 
 
 

 void raw_socket_recv(char *dev)
 {
     struct ip *ip_header;
     struct tcphdr *tcp_header;
     int sock_raw_fd, ret_len;
     struct sockaddr_ll         m_AddrSll;  

      
    struct sigevent evp;
    struct itimerspec ts;
    timer_t timer;
    int ret;
	
	evp.sigev_value.sival_ptr = &timer;
	evp.sigev_notify = SIGEV_SIGNAL;
	evp.sigev_signo = SIGUSR1;
	
	signal(SIGUSR1, handle_receive);
	
    timer_create(CLOCK_REALTIME, &evp, &timer);

	
	ts.it_interval.tv_sec = 2;
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 2;
	ts.it_value.tv_nsec = 0;
	
    
    signal(SIGINT,rev_sig_handle);
    
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
         
    ret = timer_settime(timer, 0, &ts, NULL);
	if( ret ){
        printf("timer_settime error\n");
        return ;
    }
    
    printf("receiving..\n");
     while (1)
     {
         
         // bzero(ether_frame, IP_TCP_BUFF_SIZE);
         ret_len = recv(sock_raw_fd, ether_frame, sizeof(ether_frame), 0);

         if(ret_len >0)
             rev_len += ret_len;
     }
     
     close(sock_raw_fd);
 }
 
 
 int main(int argc, const char *argv[])
 {
    unsigned int size;
    pid_t pid;

    if(argc < 3){
        printf("parameters error, use like: send [net_interface] [dest MAC_addr] [send string]\n    e.g. send eth0 fc:c2:3d:03:11:48 123456\n");
        return 0;
    }
    
    

        

    printf("send processer:\n");
    raw_send(argv[1],argv[2], argv[3]);



    
    
     return 0;
 }
