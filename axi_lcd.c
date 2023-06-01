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
#define FPGA_DMA_BASE 0x60020000


void write_u32(unsigned int *addr,unsigned int val)
{
	*addr = val;
}

unsigned int read_u32(unsigned int *addr)
{
	
	return *addr;
}


void start_dma(void *sram_base)
{
		// mask IRQ, this step is a must-be
	write_u32(sram_base+0x14,0x1);
	//clear irq
	write_u32(sram_base+0x18,0xF);
	//source addr: SRAM
	write_u32(sram_base+0x68,0xC0000000);



	// dest addr : ddr
	write_u32(sram_base+0x6c,0x60070000);
	// len : ddr
	write_u32(sram_base+0x64,0x177000);

	// set config:
	write_u32(sram_base+0x60,0x0000E005);
	// start
	write_u32(sram_base+0x4,0x1);
	
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

	
    /* Map one page */
    sram_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, FPGA_DMA_BASE & ~MAP_MASK);
    if(sram_base == (void *) -1) {
		printf("mmap SRAM error\n");
		return 0;
	}
    printf("Memory mapped at address %p.\n", sram_base); 
	
	
	
	


	
	start_dma(sram_base);
	while(1){
		if( read_u32(sram_base+0x10) & 0x00000001 != 0){
			start_dma(sram_base);
		}
	}	
	
	
	if(munmap(sram_base, MAP_SIZE) == -1) {
		printf("unmap error\n");
	};
	
	
	
    return 0;
}
