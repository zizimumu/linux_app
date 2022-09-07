#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>  
#include <fcntl.h>             
#include <unistd.h>
#include <sys/mman.h> 
 #include<time.h>
#include <linux/videodev2.h>

struct buffer{  
	void *start;  
	unsigned int length;  
}*buffers; 


#define CAM_PIX_FMT V4L2_PIX_FMT_YVYU
#define  VIDEO_WIDTH  640
#define  VIDEO_HEIGHT  480
#define DEV_PATH "/dev/video0" //0/9 
#define CAPTURE_FILE "./pic/0.jpeg"

int main()
{  

    enum v4l2_buf_type type;
	//1.open device.打开摄像头设备 
	int fd = open(DEV_PATH,O_RDWR,0);//弄了好久 以阻塞模式打开摄像头  | O_NONBLOCK 非堵塞
	if(fd<0){
		printf("open device failed.\n");
	}
	printf("open device success.->fd=%d\n",fd);  

/*****************************
 * arm: /include/linux/videodev2.h
 * linux: /usr/include/linux/videodev2.h
    struct v4l2_capability  {
        u8 driver[16]; // 驱动名字
        u8 card[32]; // 设备名字
        u8 bus_info[32]; // 设备在系统中的位置
        u32 version; // 驱动版本号
        u32 capabilities; // 设备支持的操作
        u32 reserved[4]; // 保留字段 };
    };
************************** */

    //2.search device property.查询设备属性

	struct v4l2_capability cap;
	if(ioctl(fd,VIDIOC_QUERYCAP,&cap)==-1){
		printf("VIDIOC_QUERYCAP failed.\n");
	}
	printf("VIDIOC_QUERYCAP success.->DriverName:%s CardName:%s BusInfo:%s\n",\
		cap.driver,cap.card,cap.bus_info);//device info.设备信息    
		
/***********************
    struct v4l2_fmtdesc  {
        u32 index; // 要查询的格式序号，应用程序设置
        enum v4l2_buf_type type; // 帧类型，数据流类型，必须永远是V4L2_BUF_TYPE_VIDEO_CAPTURE
        u32 flags; // 是否为压缩格式
        u8 description[32]; // 格式名称
        u32 pixelformat; // 格式
        u32 reserved[4]; // 保留
    };

***********************/

	//3.show all supported format.显示所有支持的格式
//	struct v4l2_fmtdesc fmtdesc;
//	fmtdesc.index = 0; //form number
//	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//frame type  
//	while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc) != -1){  
//        //if(fmtdesc.pixelformat && fmt.fmt.pix.pixelformat){
//            printf("VIDIOC_ENUM_FMT success.->fmt.fmt.pix.pixelformat:%s\n",fmtdesc.description);
//        //}
//        fmtdesc.index ++;
//    }





    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_fmtdesc fmt_1;
    struct v4l2_frmsizeenum frmsize;
    struct v4l2_frmivalenum frmival;

    fmt_1.index = 0;
    fmt_1.type = type;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt_1) >= 0) {
        
        printf("VIDIOC_ENUM_FMT success.->fmt.fmt.pix.pixelformat:%s\n",fmt_1.description);
        
        frmsize.pixel_format = fmt_1.pixelformat;
        frmsize.index = 0;
        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0){

            if(frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE){
                printf("line:%d %dx%d\n",__LINE__, frmsize.discrete.width, frmsize.discrete.height);
            }else if(frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE){
                printf("line:%d %dx%d\n",__LINE__, frmsize.discrete.width, frmsize.discrete.height);
            }

            frmsize.index++;
        }

        fmt_1.index++;
    }


    
    
    while(1);

/*************************
    struct v4l2_format {
        enum v4l2_buf_type type; // 帧类型，应用程序设置,
        union  {
            struct v4l2_pix_format pix; // 视频设备使用
            struct v4l2_window win;
            struct v4l2_vbi_format vbi;
            struct v4l2_sliced_vbi_format sliced;
            u8 raw_data[200];
        }fmt;
    };
    struct v4l2_pix_format  {
        __u32                   width;          // 帧宽，必须是16的倍数
        __u32                   height;         // 帧高，必须是16的倍数
        __u32                   pixelformat;   // 视频数据存储类型，例如是YUV4：2：2还是RGB
        enum v4l2_field         field;
        __u32                   bytesperline;
        __u32                   sizeimage;
        enum v4l2_colorspace    colorspace;
        __u32                   priv;
    };
*************************/
    //4.set or gain current frame.设置或查看当前格式

    struct v4l2_format fmt;
	memset ( &fmt, 0, sizeof(fmt) );
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = VIDEO_WIDTH;
    fmt.fmt.pix.height      = VIDEO_HEIGHT;	
	
    //V4L2_PIX_FMT_RGB32   V4L2_PIX_FMT_YUYV   V4L2_STD_CAMERA_VGA  V4L2_PIX_FMT_JPEG
	fmt.fmt.pix.pixelformat = CAM_PIX_FMT;	
	if (ioctl(fd,VIDIOC_S_FMT,&fmt) == -1) {
	   printf("VIDIOC_S_FMT failed.\n");
	   return -1;
    }

	printf("VIDIOC_S_FMT sucess.\n");
	if (ioctl(fd,VIDIOC_G_FMT,&fmt) == -1) {
	   printf("VIDIOC_G_FMT failed.\n");
	   return -1;
    }
  	printf("VIDIOC_G_FMT sucess.->fmt.fmt.width is %ld\nfmt.fmt.pix.height is %ld\n\
		fmt.fmt.pix.colorspace is %ld\n",fmt.fmt.pix.width,fmt.fmt.pix.height,fmt.fmt.pix.colorspace);

		
	/*	
	//5.check the format.检查是否支持某种帧格式
	memset ( &fmt, 0, sizeof(fmt) );
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat=CAM_PIX_FMT;
	//if(ioctl(fd,VIDIOC_TRY_FMT,&fmt)==-1) if(errno==EINVAL)
	if (ioctl(fd,VIDIOC_TRY_FMT,&fmt) == -1) {
	   printf("VIDIOC_TRY_FMT failed.\n");
	   return -1;
    }
  	printf("VIDIOC_TRY_FMT sucess.->fmt.fmt.width is %ld\nfmt.fmt.pix.height is %ld\n\
		fmt.fmt.pix.colorspace is %ld\n",fmt.fmt.pix.width,fmt.fmt.pix.height,fmt.fmt.pix.colorspace);

	*/	
		
/***************
    v4l2_requestbuffers　结构如下：
    struct v4l2_requestbuffers {
        u32                count;//缓存数量,也就是说在缓存队列里保持多少张照片,一般不超过5个。
        enum v4l2_buf_type type; //数据流类型,必须永远是V4L2_BUF_TYPE_VIDEO_CAPTURE
        enum v4l2_memory   memory;//V4L2_MEMORY_MMAP或V4L2_MEMORY_USERPTR
        u32                reserved[2];
    };
************************/

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
	printf("VIDIOC_REQBUFS map success.\n");

  
	//6.2 manage buffers.管理缓存区
	//应用程序和设备３种交换方式：read/write，mmap，用户指针．这里用memory map.内存映射
	unsigned int n_buffers = 0;  
	//buffers = (struct buffer*) calloc (req.count, sizeof(*buffers)); 
	buffers = calloc (req.count, sizeof(*buffers));
	
/*****************************
    v4l2_buffer  结构如下：
    struct v4l2_buffer {
        __u32                 index;//序号
        enum v4l2_buf_type    type;//类型
        __u32                 bytesused;//buffer 中已使用的字节数
        __u32                 flags;//区分是MMAP还是USERPTR
        enum v4l2_field       field;
        struct timeval        timestamp;//获取第一个字节
        struct v4l2_timecode  timecode;
        __u32                 sequence;//队列中的序号
        enum v4l2_memory      memory;   // memory location
        union {
            __u32          offset;//缓存帧地址，只对MMAP有效
            unsigned long  userptr;
        } m;
        __u32                   length;//缓冲帧长度
        __u32                   input;
        __u32                   reserved;
    };

****************************/

	//struct v4l2_buffer buf;   
	for(n_buffers = 0; n_buffers < req.count; ++n_buffers)
	{  
		struct v4l2_buffer buf;  
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
        printf("VIDIOC_QUERYBUF success.\n");

/******************

    函数：void *mmap(void *start,size_t length,int prot,int flags,int fd,off_t offsize);
        参数start：指向欲映射的内存起始地址，通常设为 NULL，代表让系统自动选定地址，映射成功后返回该地址。
        参数length：代表将文件中多大的部分映射到内存。
        参数prot：映射区域的保护方式。可以为以下几种方式的组合：
            PROT_EXEC 映射区域可被执行
            PROT_READ 映射区域可被读取
            PROT_WRITE 映射区域可被写入
            PROT_NONE 映射区域不能存取
        参数flags：影响映射区域的各种特性。在调用mmap()时必须要指定MAP_SHARED 或 MAP_PRIVATE。
            MAP_FIXED 如果参数start所指的地址无法成功建立映射时，则放弃映射，不对地址做修正。通常不鼓励用此旗标。
            MAP_SHARED对映射区域的写入数据会复制回文件内，而且允许其他映射该文件的进程共享。
            MAP_PRIVATE 对映射区域的写入操作会产生一个映射文件的复制，即私人的“写入时复制”（copy on write）对此区域作的任何修改都不会写回原来的文件内容。
            MAP_ANONYMOUS建立匿名映射。此时会忽略参数fd，不涉及文件，而且映射区域无法和其他进程共享。
            MAP_DENYWRITE只允许对映射区域的写入操作，其他对文件直接写入的操作将会被拒绝。
            MAP_LOCKED 将映射区域锁定住，这表示该区域不会被置换（swap）。
        参数fd：要映射到内存中的文件描述符。如果使用匿名内存映射时，即flags中设置了MAP_ANONYMOUS，fd设为-1。
               有些系统不支持匿名内存映射，则可以使用fopen打开/dev/zero文件，然后对该文件进行映射，可以同样达到匿名内存映射的效果。
        参数offset：文件映射的偏移量，通常设置为0，代表从文件最前方开始对应，offset必须是分页大小的整数倍。
        返回值：若映射成功则返回映射区的内存起始地址，否则返回MAP_FAILED(－1)，错误原因存于errno 中。
*****************/

  		//memory map
		buffers[n_buffers].length = buf.length;	
		buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,buf.m.offset);  
		if(MAP_FAILED == buffers[n_buffers].start){  
			printf("memory map failed.\n");
			close(fd);  
			exit(-1);  
		} 

		printf("memory map success.\n"); 
		//Queen buffer.将缓冲帧放入队列 
		if (ioctl(fd , VIDIOC_QBUF, &buf) ==-1) {
		    printf("VIDIOC_QBUF failed.->n_buffers=%d\n", n_buffers);
		    return -1;
		}
		printf("VIDIOC_QBUF.->Frame buffer %d: address=0x%x, length=%d\n",\
			n_buffers, (unsigned int)buffers[n_buffers].start, buffers[n_buffers].length);

	} 

	//将缓冲帧放入队列 

	/*unsigned int ii;   
	for(ii = 0; ii < 2; ii++){  
		struct v4l2_buffer buf;  
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
		buf.memory = V4L2_MEMORY_MMAP;  
		buf.index = ii;  
		if (ioctl(fd,VIDIOC_QBUF,&buf)==-1){
			printf("VIDIOC_QBUF failed.\n");
		}
		printf("VIDIOC_QBUF success.->buf=%d: address=0x%x, length=%d\n",\
ii,(unsigned int)buffers[ii].start,buffers[ii].length);
	}*/ 

	//7.使能视频设备输出视频流
	 
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	if (ioctl(fd,VIDIOC_STREAMON,&type) == -1) {
		printf("VIDIOC_STREAMON failed.\n");
		return -1;
	}
	printf("VIDIOC_STREAMON success.\n"); 

	//8.DQBUF.取出一帧
    struct v4l2_buffer buf;  
	memset(&buf, 0, sizeof(buf)); 
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        printf("VIDIOC_DQBUF failed.->fd=%d\n",fd);
        return -1;
    }  
	printf("VIDIOC_DQBUF success.\n");

	//9.Process the frame.处理这一帧
    FILE *fp = fopen(CAPTURE_FILE, "wb");
    if (fp < 0) {
        printf("open frame-data-file failed.\n");
        return -1;
    }
	printf("open frame-data-file success.\n");
    fwrite(buffers[buf.index].start, 1, buf.length, fp);
    fclose(fp);
    printf("save one frame success.\n"); //%s, CAPTURE_FILE
	//10.QBUF.把一帧放回去
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        printf("VIDIOC_QBUF failed.->fd=%d\n",fd);
        return -1;
    } 
	printf("VIDIOC_QBUF success.\n");

/*********************

    函数：int munmap(void *start, size_t length)

        start：要取消映射的内存区域的起始地址。

        length：要取消映射的内存区域的大小。

        返回说明: 成功执行时munmap()返回0。失败时munmap返回-1。

**********************/
	//11.Release the resource．释放资源
    int i;
	for (i=0; i< 2; i++) {
        munmap(buffers[i].start, buffers[i].length);
		printf("munmap success.\n");
    }

	close(fd); 
	printf("Camera test Done.\n"); 
	return 0;  
}  