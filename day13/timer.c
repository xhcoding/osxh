#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(void) {
    int i;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    timerctl.next = 0xffffffff;
    for (i = 0; i < MAX_TIMER; i++) {
	timerctl.timers0[i].flags = 0; // 未使用
    }
    struct TIMER *t = timer_alloc(); // 取得一个
    t->timeout = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next = 0; // 末尾
    timerctl.t0 = t;
    timerctl.next = 0xffffffff;
}

struct TIMER *timer_alloc(void) {
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
	if (timerctl.timers0[i].flags == 0) {
	    timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
	    return &timerctl.timers0[i];
	}
    }
    return 0;
}

void timer_free(struct TIMER *timer) {
    timer->flags = 0; // 未使用
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data) {
    timer->fifo = fifo;
    timer->data = data;
}

void inthandler20(int *esp) {
    struct TIMER *timer;
    io_out8(PIC0_OCW2, 0x60); // IRQ-00信号接受完了的信息通知给PIC
    timerctl.count++;

    if (timerctl.next > timerctl.count) {
	return;
    }
    timer = timerctl.t0; // 把最前面的地址赋值给timer
    
    for ( ; ;) {
	if (timer->timeout > timerctl.count) {
	    break;
	}
	// 超时
	timer->flags = TIMER_FLAGS_ALLOC;
	fifo32_push(timer->fifo, timer->data);
	timer = timer->next;
    }

    // 新移位
    timerctl.t0 = timer;
    
    // 设置timerctl.next
    timerctl.next = timerctl.t0->timeout;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
    int e;
    struct TIMER *t, *s;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    e = io_load_eflags();
    io_cli();
    t = timerctl.t0;
    if (timer->timeout <= t->timeout) {
	// 插入最前面
	timerctl.t0 = timer;
	timer->next = t;
	timerctl.next = timer->timeout;
	io_store_eflags(e);
	return;
    }
    // 寻找插入位置
    for (; ; ) {
	s = t;
	t = t->next;
	if (timer->timeout <= t->timeout) {
	    // 插入到s和t之间
	    s->next = timer;
	    timer->next = t;
	    io_store_eflags(e);
	    return;
	}
    }
}

