
# Object file of driver (kernel loadable module)
obj-m += spi.o

# Path of linux src directory
LINUX_PATH = /root/Build_linux/linux-6.1

build/spi.ko:
	make -C $(LINUX_PATH) M=$(PWD) modules ARCH=riscv CROSS_COMPILE=riscv32-unknown-linux-gnu-
	@mkdir -p ../build
	@mv -t ../build .*.*.cmd *.order *.symvers *.mod *.mod.c *.o *.ko

put:build/spi.ko
	cp ../build/spi.ko   ~/buildroot-2024.02-rc1/output/target/media/

clean:
	rm -rf ../build
	#make -C $(LINUX_PATH) M=$(PWD) clean
