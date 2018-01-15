#include <stdio.h>

#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
extern struct TIMERCTL timerctl;

void task_b_main(struct SHEET *sht_back);

// TODO: 查阅键盘布局
static char keytable[0x54] = {
			      0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
			      'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0, 0,
			      'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0, 0, '}', 'Z',
			      'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0,
			      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+',
			      '1', '2', '3', '-'
};

void HariMain(void) {

    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct MOUSE_DEC mdec;
    struct FIFO32 fifo;
    char s[40];
    int fifobuf[128];
    struct TIMER *timer, *timer2, *timer3;
    struct TIMER *timer_ts;
    int mx, my;
    unsigned int memtotal, count = 0;
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back;
    unsigned char buf_mouse[256];
    unsigned char *buf_win;
    int cursor_x, cursor_c;
    int task_b_esp;
    struct TSS32 tss_a, tss_b;

    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
    
    init_gdtidt();
    
    init_pic();
    init_pit();
    io_sti(); // IDT/PIC初始化完成,开放中断

    fifo32_init(&fifo, 128, fifobuf);
    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);
 
    io_out8(PIC0_IMR, 0xf8); // 开放PIT, PIC1和键盘中断(11111000)
    io_out8(PIC1_IMR, 0xef); // 开放鼠标中断(11101111)


    // 初始化定时器
    timer = timer_alloc();
    timer_init(timer, &fifo, 10);
    timer_settime(timer, 1000);

    timer2 = timer_alloc();
    timer_init(timer2, &fifo, 3);
    timer_settime(timer2, 300);
    
    timer3 = timer_alloc();
    timer_init(timer3, &fifo, 1);
    timer_settime(timer3, 50);
    
    timer_ts = timer_alloc();
    timer_init(timer_ts, &fifo, 2);
    timer_settime(timer_ts, 2);
   
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
    make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);
    cursor_x = 8;
    cursor_c = COL8_FFFFFF;
    sheet_updown(sht_back, 0);
    sheet_updown(sht_win, 1);
    sheet_updown(sht_mouse, 2);
    sprintf(s, "memory %dMB free: %dKB ", memtotal / (1024 * 1024), memman_total(memman) / 1024); 
    putfonts8_asc((char *)buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
    sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);
 
    tss_a.ldtr = 0;
    tss_b.ldtr = 0;
    tss_a.iomap = 0x40000000;
    tss_b.iomap = 0x40000000;

    set_segmdesc(gdt + 3, 103, (int)&tss_a, AR_TSS32);
    set_segmdesc(gdt + 4, 103, (int)&tss_b, AR_TSS32);
    load_tr(3 * 8);
    
    task_b_esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
    tss_b.eip = (int)&task_b_main;
    tss_b.eflags = 0x00000202;
    tss_b.eax = 0;
    tss_b.ecx = 0;
    tss_b.edx = 0;
    tss_b.ebx = 0;
    tss_b.esp = task_b_esp;
    tss_b.ebp = 0;
    tss_b.esi = 0;
    tss_b.edi = 0;
    tss_b.es = 1 * 8;
    tss_b.cs = 2 * 8;
    tss_b.ss = 1 * 8;
    tss_b.ds = 1 * 8;
    tss_b.fs = 1 * 8;
    tss_b.gs = 1 * 8;


    *((int *) (task_b_esp + 4)) = (int) sht_back;
    
    mt_init();
    for(;;) {
	count++;
	/* sprintf(s, "%010d", timerctl.count); */
	/* boxfill8((char *)buf_win, 160, COL8_C6C6C6, 40, 28, 119, 43); */
	/* putfonts8_asc((char *)buf_win, 160, 40, 28, COL8_000000, s); */
	/* sheet_refresh(sht_win, 40, 28, 120, 44); */
	io_cli();
	int i;
	if (fifo32_status(&fifo) == 0) {
	    //io_stihlt(); // 无事休眠
	    io_sti();
	} else {
	    i = fifo32_pop(&fifo);
	    io_sti();
	    if (i >= 256 && i <= 511) {
		sprintf(s, "%02X", i - 256);
		putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
		if (i < 0x54 + 256)  {
		    if (keytable[i - 256] != 0 && cursor_x < 144)  {
			s[0] = keytable[i - 256];
			s[1] = 0;
			putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_C6C6C6, s, 1);
			cursor_x += 8;
		    }
		}
		if (i == 256 + 0x0e && cursor_x > 8)  {
		    // 退格键
		    putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
		    cursor_x -= 8;
		}
		boxfill8((char *)sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
		sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
	    } else if (i >= 512 && i <= 767) {
		io_sti();
		if (mouse_decode(&mdec, i - 512) != 0) {
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
		    putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);
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
		    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 30);
		    sheet_slide(sht_mouse, mx, my);
		    if ((mdec.btn & 0x01) != 0)  {
			// 按下左键，移动sheet_win
			sheet_slide(sht_win, mx - 80, my - 8);
		    }
		}
	    } else if (i == 10) {
		putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]", 7);
		sprintf(s, "%010d", count);
		putfonts8_asc_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s, 10);
	    } else if (i == 3) {
		putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]", 6);
		count = 0; // 开始测定
	    } else if (i <= 1) {
		if (i != 0)  {
		    timer_init(timer3, &fifo, 0);
		    cursor_c = COL8_000000;
		} else  {
		    timer_init(timer3, &fifo, 1);
		    cursor_c = COL8_FFFFFF;
		}
		timer_settime(timer3, 50);
		boxfill8((char *)sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
		sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
	    }
	}
    }	
}

void task_b_main(struct SHEET *sht_back)  {
    struct FIFO32 fifo;
    struct TIMER *timer_put, *timer_1s;
    int i, fifobuf[128], count = 0, count0 = 0;
    char s[12];

    
    fifo32_init(&fifo, 128, fifobuf);
    
    timer_put = timer_alloc();
    timer_init(timer_put, &fifo, 1);
    timer_settime(timer_put, 1);

    timer_1s = timer_alloc();
    timer_init(timer_1s, &fifo, 100);
    timer_settime(timer_1s, 100);
    
    for(;;)  {
	count++;
	io_cli();
	if (fifo32_status(&fifo) == 0)  {
	    io_sti();
	} else  {
	    i = fifo32_pop(&fifo);
	    io_sti();
	    if (i == 1)  {
		sprintf(s, "%11d", count);
		putfonts8_asc_sht(sht_back, 0, 144, COL8_FFFFFF, COL8_008484, s, 11);
		timer_settime(timer_put, 1);
	    } else if (i == 100)  {
		sprintf(s, "%11d", count - count0);
		putfonts8_asc_sht(sht_back, 0, 128, COL8_FFFFFF, COL8_008484, s, 11);
		count0 = count;
		timer_settime(timer_1s, 100);
	    }
	}
    }
}



