#include <linux/module.h> 
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

struct tasklet_struct	tasklet_rx;
spinlock_t	reg_lock;

static void atmel_tasklet_rx_func(unsigned long data)
{
    unsigned char *pt;
    printk("%s\n",__FUNCTION__); 
    
    
 #if 0   
    pt = kmalloc(1500,GFP_KERNEL);
    if(!pt){
        printk("GFP_KERNEL memory error\n");
        return;
    }
    
    memset(pt,0,1500);
    
    pt = kmalloc(1500,GFP_ATOMIC);
    if(!pt){
        printk("GFP_ATOMIC memory error\n");
        return;
    } 

    memset(pt,0x55,1500);
    
    while(1);
    #endif
    
    
    while(1);
    
    
    printk("test done\n");
}



static ssize_t sys_read_s(struct device *dev, struct device_attribute *attr,char *buf)
{

	printk("%s: test valuse \n",__FUNCTION__);
	return 0; 
 } 
 
 
 static ssize_t sys_write_s(struct device *dev, struct device_attribute *attr,const char *buf, size_t count) 
 { 
    printk("%s\n",__FUNCTION__); 
    
    tasklet_schedule(&tasklet_rx);

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


static int cc = 0;
static ssize_t
dev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    
    printk("%s,cpu %d\n",__func__,smp_processor_id());
    
    spin_lock(&reg_lock);
    cc++;
    msleep(20000);
    spin_unlock(&reg_lock);
    

    printk("spidev_read: cc %d\n",cc);
    
    return count;

}


static ssize_t
dev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
    printk("%s,cpu %d\n",__func__,smp_processor_id());
    
    spin_lock(&reg_lock);
    cc++;
    spin_unlock(&reg_lock);
    
    printk("spidev_write: cc %d\n",cc);
    return count;

}


static struct file_operations poll_dev_fops={ 
	.owner          = THIS_MODULE, 
	.open           = dev_open, 
	.release        = dev_release,
    .read           = dev_read,
    .write          = dev_write,
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
    
 	tasklet_init(&tasklet_rx, atmel_tasklet_rx_func,
			(unsigned long)0);   
            
    spin_lock_init(&reg_lock);
    
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
					