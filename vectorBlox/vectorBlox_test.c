
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
#include <sys/stat.h>
#include <time.h>

#define CTRL_REG_SOFT_RESET 0x00000001
#define CTRL_REG_OUTPUT_VALID   0x00000008
#define CTRL_REG_ERROR   0x00000010




//
// #define VBLOX_REG_BASE 0x60030000  // V500
#define VBLOX_REG_BASE 0x60070000	// V1000
#define BASE_DDR_ADDR 0xc1000000
#define FW_DDR_ADDR BASE_DDR_ADDR

// 0xc1400000
#define MODEL_DDR_ADDR (FW_DDR_ADDR+0x400000)

// 64MB
// 0xc5000000
#define INPUT_DDR_ADDR (BASE_DDR_ADDR + 0x4000000)
// 0xc5200000
#define OUTPUT_DDR_ADDR (INPUT_DDR_ADDR + 0x200000)
// 0xc5300000
#define IO_BUFF_DDR_ADDR (OUTPUT_DDR_ADDR + 0x100000)


#define PAGE_SIZE 4096UL
#define PAGE_MASK (PAGE_SIZE - 1)

#define writel(value, addr) \
	(*(volatile unsigned int *)(addr)) = (value)
#define readl(addr) \
	(*(volatile unsigned int *)(addr))




typedef int32_t fix16_t;

static const fix16_t fix16_one = 0x00010000; /*!< fix16_t value of 1 */
static inline float   fix16_to_float(fix16_t a) { return (float)a / fix16_one; }




void write_register(void *addr, unsigned int val)
{
	writel(val,addr);
}

unsigned int read_register(void *addr)
{
	return readl(addr);
}
	
	
	
void vblox_init(void *reg_base )
{
	//++++++++++++++ 初始化
	//software reset 
	write_register(reg_base, CTRL_REG_SOFT_RESET);
	//set Firemware BLOB address, must aligned to 2MB and >= 0x20_0000
	write_register(reg_base+0x8, FW_DDR_ADDR);
	// set soft_reset from 1 to 0, to trigger VectorBlox start internal processor to start the Firemware
	write_register(reg_base, 0);
}

void vblox_start(void *reg_base)
{
	//unsigned int *io_buf = (unsigned int *)IO_BUFF_DDR_ADDR;
	// +++++++++++start
	// wait until start bit is low before starting next model
	while (read_register(reg_base) & 0x2);
	
	// input buf addr
	//io_buf[0] = INPUT_DDR_ADDR;
	// output buf addr
	//io_buf[1] = OUTPUT_DDR_ADDR;

	// model  input/out buffer address, aligned to 8 and >= 0x20_0000
	// the IO buffer contain 64 bit address of input/out buffer
	write_register(reg_base+0x18 , IO_BUFF_DDR_ADDR);
	
	
	 // Address Pointing to Model BLOB.
	write_register(reg_base+0x10 , MODEL_DDR_ADDR);
	




	//start the network, Start should be written with 1, other bits should be written with zeros.
	write_register(reg_base,0x2);
	
	// printf("start CNN, ctr is 0x%x,status is 0x%x\n",read_register(reg_base),read_register(reg_base+4));
}


// +++++++++++++++ 等待完成
int vbx_cnn_model_poll(void *reg_base) 
{
  unsigned int status = read_register(reg_base);
  
  if (status & CTRL_REG_SOFT_RESET) {
	  
	   printf("wait CNN reset,ctr 0x%x, status 0x%x\n",status,read_register(reg_base+0x4));
       return -3;
  }
  if (status & CTRL_REG_ERROR) {
	  printf("wait CNN error,status 0x%x\n",read_register(reg_base+0x4));
      return -1;
  }
  if (status & CTRL_REG_OUTPUT_VALID) {
    // write 1 to clear output valid
    write_register(reg_base,CTRL_REG_OUTPUT_VALID);
    return 0;
  }
  
  return 1;

}



int load_file(unsigned long addr, char *file)
{



	int fd_file,fd_mem;
	void *map_base, *virt_addr; 
	unsigned long size ,fileSize;
	struct stat stFile;
	int ret;

    if((fd_file = open(file, O_RDWR)) == -1) {
		printf("open file %s error\n",file);
		return -1;
	}
	fstat(fd_file, &stFile);
	fileSize = stFile.st_size;
		
		
		
	
    if((fd_mem = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
		printf("open mem dev error\n");
		return -1;
	}


    size = fileSize & (~PAGE_MASK);
	if(fileSize &  PAGE_MASK)
		size += PAGE_SIZE;
	
	
    map_base = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, addr & ~PAGE_MASK);
    if(map_base == (void *) -1){
		printf("mmap failed\n");
		return -1;
	}
 

    printf("file %s size %d, map phy addr 0x%x to virt 0x%lx, map size %d\n",file, fileSize,addr,map_base,size);
    virt_addr = map_base + (addr & PAGE_MASK); 
	
	ret = read(fd_file,virt_addr,fileSize);
	if(ret !=  fileSize){
		printf("read file error\n");
		return -1;
	}
	
	munmap(map_base, size) ;
    close(fd_mem);
	close(fd_file);
	
	return 0;
	
}



void set_io_buf(void)
{
	
	int fd_mem;
	unsigned long size;
	void *map_base; 

	// the IO buffer must be 64bit unit
	unsigned long *io_buf;
	
	
    if((fd_mem = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
		printf("open mem dev error\n");
		return ;
	}

	size = PAGE_SIZE;
    map_base = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, IO_BUFF_DDR_ADDR & ~PAGE_MASK);
    if(map_base == (void *) -1){
		printf("mmap failed\n");
		return;
	}	
	
	io_buf = map_base;
	
	// input buf addr
	io_buf[0] = INPUT_DDR_ADDR;
	// output buf addr
	io_buf[1] = OUTPUT_DDR_ADDR;

	
	
	munmap(map_base, size) ;
	close(fd_mem);	
}



void get_output(float *buf,int len)
{
	int fd_mem;
	unsigned long size;
	void *map_base; 
	fix16_t *out;
	//float *out;
	int i;
	
    if((fd_mem = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
		printf("open mem dev error\n");
		return ;
	}

	size = PAGE_SIZE;
    map_base = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, OUTPUT_DDR_ADDR & ~PAGE_MASK);
    if(map_base == (void *) -1){
		printf("mmap failed\n");
		return;
	}
	
	out = map_base;
	for(i=0;i<len;i++){
		
		buf[i] = fix16_to_float(out[i]);
		//buf[i] = (out[i]);
	}
 

	munmap(map_base, size) ;
	close(fd_mem);
}

void print_float(float *buf, int len)
{
	int i;
	for(i=0;i<len;i++){
		if(i % 16 == 0)
			printf("\n");
		
		printf("%08f ",buf[i]);
	}
	printf("\n");
}



#define MODEL_OUTPUT_SIZE 1001



int wait_inference(void *map_base,float *fp_buf)
{
	int ret = 0;
	
	while(1){
		ret = vbx_cnn_model_poll(map_base);
		if(!ret){
			printf("inference done\n");
			get_output(fp_buf, MODEL_OUTPUT_SIZE);
			// print_float(fp_buf,32);	
			return 0;
		}
		else if(ret < 0){
			printf("VNN return error\n");
			return -1;
		}
		usleep(1000);
	}
	
	return 0;
}

int find_max(float *fp_buf, int len)
{
	int i,ret = -1;
	float max = fp_buf[0];
	
	for(i=0;i<len;i++){
		if(fp_buf[i] > max){
			max = fp_buf[i];
			ret = i;
		}
	}
	
	return ret;
}



long get_ms(struct timespec *spec_start,struct timespec *spec_end)
{
	long msec;
	msec = (spec_end->tv_nsec - spec_start->tv_nsec) / 1000000;
	msec = (spec_end->tv_sec - spec_start->tv_sec) * 1000 + msec;
	
	return msec;
}

int wait_inference_poll(void *map_base,float *fp_buf)
{
	int ret = 0;
	
	while(1){
		ret = vbx_cnn_model_poll(map_base);
		if(!ret){
			return 0;
		}
		else if(ret < 0){
			printf("VNN return error\n");
			return -1;
		}
	}
	
	return 0;
}

void inference_time(void *map_base,float *fp_buf)
{
	int i,ret;
	struct timespec spec_start;
	struct timespec spec_end;
	
	
	clock_gettime(CLOCK_MONOTONIC, &spec_start);
	for(i=0;i<10;i++){
		vblox_start(map_base);
		ret = wait_inference_poll(map_base,fp_buf);	
	}
	clock_gettime(CLOCK_MONOTONIC, &spec_end);
	
	//printf("start time is %lds,%ldns\n", spec_start.tv_sec,spec_start.tv_nsec);
	//printf("end   time is %lds,%ldns\n", spec_end.tv_sec,spec_end.tv_nsec);
	printf("inference speed is %dms/frame\n",get_ms(&spec_start,&spec_end) / 10);
}




int main(int argc, char *argv[])
{
	int fd_mem;
	void *map_base;
	int ret;
	float *fp_buf;
	int count,val;
	
	if(argc <= 3){
		printf("parameter error: usage %s fireware.bin model.vnnx input.bin\n",argv[0]);
		return 0;
	}
	
	

	
	printf("sizeof float %d, long %d\n",sizeof(float),sizeof(unsigned long));
	
	
	load_file(FW_DDR_ADDR,argv[1]);

	
	
	load_file(MODEL_DDR_ADDR,argv[2]);
	load_file(INPUT_DDR_ADDR,argv[3]);
	
	set_io_buf();
	
	

    if((fd_mem = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
		printf("open mem dev error\n");
		return -1;
	}

    map_base = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, VBLOX_REG_BASE & ~PAGE_MASK);
    if(map_base == (void *) -1){
		printf("VNN base mmap failed\n");
		return -1;
	}	

	
	
	
	
	vblox_init(map_base);
	printf("init done\n");
	
	
	

	
	
	fp_buf = malloc(MODEL_OUTPUT_SIZE*sizeof(float));
	//vblox_init(map_base);


	
	count = 0;
	while(1){
		vblox_start(map_base);
		printf("start...\n");
	
		ret = wait_inference(map_base,fp_buf);
		if(!ret){
			val = find_max(fp_buf,MODEL_OUTPUT_SIZE);
			if(val >= 0)
				printf("max output value %f, index %d\n",fp_buf[val],val);
			count++;
			if(count >= 3)
				break;
		}else
			break;
	}
	
	
	
	inference_time(map_base,fp_buf);
	
	write_register(map_base, CTRL_REG_SOFT_RESET);
	
	
	
	munmap(map_base, PAGE_SIZE) ;
	close(fd_mem);
	
	free(fp_buf);
	
	return 0;


}