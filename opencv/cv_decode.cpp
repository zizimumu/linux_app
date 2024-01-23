#include <opencv2/opencv.hpp>
#include <fstream>
#include <vector>
#include <sys/time.h>
#include <time.h>


struct timespec spec_start;
struct timespec spec_end;



using namespace cv;
using namespace std;
int main()
{
  // 读取图像数据
  ifstream ifs("test_4k.jpg", ios::binary|ios::ate);
  int file_size = ifs.tellg();
  Mat img;
  
  
  ifs.seekg(0, ios::beg);
  vector<char> buffer(file_size);
  ifs.read(buffer.data(), file_size);
  ifs.close();
  // 解码图像
  
  clock_gettime(CLOCK_MONOTONIC, &spec_start);
  
  for(int i=0;i<10;i++)
	img = imdecode(Mat(buffer), IMREAD_COLOR);
  
  
  clock_gettime(CLOCK_MONOTONIC, &spec_end);
  
  if(img.empty())
  {
    printf("failed to decode image\n");
    return -1;
  }
  
	printf("start time is %lds,%ldns\n", spec_start.tv_sec,spec_start.tv_nsec);
	printf("end   time is %lds,%ldns\n", spec_end.tv_sec,spec_end.tv_nsec);  
  
  
  
  
  // imshow("image", img);
  // waitKey(0);
  return 0;
}