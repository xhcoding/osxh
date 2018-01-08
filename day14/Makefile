TOOLPATH=../z_tools/
INCPATH=../z_tools/haribote/

MAKE     = make
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
MAKEFONT = $(TOOLPATH)makefont.exe
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = $(TOOLPATH)haribote/haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = cp
DEL      = rm


OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj \
				dsctbl.obj init.obj fifo.obj keyboard.obj mouse.obj \
				memory.obj sheet.obj timer.obj


img:
	make xhos.img

ipl10.bin: ipl10.nas
	$(NASK) ipl10.nas ipl10.bin  ipl10.lst


asmhead.bin : asmhead.nas Makefile
	$(NASK) asmhead.nas  asmhead.bin  asmhead.lst

hankaku.bin : hankaku.txt
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj : hankaku.bin
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

xhos.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > xhos.sys



xhos.img: ipl10.bin xhos.sys
	$(EDIMG)  imgin:../z_tools/fdimg0at.tek \
		wbinimg src:ipl10.bin len:512 from:0 to:0 \
		copy from:xhos.sys to:@: \
		imgout:xhos.img


%.gas : %.c Makefile
	$(CC1) -o $*.gas $*.c

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

run:
	qemu-system-i386 -L . -m 32 -vga std -fda xhos.img

clean:
	-$(DEL) *.bin
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) bootpack.map
	-$(DEL) bootpack.bim
	-$(DEL) bootpack.hrb
	-$(DEL) xhos.sys

src_only :
	make clean
	-$(DEL) xhos.img
