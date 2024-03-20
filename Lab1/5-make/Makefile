boot.bin:boot.asm
	nasm -f bin -o boot.bin boot.asm	

clean:
	rm boot.bin
	
run:
	qemu-system-i386 -drive format=raw,file=boot.bin