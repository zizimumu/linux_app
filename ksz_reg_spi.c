#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
// #include <endian.h>


#if 0

 #if __BYTE_ORDER == __BIG_ENDIAN 

    #define be32toh(x) (x) 
    #define be16toh(x) (x) 
    #define le32toh(x) (x)
    #define le16toh(x) (x)
 
 #else 
    #define be32toh(x) le2be_u32(x) 
    #define be16toh(x) le2be_u16(x)
    #define le32toh(x) be2le_u32(x)
    #define le16toh(x) be2le_u16(x)

 #endif

#endif
 
unsigned int le2be_u32(unsigned int src)
{

    unsigned int  val = src,ret = 0;
    unsigned char *pt1 = (unsigned char *)&val, *pt2 = (unsigned char *)&ret;
    
    pt2[0] = pt1[3];
    pt2[1] = pt1[2];
    pt2[2] = pt1[1];
    pt2[3] = pt1[0];
    
    return ret;
}

unsigned short le2be_u16(unsigned short src)
{

    unsigned short  val = src,ret = 0;
    unsigned char *pt1 = (unsigned char *)&val, *pt2 = (unsigned char *)&ret;
    
    pt2[0] = pt1[1];
    pt2[1] = pt1[0];
    return ret;
}

unsigned int be2le_u32(unsigned int src)
{

    unsigned int  val = src,ret = 0;
    unsigned char *pt1 = (unsigned char *)&val, *pt2 = (unsigned char *)&ret;
    
    pt2[0] = pt1[3];
    pt2[1] = pt1[2];
    pt2[2] = pt1[1];
    pt2[3] = pt1[0];
    
    return ret;
}

unsigned short be2le_u16(unsigned short src)
{

    unsigned short  val = src,ret = 0;
    unsigned char *pt1 = (unsigned char *)&val, *pt2 = (unsigned char *)&ret;
    
    pt2[0] = pt1[1];
    pt2[1] = pt1[0];
    return ret;
}


/* SPI frame opcodes */
#define KS_SPIOP_RD			3
#define KS_SPIOP_WR			2

#define SPI_ADDR_S			24
#define SPI_ADDR_M			((1 << SPI_ADDR_S) - 1)
#define SPI_TURNAROUND_S		5


 
#define reteck(ret)     \
        if(ret < 0){    \
            printf("%m! \"%s\" : line: %d\n", __func__, __LINE__);   \
            goto lab;   \
        }
 
#define help() \
    printf("ksz_reg:\n");                  \
    printf("read operation: ksz_reg dev mode reg_addr\n");          \
    printf("write operation: ksz_reg dev mode reg_addr value\n");    \
    printf("For example:\n");            \
    printf("ksz_reg /dev/spi1.0 w 1\n");             \
    printf("ksz_reg /dev/spi1.0 w 0 0x12\n\n");     \ 
    printf("mode: w -> word, s -> short, b -> byte");  

int fd;

int read_reg32(unsigned int reg, unsigned int *res)
{
    
    unsigned int addr,val;
    struct spi_ioc_transfer  tr[2];
    int ret;
    
    memset( (unsigned char *)tr,0,sizeof(struct spi_ioc_transfer)*2);
    *res = 0;
    addr = reg & SPI_ADDR_M;
    addr |= KS_SPIOP_RD << SPI_ADDR_S;
    addr <<= SPI_TURNAROUND_S;        

    addr = be32toh(addr);
    tr[0].tx_buf = (__u64)&addr;
    tr[0].len = 4;
    tr[1].rx_buf = (__u64)&val;
    tr[1].len = 4;   
    ret = ioctl(fd, SPI_IOC_MESSAGE(2), &tr);
    if (ret < 0){
        printf("send spi message error %d\n",ret);  
        return ret;
    }  
#if __BYTE_ORDER == __BIG_ENDIAN 
    *res = (val);
#else
    *res = be2le_u32(val);
#endif    
    

    return 0; 
}

int read_reg16(unsigned int reg, unsigned short *res)
{
    
    unsigned int addr;
    struct spi_ioc_transfer  tr[2];
    int ret;
    unsigned short val;
    
    memset( (unsigned char *)tr,0,sizeof(struct spi_ioc_transfer)*2);
    *res = 0;
    addr = reg & SPI_ADDR_M;
    addr |= KS_SPIOP_RD << SPI_ADDR_S;
    addr <<= SPI_TURNAROUND_S;        

    addr = be32toh(addr);
    tr[0].tx_buf = (__u64)&addr;
    tr[0].len = 4;
    tr[1].rx_buf = (__u64)&val;
    tr[1].len = 2;   
    ret = ioctl(fd, SPI_IOC_MESSAGE(2), &tr);
    if (ret < 0){
        printf("send spi message error %d\n",ret);  
        return ret;
    }  

#if __BYTE_ORDER == __BIG_ENDIAN 
    *res = (val);
#else
    *res = be2le_u16(val);
#endif 
    return 0; 
}

int read_reg8(unsigned int reg, unsigned char *res)
{
    
    unsigned int addr;
    struct spi_ioc_transfer  tr[2];
    int ret;
    
    memset( (unsigned char *)tr,0,sizeof(struct spi_ioc_transfer)*2);
    *res = 0;
    addr = reg & SPI_ADDR_M;
    addr |= KS_SPIOP_RD << SPI_ADDR_S;
    addr <<= SPI_TURNAROUND_S;        

    addr = be32toh(addr);
    tr[0].tx_buf = (__u64)&addr;
    tr[0].len = 4;
    tr[1].rx_buf = (__u64)res;
    tr[1].len = 1;   
    ret = ioctl(fd, SPI_IOC_MESSAGE(2), &tr);
    if (ret < 0){
        printf("send spi message error %d\n",ret);  
        return ret;
    }  

    return 0; 
}
 
 
int write_reg32(unsigned int reg, unsigned int val)
{
    
    unsigned int addr,val2;
    struct spi_ioc_transfer  tr[2];
    int ret;
    
    memset( (unsigned char *)tr,0,sizeof(struct spi_ioc_transfer)*2);

    addr = reg & SPI_ADDR_M;
    addr |= KS_SPIOP_WR << SPI_ADDR_S;
    addr <<= SPI_TURNAROUND_S;        

    addr = be32toh(addr);
    tr[0].tx_buf = (__u64)&addr;
    tr[0].len = 4;
    
    val2 = val;
    val2 = be32toh(val2);
    tr[1].tx_buf = (__u64)&val2;
    tr[1].len = 4;   
    ret = ioctl(fd, SPI_IOC_MESSAGE(2), &tr);
    if (ret < 0){
        printf("send spi message error %d\n",ret);  
        return ret;
    }  

    return 0; 
}

int write_reg16(unsigned int reg, unsigned short val)
{
    
    unsigned int addr;
    unsigned short val2;
    struct spi_ioc_transfer  tr[2];
    int ret;
    
    memset( (unsigned char *)tr,0,sizeof(struct spi_ioc_transfer)*2);

    addr = reg & SPI_ADDR_M;
    addr |= KS_SPIOP_WR << SPI_ADDR_S;
    addr <<= SPI_TURNAROUND_S;        

    addr = be32toh(addr);
    tr[0].tx_buf = (__u64)&addr;
    tr[0].len = 4;
    
    val2 = val;
    val2 = be16toh(val2);
    tr[1].tx_buf = (__u64)&val2;
    tr[1].len = 2;   
    ret = ioctl(fd, SPI_IOC_MESSAGE(2), &tr);
    if (ret < 0){
        printf("send spi message error %d\n",ret);  
        return ret;
    }  

    return 0; 
}

int write_reg8(unsigned int reg, unsigned char val)
{
    
    unsigned int addr;
    unsigned char val2;
    struct spi_ioc_transfer  tr[2];
    int ret;
    
    memset( (unsigned char *)tr,0,sizeof(struct spi_ioc_transfer)*2);

    addr = reg & SPI_ADDR_M;
    addr |= KS_SPIOP_WR << SPI_ADDR_S;
    addr <<= SPI_TURNAROUND_S;        

    addr = be32toh(addr);
    tr[0].tx_buf = (__u64)&addr;
    tr[0].len = 4;
    
    val2 = val;

    tr[1].tx_buf = (__u64)&val2;
    tr[1].len = 1;   
    ret = ioctl(fd, SPI_IOC_MESSAGE(2), &tr);
    if (ret < 0){
        printf("send spi message error %d\n",ret);  
        return ret;
    }  

    return 0; 
}

int main(int argc, char *argv[]){
        

    int ret;
    char *dev;
    int mode = 0; // 0-32bit, 1-16bit,2-8bit
    char *pt;
    unsigned int reg,val32,addr,val;
    unsigned char val8;
    unsigned short val16;
 
     if(argc == 1 || !strcmp(argv[1], "-h") || (argc !=4 && argc != 5)  ){
        help();
        return 0;
    }
    
    //memset( (unsigned char *)tr,0,sizeof(struct spi_ioc_transfer)*2);
    
    dev = argv[1];
    pt = argv[2];
    if(*pt == 'w')
        mode = 0;
    else if(*pt == 's')
        mode = 1;
    else
        mode = 2;
    
    
    fd = open(dev,O_RDWR);
	if(fd < 0){
		printf("open dev %s error\r\n",dev);
		return 0;
	}

    if(argc == 4){
 
        reg    = (uint32_t)strtoul(argv[3], NULL, 0);
        
        if(mode == 0){
            read_reg32(reg,&val32);
            printf("read reg 0x%x : value 0x%08x\n",reg,val32);
        }
        else if(mode == 1){
            read_reg16(reg,&val16);
            printf("read reg 0x%x : value 0x%04x\n",reg,val16);
        }
        else{
            read_reg8(reg,&val8);
            printf("read reg 0x%x : value 0x%02x\n",reg,val8);
        }        
	}else if(argc == 5){
        reg    = (uint32_t)strtoul(argv[3], NULL, 0);
        
        
        if(mode == 0){
            val32    = (uint32_t)strtoul(argv[4], NULL, 0);
            write_reg32(reg,val32);
            printf("write reg 0x%x : value 0x%08x\n",reg,val32);
        }
        else if(mode == 1){
            val16    = (uint16_t)strtoul(argv[4], NULL, 0);
            write_reg16(reg,val16);
            printf("write reg 0x%x : value 0x%04x\n",reg,val16);
        }
        else{
            val8    = (uint8_t)strtoul(argv[4], NULL, 0);
            write_reg8(reg,val8);
            printf("write reg 0x%x : value 0x%02x\n",reg,val8);
        } 
    }
 

    close(fd);
    return 0;
}