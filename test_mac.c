#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>




#define LEN 10000000

 unsigned char buf1[LEN];
 unsigned char buf2[LEN];
 unsigned char buf3[LEN];
 unsigned char buf4[LEN];
struct timespec spec_start;
struct timespec spec_end;


void main()
{
    
    unsigned int i=0,j;

    
    #if 0
    for(i=0;i<3;i++){
        clock_gettime(CLOCK_MONOTONIC, &spec_start);
        usleep(1000000);
        clock_gettime(CLOCK_MONOTONIC, &spec_end);
        printf("start time is %lds,%ldns\n", spec_start.tv_sec,spec_start.tv_nsec);

        printf("end   time is %lds,%ldns\n", spec_end.tv_sec,spec_end.tv_nsec);   
    }
    #endif
    
    for(i=0;i<LEN;i++){
        buf4[i] = buf3[i] = buf1[i] = buf2[2] = i;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &spec_start);
    for(j=0;j<10;j++)
    for(i=0;i<LEN;i++){
        buf4[i] = buf1[i] * buf2[i] + buf3[i];
    }
    clock_gettime(CLOCK_MONOTONIC, &spec_end);
    
    for(i=0;i<LEN;i++){
        if(buf4[i] > 200)
            buf4[i] = 0;
    }
    
    printf("start time is %lds,%ldns\n", spec_start.tv_sec,spec_start.tv_nsec);

    printf("end   time is %lds,%ldns\n", spec_end.tv_sec,spec_end.tv_nsec);   
        
}