BUILD_DIR:=./build
BUILD_BOOT_DIR:=$(BUILD_DIR)/boot
BUILD_KERNEL_DIR:=$(BUILD_DIR)/kernel
BUILD_IO_DIR:=$(BUILD_DIR)/io
BUILD_LIB_DIR:=$(BUILD_DIR)/lib
BUILD_TYPES_DIR:=$(BUILD_DIR)/types
BUILD_FS_DIR:=$(BUILD_DIR)/fs

SRC_DIR:=./src
SRC_BOOT_DIR:=$(SRC_DIR)/boot
SRC_KERNEL_DIR:=$(SRC_DIR)/kernel
SRC_IO_DIR:=$(SRC_DIR)/io
SRC_LIB_DIR:=$(SRC_DIR)/lib
SRC_TYPES_DIR:=$(SRC_DIR)/types
SRC_FS_DIR:=$(SRC_DIR)/fs

MULTIBOOT2:=0x10000
ENTRY_POINT:=0x10040

CFLAGS:= -m32
CFLAGS+= -fno-builtin			# 不需要 gcc 内置函数
CFLAGS+= -nostdinc				# 不需要标准头文件
CFLAGS+= -fno-pic				# 不需要位置无关的代码  position independent code
CFLAGS+= -fno-pie				# 不需要位置无关的可执行程序 position independent executable
CFLAGS+= -nostdlib				# 不需要标准库
CFLAGS+= -fno-stack-protector	# 不需要栈保护
CFLAGS+= -fno-exceptions
CFLAGS:=$(strip ${CFLAGS})

DEBUG:= -g
INCLUDE:= -I$(SRC_DIR)/include

all: $(BUILD_DIR)/SoulOS.img $(BUILD_DIR)/slave.img vmdk $(BUILD_DIR)/kernel.iso

$(BUILD_BOOT_DIR)/%_asm.bin: $(SRC_BOOT_DIR)/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f bin $< -o $@

$(BUILD_KERNEL_DIR)/%_asm.o: $(SRC_KERNEL_DIR)/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f elf32 $< -o $@

$(BUILD_IO_DIR)/%_asm.o: $(SRC_IO_DIR)/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f elf32 $< -o $@

$(BUILD_KERNEL_DIR)/%.o: $(SRC_KERNEL_DIR)/%.cpp
	$(shell mkdir -p $(dir $@))
	g++ $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

$(BUILD_IO_DIR)/%.o: $(SRC_IO_DIR)/%.cpp
	$(shell mkdir -p $(dir $@))
	g++ $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

$(BUILD_LIB_DIR)/%.o: $(SRC_LIB_DIR)/%.cpp
	$(shell mkdir -p $(dir $@))
	g++ $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

$(BUILD_TYPES_DIR)/%.o: $(SRC_TYPES_DIR)/%.cpp
	$(shell mkdir -p $(dir $@))
	g++ $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

$(BUILD_FS_DIR)/%.o: $(SRC_FS_DIR)/%.cpp
	$(shell mkdir -p $(dir $@))
	g++ $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

LDFLAGS:= -m elf_i386 \
		-static \
		-Ttext $(ENTRYPOINT) \
		--section-start=.multiboot2=$(MULTIBOOT2)
LDFLAGS:=$(strip ${LDFLAGS})


$(BUILD_KERNEL_DIR)/kernel_temp.bin: $(BUILD_KERNEL_DIR)/kernel_asm.o \
	$(BUILD_KERNEL_DIR)/main.o \
	$(BUILD_KERNEL_DIR)/console.o \
	$(BUILD_KERNEL_DIR)/assert.o \
	$(BUILD_KERNEL_DIR)/printk.o \
	$(BUILD_KERNEL_DIR)/debug.o \
	$(BUILD_KERNEL_DIR)/gdt.o \
	$(BUILD_KERNEL_DIR)/taskSwitch_asm.o \
	$(BUILD_KERNEL_DIR)/task.o \
	$(BUILD_KERNEL_DIR)/interrupt.o \
	$(BUILD_KERNEL_DIR)/interruptHandler_asm.o \
	$(BUILD_KERNEL_DIR)/clock.o \
	$(BUILD_KERNEL_DIR)/rtc.o \
	$(BUILD_KERNEL_DIR)/time.o \
	$(BUILD_KERNEL_DIR)/logk.o \
	$(BUILD_KERNEL_DIR)/memory.o \
	$(BUILD_KERNEL_DIR)/memory_asm.o \
	$(BUILD_KERNEL_DIR)/gate.o \
	$(BUILD_KERNEL_DIR)/thread.o \
	$(BUILD_KERNEL_DIR)/mutex.o \
	$(BUILD_KERNEL_DIR)/keyboard.o \
	$(BUILD_KERNEL_DIR)/arena.o \
	$(BUILD_KERNEL_DIR)/ide.o \
	$(BUILD_KERNEL_DIR)/device.o \
	$(BUILD_KERNEL_DIR)/buffer.o \
	$(BUILD_KERNEL_DIR)/system.o \
	$(BUILD_IO_DIR)/io_asm.o \
	$(BUILD_IO_DIR)/cursor.o \
	$(BUILD_FS_DIR)/super.o \
	$(BUILD_FS_DIR)/bmap.o \
	$(BUILD_FS_DIR)/inode.o \
	$(BUILD_LIB_DIR)/stdlib.o \
	$(BUILD_LIB_DIR)/stdio.o \
	$(BUILD_LIB_DIR)/charArray.o \
	$(BUILD_LIB_DIR)/errno.o \
	$(BUILD_LIB_DIR)/bitmap.o \
	$(BUILD_LIB_DIR)/syscall.o \
	$(BUILD_LIB_DIR)/list.o \
	$(BUILD_LIB_DIR)/fifo.o
	ld -m elf_i386 -static $^ -o $@ -Ttext $(ENTRY_POINT) --section-start=.multiboot2=$(MULTIBOOT2)


$(BUILD_KERNEL_DIR)/kernel.bin: $(BUILD_KERNEL_DIR)/kernel_temp.bin
	objcopy -O binary $< $@


$(BUILD_KERNEL_DIR)/kernel.map: $(BUILD_KERNEL_DIR)/kernel_temp.bin
	nm $< | sort > $@




include ./src/utils/image.mk
include ./src/utils/cdrom.mk
include ./src/utils/cmd.mk