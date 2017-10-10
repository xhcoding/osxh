#include <stdio.h>

#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

void HariMain(void) {

    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct MOUSE_DEC mdec;
    char s[40], keybuf[32], mousebuf[128];
    int mx, my;
    unsigned int memtotal, count = 0;
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back;
    unsigned char buf_mouse[256];
    unsigned char *buf_win;
    

    init_gdtidt();
    init_pic();
    io_sti(); // IDT/PIC初始化完成,开放中断

    fifo8_init(&keyfifo, 32, (unsigned char *)keybuf);
    fifo8_init(&mousefifo, 128, (unsigned char *)mousebuf);
    io_out8(PIC0_IMR, 0xf9); // 开放PIC1和键盘中断(11111001)
    io_out8(PIC1_IMR, 0xef); // 开放鼠标中断(11101111)

    init_keyboard();
    enable_mouse(&mdec);
    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);
    
    init_palette(); // 设定调色板
    shtctl = shtctl_init(memman, (unsigned char *)binfo->vram, (int)binfo->scrnx, (int)binfo->scrny);
    sht_back = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    sht_win = sheet_alloc(shtctl);
    buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);// 没有透明色
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); // 透明色号99
    sheet_setbuf(sht_win, buf_win, 160, 68, -1);
    init_screen8((char *)buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8((char *)buf_mouse, 99);
    make_window8(buf_win, 160, 68, "counter");
    sheet_slide(sht_back, 0, 0);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_win, 80, 72);
    sheet_updown(sht_back, 0);
    sheet_updown(sht_win, 1);
    sheet_updown(sht_mouse, 2);
    sprintf(s, "memory %dMB free: %dKB ", memtotal / (1024 * 1024), memman_total(memman) / 1024); 
    putfonts8_asc((char *)buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
    sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);
    
    for(;;) {
	count++;
	sprintf(s, "%010d", count);
	boxfill8((char *)buf_win, 160, COL8_C6C6C6, 40, 28, 119, 43);
	putfonts8_asc((char *)buf_win, 160, 40, 28, COL8_000000, s);
	sheet_refresh(sht_win, 40, 28, 120, 44);
	io_cli();
	int i;
	if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
	    //io_stihlt();
	    io_sti();
	} else {
	    if (fifo8_status(&keyfifo) != 0) {
		i = fifo8_pop(&keyfifo);
		io_sti();
		sprintf(s, "%02X", i);
		boxfill8((char *)buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
		putfonts8_asc((char *)buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
		sheet_refresh(sht_back, 0, 16, 16, 32);
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
		    boxfill8((char *)buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
		    putfonts8_asc((char *)buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
		    sheet_refresh(sht_back, 32, 16, 32 + 15 * 8, 32);
		    mx += mdec.x;
		    my += mdec.y;
		    if (mx < 0) {
			mx = 0;
		    }
		    if (my < 0) {
			my = 0;
		    }
		    if (mx > binfo->scrnx - 1) {
			mx = binfo->scrnx - 1;
		    }
		    if (my > binfo->scrny - 1) {
			my = binfo->scrny - 1;
		    }
		    sprintf(s, "(%3d, %3d)", mx, my);
		    boxfill8((char *)buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
		    putfonts8_asc((char *)buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
		    sheet_refresh(sht_back, 0, 0, 80, 16);
		    sheet_slide(sht_mouse, mx, my);
		}
	    }
	}
    } 
}


