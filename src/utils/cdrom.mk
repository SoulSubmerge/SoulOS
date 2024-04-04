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