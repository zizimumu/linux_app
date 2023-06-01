#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <math.h>

 
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

#define SAMPLE_FREQ 48000
#define SIGNAL_FREQ 200

#define T (1/SIGNAL_FREQ)

#define SRAM_ADDR 0x61000000
#define DAM_ADDR 0x03000000
#define FIFO_ADDR 0x60030000
#define pi (3.1415)

void write_u32(unsigned int *addr,unsigned int val)
{
	*addr = val;
}

unsigned int read_u32(unsigned int *addr)
{
	
	return *addr;
}

void start_dma()
{
	void *dma_base, *virt_addr;
	int fd;
	
	
	printf("long size %d, int size %d\n",sizeof(long), sizeof(int));
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
		printf("open mem error\n");
		return 0;
	}
	
	dma_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, DAM_ADDR & ~MAP_MASK);
    if(dma_base == (void *) -1) {
		printf("mmap DMA error\n");
		return 0;
	}
	
	virt_addr = dma_base + (DAM_ADDR & MAP_MASK);
	printf("Memory mapped at address %p, base addr %p\n", dma_base,virt_addr); 
	
	
	
	write_u32( (dma_base+4),1);
	
	write_u32( dma_base,0);
	write_u32( dma_base,1);
	

	//set len
	write_u32(dma_base + 8,MAP_SIZE);
	printf("addr %p, val 0x%x\n",dma_base + 8,read_u32(dma_base + 8));
	//dest 
	write_u32(dma_base + 0x10,FIFO_ADDR);
	printf("addr %p, val 0x%x\n",dma_base + 0x10,read_u32(dma_base + 0x10));
	//souce 
	write_u32(dma_base + 0x18, SRAM_ADDR);
	printf("addr %p, val 0x%x\n",dma_base + 0x18,read_u32(dma_base + 0x18));
	// configure,repeat
	write_u32(dma_base + 0x4, 0x44000008);
	// start， value must be 3
	write_u32(dma_base,0x3);

	printf("dma done\n");
	if(munmap(dma_base, MAP_SIZE) == -1) {
		printf("unmap error\n");
	};
    close(fd);
}
int main(int argc, char **argv) {
    int fd;
    void *sram_base, *virt_addr, *base; 
	unsigned long read_result, writeval;
	off_t target;
	float w, t,period,val;
	unsigned long point_num,i;
	short data;
	unsigned int *sram_buff;
	short *buff;


    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
		printf("open mem error\n");
		return 0;
	}


	buff = malloc(MAP_SIZE);
	if(buff == NULL)
		printf("malloc error\n");
	
    /* Map one page */
    sram_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, SRAM_ADDR & ~MAP_MASK);
    if(sram_base == (void *) -1) {
		printf("mmap SRAM error\n");
		return 0;
	}
    printf("Memory mapped at address %p.\n", sram_base); 

	memset(sram_base,0,MAP_SIZE);
	
	
	point_num = SAMPLE_FREQ/SIGNAL_FREQ;
	w = 2*pi * SIGNAL_FREQ;
	period = 1.0/SIGNAL_FREQ;
	printf("period %f\n",period);
	
    for(i=0;i<point_num;i++){
		val = sin(period* i * w/point_num );
		data = (short)(32768*val);
		buff[i*2] =  data;
		buff[i*2+1] = data;
		

	}




	
	base = sram_base;
	for(i=0;i< MAP_SIZE / (point_num*4) ; i++){
		memcpy(base,buff,point_num*4);
		base += (point_num*4);
	}
	


	printf("\n\nSRAM data:\n");
	sram_buff = sram_base;
    for(i=0;i<point_num;i++){

		
		if( (i % 16) == 0 )
			printf("\n");		
		printf("%08x ",sram_buff[i]);

	}	
	
	
	
	
	if(munmap(sram_base, MAP_SIZE) == -1) {
		printf("unmap error\n");
	};
    close(fd);
	
	free(buff);
	
	
	start_dma();
	
	while(1){
		sleep(1);
	}
	
	
    return 0;
}
