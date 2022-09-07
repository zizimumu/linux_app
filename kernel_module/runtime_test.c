#include <linux/module.h> 
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/kprobes.h>
#include <asm/traps.h>


static struct platform_device runtime_test_device = {
	.name = "runtime-test",
};



static ssize_t sys_read_s(struct device *dev, struct device_attribute *attr,char *buf)
{

	printk("%s: test valuse \n",__FUNCTION__);
	return 0; 
 } 
 

 static ssize_t sys_write_s(struct device *dev, struct device_attribute *attr,const char *buf, size_t count) 
 { 
	 printk("%s\n",__FUNCTION__); 

	 if(*buf == '1'){
	 	printk("pm_runtime_get\n");
		//pm_runtime_get(&runtime_test_device.dev);
        
        pm_runtime_get(&runtime_test_device.dev);
	 }
	 else if(*buf == '2'){
		printk("pm_runtime_put_autosuspend\n");
		//pm_runtime_put(&runtime_test_device.dev);
        pm_runtime_mark_last_busy(&runtime_test_device.dev);
        pm_runtime_put_autosuspend(&runtime_test_device.dev);
	 }
     else{
		printk("pm_runtime_put\n");
		//pm_runtime_put(&runtime_test_device.dev);         
         
     }

	 
	 return count;  
 }  
 
 static DEVICE_ATTR(test, S_IRUGO | S_IWUSR, sys_read_s,sys_write_s);
 
 
static struct attribute *test_attrs[]={
	&dev_attr_test.attr,

	NULL,
};

struct attribute_group test_group={
	.attrs = test_attrs,
};





static int runtime_test_suspend(struct device *dev)
{
	printk("\n\n");
	printk("%s\n",__FUNCTION__); 

	dump_stack();
	return 0;
}

static int runtime_test_resume(struct device *dev)
{
	printk("\n\n");
	printk("%s\n",__FUNCTION__); 
	dump_stack();
	return 0;
}

static int runtime_test_idle(struct device *dev)
{
	printk("\n\n");
	printk("%s\n",__FUNCTION__);
	dump_stack();
	return 0;
}





static const struct dev_pm_ops test_pm_ops = {
	SET_RUNTIME_PM_OPS(runtime_test_suspend, runtime_test_resume, runtime_test_idle)
};



static int runtime_test_probe(struct platform_device *pdev)
{
	printk("runtime_test_probe\n");

#if 1



	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
    
#else    
    
	pm_runtime_get_noresume(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	pm_runtime_set_autosuspend_delay(&pdev->dev, 1000);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_put_autosuspend(&pdev->dev);
    
#endif   
    
	return 0;
}


static int runtime_test_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);

	return 0;
}



static struct platform_driver runtime_test_driver = {
	.driver		= {
		.name	= "runtime-test",
		.pm	= &test_pm_ops,
	},
	.probe		= runtime_test_probe,
	.remove		= runtime_test_remove,
};



static int __init sys_test_init(void)  
{  

	int ret = 0;
	printk("%s\n",__func__);
 


	platform_driver_register(&runtime_test_driver);
	platform_device_register(&runtime_test_device);
	
	if(sysfs_create_group(&runtime_test_device.dev.kobj, &test_group) != 0)
	{
		printk("create %s sys-file err \n",test_group.name);
	}



	
	return ret;
} 
static void __exit sys_test_exit(void)  
{ 
	sysfs_remove_group(&runtime_test_device.dev.kobj, &test_group);
	platform_driver_unregister(&runtime_test_driver);
	platform_device_unregister(&runtime_test_device);

	printk("%s\n",__func__); 
} 

module_init(sys_test_init); 
module_exit(sys_test_exit); 
MODULE_LICENSE("GPL");  
MODULE_AUTHOR("www"); 
					
