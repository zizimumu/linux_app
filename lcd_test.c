#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

#define TRUE    1
#define FALSE   0

#define MIN(x,y)        ((x)>(y)?(y):(x))
#define MAX(x,y)        ((x)>(y)?(x):(y))

#define RED16     0xf800
#define GREEN16   0x07e0
#define BLUE16    0x001f

#define RED24     0x00ff0000
#define GREEN24   0x0000ff00
#define BLUE24    0x000000ff

#define R_MAX16	  32
#define G_MAX16	  64
#define B_MAX16	  32

#define R_MAX24	  256
#define G_MAX24	  256
#define B_MAX24   256

#define GRAY_16	  32
#define GRAY_24	  256

#define ROW_16	  4
#define COL_16	  8

#define ROW_24	  16
#define COL_24	  16

typedef struct fbdev{
        int fb;
        unsigned long fb_mem;
        struct fb_fix_screeninfo fb_fix;
        struct fb_var_screeninfo fb_var;
        char dev[20];
} FBDEV, *PFBDEV;

typedef struct point{
        int x;
        int y;
        int z;
} POINT, *PPOINT;

void draw_dot (PFBDEV pFbdev, POINT p, void *color);
void draw_line(PFBDEV pFbdev, POINT p1, POINT p2, void *color);
void draw_polygon(PFBDEV pFbdev, uint32_t num, PPOINT array, void *color);
void draw_triangle(PFBDEV pFbdev, POINT p1, POINT p2, POINT p3, void *color);
void draw_circle(PFBDEV pFbdev, POINT center, uint32_t radius, void *color);
void draw_parabola_x(PFBDEV pFbdev, POINT center, int a, void *color);
void draw_parabola_y(PFBDEV pFbdev, POINT center, int a, void *color);

int fb_open(PFBDEV pFbdev)
{
	long screensize=0;

        pFbdev->fb = open(pFbdev->dev, O_RDWR);
        if(pFbdev->fb < 0)
        {
                printf("Error opening %s: %m. Check kernel config\n", pFbdev->dev);
                return FALSE;
        }
	/*得到与framebuffer相关的变量信息*/
        if (-1 == ioctl(pFbdev->fb,FBIOGET_VSCREENINFO,&(pFbdev->fb_var)))
        {
                printf("ioctl FBIOGET_VSCREENINFO\n");
                return FALSE;
        }
	/*得到framebuffer固定的信息，比如图形硬件上实际的帧缓冲区大小，能否硬件加速*/
        if (-1 == ioctl(pFbdev->fb,FBIOGET_FSCREENINFO,&(pFbdev->fb_fix)))
        {
                printf("ioctl FBIOGET_FSCREENINFO\n");
                return FALSE;
        }
#if 0
	printf("smem_start: 0x%x\n", pFbdev->fb_fix.smem_start);
	printf("smem_len: %d\n", pFbdev->fb_fix.smem_len);
	printf("line_length: %d\n", pFbdev->fb_fix.line_length);

	printf("xres: %d\n", pFbdev->fb_var.xres);
	printf("yres: %d\n", pFbdev->fb_var.yres);
	printf("bits_per_pixel: %d\n", pFbdev->fb_var.bits_per_pixel);
#endif
	screensize = pFbdev->fb_var.xres * pFbdev->fb_var.yres * pFbdev->fb_var.bits_per_pixel / 8;	
        pFbdev->fb_mem = (char *)mmap(NULL, screensize,
                        	PROT_READ | PROT_WRITE, 
				MAP_SHARED, pFbdev->fb, 0);
        if (-1L == (long) pFbdev->fb_mem)
        {
                printf("Error: failed to map framebuffer device to memory.\n");
                return FALSE;
        }

        return TRUE;
}

void fb_close(PFBDEV pFbdev)
{
        close(pFbdev->fb);
        pFbdev->fb=-1;
}

void fb_memcpy(void *addr, void *color, size_t len)
{
        memcpy(addr, color, len);
}

void draw_dot (PFBDEV pFbdev, POINT p, void *color)
{
        uint32_t offset;
        offset = p.y * pFbdev->fb_fix.line_length + 
			p.x * (pFbdev->fb_var.bits_per_pixel / 8);
        fb_memcpy((void*)(pFbdev->fb_mem + offset), color, (pFbdev->fb_var.bits_per_pixel / 8));
}

void draw_line(PFBDEV pFbdev, POINT p1, POINT p2, void *color)
{
	POINT p;
	float delta_x,delta_y;
  	int dx, dy, steps, i;

  	dx=p2.x - p1.x;
  	dy=p2.y - p1.y;
  	if(abs(dx)>abs(dy)) 
		steps=abs(dx);
  	else 
		steps=abs(dy);
  	delta_x=(float)dx/(float)steps;
  	delta_y=(float)dy/(float)steps;
	p.x=p1.x;
	p.y=p1.y;
	for(i=0;i<=steps;i++)
	{
		draw_dot(pFbdev, p, color);
	   	p.x += delta_x;
	   	p.y += delta_y;
	}
}

void draw_polygon(PFBDEV pFbdev, uint32_t num, PPOINT array, void *color)
{
        int i;
        for(i=0; i<num; i++)
        {
                draw_line(pFbdev, array[i], array[(i+1)%num], color);
        }
}

void draw_triangle(PFBDEV pFbdev, POINT p1, POINT p2, POINT p3, void *color)
{
        POINT p[3];
        p[0] = p1;
        p[1] = p2;
        p[2] = p3;
        draw_polygon(pFbdev, 3, p, color);
}

void draw_circle(PFBDEV pFbdev, POINT center, uint32_t radius, void *color)
{
        POINT p;
        int tmp;
        for(p.x = center.x - radius; p.x <= center.x + radius; p.x++)
        {
                tmp = sqrt(radius * radius - (p.x - center.x) * (p.x - center.x));
                p.y = center.y + tmp;
                draw_dot(pFbdev, p, color);
                p.y = center.y - tmp;
                draw_dot(pFbdev, p, color);
        }
        for(p.y = center.y - radius; p.y <= center.y + radius; p.y++)
        {
                tmp = sqrt(radius * radius - (p.y - center.y) * (p.y - center.y));
                p.x = center.x + tmp;
                draw_dot(pFbdev, p, color);
                p.x = center.x - tmp;
                draw_dot(pFbdev, p, color);
        }
}

void draw_parabola_x(PFBDEV pFbdev, POINT center, int a, void *color)
{
        POINT p;
        for(p.x = center.x - 100; p.x < center.x + 100; p.x++)
        {
                p.y = (p.x - center.x) * (p.x - center.x) / a + center.y;
                draw_dot(pFbdev, p, color);
        }
}

void draw_parabola_y(PFBDEV pFbdev, POINT center, int a, void *color)
{
        POINT p;
        for(p.y = center.y - 100; p.y < center.y + 100; p.y++)
        {
                p.x = (p.y - center.y) * (p.y-center.y) / a + center.x;
                draw_dot(pFbdev, p, color);
        }
}

void rectangle_fill(PFBDEV pFbdev, POINT p1, POINT p2, void *color)
{
        POINT p_t1, p_t2;
	int y;	

	p_t1.x = MIN(p1.x, p2.x);
	p_t2.x = MAX(p1.x, p2.x);

	for(y = MIN(p1.y, p2.y); y <= MAX(p1.y, p2.y); y++){
		p_t1.y = p_t2.y = y;
		draw_line(pFbdev, p_t1, p_t2, color); 
	}
}

void rgb_test(PFBDEV pFbdev)
{
        POINT p, p1, p2;
        uint32_t color;

        p1.x = 0;
        p1.y = 0;
        p2.x = pFbdev->fb_var.xres - 1;
        p2.y = (pFbdev->fb_var.yres / 3) - 1;
        color = (pFbdev->fb_var.bits_per_pixel == 16) ? RED16 : RED24;
        rectangle_fill(pFbdev, p1, p2, (void *)&color);

	p1.x = 0;
        p1.y = pFbdev->fb_var.yres / 3;
        p2.x = pFbdev->fb_var.xres - 1;
        p2.y = (pFbdev->fb_var.yres / 3 * 2) - 1;
	color = (pFbdev->fb_var.bits_per_pixel == 16) ? GREEN16 : GREEN24;
        rectangle_fill(pFbdev, p1, p2, (void *)&color);

	p1.x = 0;
        p1.y = (pFbdev->fb_var.yres / 3 * 2);
        p2.x = pFbdev->fb_var.xres - 1;
        p2.y = pFbdev->fb_var.yres - 1;
        color = (pFbdev->fb_var.bits_per_pixel == 16) ? BLUE16 : BLUE24;
        rectangle_fill(pFbdev, p1, p2, (void *)&color);
}

void gray_test(PFBDEV pFbdev)
{
	POINT p, p1, p2;
        uint8_t x_step, y_step;
	uint32_t gray[16][16];
        int i, j;
	int GRAY_NUM, ROW_NUM, COL_NUM;

	GRAY_NUM = (pFbdev->fb_var.bits_per_pixel == 16) ? GRAY_16 : GRAY_24;
	ROW_NUM = (pFbdev->fb_var.bits_per_pixel == 16) ? ROW_16 : ROW_24;
	COL_NUM = (pFbdev->fb_var.bits_per_pixel == 16) ? COL_16 : COL_24;

	x_step = pFbdev->fb_var.xres / COL_NUM;
	y_step = pFbdev->fb_var.yres / ROW_NUM;

        for(i = 0; i< GRAY_NUM; i++){
                gray[i/ROW_NUM][i%COL_NUM] = (pFbdev->fb_var.bits_per_pixel == 16) ? 
						(i << 11 | i << 6 | i) : (i << 16 | i << 8 | i);		
	}

        for(j = 0; j < ROW_NUM; j++){
                for(i = 0; i < COL_NUM; i++){
                        p1.x = i * x_step;
                        p2.x = p1.x + x_step - 1;
                        p1.y = j * y_step;
                        p2.y = p1.y + y_step - 1;
                        rectangle_fill(pFbdev, p1, p2, (void *)&gray[j][i]);
                }
        }
}

uint32_t color_get(PFBDEV pFbdev, int x)
{
        int Gx =  pFbdev->fb_var.xres / 2;
        int Rx = Gx - (pFbdev->fb_var.xres / 3);
        int Bx = Gx + (pFbdev->fb_var.xres / 3);
	int Dmax = pFbdev->fb_var.xres / 3;
	int R, G, B;
	int R_MAX, G_MAX, B_MAX;
	int dx;

	R_MAX = (pFbdev->fb_var.bits_per_pixel == 16) ? R_MAX16 : R_MAX24;
	G_MAX = (pFbdev->fb_var.bits_per_pixel == 16) ? G_MAX16 : G_MAX24;
	B_MAX = (pFbdev->fb_var.bits_per_pixel == 16) ? B_MAX16 : B_MAX24;

	if(x > Bx){
		dx = (Dmax / 2) + (pFbdev->fb_var.xres - x);
	}else{
        	dx = abs(x - Rx);
        	dx = (dx < Dmax) ? dx : Dmax;
	}
        R = R_MAX - (R_MAX * dx / Dmax);
	R = (R < R_MAX) ? R : (R_MAX -1);

        dx = abs(x - Gx);
        dx = (dx < Dmax) ? dx : Dmax;
        G = G_MAX - (G_MAX * dx / Dmax);
	G = (G < G_MAX) ? G : (G_MAX - 1);

	if(x < Rx){
		dx = (Dmax / 2) + x;
	}else{
        	dx = abs(x - Bx);
        	dx = (dx < Dmax) ? dx : Dmax;
	}
        B = B_MAX - (B_MAX * dx / Dmax);
	B = (B < B_MAX) ? B : (B_MAX - 1);


//	printf("x: %d, R: 0x%x, G: 0x%x, B: 0x%x\n", x, R, G, B);

        return((pFbdev->fb_var.bits_per_pixel == 16) ?
                        (R << 11 | G << 5 | B) : (R << 16 | G << 8 | B));
}

void palette_test(PFBDEV pFbdev)
{
	POINT p, p1, p2;
	uint32_t color;
	int x;

	p1.y = 0;
	p2.y = pFbdev->fb_var.yres - 1;

	for(x = 0; x < pFbdev->fb_var.xres; x++){	
		p1.x = p2.x = x;
		color = color_get(pFbdev, x);
		draw_line(pFbdev, p1, p2, (void *)&color);	
	}
}

int main(int argc, char *argv[])
{
        FBDEV fbdev;
        memset(&fbdev, 0, sizeof(FBDEV));
        strcpy(fbdev.dev, "/dev/fb0");

	if(argc < 2 ){
		printf("parameter error\n");
		return 0;
}
	strcpy(fbdev.dev,argv[1]);
        if(fb_open(&fbdev)==FALSE)
        {
                printf("open frame buffer error\n");
                return 0;
        }

//	printf("fb_open success\n");

	if(3 == argc){
		if(!strncmp(argv[2], "rgb", 3)){
			rgb_test(&fbdev);
		}else if(!strncmp(argv[2], "gray", 4)){
			gray_test(&fbdev);
		}else if(!strncmp(argv[2], "palette", 7)){
			palette_test(&fbdev);
		}
	}else{
		/* rgb test */
		rgb_test(&fbdev);
	
		/* gray test */
		sleep(2);
		gray_test(&fbdev);

		/* palette test */
		sleep(2);
		palette_test(&fbdev);	
	}
        fb_close(&fbdev);
        return 0;
}
