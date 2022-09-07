#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;
using namespace cv;

#define MAX_PATH 512  //最大文件长度定义为512

int fd_data,fd_lab,fd_class;

void dirwalk(char *dir)
{
	char name[MAX_PATH];
	struct dirent *dp;
	DIR *dfd, *dfd_n;
	static unsigned int dir_count = 0;
	static unsigned int file_count = 0;
	
	if((dfd = opendir(dir)) == NULL){
		fprintf(stderr, "dirwalk: can't open %s\n", dir);
		return;
	}
	
	while((dp = readdir(dfd)) != NULL){ //读目录记录项
		if(strcmp(dp->d_name, ".") == 0 || strcmp(dp -> d_name, "..") == 0){
			continue;  //跳过当前目录以及父目录
		}
		
		if(strlen(dir) + strlen(dp -> d_name) + 2 > sizeof(name)){
			fprintf(stderr, "dirwalk : name %s %s too long\n", dir, dp->d_name);
		}else{
			if( dp->d_type == DT_DIR){
				printf("dir : %s\n",dp->d_name);
				dir_count++;

				sprintf(name, "%s/%s", dir, dp->d_name);
				dirwalk(name);

			}
			if( dp->d_type == DT_REG){
				printf("file: %s, %d\n",dp->d_name,dir_count);

				sprintf(name, "%s/%s", dir, dp->d_name);
				Mat img = imread(name,IMREAD_GRAYSCALE);
				write(fd_data,img.data,img.cols*rows)
				write(fd_lab,&dir_count,sizeof(dir_count));
			}

		}
	}
	
	closedir(dfd);
}




int main(int argc, char *argv[])
{
	fd_data = open("data.bin",O_RDWR);
	fd_lab = open("lab.bin",O_RDWR);
	fd_class = open("class.bin",O_RDWR);


	if(argc == 1){
		dirwalk(".");//未加参数执行时，从当前目录开始遍历
	}else{
		while(--argc>0){
			dirwalk(*++argv);
		}
	}



	close(fd_data);
	close(fd_lab);
	close(fd_class);

	return 0;
}

