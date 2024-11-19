# Sifive compatible Linux SPI driver

See SiFive datasheet for information about the spi module:\
https://static.dev.sifive.com/SiFive-FE310-G000-manual-v2p0.pdf#chapter.18

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

256 bytes (chars) of data can be sent at a time; this is set by `MSG_BUFFER_SIZE` in driver source. Attempting to send more data than this results in an error.

When reading device file, driver returns only the data present in the hardware fifo. Any data recieved after fifo is full is lost.
## Documentation 
The description of the functions and the structures written the spi_diver code is given below:\
[SPI_DRIVER](./spi_driver/docs/SPI_Driver.pdf)
