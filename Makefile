
# Object file of driver (kernel loadable module)
obj-m = src/spi.o  src/spi_nointerrupt.o

# Path of linux src directory
LINUX_PATH = /root/Build_linux/linux-6.1

build/spi.ko build/spi_nointerrupt.ko: src/spi.c src/spi_nointerrupt.c $(LINUX_PATH)/scripts/module.lds
	make -C $(LINUX_PATH) M=$(PWD) ARCH=riscv CROSS_COMPILE=riscv32-unknown-linux-gnu- modules 
	@mkdir -p build
	@mv -t build .*.*.cmd src/.*.*.cmd *.order *.symvers src/*.mod src/*.mod.c src/*.o src/*.ko

put: build/spi.ko build/spi_nointerrupt.ko
	cp build/spi.ko   ~/buildroot-2021.08-rc1/output/target/media/
	cp build/spi_nointerrupt.ko   ~/buildroot-2021.08-rc1/output/target/media/

$(LINUX_PATH)/scripts/module.lds: 
	cp $(LINUX_PATH)/scripts/module.lds.S   $(LINUX_PATH)/scripts/module.lds
	echo "SECTIONS {.plt : { BYTE(0) } 
        .got : { BYTE(0) } 
        .got.plt : { BYTE(0) } 
	}" >> $(LINUX_PATH)/scripts/module.lds

clean:
	rm -rf build
	make -C $(LINUX_PATH) M=$(PWD) clean
