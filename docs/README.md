# Sifive compatible Linux SPI driver

### Build
- Set path of Linux kernel source top directory in Makefile
- Inside scripts directory in Linux source directory, create a copy of module.lds.S and then rename it to module.lds. Open it and paste the following at its end
```
SECTIONS {
        .plt : { BYTE(0) }
        .got : { BYTE(0) }
        .got.plt : { BYTE(0) }
}
```
- Run `make` to build driver (build/spi.ko).
