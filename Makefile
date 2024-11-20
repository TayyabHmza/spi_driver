
# Object file of driver (kernel loadable module)
obj-m = src/spi.o  src/spi_nointerrupt.o

# Path of buildroot and linux src directories
LINUX_PATH = #ADD_PATH_HERE_TO_LINUX_FOLDER

line_number = $(shell grep -n "if SPI_MASTER" $(LINUX_PATH)/drivers/spi/Kconfig | cut -d: -f1 | head -n 1)

# Target to build kernel module
.PHONY: build
build: build/spi.ko build/spi_nointerrupt.ko
build/spi.ko build/spi_nointerrupt.ko: src/spi.c src/spi_nointerrupt.c $(LINUX_PATH)/scripts/module.lds
	make -C $(LINUX_PATH) M=$(PWD) ARCH=riscv CROSS_COMPILE=riscv32-unknown-linux-gnu- modules 
	@mkdir -p build
	@mv -t build .*.*.cmd src/.*.*.cmd *.order *.symvers src/*.mod src/*.mod.c src/*.o src/*.ko

# Target to install kernel modules in kernel
install: build/spi.ko build/spi_nointerrupt.ko
	cp src/spi.c $(LINUX_PATH)/drivers/spi/spi-stz-interrupt.c
	cp src/spi_nointerrupt.c $(LINUX_PATH)/drivers/spi/spi-stz-nointerrupt.c
	@if grep -q "spi-stz-interrupt.o" $(LINUX_PATH)/drivers/spi/Makefile; then \
		echo "SPI already configured on Linux."; \
	else \
		head -n "$(line_number)" $(LINUX_PATH)/drivers/spi/Kconfig > new_file.txt; \
		cat make_txt.txt >> new_file.txt; \
		tail -n +$$(($(line_number) + 1)) $(LINUX_PATH)/drivers/spi/Kconfig >> new_file.txt; \
		mv new_file.txt $(LINUX_PATH)/drivers/spi/Kconfig; \
		echo "Updated Kconfig file"; \
		echo "obj-\$$(CONFIG_SPI_STZ_INTERRUPT)         += spi-stz-interrupt.o" >> $(LINUX_PATH)/drivers/spi/Makefile; \
		echo "obj-\$$(CONFIG_SPI_STZ_NOINTERRUPT)       += spi-stz-nointerrupt.o" >> $(LINUX_PATH)/drivers/spi/Makefile; \
		echo "Updated SPI Driver's Makefile"; \
		echo "CONFIG_SPI_STZ_INTERRUPT=y" >> $(LINUX_PATH)/arch/riscv/configs/defconfig; \
		echo "CONFIG_SPI_STZ_NOINTERRUPT=y" >> $(LINUX_PATH)/arch/riscv/configs/defconfig; \
		echo "Created an entry in riscv defconfig."; \
	fi
	@echo ""
	@echo "Manually update the Linux config to install the SPI Driver. Go to Menuconfig > Device Drivers > SPI > SELECT ONE."

# Traget to create loader script in linux dir
$(LINUX_PATH)/scripts/module.lds: 
	cp $(LINUX_PATH)/scripts/module.lds.S   $(LINUX_PATH)/scripts/module.lds
	echo "SECTIONS {.plt : { BYTE(0) } 
        .got : { BYTE(0) } 
        .got.plt : { BYTE(0) } 
	}" >> $(LINUX_PATH)/scripts/module.lds

# Target to clean
clean:
	rm -rf build
	make -C $(LINUX_PATH) M=$(PWD) clean
