#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#define SIZE (100*1024*1024)

struct timespec spec_start;
struct timespec spec_end;


void u32_test(unsigned int *buf1, unsigned int *buf2, unsigned int len)
{
    unsigned int i;
    
    for(i=0;i<len;i++){
        buf1[i] = buf2[i];
    }
}

int main(int argc, char *arg[])
{
    unsigned char *buf1, *buf2;
    unsigned int i;
    
    buf1 = malloc(SIZE);
    buf2 = malloc(SIZE);
    
    for(i=0;i< SIZE; i++){
        buf1[i] = i;
        buf2[i] = i +255;
    }
    
    printf("memcpy write test\n");
    
    clock_gettime(CLOCK_MONOTONIC, &spec_start);
    for(i=0;i<3;i++)
        memcpy(buf1,buf2,SIZE);
    
    clock_gettime(CLOCK_MONOTONIC, &spec_end);

    printf("start time is %lds,%ldns\n", spec_start.tv_sec,spec_start.tv_nsec);
    printf("end   time is %lds,%ldns\n", spec_end.tv_sec,spec_end.tv_nsec);    
    
    printf("\nu32_test write test\n");
    clock_gettime(CLOCK_MONOTONIC, &spec_start);
    u32_test((unsigned int *)buf1, (unsigned int *)buf2,SIZE/4);
    clock_gettime(CLOCK_MONOTONIC, &spec_end);

    printf("start time is %lds,%ldns\n", spec_start.tv_sec,spec_start.tv_nsec);
    printf("end   time is %lds,%ldns\n", spec_end.tv_sec,spec_end.tv_nsec);   
    return 0;
}