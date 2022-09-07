

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>


struct timespec spec_start;
struct timespec spec_end;

struct itimerspec ts;
timer_t timer;		
		
void  handle()
{
    int ret;
 clock_gettime(CLOCK_MONOTONIC, &spec_end);



 printf("start time is %lds,%ldns\n", spec_start.tv_sec,spec_start.tv_nsec);
 
 printf("end   time is %lds,%ldns\n", spec_end.tv_sec,spec_end.tv_nsec);
 
 
 	//ret = timer_settime(timer, 0, &ts, NULL);
	//if( ret )
	//	perror("timer_settime");
}

int main()
{
	struct sigevent evp;

	int ret;
	
	evp.sigev_value.sival_ptr = &timer;
	evp.sigev_notify = SIGEV_SIGNAL;
	evp.sigev_signo = SIGUSR1;
	
	signal(SIGUSR1, handle);
	
	ret = timer_create(CLOCK_REALTIME, &evp, &timer);
	if( ret )
		perror("timer_create");
	
	ts.it_interval.tv_sec = 1;  // 第一超时后以后每次超时时间
	ts.it_interval.tv_nsec = 0;
	ts.it_value.tv_sec = 1;  // 第一次超时时间
	ts.it_value.tv_nsec = 0; //1000000;
	
	clock_gettime(CLOCK_MONOTONIC, &spec_start);
	
	ret = timer_settime(timer, 0, &ts, NULL);
	if( ret )
		perror("timer_settime");
	
	while(1);
}