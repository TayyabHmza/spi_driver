#include <linux/clk.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/io.h>
#include <linux/log2.h>
#include <linux/proc_fs.h>

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

// SPI register bit field values
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
#define INTERRUPT_TX 					0
#define INTERRUPT_RX 					1

// Definations
#define BASEADDRESS     		pseudodevice
#define MSG_BUFFER_SIZE 		256

#define NO_ERROR        		0
#define ERROR           		1

static int __init spi_init(void);
static void __exit spi_exit(void);
static int spi_probe(struct platform_device *dev);
static int spi_remove(struct platform_device *dev);
static ssize_t driver_read (struct file *file_pointer, char __user *user_space_buffer, size_t count, loff_t *offset);
static ssize_t driver_write (struct file *file_pointer, const char *user_space_buffer, size_t count, loff_t *offset);
static long read_from_reg(void __iomem *address);
static void write_to_reg(void __iomem *address, unsigned long data);

// Structures

struct spi_device_state {
    void __iomem *base_address;
	ulong sck_div;
	ulong cs_id;
	char clk_polarity;
	char clk_phase;
	char cs_inactive_state;
	char cs_mode;
	short cs_sck_delay;
	short sck_cs_delay;
	short inter_cs_time;
	short inter_frame_time;
	char protocol;
	char endianness;
	char dir;
	char len;
	char tx_mark;
	char rx_mark;
	char rx_interrupt_en;
	char tx_interrupt_en;
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

// Pseudodevice ================================================

// For testing, instead of writing to spi device registers, we can write to memory addresses.
// 15 array elements for 15 registers. For offset of 4, 16*4.
static long pseudodevice[16 * 4];

// The data will be sent/recieved to/from this buffer instead of spi device. 
static char pseudodevice_buffer[256];

// Emaulate working of spi device.
static int pseudo_wi = 0;
static int pseudo_ri = 0;

void pseudodevice_write_emulate(void) 
{	
	pseudodevice_buffer[pseudo_wi % 256] = pseudodevice[SPI_TXDATA_R];
	pseudo_wi++;
}
void pseudodevice_read_emulate(void) 
{	
	pseudodevice[SPI_RXDATA_R] = pseudodevice_buffer[pseudo_ri % 256];
	if (pseudodevice[SPI_RXDATA_R] != '\0') pseudo_ri++;
}
// ===========================================================

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
	spi_probe(NULL); // Pseudodevice is connected.

  	return NO_ERROR;
}

static void __exit spi_exit(void)
{
	printk(KERN_INFO "Goodbye, world!\n");
}

static int spi_probe(struct platform_device *dev)
{
    //struct spi_device_state *spi_device;    
    //spi_device.base_address =  devm_platform_ioremap_resource(dev, 0);
    printk("Device connected at address: %x\n", BASEADDRESS);
	return NO_ERROR;
}

static int spi_remove(struct platform_device *dev)
{
    printk("Device removed.\n");
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
	char msg_buffer[MSG_BUFFER_SIZE];
	int i=0;
    int result;
	
	do {
		pseudodevice_read_emulate();
		msg_buffer[i] = (char) read_from_reg(BASEADDRESS + SPI_RXDATA_R);
	} while (msg_buffer[i++] != '\0' && i < (MSG_BUFFER_SIZE-1));

	size_t len = strlen(msg_buffer);
    if (*offset >= len){
        return 0;
    }
    *offset += len;

    result = copy_to_user(user_space_buffer, msg_buffer, len);
    return len;
}

static ssize_t driver_write(struct file *file_pointer, 
						const char *user_space_buffer, 
						size_t count, 
						loff_t *offset)
{
	printk("Writing to spi: \n");

	char msg_buffer[MSG_BUFFER_SIZE];
	int i=0;
    int result;

	result = copy_from_user(msg_buffer, user_space_buffer, count);
	
	while (msg_buffer[i] != '\0' && i < MSG_BUFFER_SIZE) {
		write_to_reg(BASEADDRESS + SPI_TXDATA_R, msg_buffer[i]);
		pseudodevice_write_emulate();
		i++;
	}

	size_t len = strlen(msg_buffer);
    if (*offset >= len){
        return 0;
    }
    *offset += len;

    return count;
}

// kernel interface

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SALMAN, TAYYAB, ZAWAHER");
MODULE_DEVICE_TABLE(of, matching_devices);
module_init(spi_init);
module_exit(spi_exit);

