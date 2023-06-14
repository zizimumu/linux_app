#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>  
#include <fcntl.h>             
#include <unistd.h>
#include <sys/mman.h> 
 #include<time.h>
#include <linux/videodev2.h>
#include <linux/fb.h>  

struct buffer{  
	void *start;  
	unsigned int length;  
}*buffers; 


struct lcd_dev{
	int fbfd;
	void *fbfp;
	unsigned int bits_per_pixel;
	int lcd_w;
	int lcd_h;
	long screensize;
	
	
};

#define CAM_PIX_FMT V4L2_PIX_FMT_YUYV
#define  VIDEO_WIDTH  640
#define  VIDEO_HEIGHT  480
#define DEV_PATH "/dev/video0" //0/9 
#define CAPTURE_FILE "yuv_raw.bin"




#define CLIPVALUE(x, minValue, maxValue) ((x) < (minValue) ? (minValue) : ((x) > (maxValue) ? (maxValue) : (x)))
#define YUVToR(Y, U, V) ( (Y) + 1.4075 * ((V) - 128) )
#define YUVToG(Y, U, V) ( (Y) - 0.3455 * ((U) - 128) - 0.7169 * ((V) - 128) )
#define YUVToB(Y, U, V) ( (Y) + 1.779 * ((U) - 128) )



static void YUV422p_to_RGB24(unsigned char *yuv422[3], unsigned char *rgb24, int width, int height)
{
	int R,G,B,Y,U,V;
	int x,y;
	int nWidth = width>>1; //色度信号宽度
	for (y=0;y<height;y++)
	{
		for (x=0;x<width;x++)
		{
			Y = *(yuv422[0] + y*width + x);
			U = *(yuv422[1] + y*nWidth + (x>>1));
			V = *(yuv422[2] + y*nWidth + (x>>1));
			R = Y + 1.402*(V-128);
			G = Y - 0.34414*(U-128) - 0.71414*(V-128);
			B = Y + 1.772*(U-128);

			//防止越界
			if (R>255)R=255;
			if (R<0)R=0;
			if (G>255)G=255;
			if (G<0)G=0;
			if (B>255)B=255;
			if (B<0)B=0;

			*(rgb24 + ((height-y-1)*width + x)*3) = B;
			*(rgb24 + ((height-y-1)*width + x)*3 + 1) = G;
			*(rgb24 + ((height-y-1)*width + x)*3 + 2) = R; 
		}
	}
}




// only support 32 bit lcd pixdeep
int yuv422torgb(struct lcd_dev *lcd_dev,unsigned char *rgbImageData, int lcd_w, int lcd_h, unsigned char *yuvImageData, short width, short height)
{
	unsigned char* pRGB;
	unsigned char* pY;
	unsigned char* pU;
	unsigned char* pV,*pt;
	unsigned short val;
	unsigned char b,g,r;
	int y, u, v;
	int frame_size = width * height;

	int offset = ((lcd_w - width) / 2) * (lcd_dev->bits_per_pixel/8);

	
	if (yuvImageData == NULL)
	{
		return -1;
	}

	if (rgbImageData == NULL)
	{
		return -1;
	}


	
	for (int j = 0; j < height; j++)
	{
		pRGB = rgbImageData + j * lcd_w * (lcd_dev->bits_per_pixel/8) + offset;
		pY = yuvImageData + j * width*2;
		//pU = pY + 1 ;
		//pV = pY + 3;
		
		
		for (int i = 0; i < width; i++)
		{
			if( (i % 2) == 0){
				pt = pY + i*2;
				y = *pt;
				u = *(pt + 1);
				v = *(pt + 3);
			}
			else{
				pt = pY + i*2;
				y = *(pt);
				u = *(pt -1);
				v = *(pt + 1);				
			}

			
			if(lcd_dev->bits_per_pixel == 24){
				*(pRGB) = CLIPVALUE(YUVToB(y, u, v), 0, 255);
				*(pRGB + 1) = CLIPVALUE(YUVToG(y, u, v), 0, 255);
				*(pRGB + 2) = CLIPVALUE(YUVToR(y, u, v), 0, 255);				
				pRGB += 3;
			}
			else if(lcd_dev->bits_per_pixel == 32){
				*(pRGB) = CLIPVALUE(YUVToB(y, u, v), 0, 255);
				*(pRGB + 1) = CLIPVALUE(YUVToG(y, u, v), 0, 255);
				*(pRGB + 2) = CLIPVALUE(YUVToR(y, u, v), 0, 255);
				pRGB += 4;
			}
			else if(lcd_dev->bits_per_pixel == 16){
				b = CLIPVALUE(YUVToB(y, u, v), 0, 255);
				b = b >> 3;
				g = CLIPVALUE(YUVToG(y, u, v), 0, 255);
				g = g >> 2;
				r = CLIPVALUE(YUVToR(y, u, v), 0, 255);	
				r = r >> 3;
				*(unsigned short *)pRGB = ((unsigned short)r << 11) | ( (unsigned short)g  << 5 )  | (unsigned short)b ;
				
				pRGB += 2;
			}
		}
	}

	return 0;
}


int lcd_init(struct lcd_dev *lcd_dev)
{
	 
 
	struct fb_var_screeninfo vinfo;   
	struct fb_fix_screeninfo finfo;   
	long int screensize = 0;  

	int fbfd;
	void *fbp;


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
	/*计算屏幕缓冲区大小*/   
	 screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;  


	printf("fd device: xres %d, yres %d, pixel deep %d\n",vinfo.xres,vinfo.yres,vinfo.bits_per_pixel);
	/*映射屏幕缓冲区到用户地址空间*/   
	fbp= mmap(0,screensize,PROT_READ|PROT_WRITE,MAP_SHARED, fbfd, 0);
	if(fbp == MAP_FAILED)
		printf("mmap err %d\n",(int)fbp);
	 
     


	//munmap(fbp, screensize);  

	//close(fbfd);  
	
	lcd_dev->fbfd = fbfd;
	lcd_dev->fbfp = fbp;
	lcd_dev->bits_per_pixel = vinfo.bits_per_pixel;
	lcd_dev->lcd_w = vinfo.xres;
	lcd_dev->lcd_h = vinfo.yres;
	lcd_dev->screensize = screensize;

	return 0;

}


int main(int argc, char *argv[])
{  
	//1.open device.打开摄像头设备 
	int index = -1;
	int i;
	struct lcd_dev lcd_dev;
	
	
	int fd = open(argv[1],O_RDWR,0);
	if(fd<0){
		printf("open device failed.\n");
	}
 

    //2.search device property.查询设备属性

	struct v4l2_capability cap;
	if(ioctl(fd,VIDIOC_QUERYCAP,&cap)==-1){
		printf("VIDIOC_QUERYCAP failed.\n");
	}
	printf("VIDIOC_QUERYCAP success.->DriverName:%s CardName:%s BusInfo:%s\n",\
		cap.driver,cap.card,cap.bus_info);//device info.设备信息    
		

	//3.show all supported format.显示所有支持的格式
	struct v4l2_fmtdesc fmtdesc;
	fmtdesc.index = 0; //form number
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//frame type  
	while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc) != -1){  
        //if(fmtdesc.pixelformat && fmt.fmt.pix.pixelformat){
            printf("VIDIOC_ENUM_FMT pixelformat:%s,%d\n",fmtdesc.description,fmtdesc.pixelformat);
        //}

		if(fmtdesc.pixelformat == CAM_PIX_FMT)
			index = fmtdesc.index;
		
        fmtdesc.index ++;
    }


	if(index == -1){
		printf("camero dont support YUYV format\n");
		return 0;
	}

    struct v4l2_format fmt;
	memset ( &fmt, 0, sizeof(fmt) );
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(fd,VIDIOC_G_FMT,&fmt) == -1) {
	   printf("VIDIOC_G_FMT failed.\n");
	   return -1;
    }

  	printf("VIDIOC_G_FMT width %ld, height %d, olorspace is %ld\n",fmt.fmt.pix.width,fmt.fmt.pix.height,fmt.fmt.pix.colorspace);
	
	
    //V4L2_PIX_FMT_RGB32   V4L2_PIX_FMT_YUYV   V4L2_STD_CAMERA_VGA  V4L2_PIX_FMT_JPEG
	fmt.fmt.pix.pixelformat = CAM_PIX_FMT;	
	if (ioctl(fd,VIDIOC_S_FMT,&fmt) == -1) {
	   printf("VIDIOC_S_FMT failed.\n");
	   return -1;
    }


	if (ioctl(fd,VIDIOC_G_FMT,&fmt) == -1) {
	   printf("VIDIOC_G_FMT failed.\n");
	   return -1;
    }
  	//printf("VIDIOC_G_FMT sucess.->fmt.fmt.width is %ld\nfmt.fmt.pix.height is %ld\n\
	//	fmt.fmt.pix.colorspace is %ld\n",fmt.fmt.pix.width,fmt.fmt.pix.height,fmt.fmt.pix.colorspace);


	//6.1 request buffers.申请缓冲区
	struct v4l2_requestbuffers req;  
	req.count = 2;//frame count.帧的个数 
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;//automation or user define．自动分配还是自定义
	if ( ioctl(fd,VIDIOC_REQBUFS,&req)==-1){  
		printf("VIDIOC_REQBUFS map failed.\n");  
		close(fd);  
		exit(-1);  
	} 

	//6.2 manage buffers.管理缓存区
	//应用程序和设备３种交换方式：read/write，mmap，用户指针．这里用memory map.内存映射
	unsigned int n_buffers = 0;  
	//buffers = (struct buffer*) calloc (req.count, sizeof(*buffers)); 
	buffers = calloc (req.count, sizeof(*buffers));

	struct v4l2_buffer buf;  

		
	printf("queue buf\n");
	//struct v4l2_buffer buf;   
	for(n_buffers = 0; n_buffers < req.count; ++n_buffers)
	{  
		memset(&buf,0,sizeof(buf)); 
		buf.index = n_buffers; 
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
		buf.memory = V4L2_MEMORY_MMAP;  
		
		//查询序号为n_buffers 的缓冲区，得到其起始物理地址和大小
		if(ioctl(fd,VIDIOC_QUERYBUF,&buf) == -1)
		{ 
			printf("VIDIOC_QUERYBUF failed.\n");
			close(fd);  
			exit(-1);  
		} 

  		//memory map
		buffers[n_buffers].length = buf.length;	
		buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,buf.m.offset);  
		if(MAP_FAILED == buffers[n_buffers].start){  
			printf("memory map failed.\n");
			close(fd);  
			exit(-1);  
		} 

		//Queen buffer.将缓冲帧放入队列 
		if (ioctl(fd , VIDIOC_QBUF, &buf) ==-1) {
		    printf("VIDIOC_QBUF failed.->n_buffers=%d\n", n_buffers);
		    return -1;
		}

	} 

	//7.使能视频设备输出视频流
	printf("enable caputre\n");
	enum v4l2_buf_type type; 
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	if (ioctl(fd,VIDIOC_STREAMON,&type) == -1) {
		printf("VIDIOC_STREAMON failed.\n");
		return -1;
	}


	//8.DQBUF.取出一帧
	printf("get buf\n");
    struct v4l2_buffer buf_q;  
	memset(&buf_q, 0, sizeof(buf_q)); 
	
	buf_q.index = 0; 
    buf_q.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    buf_q.memory = V4L2_MEMORY_MMAP;	
	
/*	
    if (ioctl(fd, VIDIOC_DQBUF, &buf_q) == -1) {
        printf("VIDIOC_DQBUF failed.->fd=%d\n",fd);
        return -1;
    } 
*/	
	printf("lcd init...\n");
	lcd_init(&lcd_dev);
	// yuv422torgb(&lcd_dev,lcd_dev.fbfp, lcd_dev.lcd_w, lcd_dev.lcd_h, buffers[0].start, fmt.fmt.pix.width, fmt.fmt.pix.height);
	//while(1);
	printf("displaying\n");
	
	//buf_q.index = 0; 
	//ioctl(fd, VIDIOC_DQBUF, &buf_q);	
	
	while(1){
		buf_q.index = 0; 
		ioctl(fd, VIDIOC_DQBUF, &buf_q);
		
		buf_q.index = 1; 
		ioctl(fd, VIDIOC_QBUF, &buf_q);
		yuv422torgb(&lcd_dev,lcd_dev.fbfp, lcd_dev.lcd_w, lcd_dev.lcd_h, buffers[0].start, fmt.fmt.pix.width, fmt.fmt.pix.height);	




		buf_q.index = 1; 
		ioctl(fd, VIDIOC_DQBUF, &buf_q);
		
		buf_q.index = 0; 
		ioctl(fd, VIDIOC_QBUF, &buf_q);	
		yuv422torgb(&lcd_dev,lcd_dev.fbfp, lcd_dev.lcd_w, lcd_dev.lcd_h, buffers[1].start, fmt.fmt.pix.width, fmt.fmt.pix.height);
		
		

	}

	
	
	
	

	//11.Release the resource．释放资源
    
	for (i=0; i< req.count; i++) {
        munmap(buffers[i].start, buffers[i].length);
		printf("munmap success.\n");
    }

	close(fd); 
	
	
	munmap(lcd_dev.fbfp, lcd_dev.screensize);  
	close(lcd_dev.fbfd);  
	
	printf("Camera test Done.\n"); 
	return 0;  
}  
