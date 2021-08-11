#!Makefile
#
# --------------------------------------------------------
#
#    默认使用的C语言编译器是 GCC、汇编语言编译器是 nasm
#
# --------------------------------------------------------
#

# patsubst 处理所有在 C_SOURCES 字列中的字（一列文件名），如果它的 结尾是 '.c'，就用 '.o' 把 '.c' 取代
C_SOURCES = $(shell find . -name "*.c")
C_OBJECTS = $(patsubst %.c, %.o, $(C_SOURCES))
S_SOURCES = $(shell find . -name "*.s")
S_OBJECTS = $(patsubst %.s, %.o, $(S_SOURCES))

CC = gcc
LD = ld
ASM = nasm

C_FLAGS = -c -Wall -m32 -ggdb -gstabs+ -nostdinc -fno-builtin -fno-stack-protector -I include
LD_FLAGS = -T scripts/kernel.ld -m elf_i386 -nostdlib
ASM_FLAGS = -f elf -g -F stabs

all: clean $(S_OBJECTS) $(C_OBJECTS) link update_image

# The automatic variable `$<' is just the first prerequisite
.c.o:
	@echo 编译代码文件 $< ...
	$(CC) $(C_FLAGS) $< -o $@

.s.o:
	@echo 编译汇编文件 $< ...
	$(ASM) $(ASM_FLAGS) $<

link:
	@echo 链接内核文件...
	$(LD) $(LD_FLAGS) $(S_OBJECTS) $(C_OBJECTS) -o BaGua_OS

.PHONY:clean
clean:
	$(RM) $(S_OBJECTS) $(C_OBJECTS) BaGua_OS BaGua_HD.img

.PHONY:update_image
update_image:
	sudo mount ./hd.img /mnt/kernel
	sudo rm -rf /mnt/kernel/BaGua_OS
	sudo cp -i BaGua_OS /mnt/kernel/BaGua_OS
	sleep 3
	qemu-img convert -f raw -O qcow2 hd.img BaGua_HD.img
	sleep 3
	sudo umount /mnt/kernel

.PHONY:mount_image
mount_image:
	sudo mount hd.img /mnt/kernel

.PHONY:umount_image
umount_image:
	sudo umount /mnt/kernel

.PHONY:qemu
qemu:
	qemu -hda BaGua_HD.img -boot a

.PHONY:debug
debug:
	qemu -S -s -hda BaGua_HD.img -boot a &
	sleep 1
	cgdb -x scripts/gdbinit