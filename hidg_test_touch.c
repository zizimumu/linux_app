#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <linux/fb.h>   
#include <fcntl.h>             
#include <unistd.h>
#include <string.h>


static unsigned char ui_hid_report[14] = {0};
#define UDI_HID_REPORT_ID_MTOUCH    1


void generate_report(unsigned short x, unsigned short y)
{
    
    ui_hid_report[0] = UDI_HID_REPORT_ID_MTOUCH;
 
	// 1st touch
	ui_hid_report[1] = 3;
	ui_hid_report[2] = 1;
	ui_hid_report[3] = (unsigned char)x;
	ui_hid_report[4] = (unsigned char) (x >> 8);
	ui_hid_report[5] = (unsigned char)y;
	ui_hid_report[6] = (unsigned char) (y >> 8);
 
	// 2nd touch
	ui_hid_report[7] = 3;
	ui_hid_report[8] = 2;
	ui_hid_report[9] = (unsigned char)x;
	ui_hid_report[10] = (unsigned char) (x >> 8);
	ui_hid_report[11] = (unsigned char)y;
	ui_hid_report[12] = (unsigned char) (y >> 8);
 
	// Number of touches   
	ui_hid_report[13] = 2;
}

unsigned short ui_step = 0; ui_y = 0;
void generate_xy(void)
{
	// Report ID
	ui_hid_report[0] = UDI_HID_REPORT_ID_MTOUCH;
 
	// 1st touch
	ui_hid_report[1] = 3;
	ui_hid_report[2] = 1;
	ui_hid_report[3] = (unsigned char)((1365 + 10 * ui_step) & 0x00FF);
	ui_hid_report[4] = (unsigned char)(((1365 + 10 * ui_step) & 0xFF00) >> 8);
	ui_hid_report[5] = (unsigned char)(( 500+ui_y) & 0x00FF);
	ui_hid_report[6] = (unsigned char)(( (500+ui_y) & 0xFF00) >> 8);
 
	// 2nd touch
	ui_hid_report[7] = 0;
	ui_hid_report[8] = 2;
	//ui_hid_report[9] = (unsigned char)(1365 & 0x00FF);
	//ui_hid_report[10] = (unsigned char)((1365 & 0xFF00) >> 8);
	//ui_hid_report[11] = (unsigned char)((1365 + 10 * ui_step) & 0x00FF);
	//ui_hid_report[12] = (unsigned char)(((1365 + 10 * ui_step) & 0xFF00) >> 8);
 
	// Number of touches   
	ui_hid_report[13] = 2;


}

void generate_release( )
{
    memset(ui_hid_report,0,sizeof(ui_hid_report));
    
    ui_hid_report[0] = UDI_HID_REPORT_ID_MTOUCH;
 
	// 1st touch
	ui_hid_report[1] = 0;
	ui_hid_report[2] = 1;

 
	// 2nd touch
	ui_hid_report[7] = 0;
	ui_hid_report[8] = 2;

 
	// Number of touches   
	ui_hid_report[13] = 2;
}
int main(int argc, const char *argv[])
{
    int fd;
    unsigned char buf[128];
    int ret,count,i;
    unsigned short x,y;
    

    fd = open(argv[1],O_RDWR);
    if(fd <=0){
        printf("open error\n");
        return 0;
    }
    
    
    
    
    while(1){
        buf[0] = getchar();
        printf("get char %c\n",buf[0]);
        ui_step++;
        generate_xy();
        ret = write(fd,ui_hid_report,sizeof(ui_hid_report));
        if(ret <=0){
            printf("write error %d\n",ret);
        }        
        
        if(ui_step > 0 && (ui_step % 10 == 0) ){
            generate_release();
            write(fd,ui_hid_report,sizeof(ui_hid_report));
            ui_y += 500;
        }
    }
    
    
    
    x =  (unsigned short)strtoul(argv[2], NULL, 0);
    y =  (unsigned short)strtoul(argv[3], NULL, 0);
    
    generate_report(x,y);
    
    ret = write(fd,ui_hid_report,sizeof(ui_hid_report));
    if(ret <=0){
        printf("write error %d\n",ret);
    }
    
    printf("get x %d, y %d\n",x,y);
    sleep(5);
    // need to send all 0 otherwise host will treat as hold status
    
    printf("change point \n");
    generate_report(x+500,y+500);
    ret = write(fd,ui_hid_report,sizeof(ui_hid_report));
    if(ret <=0){
        printf("write error %d\n",ret);
    }    
    
    printf("set to zero\n");
    generate_release();
    
    ret = write(fd,ui_hid_report,sizeof(ui_hid_report));
    if(ret <=0){
        printf("write error %d\n",ret);
    }
    
    printf("done\n");
    close(fd);
    

	return 0;
}
