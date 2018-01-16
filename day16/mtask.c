#include "bootpack.h"


struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman)  {
    int i;
    struct TASK *task;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
    taskctl = (struct TASKCTL *)memman_alloc_4k(memman, sizeof(struct TASKCTL));
    for (i = 0; i < MAX_TASKS; i++)  {
	taskctl->task0[i].flags = 0;
	taskctl->task0[i].sel = (TASK_GDT0 + i) * 8;
	set_segmdesc(gdt + TASK_GDT0 + i, 103, (int)&taskctl->task0[i].tss, AR_TSS32);
    }

    for (i = 0; i < MAX_TASKLEVELS; i++)  {
	taskctl->level[i].runing = 0;
	taskctl->level[i].now = 0;
    }
    task = task_alloc();
    task->flags = 2; // 活动中的标志
    task->priority = 2;
    task_add(task);
    task_switchsub(); // level 设置
    load_tr(task->sel);
    task_timer = timer_alloc();
    timer_settime(task_timer, task->priority);
    return task;
}

struct TASK *task_alloc(void)  {
    int i;
    struct TASK *task;
    for (i = 0; i < MAX_TASKS; i++)  {
	if (taskctl->task0[i].flags == 0)  {
	    task = &taskctl->task0[i];
	    task->flags = 1;
	    task->tss.eflags = 0x00000202;
	    task->tss.eax = 0;
	    task->tss.ecx = 0;
	    task->tss.edx = 0;
	    task->tss.ebx = 0;
	    task->tss.ebp = 0;
	    task->tss.esi = 0;
	    task->tss.edi = 0;
	    task->tss.es = 0;
	    task->tss.ds = 0;
	    task->tss.fs = 0;
	    task->tss.gs = 0;
	    task->tss.ldtr = 0;
	    task->tss.iomap = 0x40000000;
	    return task;
	}
    }
    return 0;
}

void task_run(struct TASK *task, int level, int priority)  {
    if (level < 0)  {
	level = task->level;
    }
    
    if (priority > 0)  {
	task->priority = priority;
    }
    if (task->flags == 2 && task->level != level)  {
	task_remove(task);
    }
    if (task->flags != 2)  {
	task->level = level;
	task_add(task);
    }
    taskctl->lv_change = 1;
    return;
}

void task_switch(void)  {
    struct TASKLEVEL *t1 = &taskctl->level[taskctl->now_lv];
    
    struct TASK *new_task, *now_task = t1->tasks[t1->now];
    
    t1->now++;
    if (t1->now == t1->runing)  {
	t1->now = 0;
    }
    if (taskctl->lv_change != 0)  {
	task_switchsub();
	t1 = &taskctl->level[taskctl->now_lv];
    }
    new_task = t1->tasks[t1->now];
    
    timer_settime(task_timer, new_task->priority);
    if (new_task != now_task)  {
	farjmp(0, new_task->sel);
    }
    return;
}

void task_sleep(struct TASK *task)  {
    struct TASK *now_task;
    if (task->flags == 2)  {
	// 如果处于活动状态
	now_task = task_now();
	task_remove(task);
	if (task == now_task)  {
	    task_switchsub();
	    now_task = task_now();
	    farjmp(0, now_task->sel);
	}
    }
    return;
 }

struct TASK *task_now(void)  {
    struct TASKLEVEL *t1 = &taskctl->level[taskctl->now_lv];
    return t1->tasks[t1->now];
}

void task_add(struct TASK *task)  {
    struct TASKLEVEL *t1 = &taskctl->level[task->level];
    t1->tasks[t1->runing] = task;
    t1->runing++;
    task->flags = 2;
    return;
}

void task_remove(struct TASK *task)  {
    int i;
    struct TASKLEVEL *t1 = &taskctl->level[task->level];
    for (i = 0; i < t1->runing; i++)  {
	if (t1->tasks[i] == task)  {
	    break;
	}
    }
    t1->runing--;
    if (i < t1->now)  {
	t1->now--;
    }
    if (t1->now >= t1->runing)  {
	t1->now = 0;
    }
    task->flags = 1;
    for(;i < t1->runing; i++)  {
	t1->tasks[i] = t1->tasks[i + 1];
    }
    return;
}

void task_switchsub(void)  {
    int i;
    for (i = 0; i < MAX_TASKLEVELS; i++)  {
	if (taskctl->level[i].runing > 0)  {
	    break;
	}
    }
    taskctl->now_lv = i;
    taskctl->lv_change = 0;
    return;
}
