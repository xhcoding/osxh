#ifndef BOOTPACK_H_
#define BOOTPACK_H_

// asmhead.nas
struct BOOTINFO {
    char cyls; // 启动区读硬盘读到何处为止 
    char leds; // 启动键盘led的状态
    char vmode; // 显卡模式为多少位彩色
    char reserve;
    short scrnx, scrny; // 画面分辨率
    char *vram;
};

#define ADR_BOOTINFO 0x00000ff0

// naskfunc.nas
void io_hlt(void); 
void io_cli(void);
void io_sti(void);
void io_stihlt();
void io_out8(int port, int data);
int io_in8(int port);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
int load_cr0(void);
void store_cr0(int cr0);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void load_tr(int tr);
void farjmp(int eip, int cs);

// graphic.c
struct SHEET;
void init_palette(void);
void set_palette(int start, int end, const unsigned char* rgb);
void init_screen8(char *vram, int xsize, int ysize);
void init_mouse_cursor8(char *mouse, char bgcolor);
void boxfill8(char *vram, int xsize, unsigned char color, int x0, int y0, int x1, int y1);
void putfont8(char *vram, int xsize, int x, int y, char color, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char color, const char *s);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, char color, char bcolor, char *s, int len);
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void make_textbox8(struct SHEET* sht, int x0, int y0, int sx, int sy, int c);
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

// dsctbl.c
// GDT
struct SEGMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};
// IDT
struct GATE_DESCRIPTOR {
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};

void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);

#define ADR_IDT       0x0026f800
#define LIMIT_IDT     0x000007ff
#define ADR_GDT       0x00270000
#define LIMIT_GDT     0x0000ffff
#define ADR_BOTPAK    0x00280000
#define LIMIT_BOTPAC  0x0007ffff
#define AR_DATA32_RW  0x4092
#define AR_CODE32_ER  0x409a
#define AR_INTGATE32  0x008e
#define AR_TSS32      0x0089

// int.c
void init_pic(void);
void inthandler21(int *esp);
void inthandler2c(int *esp);
void inthandler27(int *esp);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1


struct KEYBUF {
    unsigned char data[32];
    int next_r, next_w, len;
};

#define PORT_KEYDAT             0x0060

// fifo.c
struct  FIFO32 {
    int *buf;
    int p, q, size, free, flags;
    struct TASK *task;
};

#define FLAGS_OVERRUN           0x0001 // 溢出标志

void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_push(struct FIFO32 *fifo, int data);
int fifo32_pop(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);


// keyboard.c
void wait_KBC_sendready(void);
void init_keyboard(struct  FIFO32* fifo, int data0);

#define PORT_KEYDAT             0x0060
#define PORT_KEYSTA             0x0064
#define PORT_KEYCMD             0x0064
#define KEYSTA_SEND_NOTREADY    0x02
#define KEYCMD_WRITE_MODE       0x60
#define KBC_MODE                0x47

// mouse.c
struct MOUSE_DEC {
    unsigned char buf[3], phase;
    int x, y, btn;
};

void enable_mouse(struct FIFO32* fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

#define KEYCMD_SENDTO_MOUSE     0xd4
#define MOUSECMD_ENABLE         0xf4


// memory.c
#define EFLAGS_AC_BIT           0x00040000
#define CR0_CACHE_DISABLE       0x60000000
#define MEMMAN_ADDR             0x003c0000
#define MEMMAN_FREES            4096

// 可用信息
struct FREEINFO {
    unsigned int addr, size;
};

// 内存管理
struct MEMMAN {
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};


unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);


// sheet.h

struct SHTCTL;

struct SHEET {
    unsigned char *buf;
    int bxsize, bysize, vx0, vy0, col_inv, height, flags;
    struct SHTCTL *ctl;
};


#define MAX_SHEETS              256

struct SHTCTL {
    unsigned char *vram, *map;
    int xsize, ysize, top;
    struct SHEET *sheets[MAX_SHEETS];
    struct SHEET sheets0[MAX_SHEETS];
};

#define SHEET_USE               1

struct SHEET *sheet_alloc(struct SHTCTL *ctl);
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0) ;

void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0);

// timer.c
#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040


#define MAX_TIMER               500
#define TIMER_FLAGS_ALLOC       1
#define TIMER_FLAGS_USING       2

struct TIMER {
    struct TIMER *next;
    unsigned int timeout, flags;
    struct  FIFO32 *fifo;
    unsigned char data;
};

struct TIMERCTL {
    unsigned int count, next;
    struct TIMER *t0;
    struct TIMER timers0[MAX_TIMER];
};



void init_pit(void);
void inthandler20(int *esp);

void timer_settime(struct TIMER *timer, unsigned int timeout);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_free(struct TIMER *timer);
struct TIMER *timer_alloc(void);
void init_pit(void);

// mtask.c

#define MAX_TASKS 1000 // 最大任务量
#define TASK_GDT0 3    // 定义从gdt 的几号开始分配
#define MAX_TASKS_LV 100
#define MAX_TASKLEVELS 10

extern struct TIMER *task_timer;

struct TSS32 {
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    int es, cs, ss, ds, fs, gs;
    int ldtr, iomap;
};

struct TASK {
    int sel, flags; // sel 用来存放GDT的编号
    int level, priority; // 优先级
    struct TSS32 tss;
};

struct TASKLEVEL {
    int runing; // 正在运行的任务数量
    int now;    // 记录正在运行的任务
    struct TASK *tasks[MAX_TASKS_LV];
};

struct TASKCTL {
    int now_lv; // 现在活动中的level
    char lv_change; // 在下次切换任务时是否需要改变LEVEL
    struct TASKLEVEL level[MAX_TASKLEVELS];
    struct TASK task0[MAX_TASKS];
};

void mt_init(void);
void mt_taskswitch(void);


struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
struct TASK *task_now(void);
void task_add(struct TASK *task);
void task_remove(struct TASK *task);
void task_switchsub(void);

#endif
