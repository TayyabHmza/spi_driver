
# Object file of driver (kernel loadable module)
obj-m += build/spi.o

# Path of linux src directory
LINUX_SRC_PATH = /root/Build_linux/linux-6.1

# Build kernel loadable module from object file
all:
	make -C $(LINUX_SRC_PATH) M=$(PWD) modules

# Build driver object file using source, including kernel headers from source
build/spi.o:
	riscv32-unknown-linux-gnu-gcc -c src/spi.c -o build/spi.o -I $(LINUX_SRC_PATH)/include/ $(LINUX_SRC_PATH)arch/riscv/include/

clean:
	make -C $(LINUX_SRC_PATH) M=$(PWD) clean
