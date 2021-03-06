#include <stdio.h>

#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;




void HariMain(void) {

    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct MOUSE_DEC mdec;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx = 100;
    int my = 100;
    unsigned int memtotal;
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    

    init_gdtidt();
    init_pic();
    io_sti(); // IDT/PIC初始化完成,开放中断

    fifo8_init(&keyfifo, 32, (unsigned char *)keybuf);
    fifo8_init(&mousefifo, 128, (unsigned char *)mousebuf);
    io_out8(PIC0_IMR, 0xf9); // 开放PIC1和键盘中断(11111001)
    io_out8(PIC1_IMR, 0xef); // 开放鼠标中断(11101111)

    init_keyboard();
    
    init_palette(); // 设定调色板
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);

    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

    enable_mouse(&mdec);

    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);
    sprintf(s, "memory %dMB free: %dKB ", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

    for(;;) {
	io_cli();
	int i;
	if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
	    io_stihlt();
	} else {
	    if (fifo8_status(&keyfifo) != 0) {
		i = fifo8_pop(&keyfifo);
		io_sti();
		sprintf(s, "%02X", i);
		boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
		putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
	    } else if (fifo8_status(&mousefifo) != 0) {
		i = fifo8_pop(&mousefifo);
		io_sti();
		if (mouse_decode(&mdec, i) != 0) {
		    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
		    if ((mdec.btn & 0x01) != 0) {
			s[1] = 'L';
		    }
		    if ((mdec.btn & 0x02) != 0) {
			s[3] = 'R';
		    }
		    if ((mdec.btn & 0x04) != 0) {
			s[2] = 'C';
		    }
		    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
		    putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

		    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
		    mx += mdec.x;
		    my += mdec.y;
		    if (mx < 0) {
			mx = 0;
		    }
		    if (my < 0) {
			my = 0;
		    }
		    if (mx > binfo->scrnx - 16) {
			mx = binfo->scrnx - 16;
		    }
		    if (my > binfo->scrny - 16) {
			my = binfo->scrny - 16;
		    }
		    sprintf(s, "(%3d, %3d)", mx, my);
		    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
		    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
		    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
		}
	    }
	}
    }
}


