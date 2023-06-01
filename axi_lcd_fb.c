#include <linux/pci.h>
#include <linux/module.h> 
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/timex.h>
#include <linux/timer.h>
#include <linux/fb.h>




#define debug_log printk
#define DAM_BUFF_SIZE (5*1024*1024)

#define BITS_PER_PIX 32
#define IMAGE_WIDTH 800 
#define IMAGE_HEIGHT 480


#define DMA_REG_START_OFF 0x4
#define DMA_REG_IRQ_STAT_OFF 0x10
#define DMA_REG_CLEAR_IRQ_OFF 0x18
#define DMA_REG_MASK_IRQ_OFF 0x14
#define DMA_REG_SRC_OFF 0x68
#define DMA_REG_DEST_OFF 0x6c
#define DMA_REG_LEN_OFF 0x64
#define DMA_REG_CFG_OFF 0x60



#define DMA_STATUS_RUN 1
#define DMA_STATUS_IDLE 0

struct lcd_dev{
	int dma_irq;
	void *frame_buffer_cpu;
	resource_size_t frame_buffer_phy;
	resource_size_t fifo_addr;
	void *dma_base;
	struct fb_info *fb_info;
	u32 frame_size;
	int status;
	spinlock_t	lock;

};

void dma_init(struct *lcd_dev)
{
	void *dma_base = lcd_dev->dma_base;


	// mask IRQ
	iowrite32(0x0,dma_base+DMA_REG_MASK_IRQ_OFF);	
	// clear IRQ
	iowrite32(0xF,dma_base+DMA_REG_CLEAR_IRQ_OFF);	
	// source addr
	iowrite32((u32)lcd_dev->frame_buffer_phy,dma_base+DMA_REG_SRC_OFF);	
	// dest addr
	iowrite32((u32)lcd_dev->fifo_addr,dma_base+DMA_REG_DEST_OFF);	
	// len
	
	iowrite32(lcd_dev->frame_size,dma_base+DMA_REG_LEN_OFF);	
	// config
	iowrite32(0x0000E405,dma_base+DMA_REG_CFG_OFF);	


}


static irqreturn_t dma_irq_hanlder(int irqno, void *dev_id)
{
	struct *lcd_dev = dev_id;
	u32 reg;
	static int flag = 0;

	if(flag == 0){
		debug_log("dma irq\n");
		flag = 1;
	}

	if(ioread32(lcd_dev->dma_base+DMA_REG_IRQ_STAT_OFF) & 0x1){
		dma_init(lcd_dev);
		// enable IRQ
		iowrite32(0x1,lcd_dev->dma_base+DMA_REG_MASK_IRQ_OFF);
		// start
		iowrite32(0x1,lcd_dev->dma_base+DMA_REG_START_OFF);
	}
	
	
	return IRQ_HANDLED;
}



static struct fb_var_screeninfo axi_lcd_fb_default = {
	.activate = FB_ACTIVATE_NOW,
	.height = -1,
	.width = -1,
	.vmode = FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo axi_lcd_fb_fix = {
	.id = "axi_lcd_fb",
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_TRUECOLOR, 
	.accel = FB_ACCEL_NONE,
};

static int fb_open(struct fb_info *info, int user)
{
	struct *lcd_dev = info->par;
	debug_log("fb_open open\n");


	spin_lock(&lcd_dev->lock);
	if(lcd_dev->status != DMA_STATUS_RUN){

		debug_log("start DMA\n");
		
		dma_init(lcd_dev);
		
		// enable IRQ
		iowrite32(0x1,lcd_dev->dma_base+DMA_REG_MASK_IRQ_OFF);
		// start
		iowrite32(0x1,lcd_dev->dma_base+DMA_REG_START_OFF);

		lcd_dev->status = DMA_STATUS_RUN;

	}

	spin_unlock(&lcd_dev->lock);
	
	return 0;
}
static int fb_release(struct fb_info *info, int user)
{
	struct *lcd_dev = info->par;

	
	debug_log("pciefb close\n");

	//iowrite32(0x0,lcd_dev->dma_base+DMA_REG_MASK_IRQ_OFF);
	//iowrite32(0xF,lcd_dev->dma_base+DMA_REG_CLEAR_IRQ_OFF);	
	
	return 0;
}


static struct fb_ops axi_lcd_fb_ops = {
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.owner		= THIS_MODULE,
	.fb_open	= fb_open,
	.fb_release = fb_release,
};

struct fb_info * fb_init(struct device *dev, void *fbmem_virt,unsigned long phy_start)
{

	struct fb_info *info;

	int retval = -ENOMEM;

	info = framebuffer_alloc(sizeof(u32) * 256, dev);
	if (!info)
		goto error0;


	info->fbops = &axi_lcd_fb_ops;
	info->var = axi_lcd_fb_default;
	info->fix = axi_lcd_fb_fix;

	info->var.bits_per_pixel = BITS_PER_PIX;

	info->var.xres = IMAGE_WIDTH;
	info->var.yres = IMAGE_HEIGHT;


	info->var.xres_virtual = info->var.xres,
	info->var.yres_virtual = info->var.yres;


	if(info->var.bits_per_pixel == 16) {
		info->var.red.offset = 11;
		info->var.red.length = 5;
		info->var.red.msb_right = 0;
		info->var.green.offset = 5;
		info->var.green.length = 6;
		info->var.green.msb_right = 0;
		info->var.blue.offset = 0;
		info->var.blue.length = 5;
		info->var.blue.msb_right = 0;
	} else {
	// 32 bit
		info->var.red.offset = 16;
		info->var.red.length = 8;
		info->var.red.msb_right = 0;
		info->var.green.offset = 8;
		info->var.green.length = 8;
		info->var.green.msb_right = 0;
		info->var.blue.offset = 0;
		info->var.blue.length = 8;
		info->var.blue.msb_right = 0;
	
	}

	info->fix.line_length = (info->var.xres * (info->var.bits_per_pixel >> 3));
	info->fix.smem_len = info->fix.line_length * info->var.yres;

	info->fix.smem_start = phy_start;
	info->screen_base = fbmem_virt;
	info->pseudo_palette = info->par;
	info->par = NULL;
	info->flags = FBINFO_FLAG_DEFAULT;


	retval = fb_alloc_cmap(&info->cmap, 256, 0);
	if (retval < 0){
		dev_err(dev, "unable to allocate  fb cmap\n");
		goto error1;
	}


	retval = register_framebuffer(info);
	if (retval < 0){
		dev_err(dev, "unable to register framebuffer\n");
		goto error2;
	}

	dev_info(dev, "fb%d: %s frame buffer device at 0x%x+0x%x\n",
		info->node, info->fix.id, (unsigned)info->fix.smem_start,
		info->fix.smem_len);

	
	return info;


	// unregister_framebuffer(dev->fb_info);
error2:
	fb_dealloc_cmap(&info->cmap);
error1:
	framebuffer_release(info);

error0:
	return NULL;
}


void release_fb(struct fb_info *info)
{

	unregister_framebuffer(info);
	fb_dealloc_cmap(&info->cmap);
	framebuffer_release(info);
}


static int lcd_probe(struct platform_device *pdev)
{
	
	
	struct lcd_dev *lcd_dev;
	struct resource *mem,*dma,*fifo;
	void *frame_buffer,*dma_base;
	int irq,ret;

	printk("test lcd_probe probe\n");

	lcd_dev = devm_kzalloc(&pdev->dev, sizeof(*lcd_dev), GFP_KERNEL);
	if (!lcd_dev)
		return -ENOMEM;

	dma = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dma_reg");
	if(!dma){
		dev_err(&pdev->dev, "no DMA addr found\n");
		return -ENODEV;
	}
	dma_base = devm_ioremap_resource(&pdev->dev, dma);
	if (IS_ERR(dma_base))
		return PTR_ERR(dma_base);
	

	irq = platform_get_irq(pdev, 0);
	if (irq < 0){
		dev_err(&pdev->dev, "get irq faild\n");
		return irq;
	}
	
	mem = platform_get_resource_byname(pdev, IORESOURCE_MEM, "memory");
	if(!mem)
		dev_err(&pdev->dev, "no memory reg found\n");

	
	
	frame_buffer = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(frame_buffer))
		return PTR_ERR(frame_buffer);


	fifo = platform_get_resource_byname(pdev, IORESOURCE_MEM, "fifo");
	if(!fifo){
		dev_err(&pdev->dev, "no fifo addr found\n");
		return -ENODEV;
	}
	
	lcd_dev->frame_buffer_cpu = frame_buffer;
	lcd_dev->frame_buffer_phy = mem->start;
	lcd_dev->dma_base = dma_base;
	lcd_dev->fifo_addr = fifo->start;
	lcd_dev->dma_irq = irq;
	
	
/*
	if (dma_set_mask_and_coherent(&pdev->dev,
				      DMA_BIT_MASK(64))) {
		if (dma_set_mask_and_coherent(&pdev->dev,
					      DMA_BIT_MASK(32))) {
			dev_warn(&pdev->dev,
				 "No suitable DMA available\n");
			ret = -ENOMEM;
			goto disable_device;
		}
	}
	
	cpu_ptr = dma_alloc_coherent(&pdev->dev,DAM_BUFF_SIZE, &dma_ptr,
				     GFP_KERNEL);
	if (!cpu_ptr) {
	
		dev_warn(&pdev->dev,"dma alloc failed\n");
		ret = -ENOMEM;
		goto dma_err;
	}
	
*/
	memset(frame_buffer,0,DAM_BUFF_SIZE);

	lcd_dev->fb_info = fb_init(&pdev->dev,frame_buffer,mem->start);
	lcd_dev->fb_info->par = lcd_dev;
	lcd_dev->frame_size = lcd_dev->fb_info->var.xres * lcd_dev->fb_info->var.xres * (BITS_PER_PIX / 8) ;
	lcd_dev->status = DMA_STATUS_IDLE;
	
	ret = devm_request_irq(irq, dma_irq_hanlder, 0, "axi-lcd", lcd_dev);
	if (ret < 0) {
		pr_err("fail to claim dam irq , ret = %d\n", ret);
	}

	spin_lock_init(&lcd_dev->lock);

	platform_set_drvdata(pdev, lcd_dev);

	
	return ret;
	
}				
	



static int lcd_remove(struct platform_device *pdev)
{
	struct lcd_dev *lcd_dev;

	lcd_dev = platform_get_drvdata(pdev);
	release_fb(lcd_dev->fb_info);
	
	
	return 0;
}




static const struct of_device_id axi_lcd_of_match[] = {
	{ .compatible = "mx,axi-lcd", },
	{},
};
MODULE_DEVICE_TABLE(of, axi_lcd_of_match);

static struct platform_driver mx_lcd_driver = {
	.driver = {
		.name   = "axi-lcd",
		.of_match_table = axi_lcd_of_match,
	},
	.probe	  = lcd_probe,
	.remove	 = lcd_remove,
};

module_platform_driver(mx_lcd_driver);


MODULE_DESCRIPTION("AXI LCD driver");
MODULE_AUTHOR("Murphy xu");
MODULE_LICENSE("GPL v2");