#include <linux/clk.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/io.h>
#include <linux/log2.h>

// Definations
MODULE_LICENSE("GPL");
#define NO_ERROR        0
#define ERROR           1

static int __init spi_init(void);
static void __exit spi_exit(void);
static int spi_probe(struct platform_device *dev);
static int spi_remove(struct platform_device *dev);

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

// Functions

static int __init spi_init(void)
{
	printk("Hello, world!\n");
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

// Register functions / structures with kernel
MODULE_DEVICE_TABLE(of, matching_devices);
//module_init(spi_init);
//module_exit(spi_exit);

