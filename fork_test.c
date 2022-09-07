#include <stdio.h>
#include <unistd.h>


int g_a = 0;



int main()
{
	pid_t pid;
	 pid = fork();
	if(pid == -1){
		printf("fork error\n");
	}
	else if(pid == 0){
		while(1){
			g_a++;
			printf("child process, %d\n",g_a);
			sleep(2);
		}
	}
	else{
		while(1){
		
			printf("main process, %d\n",g_a);
			sleep(2);
		}	
	}

}
