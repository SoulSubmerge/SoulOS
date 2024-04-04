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
# 磁盘分区
	sfdisk $@ < $(SRC_DIR)/utils/master.sfdisk

# 挂载设备
	sudo losetup /dev/loop66 --partscan $@

# 创建 minux 文件系统
	sudo mkfs.minix -1 -n 14 /dev/loop66p1

# 挂载文件系统
	sudo mount /dev/loop66p1 /mntSoulOS66

# 切换所有者
	sudo chown ${USER} /mntSoulOS66

# 创建目录
	mkdir -p /mntSoulOS66/home
	mkdir -p /mntSoulOS66/d1/d2/d3/d4

# 创建文件
	echo "hello SoulOS!!!, from root direcotry file..." > /mntSoulOS66/hello.txt
	echo "hello SoulOS!!!, from home direcotry file..." > /mntSoulOS66/home/hello.txt

# 卸载文件系统
	sudo umount /mntSoulOS66

# 卸载设备
	sudo losetup -d /dev/loop66


$(BUILD_DIR)/slave.img:
# 创建一个 32M 的硬盘镜像
	yes | ./debug-tools/bochs-2.7/bin/bximage -q -hd=32 -func=create -sectsize=512 -imgmode=flat $@

# 执行硬盘分区
	sfdisk $@ < $(SRC_DIR)/utils/slave.sfdisk

# 挂载设备
	sudo losetup /dev/loop99 --partscan $@

# 创建 minux 文件系统
	sudo mkfs.minix -1 -n 14 /dev/loop99p1

# 挂载文件系统
	sudo mount /dev/loop99p1 /mntSoulOS99

# 切换所有者
	sudo chown ${USER} /mntSoulOS99 

# 创建文件
	echo "slave root direcotry file..." > /mntSoulOS99/hello.txt

# 卸载文件系统
	sudo umount /mntSoulOS99

# 卸载设备
	sudo losetup -d /dev/loop99

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