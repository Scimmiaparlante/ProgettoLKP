KERNEL_DIR ?= /lib/modules/`uname -r`/build

obj-m = module_main.o

all: test/test_main module

module: module_main.c
	make -C $(KERNEL_DIR) M=`pwd` modules
	find *.ko | cpio -H newc -o | gzip > file_system/module_fs.gz

test/test_main: test/test_main.c
	gcc test/test_main.c -Wall -Wextra -o test/test_main -pthread
	find test | cpio -H newc -o | gzip > file_system/test_fs.gz


clean:
	make -C $(KERNEL_DIR) M=`pwd` clean
	rm test/test_main
	rm file_system/module_fs.gz file_system/test_fs.gz file_system/complete_fs.gz
	

run:
	cat file_system/tinyfs.gz file_system/test_fs.gz file_system/keyb.gz file_system/module_fs.gz > file_system/complete_fs.gz
	qemu-system-x86_64 -kernel ../linux-5.8.14/arch/x86_64/boot/bzImage -initrd file_system/complete_fs.gz

