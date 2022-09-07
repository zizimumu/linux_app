/*
 * USB Skeleton driver - 2.2
 *
 * Copyright (C) 2001-2004 Greg Kroah-Hartman (greg@kroah.com)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 * This driver is based on the 2.6.3 version of drivers/usb/usb-skeleton.c
 * but has been rewritten to be easier to read and use.
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>

// mm.h must be included before vmstat.h
#include <linux/mm.h>
#include <linux/vmstat.h>
#include <linux/atomic.h>
#include <linux/vmalloc.h>
#include <linux/cma.h>
#include <asm/page.h>
#include <asm/pgtable.h>

/* Define these values to match your devices */
// #define USB_SKEL_VENDOR_ID	0xfff0
// #define USB_SKEL_PRODUCT_ID	0xfff0

#define USB_SKEL_VENDOR_ID	0x0525		/* NetChip */
#define USB_SKEL_PRODUCT_ID	0xa4a0		/* Linux-USB "Gadget Zero" */

/* table of devices that work with this driver */
static const struct usb_device_id skel_table[] = {


	{ USB_DEVICE(USB_SKEL_VENDOR_ID, USB_SKEL_PRODUCT_ID) },
	{ }					/* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, skel_table);

enum{
	TEST_BASE = 10,
	TEST_BULK_OUT,
	TEST_ISO_OUT
};


static ktime_t timer_start;
static ktime_t timer_end;
static int state_end;
struct usb_skel *g_dev;

#define TEST_LEN (1024*1024)
#define URB_BUF_SIZE (18*1024)
#define TARGET_TEST_BYTES (500*1024*1024)
#define URB_QUEUE_LEN 16


/* Get a minor range for your devices from the usb maintainer */
#define USB_SKEL_MINOR_BASE	192

/* our private defines. if this grows any larger, use your own .h file */
#define MAX_TRANSFER		(PAGE_SIZE - 512)
/* MAX_TRANSFER is chosen so that the VM is not stressed by
   allocations > PAGE_SIZE and the number of packets in a page
   is an integer 512 is the largest possible packet on EHCI */
#define WRITES_IN_FLIGHT	8
/* arbitrarily chosen */

/* Structure to hold all of our device specific stuff */
struct usb_skel {
	struct usb_device	*udev;			/* the usb device for this device */
	struct usb_interface	*interface;		/* the interface for this device */
	struct semaphore	limit_sem;		/* limiting the number of writes in progress */
	struct usb_anchor	submitted;		/* in case we need to retract our submissions */
	struct urb		*bulk_in_urb;		/* the urb to read data with */
	struct urb		*bulk_out_urb[URB_QUEUE_LEN];		/* the urb to read data with */
	struct urb		*bulk_out_urb2[URB_QUEUE_LEN];		/* the urb to read data with */
	struct urb		*iso_out_urb[URB_QUEUE_LEN];		/* the urb to read data with */
	struct urb		*iso_out_urb2[URB_QUEUE_LEN];		/* the urb to read data with */
	unsigned char           *bulk_in_buffer;	/* the buffer to receive data */
	size_t			bulk_in_size;		/* the size of the receive buffer */
	size_t			bulk_in_filled;		/* number of bytes in the buffer */
	size_t			bulk_in_copied;		/* already copied to user space */
	__u8			bulk_in_endpointAddr;	/* the address of the bulk in endpoint */
	__u8			bulk_out_endpointAddr;	/* the address of the bulk out endpoint */
	__u8			bulk_out_endpointAddr2;	/* the address of the bulk out endpoint */
	__u8			sync_out_endpointAddr;	/* the address of the bulk out endpoint */
	__u8			sync_out_endpointAddr2;	/* the address of the bulk out endpoint */
	__u8			test_mode;
	wait_queue_head_t wait_q;
	struct usb_endpoint_descriptor *			sync_out_ep_desc;
	struct usb_endpoint_descriptor *			sync_out_ep_desc2;	
	int			errors;			/* the last request tanked */
	bool			ongoing_read;		/* a read is going on */
	spinlock_t		err_lock;		/* lock for errors */
	struct kref		kref;
	struct mutex		io_mutex;		/* synchronize I/O with disconnect */
	wait_queue_head_t	bulk_in_wait;		/* to wait for an ongoing read */
};
#define to_skel_dev(d) container_of(d, struct usb_skel, kref)

static struct usb_driver skel_driver;
static void skel_draw_down(struct usb_skel *dev);

static void skel_delete(struct kref *kref)
{
	struct usb_skel *dev = to_skel_dev(kref);

	usb_free_urb(dev->bulk_in_urb);
	usb_put_dev(dev->udev);
	kfree(dev->bulk_in_buffer);
	kfree(dev);
}

static int skel_open(struct inode *inode, struct file *file)
{
	struct usb_skel *dev;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;

	printk("+++skel_open\n");

	
	subminor = iminor(inode);

	interface = usb_find_interface(&skel_driver, subminor);
	if (!interface) {
		pr_err("%s - error, can't find device for minor %d\n",
			__func__, subminor);
		retval = -ENODEV;
		goto exit;
	}
	printk("usb_find_interface\n");
	dev = usb_get_intfdata(interface);
	if (!dev) {
		retval = -ENODEV;
		goto exit;
	}
printk("usb_get_intfdata\n");
	//retval = usb_autopm_get_interface(interface);
	//if (retval)
	//	goto exit;
printk("usb_autopm_get_interface\n");
	/* increment our usage count for the device */
	kref_get(&dev->kref);

	/* save our object in the file's private structure */
	file->private_data = dev;


	printk("retval %d\n",retval);

exit:
	return retval;
}

static int skel_release(struct inode *inode, struct file *file)
{
	struct usb_skel *dev;

	dev = file->private_data;
	if (dev == NULL)
		return -ENODEV;

	/* allow the device to be autosuspended */
	mutex_lock(&dev->io_mutex);
	//if (dev->interface)
	//	usb_autopm_put_interface(dev->interface);
	mutex_unlock(&dev->io_mutex);

	/* decrement the count on our device */
	kref_put(&dev->kref, skel_delete);
	return 0;
}

static int skel_flush(struct file *file, fl_owner_t id)
{
	struct usb_skel *dev;
	int res;

	dev = file->private_data;
	if (dev == NULL)
		return -ENODEV;

	/* wait for io to stop */
	mutex_lock(&dev->io_mutex);
	skel_draw_down(dev);

	/* read out errors, leave subsequent opens a clean slate */
	spin_lock_irq(&dev->err_lock);
	res = dev->errors ? (dev->errors == -EPIPE ? -EPIPE : -EIO) : 0;
	dev->errors = 0;
	spin_unlock_irq(&dev->err_lock);

	mutex_unlock(&dev->io_mutex);

	return res;
}

static void skel_read_bulk_callback(struct urb *urb)
{
	struct usb_skel *dev;

	dev = urb->context;

	spin_lock(&dev->err_lock);
	/* sync/async unlink faults aren't errors */
	if (urb->status) {
		if (!(urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN))
			dev_err(&dev->interface->dev,
				"%s - nonzero write bulk status received: %d\n",
				__func__, urb->status);

		dev->errors = urb->status;
	} else {
		dev->bulk_in_filled = urb->actual_length;
	}
	dev->ongoing_read = 0;
	spin_unlock(&dev->err_lock);

	wake_up_interruptible(&dev->bulk_in_wait);
}

static int skel_do_read_io(struct usb_skel *dev, size_t count)
{
	int rv;

	/* prepare a read */
	usb_fill_bulk_urb(dev->bulk_in_urb,
			dev->udev,
			usb_rcvbulkpipe(dev->udev,
				dev->bulk_in_endpointAddr),
			dev->bulk_in_buffer,
			min(dev->bulk_in_size, count),
			skel_read_bulk_callback,
			dev);
	/* tell everybody to leave the URB alone */
	spin_lock_irq(&dev->err_lock);
	dev->ongoing_read = 1;
	spin_unlock_irq(&dev->err_lock);

	/* submit bulk in urb, which means no data to deliver */
	dev->bulk_in_filled = 0;
	dev->bulk_in_copied = 0;

	/* do it */
	rv = usb_submit_urb(dev->bulk_in_urb, GFP_KERNEL);
	if (rv < 0) {
		dev_err(&dev->interface->dev,
			"%s - failed submitting read urb, error %d\n",
			__func__, rv);
		rv = (rv == -ENOMEM) ? rv : -EIO;
		spin_lock_irq(&dev->err_lock);
		dev->ongoing_read = 0;
		spin_unlock_irq(&dev->err_lock);
	}

	return rv;
}

static ssize_t skel_read(struct file *file, char *buffer, size_t count,
			 loff_t *ppos)
{
	struct usb_skel *dev;
	int rv;
	bool ongoing_io;

	dev = file->private_data;

	/* if we cannot read at all, return EOF */
	if (!dev->bulk_in_urb || !count)
		return 0;

	/* no concurrent readers */
	rv = mutex_lock_interruptible(&dev->io_mutex);
	if (rv < 0)
		return rv;

	if (!dev->interface) {		/* disconnect() was called */
		rv = -ENODEV;
		goto exit;
	}

	/* if IO is under way, we must not touch things */
retry:
	spin_lock_irq(&dev->err_lock);
	ongoing_io = dev->ongoing_read;
	spin_unlock_irq(&dev->err_lock);

	if (ongoing_io) {
		/* nonblocking IO shall not wait */
		if (file->f_flags & O_NONBLOCK) {
			rv = -EAGAIN;
			goto exit;
		}
		/*
		 * IO may take forever
		 * hence wait in an interruptible state
		 */
		rv = wait_event_interruptible(dev->bulk_in_wait, (!dev->ongoing_read));
		if (rv < 0)
			goto exit;
	}

	/* errors must be reported */
	rv = dev->errors;
	if (rv < 0) {
		/* any error is reported once */
		dev->errors = 0;
		/* to preserve notifications about reset */
		rv = (rv == -EPIPE) ? rv : -EIO;
		/* report it */
		goto exit;
	}

	/*
	 * if the buffer is filled we may satisfy the read
	 * else we need to start IO
	 */

	if (dev->bulk_in_filled) {
		/* we had read data */
		size_t available = dev->bulk_in_filled - dev->bulk_in_copied;
		size_t chunk = min(available, count);

		if (!available) {
			/*
			 * all data has been used
			 * actual IO needs to be done
			 */
			rv = skel_do_read_io(dev, count);
			if (rv < 0)
				goto exit;
			else
				goto retry;
		}
		/*
		 * data is available
		 * chunk tells us how much shall be copied
		 */

		if (copy_to_user(buffer,
				 dev->bulk_in_buffer + dev->bulk_in_copied,
				 chunk))
			rv = -EFAULT;
		else
			rv = chunk;

		dev->bulk_in_copied += chunk;

		/*
		 * if we are asked for more than we have,
		 * we start IO but don't wait
		 */
		if (available < count)
			skel_do_read_io(dev, count - chunk);
	} else {
		/* no data in the buffer */
		rv = skel_do_read_io(dev, count);
		if (rv < 0)
			goto exit;
		else
			goto retry;
	}
exit:
	mutex_unlock(&dev->io_mutex);
	return rv;
}


static unsigned int g_count = 0;
static void skel_write_callback(struct urb *urb)
{
	struct usb_skel *dev;
	int retval = 0;
	

	dev = urb->context;

	/* sync/async unlink faults aren't errors */
	if (urb->status) {
		if (!(urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN))
			dev_err(&dev->interface->dev,
				"%s - nonzero write bulk status received: %d\n",
				__func__, urb->status);


		return;
	}


	
	if(dev->test_mode == TEST_BASE){
		printk("write done\n");
		
		/* free up our allocated buffer */
		usb_free_coherent(urb->dev, urb->transfer_buffer_length,
				  urb->transfer_buffer, urb->transfer_dma);
		
		usb_free_urb(urb);
		


	}
	else if(dev->test_mode == TEST_BULK_OUT || dev->test_mode == TEST_ISO_OUT){

	
		g_count += URB_BUF_SIZE;
		if(g_count >= TARGET_TEST_BYTES && state_end == 0){
			// wake_up(&dev->wait_q);

		
			state_end = 1;
			timer_end = ktime_get();

			printk("start time : %lld ns\n",timer_start.tv64);
			printk("end time   : %lld ns\n",timer_end.tv64);

			printk("write done\n");
		}
		else if(state_end == 1){

			
		}
		else{
		
			retval = usb_submit_urb(urb, GFP_KERNEL);
			if (retval) {
				dev_err(&dev->interface->dev,
					"%s - failed submitting write urb, error %d\n",
					__func__, retval);
			}
		}
	}



	
		
}


#define MIN(x,y) ((x)>=(y)?(y):(x) ) 


static void skel_repare_send_urb(struct usb_skel *dev,int queue_len)
{

	int retval = 0;
	struct urb *urb = NULL;
	char *buf = NULL;

	int i;


	for(i=0;i<queue_len;i++){
		
		urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!urb) {
			dev_err(&dev->interface->dev," failed alloc urb\n");

			break;
		}
		
		
		buf = usb_alloc_coherent(dev->udev, URB_BUF_SIZE, GFP_KERNEL,
					 &urb->transfer_dma);
		if (!buf) {
			dev_err(&dev->interface->dev," failed alloc usb memory\n");
			break;
		}
		memset(buf,0x55,URB_BUF_SIZE);
		

		/* initialize the urb properly */
		usb_fill_bulk_urb(urb, dev->udev,
				  usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),
				  buf, URB_BUF_SIZE, skel_write_callback, dev);
		urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

		//usb_anchor_urb(urb, &dev->submitted);
		
		/* send the data out the bulk port */
		retval = usb_submit_urb(urb, GFP_KERNEL);
		if (retval) {
			dev_err(&dev->interface->dev,
				"%s - failed submitting write urb, error %d\n",
				__func__, retval);
			break;

		}
	}

}

static void skel_prepare_iso_urbs(struct usb_skel *dev,struct urb		*urbs[], struct usb_endpoint_descriptor *	ep)
{

	int retval = 0;
	struct urb *urb = NULL;
	char *buf = NULL;
	size_t writesize ;

	int maxp,packets,i,j,bytes;

    maxp = 0x7ff & ep->wMaxPacketSize;
    maxp *= 1 + (0x3 & (ep->wMaxPacketSize >> 11));

	//maxp = 1024*3;
    packets = DIV_ROUND_UP(URB_BUF_SIZE, maxp);

    printk("EP size %d, packets %d, CMA free %d\n",maxp,packets, global_page_state(NR_FREE_CMA_PAGES) );
		


	for(i=0;i<URB_QUEUE_LEN;i++){
		urb = usb_alloc_urb(packets, GFP_KERNEL);

        if (!urb) {
			dev_err(&dev->interface->dev,"%s  failed alloc urb\n",__func__);
			break;
		}
        
		urb->dev = dev->udev;
		urb->pipe =  usb_sndisocpipe(dev->udev, ep->bEndpointAddress
					& USB_ENDPOINT_NUMBER_MASK);

		
		urb->number_of_packets = packets;
		urb->transfer_buffer_length = URB_BUF_SIZE;
		urb->transfer_buffer = usb_alloc_coherent(dev->udev, URB_BUF_SIZE,
								GFP_KERNEL,
								&urb->transfer_dma);
		if (!urb->transfer_buffer) {
			dev_err(&dev->interface->dev,"%s  failed alloc %d usb memory\n",__func__,i);
			break;
		}
		// printk("loop %d, CMA free %d\n",i, global_page_state(NR_FREE_CMA_PAGES) );



		

		bytes = URB_BUF_SIZE;
		for (j = 0; j < packets; j++) {
			/* here, only the last packet will be short */
			urb->iso_frame_desc[j].length = MIN((unsigned) URB_BUF_SIZE, maxp);
			bytes -= urb->iso_frame_desc[j].length;
		
			urb->iso_frame_desc[j].offset = maxp * j;
		}
		
		urb->complete = skel_write_callback;
		urb->context = dev;
		urb->interval = 1; // 1 << (dev->sync_out_ep_desc->bInterval - 1);
		urb->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;

		urbs[i] = urb;

	}
}
static void skel_prepare_urbs(struct usb_skel *dev,struct urb		*urbs[],__u8 endpoint )
{

	int retval = 0;
	struct urb *urb = NULL;
	char *buf = NULL;

	int i;


	for(i=0;i<URB_QUEUE_LEN;i++){
		
		urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!urb) {
			dev_err(&dev->interface->dev," failed alloc urb\n");

			break;
		}
		
		
		buf = usb_alloc_coherent(dev->udev, URB_BUF_SIZE, GFP_KERNEL,
					 &urb->transfer_dma);
		if (!buf) {
			dev_err(&dev->interface->dev," failed alloc usb memory\n");
			break;
		}
		memset(buf,0x55,URB_BUF_SIZE);
		

		/* initialize the urb properly */
		usb_fill_bulk_urb(urb, dev->udev,
				  usb_sndbulkpipe(dev->udev, endpoint),
				  buf, URB_BUF_SIZE, skel_write_callback, dev);
		urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

		urbs[i] = urb;
	}

}


static void skel_send_urbs(struct usb_skel *dev,struct urb		*urbs[])
{

	int retval = 0;

	int i;
	static int c = 0;


	for(i=0;i<URB_QUEUE_LEN;i++){

		if(urbs[i]){
			c++;
			retval = usb_submit_urb(urbs[i], GFP_KERNEL);

			if (retval) {
				dev_err(&dev->interface->dev,
					"%s - failed submitting write urb %d, error %d\n",
					__func__, c,retval);
				break;
			}
		}

	}

}



static void skel_kill_urbs(struct usb_skel *dev,struct urb		*urbs[])
{
	int retval = 0;
	int i;

	for(i=0;i<URB_QUEUE_LEN;i++){
		if(urbs[i]){
			usb_kill_urb(urbs[i]);
		}
	}

}
static void skel_kill_iso_urbs(struct usb_skel *dev)
{
	int retval = 0;
	int i;

	for(i=0;i<URB_QUEUE_LEN;i++){
		
		usb_kill_urb(dev->iso_out_urb[i]);
	}

}


static ssize_t skel_write(struct file *file, const char __user *user_buffer,
			  size_t count, loff_t *ppos)
{
	struct usb_skel *dev;
	int retval = 0;
	struct urb *urb = NULL;
	char *buf = NULL;
	size_t writesize = MIN(count, (size_t)MAX_TRANSFER);
	unsigned char mode = 0;
	int maxp,packets,i,bytes;

	dev = file->private_data;


	printk("skel_write\n");
	copy_from_user(&mode, user_buffer, 1);

	if(mode == '1' || mode == '0'){ // bulk out

		printk("bulk out write starting... ");

		dev->test_mode = TEST_BULK_OUT;
		skel_kill_urbs(dev,dev->bulk_out_urb);
		skel_kill_urbs(dev,dev->bulk_out_urb2);
		msleep(1);

		g_count = 0;
		skel_send_urbs(dev,dev->bulk_out_urb);
		skel_send_urbs(dev,dev->bulk_out_urb2);
		
		state_end = 0;
		timer_start = ktime_get();

		// wait_event(dev->wait_q,1);

		
	}

	else if(mode == '2'){ // ISO out

		printk("ISO out write starting...\n");

		dev->test_mode = TEST_ISO_OUT;
			
	
		skel_kill_urbs(dev,dev->iso_out_urb);
		skel_kill_urbs(dev,dev->iso_out_urb2);
		msleep(1);

		g_count = 0;
		skel_send_urbs(dev,dev->iso_out_urb);
		skel_send_urbs(dev,dev->iso_out_urb2);

		state_end = 0;
		timer_start = ktime_get();

	}

	/*
	 * release our reference to this urb, the USB core will eventually free
	 * it entirely
	 */
	//usb_free_urb(urb);




	return writesize;

error_unanchor:
	usb_unanchor_urb(urb);
error:
	if (urb) {
		usb_free_coherent(dev->udev, writesize, buf, urb->transfer_dma);
		usb_free_urb(urb);
	}
	up(&dev->limit_sem);

exit:
	return retval;
}

static const struct file_operations skel_fops = {
	.owner =	THIS_MODULE,
	.read =		skel_read,
	.write =	skel_write,
	.open =		skel_open,
	.release =	skel_release,
	.flush =	skel_flush,
	.llseek =	noop_llseek,
};

/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with the driver core
 */
static struct usb_class_driver skel_class = {
	.name =		"skel%d",
	.fops =		&skel_fops,
	.minor_base =	USB_SKEL_MINOR_BASE,
};



static ssize_t sys_read_s(struct device *dev, struct device_attribute *attr,char *buf)
{

	printk("%s: test valuse \n",__FUNCTION__);
	return 0; 
 } 
 

 static ssize_t sys_write_s(struct device *dev, struct device_attribute *attr,const char *buf, size_t count) 
 { 
	 printk("%s\n",__FUNCTION__); 


/*
	if(*buf == '1'){

		printk("bulk test\n");
		g_dev->test_mode = TEST_BULK_OUT;
		skel_kill_urbs(g_dev);
		msleep(1);

		skel_send_urbs(g_dev);
		state_end = 0;
		timer_start = ktime_get();

	}
	else{
		printk("iso test\n");
		g_dev->test_mode = TEST_ISO_OUT;
			
	
		skel_kill_iso_urbs(g_dev);
		msleep(1);

		skel_send_iso_urbs(g_dev);
		state_end = 0;
		timer_start = ktime_get();
	}
*/
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



static int skel_probe(struct usb_interface *interface,
		      const struct usb_device_id *id)
{
	struct usb_skel *dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	size_t buffer_size;
	int i;
	int retval = -ENOMEM;

	printk("skel_probe\n");
	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev){
		printk("kzalloc error\n");
		goto error;
	}
	kref_init(&dev->kref);
	sema_init(&dev->limit_sem, WRITES_IN_FLIGHT);
	mutex_init(&dev->io_mutex);
	spin_lock_init(&dev->err_lock);
	init_usb_anchor(&dev->submitted);
	//init_waitqueue_head(&dev->bulk_in_wait);

	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	dev->interface = interface;

	/* set up the endpoint information */
	/* use only the first bulk-in and bulk-out endpoints */




	iface_desc = interface->cur_altsetting;			
	

	retval = usb_set_interface(dev->udev,
				iface_desc->desc.bInterfaceNumber,1);
	if(retval){
		printk("set alternate falied %d\n",retval);
		return retval;
	}


	printk("num_altsetting %d\n",interface->num_altsetting);
	
	for(i=0;i<interface->num_altsetting;i++){
		
		
		printk("num_altsetting %d, bNumEndpoints %d\n",
				i,interface->altsetting[i].desc.bNumEndpoints);
	}
	

	iface_desc = &interface->altsetting[1];
	
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if(endpoint){
			printk("EP addr 0x%x\n",endpoint->bEndpointAddress);
		}

		if (!dev->bulk_in_endpointAddr &&
		    usb_endpoint_is_bulk_in(endpoint)) {
			/* we found a bulk in endpoint */
			buffer_size = usb_endpoint_maxp(endpoint);
			dev->bulk_in_size = buffer_size;
			dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			dev->bulk_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
			if (!dev->bulk_in_buffer){
				printk("bulk_in_buffer error\n");
				goto error;
			}
			dev->bulk_in_urb = usb_alloc_urb(0, GFP_KERNEL);
			if (!dev->bulk_in_urb){
				printk("bulk_in_urb error\n");
				goto error;
			}
		}

		if (!dev->bulk_out_endpointAddr &&
		    usb_endpoint_is_bulk_out(endpoint)) {
			printk("found bulk out ENP\n");
			dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
		}
		else if(!dev->bulk_out_endpointAddr2 &&
			usb_endpoint_is_bulk_out(endpoint)) {
			printk("found bulk out 2 ENP\n");
			dev->bulk_out_endpointAddr2 = endpoint->bEndpointAddress;


		}

		if (!dev->sync_out_endpointAddr &&
		    usb_endpoint_is_isoc_out(endpoint)) {
			printk("found ISO out ENP\n");
			dev->sync_out_endpointAddr = endpoint->bEndpointAddress;
			dev->sync_out_ep_desc = endpoint;
			endpoint->wMaxPacketSize = 1024 | (1<<12)  ;
		}else if(!dev->sync_out_endpointAddr2 &&
		    usb_endpoint_is_isoc_out(endpoint)) {

			printk("found ISO 2out ENP\n");
			dev->sync_out_endpointAddr2 = endpoint->bEndpointAddress;
			dev->sync_out_ep_desc2 = endpoint;
			endpoint->wMaxPacketSize = 1024 | (1<<12)  ;

		}

			
	}
	//if (!(dev->bulk_in_endpointAddr && dev->bulk_out_endpointAddr)) {
	//	dev_err(&interface->dev,
	//		"Could not find both bulk-in and bulk-out endpoints\n");
	//	goto error;
	//}

	/* save our data pointer in this interface device */
	usb_set_intfdata(interface, dev);

	/* we can register the device now, as it is ready */
	retval = usb_register_dev(interface, &skel_class);
	if (retval) {
		/* something prevented us from registering this driver */
		dev_err(&interface->dev,
			"Not able to get a minor for this device.\n");
		usb_set_intfdata(interface, NULL);
		goto error;
	}

	g_dev = dev;
	dev->test_mode = TEST_BASE;
	if(dev->bulk_out_endpointAddr)
		skel_prepare_urbs(dev,dev->bulk_out_urb, dev->bulk_out_endpointAddr);

	if(dev->bulk_out_endpointAddr2)
		skel_prepare_urbs(dev,dev->bulk_out_urb2, dev->bulk_out_endpointAddr2);

	if(dev->sync_out_ep_desc)
		skel_prepare_iso_urbs(dev,dev->iso_out_urb,dev->sync_out_ep_desc);
	if(dev->sync_out_ep_desc2)
		skel_prepare_iso_urbs(dev,dev->iso_out_urb2,dev->sync_out_ep_desc2);
	//init_waitqueue_head(&dev->wait_q);

	//if(sysfs_create_group(&dev->udev->dev.kobj, &uart_group) != 0)
	//{
	//	printk("create %s sys-file err \n",uart_group.name);
	//}

	/* let the user know what node this device is now attached to */
	dev_info(&interface->dev,
		 "USB Skeleton device now attached to USBSkel-%d",
		 interface->minor);
	return 0;

error:
	if (dev)
		/* this frees allocated memory */
		kref_put(&dev->kref, skel_delete);
	return retval;
}

static void skel_disconnect(struct usb_interface *interface)
{
	struct usb_skel *dev;
	int minor = interface->minor;
	struct urb *urb = NULL;
	int i;

	dev = usb_get_intfdata(interface);

	for(i=0;i< URB_QUEUE_LEN;i++){
		urb = dev->bulk_out_urb[i];

		if(urb){
			usb_free_coherent(urb->dev, urb->transfer_buffer_length,
						urb->transfer_buffer, urb->transfer_dma);
			
			usb_free_urb(urb);
		}


		urb = dev->bulk_out_urb2[i];
		if(urb){
			usb_free_coherent(urb->dev, urb->transfer_buffer_length,
						urb->transfer_buffer, urb->transfer_dma);
			
			usb_free_urb(urb);
		}
		

		urb = dev->iso_out_urb[i];
		if(urb){
			usb_free_coherent(urb->dev, urb->transfer_buffer_length,
						urb->transfer_buffer, urb->transfer_dma);
			
			usb_free_urb(urb);

		}

		urb = dev->iso_out_urb2[i];
		if(urb){
			usb_free_coherent(urb->dev, urb->transfer_buffer_length,
						urb->transfer_buffer, urb->transfer_dma);
			
			usb_free_urb(urb);		
		}

	}



	usb_set_intfdata(interface, NULL);



	/* give back our minor */
	usb_deregister_dev(interface, &skel_class);

	/* prevent more I/O from starting */
	mutex_lock(&dev->io_mutex);
	dev->interface = NULL;
	mutex_unlock(&dev->io_mutex);

	usb_kill_anchored_urbs(&dev->submitted);

	/* decrement our usage count */
	kref_put(&dev->kref, skel_delete);

	dev_info(&interface->dev, "USB Skeleton #%d now disconnected", minor);
}

static void skel_draw_down(struct usb_skel *dev)
{
	int time;

	time = usb_wait_anchor_empty_timeout(&dev->submitted, 1000);
	if (!time)
		usb_kill_anchored_urbs(&dev->submitted);
	usb_kill_urb(dev->bulk_in_urb);
}

static int skel_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct usb_skel *dev = usb_get_intfdata(intf);

	if (!dev)
		return 0;
	skel_draw_down(dev);
	return 0;
}

static int skel_resume(struct usb_interface *intf)
{
	return 0;
}

static int skel_pre_reset(struct usb_interface *intf)
{
	struct usb_skel *dev = usb_get_intfdata(intf);

	mutex_lock(&dev->io_mutex);
	skel_draw_down(dev);

	return 0;
}

static int skel_post_reset(struct usb_interface *intf)
{
	struct usb_skel *dev = usb_get_intfdata(intf);

	/* we are sure no URBs are active - no locking needed */
	dev->errors = -EPIPE;
	mutex_unlock(&dev->io_mutex);

	return 0;
}

static struct usb_driver skel_driver = {
	.name =		"skeleton",
	.probe =	skel_probe,
	.disconnect =	skel_disconnect,
	.suspend =	skel_suspend,
	.resume =	skel_resume,
	.pre_reset =	skel_pre_reset,
	.post_reset =	skel_post_reset,
	.id_table =	skel_table,
	.supports_autosuspend = 0,
};

module_usb_driver(skel_driver);

MODULE_LICENSE("GPL");
