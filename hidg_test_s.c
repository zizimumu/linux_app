#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <linux/fb.h>   
#include <fcntl.h>             
#include <unistd.h>
#include <string.h>

static uint8_t ui_hid_report[14] = {0};
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
	ui_hid_report[7] = 0;
	ui_hid_report[8] = 2;
	//ui_hid_report[9] = (uint8_t)(1365 & 0x00FF);
	//ui_hid_report[10] = (uint8_t)((1365 & 0xFF00) >> 8);
	//ui_hid_report[11] = (uint8_t)((1365 + 10 * ui_step) & 0x00FF);
	//ui_hid_report[12] = (uint8_t)(((1365 + 10 * ui_step) & 0xFF00) >> 8);
 
	// Number of touches   
	ui_hid_report[13] = 2;
}
int main(int argc, const char *argv[])
{
    int fd;
    unsigned char buf[128];
    int ret,count,i;
    
    // need to write 3 bytes: [type] [x] [y], for type: 1 -> left key, 2 ->right key, 4 ->mid key, 0-> move
    if(argc < 5){
        printf("parameter error, use as : %s: /dev/hidg0 1 0 0\n",argv[0]);
    }
    
    count = argc;
    
    printf("writing:\n");
    for(i=0;i<count-2;i++){
        buf[i] = (unsigned char)strtoul(argv[i+2], NULL, 0);
        printf("%d ",buf[i]);
    }
    printf("\n");
    fd = open(argv[1],O_RDWR);
    if(fd <=0){
        printf("open error\n");
        return 0;
    }
    ret = write(fd,buf,count-2);
    if(ret <=0){
        printf("write error %d\n",ret);
    }
    
    // need to send all 0 otherwise host will treat as hold status
    memset(buf,0,4);
    ret = write(fd,buf,count-2);
    if(ret <=0){
        printf("write error %d\n",ret);
    }
    
    close(fd);
    

	return 0;
}
