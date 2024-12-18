# Sifive compatible Linux SPI driver
Designed and tested for Linux v6.1.

See SiFive datasheet for information about hardware:\
https://static.dev.sifive.com/SiFive-FE310-G000-manual-v2p0.pdf#chapter.18

There are two versions of the driver:\
spi_nointerrupt.c: works without interrupts, current version.\
spi.c: works with interrupts, to be updated.

### Build
- Set path of Linux kernel source top directory in Makefile

**Build with Linux kernel**
- Run `make install` to install the driver in kernel directories.
- Update the Linux config to build the spi driver with kernel.
  Go to menuconfig > Device Drivers > SPI > SELECT ONE.
- The kernel will now include the driver when it is built.

**Build loadable kernel module separately**
- Run `make build` to generate build/spi.ko build/spi_nointerrupt.ko
- Use `insmod` to load the driver into kernel.

### Send/recieve data
The driver supports two CS lines (two slaves). It creates two device files: `/dev/spi0` and `/dev/spi1` corresponding to the two CS lines.

To send/recieve data over spi, write/read data to one of the device files.

256 bytes (chars) of data can be sent at a time; this is set by `MSG_BUFFER_SIZE` in driver source. Attempting to send more data than this generates an error.

When reading device file, driver returns only the data present in the hardware fifo. Any data recieved after fifo is full is lost.

The driver also creates `/proc/stz_spidriver` file, which works like `/dev/spi0`.

## Documentation 
The description of the functions and the structures written the spi_diver code is given below:\
[SPI_DRIVER](https://github.com/TayyabHmza/spi_driver/blob/main/docs/SPI_Driver.pdf)
