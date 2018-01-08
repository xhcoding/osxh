
#include "bootpack.h"

void init_palette(void) {
    static unsigned char table_rgb[16 * 3] = {
	0x00, 0x00, 0x00, // 0: 黑
	0xff, 0x00, 0x00, // 1: 亮红
	0x00, 0xff, 0x00, // 2: 亮绿
	0xff, 0xff, 0x00, // 3. 亮黄
	0x00, 0x00, 0xff, // 4: 亮蓝
	0xff, 0x00, 0xff, // 5: 亮紫
	0x00, 0xff, 0xff, // 6: 浅亮蓝
	0xff, 0xff, 0xff, // 7: 白
	0xc6, 0xc6, 0xc6, // 8: 亮灰
	0x84, 0x00, 0x00, // 9: 暗红
	0x00, 0x84, 0x00, // 10: 暗绿
	0x84, 0x84, 0x00, // 11: 暗黄
	0x00, 0x00, 0x84, // 12: 暗青
	0x84, 0x00, 0x84, // 13: 暗紫
	0x00, 0x84, 0x84, // 14: 浅暗蓝
	0x84, 0x84, 0x84  // 15: 暗灰
    };
    set_palette(0, 15, table_rgb);
}


void set_palette(int start, int end, const unsigned char* rgb) {
    int i, eflags;
    eflags = io_load_eflags();// 记录中断许可标志的值
    io_cli();// 将中断许可标志置0，禁止中断
    io_out8(0x03c8, start);
    for (i = 0; i <= end; ++i) {
	io_out8(0x03c9, rgb[0] / 4);
	io_out8(0x03c9, rgb[1] / 4);
	io_out8(0x03c9, rgb[2] / 4);
	rgb += 3;
    }
    io_store_eflags(eflags);// 复原中断许可标志
}


void boxfill8(char* vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1) {
    int x, y;
    for (y = y0; y <= y1;++y) {
	for (x = x0; x <= x1;++x) {
	    vram[y * xsize + x] = c;
	}
    }
}



void init_screen8(char *vram, int xsize, int ysize) {
    
    boxfill8(vram, xsize, COL8_008484, 0, 0, xsize - 1, ysize - 29);
    boxfill8(vram, xsize, COL8_C6C6C6, 0, ysize - 28, xsize - 1, ysize - 28);
    boxfill8(vram, xsize, COL8_FFFFFF, 0, ysize - 27, xsize - 1, ysize - 27);
    boxfill8(vram, xsize, COL8_C6C6C6, 0, ysize - 26, xsize - 1, ysize - 1);

    boxfill8(vram, xsize, COL8_FFFFFF, 3, ysize - 24, 59, ysize - 24);
    boxfill8(vram, xsize, COL8_FFFFFF, 2, ysize - 24, 2, ysize - 4);
    boxfill8(vram, xsize, COL8_848484, 3, ysize - 4, 59, ysize - 4);
    boxfill8(vram, xsize, COL8_848484, 59, ysize - 23, 59, ysize - 5);
    boxfill8(vram, xsize, COL8_000000, 2, ysize - 3, 59, ysize - 3);
    boxfill8(vram, xsize, COL8_000000, 60, ysize - 24, 60, ysize - 3);

    boxfill8(vram, xsize, COL8_848484, xsize -47, ysize - 24, xsize - 4, ysize - 24);
    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize - 4);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize - 3, xsize - 4, ysize - 3);
    boxfill8(vram, xsize, COL8_FFFFFF, xsize - 3, ysize - 24, xsize - 3, ysize - 3);

 
}


void putfont8(char *vram, int xsize, int x, int y, char color, char *font) {
    int i;
    char d, *p;
    for (i = 0; i < 16; ++i) {
	d = font[i];
	p = vram + (y + i) * xsize + x;
	if ((d & 0x80) != 0) {
	    p[0] = color;
	}
	if ((d & 0x40) != 0) {
	    p[1] = color;
	}
	if ((d & 0x20) != 0) {
	    p[2] = color;
	}
	if ((d & 0x10) != 0) {
	    p[3] = color;
	}
	if ((d & 0x08) != 0) {
	    p[4] = color;
	}
	if ((d & 0x04) != 0) {
	    p[5] = color;
	}
	if ((d & 0x02) != 0) {
	    p[6] = color;
	}
	if ((d & 0x01) != 0) {
	    p[7] = color;
	}

    }

}


void putfonts8_asc(char *vram, int xsize, int x, int y, char color, const char *s) {
    extern char hankaku[4096];
    for (; *s != 0x00; ++s) {
	putfont8(vram, xsize, x, y, color, hankaku + *s * 16);
	x += 8;
    }
    
}

void putfonts8_asc_sht(struct SHEET *sht, int x, int y, char color, char bcolor, char *s, int len) {
    boxfill8((char *)sht->buf, sht->bxsize, bcolor, x, y, x + len * 8 - 1, y + 15);
    putfonts8_asc((char *)sht->buf, sht->bxsize, x, y, color, s);
    sheet_refresh(sht, x, y, x + len * 8, y + 16);
}

void init_mouse_cursor8(char *mouse, char bgcolor) {
    static char cursor[16][16] = {
	"**************..",
	"*OOOOOOOOOOO*...",
	"*OOOOOOOOOO*....",
	"*OOOOOOOOO*.....",
	"*OOOOOOOO*......",
	"*OOOOOOO*.......",
	"*OOOOOOO*.......",
	"*OOOOOOOO*......",
	"*OOOO**OOO*.....",
	"*OOO*..*OOO*....",
	"*OO*....*OOO*...",
	"*O*......*OOO*..",
	"**........*OOO*.",
	"*..........*OOO*",
	"............*OO*",
	".............***"
    };

    int x, y;
    for (y = 0; y < 16; ++y) {
	for (x = 0; x < 16; ++x) {
	    if (cursor[y][x] == '*') {
		mouse[y * 16 + x] = COL8_000000;
	    }
	    if (cursor[y][x] == 'O') {
		mouse[y * 16 + x] = COL8_FFFFFF;
	    }
	    if (cursor[y][x] == '.') {
		mouse[y * 16 + x] = bgcolor;
	    }

	}
    }
    
}


void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize) {
    int x, y;
    for (y = 0; y < pysize; ++y) {
	for (x = 0; x < pxsize; ++x) {
	    vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * pysize + x];
	}
    }
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title) {
    static char closebtn[14][16] = {
	"OOOOOOOOOOOOOOO@",
	"OQQQQQQQQQQQQQ$@",
	"OQQQQQQQQQQQQQ$@",
	"OQQQ@@QQQQ@@QQ$@",
	"OQQQQ@@QQ@@QQQ$@",
	"OQQQQQ@@@@QQQQ$@",
	"OQQQQQQ@@QQQQQ$@",
	"OQQQQQ@@@@QQQQ$@",
	"OQQQQ@@QQ@@QQQ$@",
	"OQQQ@@QQQQ@@QQ$@",
	"OQQQQQQQQQQQQQ$@",
	"OQQQQQQQQQQQQQ$@",
	"O$$$$$$$$$$$$$$@",
	"@@@@@@@@@@@@@@@@"
    };
    int x, y;
    char c;
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize - 1, 0);
    boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, xsize - 2, 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, 1, ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0, xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_000084, 3, 3, xsize - 4, 20);
    boxfill8(buf, xsize, COL8_848484, 1, ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0, ysize - 1, xsize - 1, ysize - 1);
    putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
    for (y = 0; y < 14; y++) {
	for (x = 0; x < 16; x++) {
	    c = closebtn[y][x];
	    if (c == '@') {
		c = COL8_000000;
	    } else if (c == '$') {
		c = COL8_848484;
	    } else if (c == 'Q') {
		c = COL8_C6C6C6;
	    } else {
		c = COL8_FFFFFF;
	    }
	    buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
	}
    }
}

void make_textbox8(struct SHEET* sht, int x0, int y0, int sx, int sy, int c)  {
    int x1 = x0 + sx;
    int y1 = y0 + sy;
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 2, x1 + 1, y0 - 3);
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, c, x0 - 1, y0 - 1, x1 + 0, y1 + 0);
}
