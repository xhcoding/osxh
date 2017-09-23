#include <stdio.h>



#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15

struct BOOTINFO {
    char cyls, leds, vmode, reserve;
    short scrnx, scrny;
    char *vram;
};

struct SEGMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};

struct GATE_DESCRIPTOR {
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};



void io_hlt(void); //hlt
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

void init_palette(void);
void set_palette(int start, int end, const unsigned char* rgb);
void init_screen(char *vram, int xsize, int ysize);
void init_mouse_cursor8(char *mouse, char bgcolor);
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);

void boxfill8(char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);

void putfont8(char *vram, int xsize, int x, int y, char color, char *font);

void putfonts8_asc(char *vram, int xsize, int x, int y, char color, const char *s);

void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);
void HariMain(void) {

    init_palette(); // 设定调色板

    struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);


    char s[100];
    sprintf(s, "scrnx = %d", binfo->scrnx);
    putfonts8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, s);

    char mcursor[16 * 16];
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, 100, 100, mcursor, 16);
    
    for(;;) {
	io_hlt();
    }

}

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

void init_screen(char *vram, int xsize, int ysize) {
    
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

void init_gdtidt(void) {
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) 0x00270000;
    struct GATE_DESCRIPTOR *idt = (struct GATE_DESCRIPTOR *) 0x0026f800;
    int i;

    // GDT的初始化
    for (i = 0; i < 8129; ++i) {
	set_segmdesc(gdt + i, 0, 0, 0);
    }
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4096);
    set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a);
    /* TODO:  load_gdtr*/

    // IDT的初始化
    for (i = 0; i < 256; ++i) {
	set_gatedesc(idt + i, 0, 0, 0);
    }
    /* TODO:  load_idt*/

}

void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar) {
    if (limit > 0xfffff) {
	ar |= 0x8000; // G_bit = 1
	limit /= 0x1000;
    }
    sd->limit_low = limit & 0xffff;
    sd->base_low = base & 0xffff;
    sd->base_mid = (base >> 16) & 0xff;
    sd->access_right = ar & 0xff;
    sd->limit_high = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_high = (base >> 24) & 0xff;
}


void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar) {
    gd->offset_low = offset & 0xffff;
    gd->selector = selector;
    gd->dw_count = (ar >> 8) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high = (offset >> 16) & 0xffff;
}
