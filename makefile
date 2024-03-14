all: boot.bin kernelLoader.bin SoulOS.img


%.bin: ./src/%.asm
	nasm -f bin $< -o ./temp/$@

SoulOS.img: ./temp/boot.bin ./temp/kernelLoader.bin
	yes | ./debug-tools/bochs-2.7/bin/bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat ./temp/SoulOS.img
	dd if=./temp/boot.bin of=./temp/SoulOS.img bs=512 count=1 conv=notrunc
	dd if=./temp/kernelLoader.bin of=./temp/SoulOS.img bs=512 count=4 seek=1 conv=notrunc

bochs:
	./debug-tools/bochs-2.7/bin/bochs -f ./bochsrc -q

clean:
	rm -rf ./temp/*