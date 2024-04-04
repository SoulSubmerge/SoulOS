$(BUILD_DIR)/SoulOS.img: $(BUILD_BOOT_DIR)/boot_asm.bin \
	$(BUILD_BOOT_DIR)/kernelLoader_asm.bin \
	$(BUILD_KERNEL_DIR)/kernel.bin \
	$(BUILD_KERNEL_DIR)/kernel_temp.bin \
	$(BUILD_KERNEL_DIR)/kernel.map \
	$(SRC_DIR)/utils/master.sfdisk
	yes | ./debug-tools/bochs-2.7/bin/bximage -q -hd=128 -func=create -sectsize=512 -imgmode=flat $@
	dd if=$(BUILD_BOOT_DIR)/boot_asm.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD_BOOT_DIR)/kernelLoader_asm.bin of=$@ bs=512 count=4 seek=1 conv=notrunc
	test -n "$$(find $(BUILD_KERNEL_DIR)/kernel.bin -size -128k)"
	dd if=$(BUILD_KERNEL_DIR)/kernel.bin of=$@ bs=512 count=256 seek=5 conv=notrunc


$(BUILD_DIR)/slave.img:
# 创建一个 32M 的硬盘镜像
	yes | ./debug-tools/bochs-2.7/bin/bximage -q -hd=32 -func=create -sectsize=512 -imgmode=flat $@


.PHONY: mount66
mount66: $(BUILD_DIR)/SoulOS.img
	sudo losetup /dev/loop66 --partscan $<
	sudo mount /dev/loop66p1 /mntSoulOS66
	sudo chown ${USER} /mntSoulOS66 

.PHONY: umount66
umount66: /dev/loop66
	-sudo umount /mntSoulOS66
	-sudo losetup -d $<

.PHONY: mount99
mount99: $(BUILD_DIR)/slave.img
	sudo losetup /dev/loop99 --partscan $<
	sudo mount /dev/loop99p1 /mntSoulOS99
	sudo chown ${USER} /mntSoulOS99 

.PHONY: umount99
umount99: /dev/loop99
	-sudo umount /mntSoulOS99
	-sudo losetup -d $<


IMAGES:= $(BUILD_DIR)/SoulOS.img \
	$(BUILD_DIR)/slave.img