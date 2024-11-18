
// SPI driver without interrupts

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/cdev.h>

// SPI register offsets
#define SPI_SCK_DIV_R   0x00 // Serial clock divisor
#define SPI_SCK_MODE_R  0x04 // Serial clock mode
#define SPI_CS_ID_R     0x10 // Chip select ID
#define SPI_CS_DEF_R    0x14 // Chip select default
#define SPI_CS_MODE_R   0x18 // Chip select mode
#define SPI_DELAY_0_R   0x28 // Delay control 0
#define SPI_DELAY_1_R   0x2C // Delay control 1
#define SPI_FMT_R       0x40 // Frame format
#define SPI_TXDATA_R    0x48 // Tx FIFO data
#define SPI_RXDATA_R    0x4C // Rx FIFO data
#define SPI_TX_MARK_R   0x50 // Tx FIFO watermark
#define SPI_RX_MARK_R   0x54 // Rx FIFO watermark
#define SPI_FCTRL_R     0x60 // SPI flash interface control
#define SPI_FFMT_R      0x64 // SPI flash instruction format
#define SPI_IE_R        0x70 // SPI interrupt enable
#define SPI_IP_R        0x74 // SPI interrupt pending

// Parameters
#define MSG_BUFFER_SIZE					256

// SPI register bit fields
#define CLK_POLARITY_HIGH 				1
#define CLK_POLARITY_LOW 				0
#define CLK_PHASE_SAMPLE_LEAD_EDGE 		0
#define CLK_PHASE_SAMPLE_TRAIL_EDGE 	1
#define CS_HIGH_INACTIVE_STATE 			0
#define CS_LOW_INACTIVE_STATE 			1
#define CS_MODE_AUTO 				    00
#define CS_MODE_HOLD 				    10
#define CS_MODE_OFF 				    11
#define PROTOCOL_SINGLE 				00
#define MSB_ENDIANNESS 					0
#define LSB_ENDIANNESS 					1
#define TX_FIFO_FULL					0x80000000
#define RX_FIFO_EMPTY					0x80000000
#define SPI_DATA						0x000000FF

// Definations
#define BASEADDRESS     				spi_device->base_address
#define EOT								'\0'		// end of transmission char
#define NO_ERROR        				0
#define ERROR           				1

static int __init spi_init(void);
static void spi_exit(void);
static int spi_probe(struct platform_device *pdev);
static int spi_remove(struct platform_device *pdev);
static int driver_open(struct inode *inode, struct file *file_ptr);
static int driver_close(struct inode *inode, struct file *file_ptr);
static ssize_t driver_read (struct file *file_pointer, char __user *user_space_buffer, size_t count, loff_t *offset);
static ssize_t driver_write (struct file *file_pointer, const char *user_space_buffer, size_t count, loff_t *offset);
static void device_write(void);
static void device_read(void);
static long read_from_reg(void __iomem *address);
static void write_to_reg(void __iomem *address, unsigned long data);

// Structures

static struct class *dev_class;

struct spi_device_state {
	dev_t major_no;
	struct cdev cdev;
    void __iomem *base_address;
	char rx_data_buffer[MSG_BUFFER_SIZE+1];
	char tx_data_buffer[MSG_BUFFER_SIZE+1];
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

struct file_operations driver_dev_ops = {
	.owner = THIS_MODULE,
	.open = driver_open,
    .read = driver_read,
    .write = driver_write,
	.release = driver_close
};

// Functions

static int __init spi_init(void)
{
	printk("SPI driver loaded.\n");
	platform_driver_register(&spi_driver);
  	return NO_ERROR;
}

static void spi_exit(void)
{
	platform_driver_unregister(&spi_driver);
	printk(KERN_INFO "SPI driver removed.\n");
}

static struct spi_device_state *spi_device;
static int spi_probe(struct platform_device *pdev)
{
	/*
		Called when device is registered with driver.
		Allocates resources for device, enables interrupts and initializes device.
	*/

	// Allocate dynamic memory struct to store device info.
	// This is freed automatically by kernel when device or driver is removed: no need to manually free.
	spi_device = devm_kzalloc(&pdev->dev, sizeof(struct spi_device_state), GFP_KERNEL);
	if (spi_device == NULL) {
		printk("SPI device: memory allocate error.\n");
		return ERROR;
	}

	// Store struct pointer with device
	platform_set_drvdata(pdev, spi_device);

	// Get base address of device
	spi_device->base_address =  devm_platform_ioremap_resource(pdev, 0);
	if (spi_device->base_address == NULL) {
		printk("SPI device: address error.\n");
		return ERROR;
	}

	// Setup proc dirs
	static struct proc_dir_entry *spi_proc_node;
	spi_proc_node = proc_create("spi", 0, NULL, &driver_proc_ops);
    if  (spi_proc_node == NULL) {
        printk ("SPI device: device file error.\n");
        return ERROR;
    }
	
	// Setup dev dirs
	alloc_chrdev_region(&spi_device->major_no, 0, 2, "spi");				// allocate device major num and two minor nums (0,1) for two CS lines
	dev_class = class_create(THIS_MODULE, "spi");							// create device file in /dev/class
	device_create(dev_class, NULL, spi_device->major_no, NULL, "spi0");		// create two files /dev/spi0 and /dev/spi1 for the two CS lines
	device_create(dev_class, NULL, MKDEV(MAJOR(spi_device->major_no), MINOR(spi_device->major_no) + 1), NULL, "spi1");

																	// initialize spi as a character device by setting struct cdev
	cdev_init(&(spi_device->cdev), &driver_dev_ops);				// register file operations
	spi_device->cdev.owner=THIS_MODULE;								// set this driver as owner of char device files
	cdev_add(&(spi_device->cdev), spi_device->major_no, 2);			// register cdev structure with kernel

	// Initialize registers
	write_to_reg(BASEADDRESS+SPI_CS_ID_R, 1);
	write_to_reg(BASEADDRESS+SPI_IE_R, 0);

	spi_device->rx_data_buffer[MSG_BUFFER_SIZE] = EOT;
	spi_device->tx_data_buffer[MSG_BUFFER_SIZE] = EOT;

    printk("SPI probe: Device connected at address: %x\n", BASEADDRESS);
	return NO_ERROR;
}

static int spi_remove(struct platform_device *pdev)
{	
	/*
		Called when device is disconnected.
		Deletes device files.
	*/
	device_destroy(dev_class, spi_device->major_no);
	class_destroy(dev_class);
	unregister_chrdev_region(spi_device->major_no, 2);
	cdev_del(&(spi_device->cdev));

    printk("SPI device removed.\n");
	return NO_ERROR;
}

static void write_to_reg(void __iomem *address,
						 unsigned long data)
{
	iowrite32(data, address);
}

static long read_from_reg(void __iomem *address)
{
	return ioread32(address);
}

static int driver_open(struct inode *inode, struct file *file_ptr) {
	/*
		Called when /dev files are accessed (opened)
		Selects one of the two CS lines according to the file:
			spi0: CS 0
			spi1: CS 1
	*/
	uint minor_no = iminor(inode);
	if (minor_no == 0) {
		write_to_reg(BASEADDRESS+SPI_CS_ID_R, 1);
	}
	else if (minor_no == 1) {
		write_to_reg(BASEADDRESS+SPI_CS_ID_R, 2);
	}
	return NO_ERROR;
}

static int driver_close(struct inode *inode, struct file *file_ptr) {
	/*
		Called when /dev files are accessed (closed)
	*/
	return 0;
}

static ssize_t driver_write(struct file *file_pointer, 
						const char *user_space_buffer, 
						size_t count, 
						loff_t *offset)
{
	/*
		Called when /proc/spi file is written.
		Transfers data from file to tx_data_buffer.
	*/
	// printk("SPI driver_write\n");

	if (count-1 > MSG_BUFFER_SIZE) {
		printk("Maximum data length allowed is %d.\n", MSG_BUFFER_SIZE);
		return 0;
	}

	int result = copy_from_user(spi_device->tx_data_buffer, user_space_buffer, count);
	spi_device->tx_data_buffer[count-1] = EOT;

	size_t len = strlen(spi_device->tx_data_buffer);
    if (*offset >= len){
        return 0;
    }
    *offset += len;

	// Write data to device
 	device_write();
    return count;
}

static void device_write(void)
{
	/*
		Writes data from tx_data_buffer into spi txdata fifo.
	*/
	// printk("SPI device_write\n");

	uint i = 0;

	// Loop until fifo is full, or end of message.
	while ( !(read_from_reg(BASEADDRESS + SPI_TXDATA_R) & TX_FIFO_FULL) ) {

		// Write character to TXDATA register
		write_to_reg(BASEADDRESS + SPI_TXDATA_R, spi_device->tx_data_buffer[i]);
		
		if (spi_device->tx_data_buffer[i] == EOT) {
			break ;
		}
		//printk("\ntx_data_buffer: %d\n", spi_device->tx_data_buffer[i]);
		i++;
	}
}

static ssize_t driver_read (struct file *file_pointer,
                    	 	char __user *user_space_buffer,
                    	 	size_t count,
                    	 	loff_t *offset)
{
	/*
		Called when /proc/spi file is read.
		Transfers data from rx_data_buffer to file.
	*/
	// printk("SPI driver_read\n");
	device_read();

	char *msg_buffer = spi_device->rx_data_buffer;
	size_t len = strlen(msg_buffer);
    if (*offset >= len){
        return 0;
    }
    *offset += len;

    int result = copy_to_user(user_space_buffer, msg_buffer, len);
    return len;
}

static void device_read(void) 
{
	/*
		Reads data from spi rxdata fifo into rx_data_buffer.
	*/
	// printk("SPI device_read\n");

	uint i = 0;
	char empty_flag=0;
	ulong data;

	// Read data, loop until fifo is empty.
	data = read_from_reg(BASEADDRESS + SPI_RXDATA_R);
	empty_flag = (data & RX_FIFO_EMPTY) ? 1 : 0;
	while (!empty_flag) {
		// Read character from RXDATA register	
		spi_device->rx_data_buffer[i] = (char) (data & SPI_DATA);
		//printk("\nrx_data_buffer: %c\n",spi_device->rx_data_buffer[i]);
		data = read_from_reg(BASEADDRESS + SPI_RXDATA_R);
		empty_flag = (data & RX_FIFO_EMPTY) ? 1 : 0;
		i++;
	}
}

// kernel interface

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SALMAN, TAYYAB, ZAWAHER");
MODULE_DEVICE_TABLE(of, matching_devices);
module_init(spi_init);
module_exit(spi_exit);

