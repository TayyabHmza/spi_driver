// For testing code on local pc
// Compile and run using gcc

#include <stdio.h>
#include <string.h>

#define ssize_t int
#define __user unsigned
#define __iomem long
#define loff_t int
#define ulong long 
#define uint int
#define copy_to_user(a,b,c) strcpy(a,b)
#define copy_from_user(a,b,c) strcpy(a,b)
#define printk printf
#define IRQ_HANDLED 0
typedef int irq_int;

static long pseudodevice[16];
static char pseudodevice_buffer[256];

// =============================

// SPI register offsets
#define SPI_SCK_DIV_R  0 // 0x00 // Serial clock divisor
#define SPI_SCK_MODE_R 1 // 0x04 // Serial clock mode
#define SPI_CS_ID_R    2 // 0x10 // Chip select ID
#define SPI_CS_DEF_R   3 // 0x14 // Chip select default
#define SPI_CS_MODE_R  4 // 0x18 // Chip select mode
#define SPI_DELAY_0_R  5 // 0x28 // Delay control 0
#define SPI_DELAY_1_R  6 // 0x2C // Delay control 1
#define SPI_FMT_R      7 // 0x40 // Frame format
#define SPI_TXDATA_R   8 // 0x48 // Tx FIFO data
#define SPI_RXDATA_R   9 // 0x4C // Rx FIFO data
#define SPI_TX_MARK_R  10 // 0x50 // Tx FIFO watermark
#define SPI_RX_MARK_R  11 // 0x54 // Rx FIFO watermark
#define SPI_FCTRL_R    12 // 0x60 // SPI flash interface control
#define SPI_FFMT_R     13 // 0x64 // SPI flash instruction format
#define SPI_IE_R       14 // 0x70 // SPI interrupt enable
#define SPI_IP_R       15 // 0x74 // SPI interrupt pending

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

struct file {int none;};

static int spi_interrupt_handler(int irq, void* dev);
static void device_write(void);

struct spi_device_state {
    __iomem *base_address;

	unsigned long sck_div;
	short cs_sck_delay;
	short sck_cs_delay;
	short inter_cs_time;
	short inter_frame_time;
	char clk_polarity;
	char clk_phase;
	char cs_inactive_state;
	char cs_mode;
	char protocol;
	char endianness;
	char direction;
	char length;
	char tx_watermark;
	char rx_watermark;
	
	char data_tx_available;
	unsigned int rx_index;
	unsigned int tx_index;
	unsigned int user_rx_index;
	unsigned int user_tx_index;
	char rx_data_buffer[MSG_BUFFER_SIZE];
	char tx_data_buffer[MSG_BUFFER_SIZE];
};

static struct spi_device_state spi_device_struct = {.base_address = pseudodevice};
static struct spi_device_state *spi_device = &spi_device_struct;

static void write_to_reg(__iomem *address,
						 unsigned long data)
{
	//iowrite32(data, address);
    *address = data;
}

static long read_from_reg(__iomem *address)
{
	//return ioread32(address);
    return *address;
}

static void device_read(void) 
{
	/*
		Reads data from spi rxdata fifo into rx_data_buffer.
	*/
	printk("SPI device_read\n");

	uint *i = &(spi_device->rx_index);

	// Loop until fifo is empty.
	do {
		// Read character from RXDATA register
		spi_device->rx_data_buffer[*i] = (char) read_from_reg(BASEADDRESS + SPI_RXDATA_R);

	} while (read_from_reg(BASEADDRESS + SPI_RXDATA_R) & RX_FIFO_EMPTY);

	// Enable rx interrupts
	ulong interrupts_enables = read_from_reg(BASEADDRESS+SPI_IE_R);
	write_to_reg(BASEADDRESS+SPI_IE_R, INTERRUPT_RX | interrupts_enables);
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
	unsigned long interrupts_enables = read_from_reg(BASEADDRESS+SPI_IE_R);
	write_to_reg(BASEADDRESS+SPI_IE_R, INTERRUPT_TX | interrupts_enables);
    
	// raise interrupt
	write_to_reg(BASEADDRESS+SPI_IP_R, INTERRUPT_TX);
    spi_interrupt_handler(0, 0);

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
		(*i)++;
	}

	// If no more data available, unset data_tx_available
	if (spi_device->tx_data_buffer[*i] == '\0') {
		spi_device->data_tx_available = 0;
	}

	// Enable tx interrupts
	ulong interrupts_enables = read_from_reg(BASEADDRESS+SPI_IE_R);
	write_to_reg(BASEADDRESS+SPI_IE_R, INTERRUPT_TX | interrupts_enables);
}

static int spi_interrupt_handler(int irq, void* dev) 
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

	// Transmit actions
	if (interrupts_status & INTERRUPT_TX) {												// when device is available to transmit
		write_to_reg(BASEADDRESS+SPI_IE_R, interrupts_enables & ~INTERRUPT_TX);			// disable tx interrupts
		if (spi_device->data_tx_available) {											// if there is more data to send
			device_write();																// call write function
		}
	}

	// Recieve actions
	if (interrupts_status & INTERRUPT_RX) {												// when device has data available to read
		write_to_reg(BASEADDRESS+SPI_IE_R, interrupts_enables & ~INTERRUPT_RX);			// disable rx interrupts
		device_read();																	// call read function
	}

	return IRQ_HANDLED;
}

int main() {
    // Write C code here
    printf("Start\n");
    char user_space_buffer[256];
    loff_t offset;

    driver_write(NULL, "abc", 0, &offset);
    
    printf("\n\n");
    return 0;
}