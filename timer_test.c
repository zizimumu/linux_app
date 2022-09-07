#include <linux/module.h> 
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>

static ssize_t sys_read_s(struct device *dev, struct device_attribute *attr,char *buf)
{

	printk("%s: test valuse \n",__FUNCTION__);
	return 0; 
 } 
 
 
 
static struct hrtimer timer;
ktime_t kt;


ktime_t star;
ktime_t end;


static enum hrtimer_restart hrtimer_handler(struct hrtimer *timer)
{
	
	end = timer->base->get_time();
	//hrtimer_forward(timer, now, kt);
	printk("start time : %lld ns\n",star.tv64);
	printk("end time   : %lld ns\n",end.tv64);
	
	//return HRTIMER_RESTART;
	return HRTIMER_NORESTART;
}

void hr_timer_test(void)
{
	
	
	kt = ktime_set(0, 1000*1000); /* 1 sec, 10 nsec */
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = hrtimer_handler;
	
	hrtimer_start(&timer, kt, HRTIMER_MODE_REL);

	star = timer.base->get_time();
	//printk("start time : %lld ns\n",now.tv64);
		 
}


#if 0
struct timer_list mytimer;

static void myfunc(unsigned long data)

{

       printk("%s\n", (char *)data);

       mod_timer(&mytimer,jiffies + 2*HZ);

}
static void normal_time_test()
{
	setup_timer(&mytimer, myfunc, (unsigned long)"Hello,world!");

	mytimer.expires =jiffies + HZ;

	add_timer(&mytimer);
	
}


#endif
 static ssize_t sys_write_s(struct device *dev, struct device_attribute *attr,const char *buf, size_t count) 
 { 
	 printk("%s\n",__FUNCTION__); 
	 
	if(*buf == '0')
		hr_timer_test();
	else
		hrtimer_cancel(&timer);

		
	 
	 return count;  
 }  
 
 static DEVICE_ATTR(uart_gpio, S_IRUGO | S_IWUSR, sys_read_s,sys_write_s);
 
 
static struct attribute *gk7101_uart_attrs[]={
	&dev_attr_uart_gpio.attr,

	NULL,
};

struct attribute_group uart_group={
	.attrs = gk7101_uart_attrs,
};

static int dev_open(struct inode *inode, struct file *file){
	return 0; 
}  
static int dev_release(struct inode *node, struct file *filp) 
{  
	return 0; 
} 
static struct file_operations poll_dev_fops={ 
	.owner          = THIS_MODULE, 
	.open           = dev_open, 
	.release        = dev_release,
};  

static struct miscdevice poll_dev = {
	.minor          = MISC_DYNAMIC_MINOR,
	.name           = "sys_test",
	.fops           = &poll_dev_fops,
}; 
static int __init sys_test_init(void)  
{  

	int ret = 0;
	printk("%s\n",__func__);

	ret = misc_register(&poll_dev); 
	if(sysfs_create_group(&poll_dev.this_device->kobj, &uart_group) != 0)
	{
		printk("create %s sys-file err \n",uart_group.name);
	}
	return ret;
} 
static void __exit sys_test_exit(void)  
{ 
	sysfs_remove_group(&poll_dev.this_device->kobj, &uart_group);
	misc_deregister(&poll_dev);
	printk("%s\n",__func__); 
} 

module_init(sys_test_init); 
module_exit(sys_test_exit); 
MODULE_LICENSE("GPL");  
MODULE_AUTHOR("www"); 
					