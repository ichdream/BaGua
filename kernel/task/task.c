/**
 * 内核线程的实现
 * 
 * 
 * 
 */ 

#include "gdt.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "task.h"
// #include "sched.h"
#include "string.h"
#include "debug.h"
#include "print.h"

// 全局 pid 值
pid_t now_pid = 0;

// // 内核线程创建
// int32_t kernel_thread(int (*fn)(void *), void *arg)
// {
// 	struct task_struct *new_task = (struct task_struct *)kmalloc(STACK_SIZE);
// 	assert(new_task != NULL, "kern_thread: kmalloc error");

// 	// 将栈低端结构信息初始化为 0 
// 	bzero(new_task, sizeof(struct task_struct));

// 	new_task->state = TASK_RUNNABLE;
// 	new_task->stack = current;
// 	new_task->pid = now_pid++;
// 	new_task->mm = NULL;

// 	uint32_t *stack_top = (uint32_t *)((uint32_t)new_task + STACK_SIZE);

// 	\*(--stack_top) = (uint32_t)arg;
// 	\*(--stack_top) = (uint32_t)kthread_exit;
// 	\*(--stack_top) = (uint32_t)fn;

// 	new_task->context.esp = (uint32_t)new_task + STACK_SIZE - sizeof(uint32_t) * 3;

// 	// 设置新任务的标志寄存器未屏蔽中断，很重要
// 	new_task->context.eflags = 0x200;
// 	new_task->next = running_proc_head;
	
// 	// 找到当前进任务队列，插入到末尾
// 	struct task_struct *tail = running_proc_head;
// 	assert(tail != NULL, "Must init sched!");

// 	while (tail->next != running_proc_head) {
// 		tail = tail->next;
// 	}
// 	tail->next = new_task;

// 	return new_task->pid;
// }

void kthread_exit()
{
	register uint32_t val asm ("eax");

	printk("Thread exited with value %d\n", val);

	while (1);
}

# define PAGE_SIZE 4096

struct task_struct* main_thread;
struct list thread_ready_list;
struct list thread_all_list;
static struct list_elem* thread_tag;


/**
 * 任务切换.
 */ 
extern void switch_to(struct task_struct* cur, struct task_struct* next);

static void kernel_thread(thread_func* function, void* func_args);
static void make_main_thread();

static void kernel_thread(thread_func* function, void* func_args) {
    intr_enable();
    function(func_args);
}

static void make_main_thread() {
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);

    // main线程正在运行，故无需加到ready队列
    ASSERT(!list_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

/**
 * 获取当前线程PCB地址.
 */ 
struct task_struct* running_thread() {
    uint32_t esp;
    asm ("mov %%esp, %0" : "=g" (esp));
    return (struct task_struct*) (esp & 0xfffff000);
}

/**
 * 初始化线程栈.
 */ 
void thread_create(struct task_struct* pthread, thread_func function, void* func_args) {
	
    pthread->self_kstack -= sizeof(struct intr_stack);
    pthread->self_kstack -= sizeof(struct thread_stack);

    struct thread_stack* kthread_stack = (struct thread_stack*) pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_args = func_args;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->edi = kthread_stack->esi = 0;
}

/**
 * 初始化线程基本信息.
 */ 
void init_thread(struct task_struct* pthread, char* name, int prio) {
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);

    if (pthread == main_thread) {
		pthread->pid = 0;
        pthread->status = TASK_RUNNING;
    } else {
        pthread->status = TASK_READY;
		pthread->pid = now_pid++;
    }
	
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elaspsed_ticks = 0;
    pthread->pgdir = NULL;
    // PCB所在物理页的顶端地址
    pthread->self_kstack = (uint32_t*) ((uint32_t) pthread + PAGE_SIZE);
    pthread->stack_magic = 0x77777777;
}

/**
 * 创建线程.
 */ 
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_args) {
    struct task_struct* thread = get_kernel_pages(1);

    init_thread(thread, name, prio);
    thread_create(thread, function, func_args);

    ASSERT(!list_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);

    ASSERT(!list_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);
    return thread;
}

/**
 * 线程模块初始化.
 */ 
void thread_init() {
    put_str("Start to init thread...\n");
    list_init(&thread_all_list);
    list_init(&thread_ready_list);
    make_main_thread();
    put_str("Thread init done.\n");
}

/**
 * 线程调度.
 */ 
void schedule() {
    ASSERT(intr_get_status() == INTR_OFF);

    struct task_struct* cur_thread = running_thread();
    if (cur_thread->status == TASK_RUNNING) {
        // 时间片用完，重新加到就绪队列
        ASSERT(!list_find(&thread_ready_list, &cur_thread->general_tag));
        list_append(&thread_ready_list, &cur_thread->general_tag);
        cur_thread->ticks = cur_thread->priority;
        cur_thread->status = TASK_READY;
    }
    
    // 当前没有实现idle线程，所以要保证必须有可调度的线程存在
    ASSERT(!list_empty(&thread_ready_list));

    thread_tag = NULL;
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    
    switch_to(cur_thread, next);
}