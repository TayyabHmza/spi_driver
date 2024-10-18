/*
    Filename: spi.h
    Author: Muhammad Tayyab
    Date: 2-10-2024
    Description: UETRV_PCore spi registers definations.
*/

#define SPI_BASEADDRESS 0x9000_0000

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
