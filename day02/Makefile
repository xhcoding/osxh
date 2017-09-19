ipl.bin: ipl.nas
	nasm ipl.nas -o ipl.bin -l ipl.lst

helloos.img:
	nasm ipl.nas -o helloos.img

run:
	qemu-system-i386 helloos.img

clean:
	rm ipl.bin ipl.lst helloos.img
