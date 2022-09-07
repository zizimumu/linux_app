

CROSS_COMPILE = arm-linux-gnueabihf-




default:
	${CROSS_COMPILE}gcc receiver_frames.c -o receiver_frames -lrt
	${CROSS_COMPILE}gcc send_frames.c -o send_frames

clean:
	rm -rf *.o *.ko *.mod.c .*.cmd *.markers *.order *.symvers .tmp_versions

