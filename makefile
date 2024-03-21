BUILD_DIR:=./build
BUILD_BOOT_DIR:=$(BUILD_DIR)/boot
BUILD_KERNEL_DIR:=$(BUILD_DIR)/kernel
BUILD_IO_DIR:=$(BUILD_DIR)/io
BUILD_LIB_DIR:=$(BUILD_DIR)/lib
BUILD_TYPES_DIR:=$(BUILD_DIR)/types

SRC_DIR:=./src
SRC_BOOT_DIR:=$(SRC_DIR)/boot
SRC_KERNEL_DIR:=$(SRC_DIR)/kernel
SRC_IO_DIR:=$(SRC_DIR)/io
SRC_LIB_DIR:=$(SRC_DIR)/lib
SRC_TYPES_DIR:=$(SRC_DIR)/types

MULTIBOOT2:=0xB00000
ENTRY_POINT:=0xB00040

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

all: $(BUILD_DIR)/SoulOS.img vmdk $(BUILD_DIR)/kernel.iso

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
	$(BUILD_IO_DIR)/io_asm.o \
	$(BUILD_IO_DIR)/cursor.o \
	$(BUILD_LIB_DIR)/stdlib.o \
	$(BUILD_LIB_DIR)/stdio.o \
	$(BUILD_LIB_DIR)/charArray.o \
	$(BUILD_LIB_DIR)/errno.o \
	$(BUILD_LIB_DIR)/bitmap.o \
	$(BUILD_LIB_DIR)/syscall.o \
	$(BUILD_LIB_DIR)/list.o
	ld -m elf_i386 -static $^ -o $@ -Ttext $(ENTRY_POINT) --section-start=.multiboot2=$(MULTIBOOT2)


$(BUILD_KERNEL_DIR)/kernel.bin: $(BUILD_KERNEL_DIR)/kernel_temp.bin
	objcopy -O binary $< $@


$(BUILD_KERNEL_DIR)/kernel.map: $(BUILD_KERNEL_DIR)/kernel_temp.bin
	nm $< | sort > $@



$(BUILD_DIR)/SoulOS.img: $(BUILD_BOOT_DIR)/boot_asm.bin \
	$(BUILD_BOOT_DIR)/kernelLoader_asm.bin \
	$(BUILD_KERNEL_DIR)/kernel.bin \
	$(BUILD_KERNEL_DIR)/kernel_temp.bin \
	$(BUILD_KERNEL_DIR)/kernel.map
	yes | ./debug-tools/bochs-2.7/bin/bximage -q -hd=100 -func=create -sectsize=512 -imgmode=flat $@
	dd if=$(BUILD_BOOT_DIR)/boot_asm.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD_BOOT_DIR)/kernelLoader_asm.bin of=$@ bs=512 count=4 seek=1 conv=notrunc
	dd if=$(BUILD_KERNEL_DIR)/kernel.bin of=$@ bs=512 count=102400 seek=5 conv=notrunc

$(BUILD_DIR)/kernel.iso : $(BUILD_KERNEL_DIR)/kernel_temp.bin $(SRC_DIR)/utils/grub.cfg
# 检测内核文件是否合法
	grub-file --is-x86-multiboot2 $<
# 创建 iso 目录
	mkdir -p $(BUILD_DIR)/iso/boot/grub
# 拷贝内核文件
	cp $< $(BUILD_DIR)/iso/boot
# 拷贝 grub 配置文件
	cp $(SRC_DIR)/utils/grub.cfg $(BUILD_DIR)/iso/boot/grub
# 生成 iso 文件
	grub-mkrescue -o $@ $(BUILD_DIR)/iso


.PHONY: bochs
bochs:
	./debug-tools/bochs-2.7/bin/bochs -f ./bochsrc -q

.PHONY: bochsb
bochsb:
	./debug-tools/bochs-2.7/bin/bochs -f ./bochsrc.grub -q


.PHONY: qemub
qemub:
	qemu-system-i386 -cdrom ./build/kernel.iso -boot d -m 1024 -smp 1

.PHONY: vmdk
vmdk:
	qemu-img convert -pO vmdk ./build/SoulOS.img ./build/SoulOS.vmdk

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/*