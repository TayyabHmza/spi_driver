#include <linux/clk.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/io.h>
#include <linux/log2.h>

MODULE_LICENSE("GPL");

struct spi_device_state {
    void __iomem base_address;
};

static int spi_probe(struct platform_device *dev)
{
    struct spi_device_state *spi_device_state;
    
    spi_device_state->base_address =  devm_platform_ioremap_resource(dev, 0);
    
    printk("Device connected at address: %x\n",spi_device_state->base_address);
}

static int spi_remove(struct platform_device *dev)
{
    printk("Device removed.\n");
}

static const struct of_device_id matching_devices[] = {
	{ .compatible = "sifive,spi0", },
	{}
};
MODULE_DEVICE_TABLE(of, matching_devices);

static struct platform_driver spi_driver = {
	.probe = spi_probe,
	.remove = spi_remove,
	.driver = {
		.name = "Salman-Tayyab-Zawaher's_driver",
		.of_match_table = matching_devices
	},
};
module_platform_driver(spi_driver);