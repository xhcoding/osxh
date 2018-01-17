#include <stdio.h>

#include "bootpack.h"

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
extern struct TIMERCTL timerctl;

#define KEYCMD_LED 0xed

void console_task(struct SHEET *sheet);


static char
keytable0[0x80] = {
		   0,   0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
		   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0,   0,   'A', 'S',
		   'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',   0, '\\', 'Z', 'X', 'C', 'V',
		   'B', 'N', 'M', ',', '.', '/', 0,   '*',  0,   ' ', 0,   0,   0,   0,   0,   0,
		   0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		   '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		   0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
};


static char
keytable1[0x80] = {
		   0,   0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
		   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,   'A', 'S',
		   'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',   0,   '|', 'Z', 'X', 'C', 'V',
		   'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		   0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		   '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		   0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
};


void HariMain(void) {

    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct MOUSE_DEC mdec;
    struct FIFO32 fifo, keycmd;
    char s[40];
    int fifobuf[128], keycmd_buf[32];
    struct TIMER *timer;
    int mx, my;
    unsigned int memtotal;
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;
    unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
    int cursor_x, cursor_c;
    struct TASK *task_a, *task_cons;
    int i;
    int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7;
    int keycmd_wait = -1;
    
    init_gdtidt();
    
    init_pic();
    init_pit();
    io_sti(); // IDT/PIC初始化完成,开放中断

    fifo32_init(&fifo, 128, fifobuf, 0);
    fifo32_init(&keycmd, 32, keycmd_buf, 0);
    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);
 
    io_out8(PIC0_IMR, 0xf8); // 开放PIT, PIC1和键盘中断(11111000)
    io_out8(PIC1_IMR, 0xef); // 开放鼠标中断(11101111)


    
   
    // 内存
    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

       
    init_palette(); // 设定调色板
    shtctl = shtctl_init(memman, (unsigned char *)binfo->vram, (int)binfo->scrnx, (int)binfo->scrny);

    task_a = task_init(memman);
    fifo.task = task_a;
    task_run(task_a, 1, 0);

    // sht_back
    sht_back = sheet_alloc(shtctl);
    buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);// 没有透明色
    init_screen8((char *)buf_back, binfo->scrnx, binfo->scrny);


    // sht_win
    sht_win = sheet_alloc(shtctl);
    buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
    sheet_setbuf(sht_win, buf_win, 144, 52, 1);
    make_window8(buf_win, 144, 52, "task_a", 1);
    make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF);
    cursor_x = 8;
    cursor_c = COL8_FFFFFF;
    // 初始化定时器
    timer = timer_alloc();
    timer_init(timer, &fifo, 1);
    timer_settime(timer, 50);

    // sht_mouse
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); // 透明色号99
    init_mouse_cursor8((char *)buf_mouse, 99);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;

    // sht_cons
    sht_cons = sheet_alloc(shtctl);
    buf_cons = (unsigned char *)memman_alloc_4k(memman, 256 * 165);
    sheet_setbuf(sht_cons, buf_cons, 256, 165, -1);
    make_window8(buf_cons, 256, 165, "console", 0);
    make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
    task_cons = task_alloc();
    task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
    task_cons->tss.eip = (int)&console_task;
    task_cons->tss.es = 1 * 8;
    task_cons->tss.cs = 2 * 8;
    task_cons->tss.ss = 1 * 8;
    task_cons->tss.ds = 1 * 8;
    task_cons->tss.fs = 1 * 8;
    task_cons->tss.gs = 1 * 8;
    *((int *)(task_cons->tss.esp + 4)) = (int)sht_cons;
    task_run(task_cons, 2, 2);
    
    sheet_slide(sht_back, 0, 0);
    sheet_slide(sht_cons, 32, 4);
    sheet_slide(sht_win, 8, 56);
    sheet_slide(sht_mouse, mx, my);
    sheet_updown(sht_back, 0);
    sheet_updown(sht_cons, 1);
    sheet_updown(sht_win, 2);
    sheet_updown(sht_mouse, 3);

    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
    sprintf(s, "memory %dMB free: %dKB ", memtotal / (1024 * 1024), memman_total(memman) / 1024); 
    putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);

    fifo32_push(&keycmd, KEYCMD_LED);
    fifo32_push(&keycmd, key_leds);
    
    for(;;) {
	if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0)  {
	    keycmd_wait = fifo32_pop(&keycmd);
	    wait_KBC_sendready();
	    io_out8(PORT_KEYDAT, keycmd_wait);
	}
	io_cli();
	if (fifo32_status(&fifo) == 0) {
	    task_sleep(task_a);
	    io_sti();
	} else {
	    i = fifo32_pop(&fifo);
	    io_sti();
	    if (i >= 256 && i <= 511) {
		sprintf(s, "%02X", i - 256);
		putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
		if (i < 0x80 + 256)  {
		    if (key_shift == 0)  {
			s[0] = keytable0[i - 256];
		    } else  {
			s[0] = keytable1[i - 256];
		    }
		} else  {
		    s[0] = 0;
		}
		if (s[0] >= 'A' && s[0] <= 'Z')  {
		    if (((key_leds & 4) == 0 && key_shift == 0) ||
			((key_leds & 4) != 0 && key_shift != 0))  {
			s[0] += 0x20;
		    }
		}
		if (s[0] != 0)  {
		    // 一般字符
		    if (key_to == 0)  {
			if (cursor_x < 128)  {
			    s[1] = 0;
			    putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
			    cursor_x += 8;
			}
		    } else  {
			// 发送给命令窗口
			fifo32_push(&task_cons->fifo, s[0] + 256);
		    }
		}
		if (i == 256 + 0x0e)  {
		    // 退格键
		    if (key_to == 0)  {
			if (cursor_x > 8)  {
			    putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
			    cursor_x -= 8;
			}
		    } else  {
			fifo32_push(&task_cons->fifo, 8 + 256);
		    }
		}
		if (i == 256 + 0x0f)  {
		    // tab 键
		    if (key_to == 0)  {
			key_to = 1;
			make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
			make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
		    } else  {
			key_to = 0;
			make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
			make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
		    }
		    sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);
		    sheet_refresh(sht_cons, 0, 0, sht_win->bxsize, 21);
		}
		if (i == 256 + 0x2a)  {
		    key_shift |= 1;
		}
		if (i == 256 + 0x36)  {
		    key_shift |= 2;
		}
		if (i == 256 + 0xaa)  {
		    key_shift &= ~1;
		}
		if (i == 256 + 0xb6)  {
		    key_shift &= ~2;
		}
		if (i == 256 + 0x3A)  {
		    key_leds ^= 1;
		    fifo32_push(&keycmd, KEYCMD_LED);
		    fifo32_push(&keycmd, key_leds);
		}
		if (i == 256 + 0x45)  {
		    key_leds ^= 2;
		    fifo32_push(&keycmd, KEYCMD_LED);
		    fifo32_push(&keycmd, key_leds);
		}
		if (i == 256 + 0x46)  {
		    key_leds ^= 1;
		    fifo32_push(&keycmd, KEYCMD_LED);
		    fifo32_push(&keycmd, key_leds);
		}
		if (i == 256 + 0xfa)  {
		    keycmd_wait = -1;
		}
		if (i == 256 + 0xfe)  {
		    wait_KBC_sendready();
		    io_out8(PORT_KEYDAT, keycmd_wait);
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
	    } else if (i <= 1) {
		if (i != 0)  {
		    timer_init(timer, &fifo, 0);
		    cursor_c = COL8_000000;
		} else  {
		    timer_init(timer, &fifo, 1);
		    cursor_c = COL8_FFFFFF;
		}
		timer_settime(timer, 50);
		boxfill8((char *)sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
		sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
	    }
	}
    }	
}

void console_task(struct SHEET *sheet)  {
    struct TIMER *timer;
    struct TASK *task = task_now();
    int i, fifobuf[128], cursor_x = 16, cursor_c = COL8_000000;
    char s[2];
    
    fifo32_init(&task->fifo, 128, fifobuf, task);
    
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    // 显示提示符
    putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);
    for(;;)  {
	io_cli();
	if (fifo32_status(&task->fifo) == 0)  {
	    task_sleep(task);
	    io_sti();
	} else  {
	    i = fifo32_pop(&task->fifo);
	    io_sti();
	    if (i <= 1)  {
		if (i != 0)  {
		    timer_init(timer, &task->fifo, 0);
		    cursor_c = COL8_FFFFFF;
		} else  {
		    timer_init(timer, &task->fifo, 1);
		    cursor_c = COL8_000000;
		}
		timer_settime(timer, 50);
	    }
	    if (i >= 256 && i <= 511)  {
		if (i == 8 + 256)  {
		    if (cursor_x > 16)  {
			putfonts8_asc_sht(sheet, cursor_x, 28, COL8_FFFFFF, COL8_000000, " ", 1);
			cursor_x -= 8;
		    }
		} else  {
		    if (cursor_x < 240)  {
			s[0] = i - 256;
			s[1] = 0;
			putfonts8_asc_sht(sheet, cursor_x, 28, COL8_FFFFFF, COL8_000000, s, 1);
			cursor_x += 8;
		    }
		}
	    }
	    boxfill8((char *)sheet->buf, sheet->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
	    sheet_refresh(sheet, cursor_x, 28, cursor_x + 8, 44);

	}
    }
}



