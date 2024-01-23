
#need to source Yocto SDK environment as below:
#unset LD_LIBRARY_PATH
#source /opt/poky-xx/env-xxx

#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/mnt/home/pi/armnn-dist/opencv_4.1.0/lib_mini:/mnt/home/pi/armnn-dist/boost/lib
#CCXX=${CXX}
#CCXX=/media/disk/yocto/poky/build-microchip/tmp/work/sama5d2_xplained_sd-poky-linux-gnueabi/linux-mchp/6.1+gitAUTOINC+be80d933c7-r0/recipe-sysroot-native/usr/bin/arm-poky-linux-gnueabi/arm-poky-linux-gnueabi-gcc
#CCXX=arm-linux-gnueabihf-g++

#PRE = /home/emy/build_lib/opencv-4.1.0/__install_armv7hf
PRE = /home/emy/build_lib/opencv_yocto/usr
CFLAGS_=-I ${PRE}/include/opencv4
CFLAGS_ +=-I ${PRE}/include/opencv4/opencv2

#CFLAGS_ += -march=armv7-a -marm -mfpu=neon -mfloat-abi=hard


#LDFLAGS_=-L/media/disk/yocto/poky/build-microchip/tmp/deploy/images/rootfs2/usr/lib 
#LDFLAGS_1 =-L/media/disk/yocto/poky/build-microchip/tmp/deploy/images/rootfs2/lib
#LDFLAGS_=-L ${PRE}/lib
# -L /media/disk/yocto/poky/build-microchip/tmp/deploy/images/rootfs2/usr/lib
#LDFLAGS_+=-L /media/disk/yocto/poky/build-microchip/tmp/deploy/images/rootfs2/usr/lib
#LDFLAGS_=-L/media/disk/yocto/poky/build-microchip/tmp/deploy/images/rootfs2/usr/lib
#            /media/disk/yocto/poky/build-microchip/tmp/deploy/images/rootfs2/usr/lib/
#the libs must be put after the cpp files
#CFLAGS -lpng16 -lz -ltbb -ljpeg -lwebp -ltiff -llzma -lpthread
LIBS=-lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc  -lopencv_videoio 


#-std=c++11    $(CFLAGS_) $(LDFLAGS_) $(LDFLAGS_1) 
all:
	$(CXX)   opencv_test.cpp  -o opencv_test $(CFLAGS_) $(LIBS)
	$(CXX)   cv_decode.cpp  -o cv_decode $(CFLAGS_) $(LIBS)

.PHONY: clean
clean:
	rm -f *.o
