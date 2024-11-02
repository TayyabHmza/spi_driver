#include <linux/clk.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/io.h>
#include <linux/log2.h>
#include <linux/proc_fs.h>

// Definations
#define NO_ERROR        0
#define ERROR           1

char pseudo_device[8];

static int __init spi_init(void);
static void __exit spi_exit(void);
static int spi_probe(struct platform_device *dev);
static int spi_remove(struct platform_device *dev);
static ssize_t driver_read (struct file *file_pointer, char __user *user_space_buffer, size_t count, loff_t *offset);
static ssize_t driver_write (struct file *file_pointer, const char *user_space_buffer, size_t count, loff_t *offset);

// Structures

struct spi_device_state {
    void __iomem *base_address;
};

static const struct of_device_id matching_devices[] = {
	{ .compatible = "sifive,spi0", },
	{}
};

static struct platform_driver spi_driver = {
	.probe = spi_probe,
	.remove = spi_remove,
	.driver = {
		.name = "Salman-Tayyab-Zawaher's_driver",
		.of_match_table = matching_devices
	},
};

struct proc_ops driver_proc_ops = {
    .proc_read = driver_read,
    .proc_write = driver_write
};

// Functions

static int __init spi_init(void)
{
	printk("Hello, world!\n");

	static struct proc_dir_entry *spi_proc_node;
	spi_proc_node = proc_create("stz_spidriver", 0, NULL, &driver_proc_ops);
    if  (spi_proc_node == NULL){
        printk ("module_init : error\n");
        return ERROR;
    }

  	return NO_ERROR;
}

static void __exit spi_exit(void)
{
	printk(KERN_INFO "Goodbye, world!\n");
}


static int spi_probe(struct platform_device *dev)
{
    struct spi_device_state spi_device_state;
    
    spi_device_state.base_address =  devm_platform_ioremap_resource(dev, 0);
    
    printk("Device connected at address: %x\n",spi_device_state.base_address);

	return NO_ERROR;
}

static int spi_remove(struct platform_device *dev)
{
    printk("Device removed.\n");
	return NO_ERROR;
}

static char msg[20] = "No data recieved!\n";

static ssize_t driver_read (struct file *file_pointer,
                    	 	char __user *user_space_buffer,
                    	 	size_t count,
                    	 	loff_t *offset)
{
	printk ("Reading spi: \n");

    size_t len = strlen(msg);
    if (*offset >= len){
        return 0;
    }

    int result = copy_to_user(user_space_buffer, msg, len);
    *offset += len;
    
    return len;
}

static ssize_t driver_write(struct file *file_pointer, 
						const char *user_space_buffer, 
						size_t count, 
						loff_t *offset)
{
	printk ("Writing to spi: ");
    int result = copy_from_user(msg, user_space_buffer, count);
	msg[count] = '\0';
	printk("%s\n", msg);
	
	size_t len = strlen(msg);
    if (*offset >= len){
        return 0;
    }
    *offset += len;

    return len;
}

// kernel interface

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TAYYAB, ZAWAHER");
MODULE_DEVICE_TABLE(of, matching_devices);
module_init(spi_init);
module_exit(spi_exit);


