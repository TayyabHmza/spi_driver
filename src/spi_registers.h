/*
    Filename: spi_registers.h
    Author: Muhammad Tayyab
    Date: 2-10-2024
    Description: UETRV_PCore spi registers definations.
*/

#define SPI_BASEADDRESS 0x9000_0000
#define SPI_SCK_DIV_R   ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x00) ) ) // Serial clock divisor
#define SPI_SCK_MODE_R  ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x04) ) ) // Serial clock mode
#define SPI_CS_ID_R     ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x10) ) ) // Chip select ID
#define SPI_CS_DEF_R    ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x14) ) ) // Chip select default
#define SPI_CS_MODE_R   ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x18) ) ) // Chip select mode
#define SPI_DELAY_0_R   ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x28) ) ) // Delay control 0
#define SPI_DELAY_1_R   ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x2C) ) ) // Delay control 1
#define SPI_FMT_R       ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x40) ) ) // Frame format
#define SPI_TXDATA_R    ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x48) ) ) // Tx FIFO data
#define SPI_RXDATA_R    ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x4C) ) ) // Rx FIFO data
#define SPI_TX_MARK_R   ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x50) ) ) // Tx FIFO watermark
#define SPI_RX_MARK_R   ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x54) ) ) // Rx FIFO watermark
#define SPI_FCTRL_R     ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x60) ) ) // SPI flash interface control
#define SPI_FFMT_R      ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x64) ) ) // SPI flash instruction format
#define SPI_IE_R        ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x70) ) ) // SPI interrupt enable
#define SPI_IP_R        ( *((volatile unsigned long *) (SPI_BASEADDRESS + 0x74) ) ) // SPI interrupt pending
