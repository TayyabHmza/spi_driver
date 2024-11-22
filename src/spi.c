

// Todo: what happens when rx_index and tx_index reach MAX_BUFFER_SIZE

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/io.h>
#include <linux/log2.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/kdev_t.h>

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
#define MAX_NUM_CS 2
#define MSG_BUFFER_SIZE 		256

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
#define INTERRUPT_TX 					01
#define INTERRUPT_RX 					10
#define TX_FIFO_FULL					(1<<31)
#define RX_FIFO_EMPTY					(1<<31)
#define SPI_DATA						0x000000FF

// Definations
#define BASEADDRESS     		spi_device->base_address

#define NO_ERROR        		0
#define ERROR           		1

static int __init spi_init(void);
static void spi_exit(void);
static int spi_probe(struct platform_device *dev);
static int spi_remove(struct platform_device *dev);
static ssize_t driver_read (struct file *file_pointer, char __user *user_space_buffer, size_t count, loff_t *offset);
static ssize_t driver_write (struct file *file_pointer, const char *user_space_buffer, size_t count, loff_t *offset);
static void device_write(void);
static void device_read(void);
static long read_from_reg(void __iomem *address);
static void write_to_reg(void __iomem *address, unsigned long data);
static irqreturn_t spi_interrupt_handler(int irq, void* spi_device);

// Structures

struct spi_device_state {
    void __iomem *base_address;
	
	char data_tx_available;
	char data_rx_available;
	uint rx_index;
	uint tx_index;
	uint user_rx_index;
	uint user_tx_index;
	char rx_data_buffer[MSG_BUFFER_SIZE];
	char tx_data_buffer[MSG_BUFFER_SIZE];
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
	/*
		Called when driver is loaded into kernel.
	*/
	printk("SPI driver loaded.\n");
	platform_driver_register(&spi_driver);

  	return NO_ERROR;
}

static void spi_exit(void)
{
	/*
		Called when driver is unloaded from kernel.
	*/
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

	// Disable interrupts
	write_to_reg(BASEADDRESS+SPI_IE_R, 0);

	// Register device for interrupt
	int irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		printk("SPI device: unable to get irq number.\n");
		return ERROR;
	}
	//int ret = devm_request_irq(&pdev->dev, irq, spi_interrupt_handler, 0, dev_name(&pdev->dev), spi_device);
	int ret = request_irq(irq, spi_interrupt_handler, 0, dev_name(&pdev->dev), &pdev->dev);
	if (ret) {
		printk("SPI device: unable to register for interrupt.\n");
		return ERROR;
	}

	// Setup proc dirs
	static struct proc_dir_entry *spi_proc_node;
	spi_proc_node = proc_create("stz_spidriver", 0, NULL, &driver_proc_ops);
    if  (spi_proc_node == NULL) {
        printk ("SPI device: device file error.\n");
        return ERROR;
    }

	// Initialize flags
	spi_device->data_tx_available = 0;
	spi_device->data_rx_available = 0;
	spi_device->rx_index = 0;
	spi_device->tx_index = 0;
	spi_device->user_rx_index = 0;
	spi_device->user_tx_index = 0;
	write_to_reg(BASEADDRESS+SPI_CS_ID_R, 1);
	write_to_reg(BASEADDRESS+SPI_TX_MARK_R, 1);
	write_to_reg(BASEADDRESS+SPI_RX_MARK_R, 0);
	write_to_reg(BASEADDRESS+SPI_IE_R, INTERRUPT_RX);

    printk("SPI probe: Device connected.\n");
	return NO_ERROR;
}

static int spi_remove(struct platform_device *pdev)
{	
	/*
		Called when device is disconnected.
		Deletes device files.
	*/

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

static ssize_t driver_read (struct file *file_pointer,
                    	 	char __user *user_space_buffer,
                    	 	size_t count,
                    	 	loff_t *offset)
{
	/*
		Called when /proc/stz_spidriver file is read.
		Transfers data from rx_data_buffer to file.
	*/
	printk("SPI driver_read\n");

	char *msg_buffer = (spi_device->rx_data_buffer + spi_device->user_rx_index);
	size_t len = strlen(msg_buffer);
    if (*offset >= len){
        return 0;
    }
    *offset += len;
	spi_device->user_rx_index += len;

    int result = copy_to_user(user_space_buffer, msg_buffer, len);
    return len;
}

static ssize_t driver_write(struct file *file_pointer, 
						const char *user_space_buffer, 
						size_t count, 
						loff_t *offset)
{
	/*
		Called when /proc/stz_spidriver file is written.
		Transfers data from file to tx_data_buffer.
	*/
	printk("SPI driver_write\n");

	char *msg_buffer = (spi_device->tx_data_buffer + spi_device->user_tx_index);
	int result = copy_from_user(msg_buffer, user_space_buffer, count);

	size_t len = strlen(msg_buffer);
    if (*offset >= len){
        return 0;
    }
    *offset += len;
	spi_device->user_tx_index += len;

	// Mark data as available
	spi_device->data_tx_available = 1;
	// Enable tx interrupts
	// (If device is available to recieve more data, an interrupt will come and data will be sent.)
	write_to_reg(BASEADDRESS+SPI_IE_R, INTERRUPT_TX | INTERRUPT_RX);
    return count;
}

static void device_write(void)
{
	/*
		Writes data from tx_data_buffer into spi txdata fifo.
	*/
	printk("SPI device_write\n");

	uint *i = &(spi_device->tx_index);

	// Loop until fifo is full, or end of message.
	while ( !(read_from_reg(BASEADDRESS + SPI_TXDATA_R) & TX_FIFO_FULL) && (spi_device->tx_data_buffer[*i] != '\0') ) {

		// Write character to TXDATA register
		write_to_reg(BASEADDRESS + SPI_TXDATA_R, spi_device->tx_data_buffer[*i]);
		printk("\ntx_data_buffer: %c\n",spi_device->tx_data_buffer[*i]);
		(*i)++;
	}

	// If no more data available, unset data_tx_available
	if (spi_device->tx_data_buffer[*i] == '\0') {
		spi_device->data_tx_available = 0;
	}

	// Enable tx interrupts
	write_to_reg(BASEADDRESS+SPI_IE_R, INTERRUPT_TX | INTERRUPT_RX);
}

static void device_read(void) 
{
	/*
		Reads data from spi rxdata fifo into rx_data_buffer.
	*/
	printk("SPI device_read\n");

	uint *i = &(spi_device->rx_index);
	char empty_flag=0;
	ulong data;

	// Read data, loop until fifo is empty.
	data = read_from_reg(BASEADDRESS + SPI_RXDATA_R);
	empty_flag = (data & RX_FIFO_EMPTY) ? 1 : 0;
	while (!empty_flag) {
		// Read character from RXDATA register	
		spi_device->rx_data_buffer[*i] = (char) (data & SPI_DATA);
		printk("\nrx_data_buffer: %c\n",spi_device->rx_data_buffer[*i]);
		data = read_from_reg(BASEADDRESS + SPI_RXDATA_R);
		empty_flag = (data & RX_FIFO_EMPTY) ? 1 : 0;
		(*i)++;
	}

	// Enable rx interrupts
	//write_to_reg(BASEADDRESS+SPI_IE_R, INTERRUPT_RX);
}

static irqreturn_t spi_interrupt_handler(int irq, void* dev) 
{
	/*
		Interrupt handler.
		Calls device_read when data is available to be read.
		Calls device_write when device can accept more data, and there is more data to write.
	*/
	printk("SPI interrupt\n");

	// Get interrupt status
	ulong interrupts_status, interrupts_enables;
	interrupts_status = read_from_reg(BASEADDRESS+SPI_IP_R);
	interrupts_enables = read_from_reg(BASEADDRESS+SPI_IE_R);

	write_to_reg(BASEADDRESS+SPI_IE_R, 0);								// disable interrupts

	// Transmit actions
	if (interrupts_status & INTERRUPT_TX) {								// when device is available to transmit
		if (spi_device->data_tx_available) {							// if there is more data to send
			device_write();												// call write function
		}
	}

	// Recieve actions
	if (interrupts_status & INTERRUPT_RX) {								// when device has data available to read
		device_read();													// call read function
	}
	
	return IRQ_HANDLED;
}

// kernel interface

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SALMAN, TAYYAB, ZAWAHER");
MODULE_DEVICE_TABLE(of, matching_devices);
module_init(spi_init);
module_exit(spi_exit);

