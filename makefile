boot.bin: ./src/boot.asm
	nasm -f bin ./src/boot.asm -o ./temp/boot.bin

SoulOS.img: ./temp/boot.bin
	yes | /home/soul/Tools/bochs-2.7/bin/bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat ./temp/SoulOS.img
	dd if=./temp/boot.bin of=./temp/SoulOS.img bs=512 count=1 conv=notrunc

clean:
	rm -rf ./temp/*