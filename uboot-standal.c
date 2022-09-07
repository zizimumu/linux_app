void (*show)(char*,...) = 0x10086a0;



void before_remap(void)
{
	int a= 100;
	show("hello uboot");
	
}