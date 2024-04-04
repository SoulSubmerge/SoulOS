.PHONY: bochs
bochs:
	./debug-tools/bochs-2.7/bin/bochs -f ./bochsrc -q

.PHONY: bochsb
bochsb:
	./debug-tools/bochs-2.7/bin/bochs -f ./bochsrc.grub -q


QEMU:= qemu-system-i386 # 虚拟机
QEMU+= -m 1024M # 内存
QEMU+= -audiodev pa,id=hda # 音频设备
QEMU+= -machine pcspk-audiodev=hda # pcspeaker 设备
QEMU+= -rtc base=localtime # 设备本地时间
QEMU+= -drive file=$(BUILD_DIR)/SoulOS.img,if=ide,index=0,media=disk,format=raw # 主硬盘
QEMU+= -drive file=$(BUILD_DIR)/slave.img,if=ide,index=1,media=disk,format=raw # 从硬盘

QEMU_DISK:=-boot c

.PHONY: qemu
qemu: $(IMAGES)
	$(QEMU) $(QEMU_DISK)

.PHONY: qemub
qemub:
	qemu-system-i386 -cdrom ./build/kernel.iso -boot d -m 1024 -smp 1

.PHONY: vmdk
vmdk:
	qemu-img convert -pO vmdk ./build/SoulOS.img ./build/SoulOS.vmdk

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/*