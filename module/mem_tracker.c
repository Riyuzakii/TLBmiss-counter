#include<linux/module.h>
#include<linux/kernel.h>
#include "mem_tracker.h"

static int major;
atomic_t  device_opened;
static struct class *memtrack_class;
struct device *memtrack_device;


static int memtrack_open(struct inode *inode, struct file *file)
{
        atomic_inc(&device_opened);
        try_module_get(THIS_MODULE);
        printk(KERN_INFO "Device opened successfully\n");
        return handle_open();
}

static int memtrack_release(struct inode *inode, struct file *file)
{
        atomic_dec(&device_opened);
        module_put(THIS_MODULE);
        printk(KERN_INFO "Device closed successfully\n");

        return handle_close();
}

static ssize_t memtrack_read(struct file *filp,
                           char *buffer,
                           size_t length,
                           loff_t * offset)
{
        return handle_read(buffer, length);
}

static ssize_t
memtrack_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
            return handle_write(buff, len);
}

static struct file_operations fops = {
        .read = memtrack_read,
        .write = memtrack_write,
        .open = memtrack_open,
        .release = memtrack_release,
};

static char *memtrack_devnode(struct device *dev, umode_t *mode)
{
        if (mode && dev->devt == MKDEV(major, 0))
                *mode = 0666;
        return NULL;
}

int init_module(void)
{
        int err;
	printk(KERN_INFO "Hello kernel\n");
            
        major = register_chrdev(0, DEVNAME, &fops);
        err = major;
        if (err < 0) {      
             printk(KERN_ALERT "Registering char device failed with %d\n", major);   
             goto error_regdev;
        }                 
        
        memtrack_class = class_create(THIS_MODULE, DEVNAME);
        err = PTR_ERR(memtrack_class);
        if (IS_ERR(memtrack_class))
                goto error_class;

        memtrack_class->devnode = memtrack_devnode;

        memtrack_device = device_create(memtrack_class, NULL,
                                        MKDEV(major, 0),
                                        NULL, DEVNAME);
        err = PTR_ERR(memtrack_device);
        if (IS_ERR(memtrack_device))
                goto error_device;

        /*sysfs creation*/
         err = sysfs_create_group (kernel_kobj, &memtrack_attr_group);
         if(unlikely(err)){
                printk(KERN_INFO "memtracker: can't create sysfs\n");
                goto error_device;
         }
 
        printk(KERN_INFO "I was assigned major number %d. To talk to\n", major);                                                              
        atomic_set(&device_opened, 0);
       

	return 0;

error_device:
         class_destroy(memtrack_class);
error_class:
        unregister_chrdev(major, DEVNAME);
error_regdev:
        return  err;
}

void cleanup_module(void)
{
        device_destroy(memtrack_class, MKDEV(major, 0));
        class_destroy(memtrack_class);
        unregister_chrdev(major, DEVNAME);
        sysfs_remove_group (kernel_kobj, &memtrack_attr_group);
	printk(KERN_INFO "Goodbye kernel\n");
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Memory tracking module");
MODULE_AUTHOR("cs730");
