BUILD_DIR:=./build
BUILD_BOOT_DIR:=$(BUILD_DIR)/boot
BUILD_KERNEL_DIR:=$(BUILD_DIR)/kernel

SRC_DIR:=./src
SRC_BOOT_DIR:=$(SRC_DIR)/boot
SRC_KERNEL_DIR:=$(SRC_DIR)/kernel

all: $(BUILD_BOOT_DIR)/boot.bin $(BUILD_BOOT_DIR)/kernelLoader.bin $(BUILD_DIR)/SoulOS.img

$(BUILD_BOOT_DIR)/%.bin: $(SRC_BOOT_DIR)/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f bin $< -o $@

$(BUILD_KERNEL_DIR)/%.bin: $(SRC_KERNEL_DIR)/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f bin $< -o $@

$(BUILD_DIR)/SoulOS.img: $(BUILD_BOOT_DIR)/boot.bin $(BUILD_BOOT_DIR)/kernelLoader.bin $(BUILD_KERNEL_DIR)/kernel.bin
	yes | ./debug-tools/bochs-2.7/bin/bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $@
	dd if=$(BUILD_BOOT_DIR)/boot.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD_BOOT_DIR)/kernelLoader.bin of=$@ bs=512 count=4 seek=1 conv=notrunc
	dd if=$(BUILD_KERNEL_DIR)/kernel.bin of=$@ bs=512 count=200 seek=5 conv=notrunc

bochs:
	./debug-tools/bochs-2.7/bin/bochs -f ./bochsrc -q

clean:
	rm -rf $(BUILD_DIR)/*