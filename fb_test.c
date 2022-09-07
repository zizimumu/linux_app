// test.c
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <linux/fb.h>   
#include <fcntl.h>             
#include <unistd.h>
#include <sys/mman.h> 
 #include<time.h>
#include <string.h>
 

//RGB565 mode
#define COLOR_TYPES 20


#define BLACK_CORLOR 0x0000
#define GRAY_COLOR 0x78ef
#define WHITE_CORLOR 0xFFFF
#define RED_CORLOR 0xF800
#define GREEN_CORLOR 0x07E0
#define BLUE_CORLOR 0x001f
#define PURPLE_CORLOR 0xa254
#define ORANGE_CORLOR 0xfbe4


unsigned short color_arr[COLOR_TYPES] = {0xfe41,0x0000,0xff80,0x78ef,
				0x8802,0xe8c4,0xfbe4,0x2589,
				0x7497,0xf800,0x051d,0x3a39,
				0xa254,0xc618,0xbbca,0xfd79,
				0xb723,0x9edd,0x001f,0x07e0};



#define RED_CORLOR_32 0xff000000

#define WIDE_DIV 5

void fill_reg(unsigned char *buf,int st_x, int st_y, int width, struct fb_var_screeninfo *vinfo)
{
	unsigned int line = vinfo->xres*vinfo->bits_per_pixel/ 8;
	int i,j;
	unsigned int *pt32;
	unsigned short *pt16;
	
	for(i=st_y;i<st_y+width;i++){

		if(vinfo->bits_per_pixel == 32){
			pt32 = (unsigned int *)(buf+ i*line);
			for(j=st_x;j<st_x+width;j++){
				pt32[j] = 0xff000000;
			}
		}
		else if(vinfo->bits_per_pixel == 16){
			pt16 = (unsigned short *)(buf+ i*line);
			for(j=st_x;j<st_x+width;j++){
				pt16[j] = RED_CORLOR;
			}				
		}			
		
	}
}
void init_fb_fill(struct fb_var_screeninfo *vinfo,unsigned char *buf)
{
	unsigned int line = vinfo->xres*vinfo->bits_per_pixel/ 8;
	unsigned int i,j,pixs;
	unsigned int *pt32;
	unsigned short *pt16;
	
	pixs = vinfo->xres * vinfo->yres / WIDE_DIV;
	pt32 = (unsigned int *)buf;
	pt16 = (unsigned short *)buf;
	for(i=0;i<pixs;i++){

		if(vinfo->bits_per_pixel == 32){
			pt32[i] = RED_CORLOR_32;
		}
		else if(vinfo->bits_per_pixel == 16){
			pt16[i] = RED_CORLOR;
			
		}			
		
	}

}

void start_step(struct fb_var_screeninfo *vinfo,unsigned char *buf,unsigned char *fb)
{
	unsigned int st1_x,i;
	unsigned int st1_y;
	unsigned int *pt32,*pt32_s,*pt32_e;
	unsigned short *pt16,*pt16_s,*pt16_e;
	unsigned int line,screensize;
	int up = 1 ;
	unsigned int wide,cnt = 0;
	struct timespec time_st;
	struct timespec time_end;
	
	st1_x = 0;
	st1_y = 0;
	
	wide = vinfo->yres/WIDE_DIV;
	line = vinfo->xres*vinfo->bits_per_pixel/8;
	screensize = vinfo->xres * vinfo->yres * vinfo->bits_per_pixel / 8; 
	
	clock_gettime(CLOCK_MONOTONIC, &time_st);
	while(1){
		
		if( (up) && (st1_y +  wide < vinfo->yres)){
			//printf("up 1\n");
			
			//pt32_s = (unsigned int *) (line * st1_y+buf);
			//pt32_e = (unsigned int *) (line * (st1_y+wide)+buf);
			pt16_s = (unsigned short *) (line * st1_y+buf);
			pt16_e = (unsigned short *) (line * (st1_y+wide)+buf);			
			for(i=0;i<vinfo->xres;i++){
				pt16_s[i] = 0;
				pt16_e[i] = RED_CORLOR;
			}
			st1_y++;
			
			//printf("up 2: %d\n",st1_y);
		
		}
		else if((up) && (st1_y + wide >= vinfo->yres)){
			up = 0;
			st1_y--;
			//printf("up over\n");

		}
		else if((!up) && (st1_y)){
			//printf("down 1");
			
			pt16_s = (unsigned short *) (line * st1_y+buf);
			pt16_e = (unsigned short *) (line * (st1_y+wide)+buf);
			for(i=0;i<vinfo->xres;i++){
				pt16_e[i] = 0;
				pt16_s[i] = RED_CORLOR;
			}
			st1_y--;

			//printf("down 2");
		}
		else if((!up) && (!st1_y)){
			up = 1;
			//printf("down over\n");
		}
		
		memcpy(fb,buf,screensize);
		cnt++;
		if((cnt % 1000) == 0){
			
			memcpy(&time_end,&time_st,sizeof(time_st));
			clock_gettime(CLOCK_MONOTONIC, &time_st);
			
			if(time_st.tv_sec - time_end.tv_sec > 0 ){
				printf("current cpu write speed %dframes/s\n",1000/(time_st.tv_sec - time_end.tv_sec));
			}
			
		}
		
	}
}

void display_color16_step(unsigned int width,unsigned int height, unsigned short *fb_buf,int mode)
{
	unsigned int i,j;
	unsigned short * pt16, *buf;
	unsigned int color;

	buf = malloc(height*width*2);

	pt16 = buf;
	//the color step is from vertical
	if(mode == 1){
		for(i=0;i<height;i++){
			color = i* 0xffff/height;
			for(j=0;j<width;j++){
				pt16[j+width*i] = color;
			}
		}
	}
	else{
		for(i=0;i<width;i++){
			color = i* 0xffff/width;
			
			for(j=0;j<height;j++){
				pt16[i+j*width] = color;
			}
		}
	}

	memcpy(fb_buf,buf,height*width*2);
	free(buf); 
	return ;
}



void display_color16_bar(unsigned int width,unsigned int height, unsigned short *fb_buf,int mode)
{
	unsigned int i,j,k;
	unsigned short * pt16, *buf;
	unsigned int color;
	float id;

	buf = malloc(height*width*2);
	memset(buf,0x0,height*width*2);

	for(k=0;k<height;k++){
		for(i=0;i<width;i++){
			id = (float)(COLOR_TYPES - 1) * i / (float)width;
			buf[i+k*width] = color_arr[(int)id];
		}

	}
	memcpy(fb_buf,buf,height*width*2);
	free(buf); 
	return ;
}



void fill_rectangle(struct fb_var_screeninfo *vinfo, unsigned short *pt, int x, int y,int width,int height, unsigned short color)
{
    unsigned short *buf;
    int i,j;

    for(i=0;i<height;i++){
        
        buf = pt + (y+i)*vinfo->xres + x;
        for(j=0;j<width;j++){
            
            buf[j] = color;
            
        }
    }
    
}

int main(char argc,char *argv[])  
{     
	 int fbfd = 0,i,j,count = 0,wide;   
	 struct fb_var_screeninfo vinfo;   
	 struct fb_fix_screeninfo finfo;   
	 long int screensize = 0;  
	char *fbp;
	char *buf,*src;
	unsigned short *pt16;
	unsigned int *pt32;
	int delay = 0;
	unsigned short corlor[]= {WHITE_CORLOR,RED_CORLOR,GREEN_CORLOR,BLUE_CORLOR,BLACK_CORLOR};
	char name[32];
	int fd_f;

	struct timespec time_st;
	struct timespec time_end;
	unsigned int line;

 #if 0
	if(argc < 3){
		printf("param err\n");
		printf("use : %s delay count\n",argv[0]);
		return 1;
	}
#endif	
	//delay = atoi(argv[1]);
	//count = atoi(argv[2]);
	//printf("input delay is %d ms, count %d\n",delay,count);
	/*打开设备文件*/   
	fbfd = open("/dev/fb0", O_RDWR);   
	if(fbfd <= 0){
		printf("open device err\n");
		return ;
	}
	  
	/*取得屏幕相关参数*/   
	 ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);     
	 ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);  

 
	vinfo.activate |= FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;
	if(0 > ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo)) {
	  printf("Failed to refresh\n");
	  return -1;
	}



	printf("xres %d ,yres %d,bits_per_pixel %d\n",vinfo.xres,vinfo.yres,vinfo.bits_per_pixel); 
	printf("frame buffer physic address 0x%x\n",finfo.smem_start);	  
	/*计算屏幕缓冲区大小*/   
	 screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;  
	line = vinfo.xres*vinfo.bits_per_pixel / 8;
	 
	buf = malloc(screensize*COLOR_TYPES);
	memset(buf,0x00,screensize*COLOR_TYPES);
	


#if 0
	wide = screensize/2;
	for(i=0;i<COLOR_TYPES;i++){
		pt = (unsigned short *)(buf+i*screensize );
		
		
		for(j=0;j<(wide);j++){
			
			if(j>= i*(wide/COLOR_TYPES) && j<= (i*(wide/COLOR_TYPES) + wide/COLOR_TYPES)){
				*pt = RED_CORLOR;
			}
			pt++;
			
		}
		
		//sprintf(name,"rgb%d.bin",i);
		//fd_f = open(name,O_RDWR | O_CREAT | O_TRUNC);
		//write(fd_f,buf+i*screensize,screensize);
		//close(fd_f);
	}
#endif
	/*映射屏幕缓冲区到用户地址空间*/   
	fbp=(char*)mmap(0,screensize,PROT_READ|PROT_WRITE,MAP_SHARED, fbfd, 0);
	if(fbp == MAP_FAILED)
		printf("mmap err %d\n",(int)fbp);
	  

	
	
#if 0
	clock_gettime(CLOCK_MONOTONIC, &time_st);
	//while(count>0){
		for(i=0;i<COLOR_TYPES;i++){
			src = buf+i*screensize;
			memcpy(fbp,src,screensize);
			if(delay){
				usleep(delay*1000);
				printf("change color\n");
			}
		}
		
	//	count--;
	//}
	clock_gettime(CLOCK_MONOTONIC, &time_end);
#endif	
	//printf("test done , start %ld, end %ld\n",time_st.tv_sec,time_end.tv_sec);

	init_fb_fill(&vinfo,buf);
	memcpy(fbp,buf,screensize);
	
	
	
	//start_step(&vinfo,buf,fbp);
	
#if 0
	pt16 = (unsigned short *)fbp;
	while(1){
		
		for(i=0;i<vinfo.xres * vinfo.yres;i++)
			pt16[i] = RED_CORLOR;
		
		usleep(500000);
		for(i=0;i<vinfo.xres * vinfo.yres;i++)
			pt16[i] = GREEN_CORLOR;
		
		usleep(500000);
		for(i=0;i<vinfo.xres * vinfo.yres;i++)
			pt16[i] = BLUE_CORLOR;
		usleep(500000);
	}
#endif	

    while(1){
        fill_rectangle(&vinfo,(unsigned short *)fbp,200,100,200,200,RED_CORLOR);
        usleep(500000);
        fill_rectangle(&vinfo,(unsigned short *)fbp,200,100,200,200,GREEN_CORLOR);
        usleep(500000);
    }
	//display_color16_bar(vinfo.xres,vinfo.yres,(unsigned short *)fbp,1);
	while(1);

#if 0
	while(1){
		display_color16_step(vinfo.xres,vinfo.yres,(unsigned short *)fbp,1);
		usleep(8000000);
		display_color16_step(vinfo.xres,vinfo.yres,(unsigned short *)fbp,0);
		usleep(8000000);

	}
#endif

	munmap(fbp, screensize);  
	free(buf); 
	close(fbfd);  
} 
