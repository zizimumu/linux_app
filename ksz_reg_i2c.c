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
#include <endian.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
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
    printf("ksz_reg /dev/spi1.0 w 0 0x12\n\n");      
 

int fd;
unsigned char i2c_addr;

int read_reg32(unsigned int reg, unsigned int *res)
{
    
 	struct i2c_rdwr_ioctl_data rdwr_data;
	struct i2c_msg msg[2];
	unsigned char addr[2];
	
	int ret;
	unsigned int val;

	memset( (unsigned char *)&rdwr_data,0,sizeof(struct i2c_rdwr_ioctl_data));
	memset( (unsigned char *)&msg,0,sizeof(struct i2c_msg)*2);

	addr[0] = (unsigned char) (reg>>8);
	addr[1] = (unsigned char) (reg);
	
	msg[0].addr = i2c_addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = &addr[0];
	

	msg[1].addr = i2c_addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 4;
	msg[1].buf = (unsigned char *)&val;


	rdwr_data.msgs = &msg[0];
	rdwr_data.nmsgs = 2;

    ret = ioctl(fd, I2C_RDWR, &rdwr_data);
    if (ret < 0){
        printf("send i2c message error %d\n",ret);  
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
    

 	struct i2c_rdwr_ioctl_data rdwr_data;
	struct i2c_msg msg[2];
	unsigned char addr[2];
	
	int ret;
	unsigned short val;

	memset( (unsigned char *)&rdwr_data,0,sizeof(struct i2c_rdwr_ioctl_data));
	memset( (unsigned char *)&msg,0,sizeof(struct i2c_msg)*2);

	addr[0] = (unsigned char) (reg>>8);
	addr[1] = (unsigned char) (reg);
	
	msg[0].addr = i2c_addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = &addr[0];
	

	msg[1].addr = i2c_addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = (unsigned char *)&val;


	rdwr_data.msgs = &msg[0];
	rdwr_data.nmsgs = 2;

    ret = ioctl(fd, I2C_RDWR, &rdwr_data);
    if (ret < 0){
        printf("send i2c message error %d\n",ret);  
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
    

 	struct i2c_rdwr_ioctl_data rdwr_data;
	struct i2c_msg msg[2];
	unsigned char addr[2];
	
	int ret;


	memset( (unsigned char *)&rdwr_data,0,sizeof(struct i2c_rdwr_ioctl_data));
	memset( (unsigned char *)&msg,0,sizeof(struct i2c_msg)*2);

	addr[0] = (unsigned char) (reg>>8);
	addr[1] = (unsigned char) (reg);
	
	msg[0].addr = i2c_addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = &addr[0];
	

	msg[1].addr = i2c_addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = (unsigned char *)res;


	rdwr_data.msgs = &msg[0];
	rdwr_data.nmsgs = 2;

    ret = ioctl(fd, I2C_RDWR, &rdwr_data);
    if (ret < 0){
        printf("send i2c message error %d\n",ret);  
        return ret;
    }  

	 return 0;     
}
 
 #if 1
int write_reg32(unsigned int reg, unsigned int val)
{
    
 	struct i2c_rdwr_ioctl_data rdwr_data;
	struct i2c_msg msg[2];
	unsigned char buf[8];
	
	int ret;

	memset( (unsigned char *)&rdwr_data,0,sizeof(struct i2c_rdwr_ioctl_data));
	memset( (unsigned char *)&msg,0,sizeof(struct i2c_msg)*2);

	buf[0] = (unsigned char) (reg>>8);
	buf[1] = (unsigned char) (reg);
	buf[2] = (unsigned char)(val >> 24);
	buf[3] = (unsigned char)(val >> 16);
	buf[4] = (unsigned char)(val >> 8);
	buf[5] = (unsigned char)(val);
	
	msg[0].addr = i2c_addr;
	msg[0].flags = 0;
	msg[0].len = 6;
	msg[0].buf = &buf[0];

	rdwr_data.msgs = &msg[0];
	rdwr_data.nmsgs = 1;

    ret = ioctl(fd, I2C_RDWR, &rdwr_data);
    if (ret < 0){
        printf("send i2c message error %d\n",ret);  
        return ret;
    }  


    return 0; 
}

int write_reg16(unsigned int reg, unsigned short val)
{
    
 	struct i2c_rdwr_ioctl_data rdwr_data;
	struct i2c_msg msg[2];
	unsigned char buf[8];
	
	int ret;

	memset( (unsigned char *)&rdwr_data,0,sizeof(struct i2c_rdwr_ioctl_data));
	memset( (unsigned char *)&msg,0,sizeof(struct i2c_msg)*2);

	buf[0] = (unsigned char) (reg>>8);
	buf[1] = (unsigned char) (reg);
	buf[2] = (unsigned char)(val >> 8);
	buf[3] = (unsigned char)(val);
	
	msg[0].addr = i2c_addr;
	msg[0].flags = 0;
	msg[0].len = 4;
	msg[0].buf = &buf[0];

	rdwr_data.msgs = &msg[0];
	rdwr_data.nmsgs = 1;

    ret = ioctl(fd, I2C_RDWR, &rdwr_data);
    if (ret < 0){
        printf("send i2c message error %d\n",ret);  
        return ret;
    }  


    return 0; 
}

int write_reg8(unsigned int reg, unsigned char val)
{
    
 	struct i2c_rdwr_ioctl_data rdwr_data;
	struct i2c_msg msg[2];
	unsigned char buf[8];
	
	int ret;

	memset( (unsigned char *)&rdwr_data,0,sizeof(struct i2c_rdwr_ioctl_data));
	memset( (unsigned char *)&msg,0,sizeof(struct i2c_msg)*2);

	buf[0] = (unsigned char) (reg>>8);
	buf[1] = (unsigned char) (reg);
	buf[2] = (unsigned char)(val );
	
	msg[0].addr = i2c_addr;
	msg[0].flags = 0;
	msg[0].len = 3;
	msg[0].buf = &buf[0];

	rdwr_data.msgs = &msg[0];
	rdwr_data.nmsgs = 1;

    ret = ioctl(fd, I2C_RDWR, &rdwr_data);
    if (ret < 0){
        printf("send i2c message error %d\n",ret);  
        return ret;
    } 

    return 0; 
}




#endif

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
    
    i2c_addr = 0x5f;
    fd = open(dev,O_RDWR);
	if(fd < 0){
		printf("open dev %s error\r\n",dev);
		return 0;
	}
    printf("opening device\n");

#if 1

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
 
#endif
    close(fd);
    return 0;
}
