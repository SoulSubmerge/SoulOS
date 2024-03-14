BUILD_DIR:=./build
BUILD_BOOT_DIR:=$(BUILD_DIR)/boot

SRC_DIR:=./src
SRC_BOOT_DIR:=$(SRC_DIR)/boot

all: $(BUILD_BOOT_DIR)/boot.bin $(BUILD_BOOT_DIR)/kernelLoader.bin SoulOS.img

$(BUILD_BOOT_DIR)/%.bin: $(SRC_BOOT_DIR)/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f bin $< -o $@

SoulOS.img: $(BUILD_BOOT_DIR)/boot.bin $(BUILD_BOOT_DIR)/kernelLoader.bin
	yes | ./debug-tools/bochs-2.7/bin/bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $(BUILD_DIR)/SoulOS.img
	dd if=$(BUILD_BOOT_DIR)/boot.bin of=$(BUILD_DIR)/SoulOS.img bs=512 count=1 conv=notrunc
	dd if=$(BUILD_BOOT_DIR)/kernelLoader.bin of=$(BUILD_DIR)/SoulOS.img bs=512 count=4 seek=1 conv=notrunc

bochs:
	./debug-tools/bochs-2.7/bin/bochs -f ./bochsrc -q

clean:
	rm -rf $(BUILD_DIR)/*