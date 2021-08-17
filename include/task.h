/**
 * 内核线程的接口定义
 * 
 * 
 * 
 */

#ifndef INCLUDE_TASK_H_
#define INCLUDE_TASK_H_

#include "types.h"
#include "pmm.h"
#include "vmm.h"
#include "list.h"


/**
 * 自定义通用函数类型.
 */ 
typedef void thread_func(void*);

// 进程状态描述
// typedef
// enum task_state {
// 	TASK_UNINIT = 0, 	// 未初始化
// 	TASK_SLEEPING = 1, 	// 睡眠中
// 	TASK_RUNNABLE = 2, 	// 可运行(也许正在运行)
// 	TASK_ZOMBIE = 3, 	// 僵尸状态
// } task_state;
enum task_status {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITTING,
    TASK_HANGING,
    TASK_DIED
};


// // 内核线程的上下文切换保存的信息
// struct context {
// 	uint32_t esp;
// 	uint32_t ebp;
// 	uint32_t ebx;
// 	uint32_t esi;
// 	uint32_t edi;
// 	uint32_t eflags;
// };
/**
 * 中断栈.
 */
struct intr_stack {
    uint32_t vec_no;
    uint32_t edi;  
    uint32_t esi;  
    uint32_t ebp;  
    uint32_t esp_dummy;  
    uint32_t ebx;  
    uint32_t edx;  
    uint32_t ecx;  
    uint32_t eax;  
    uint32_t gs;  
    uint32_t fs;  
    uint32_t es;  
    uint32_t ds;

    // 下面的属性由CPU从低特权级进入高特权级时压入
    uint32_t err_code;  
    void (*eip) (void);
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;
};

struct thread_stack {
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    // 第一次执行时指向待调用的函数kernel_thread，其它时候指向switch_to的返回地址.
    void (*eip) (thread_func* func, void* func_args);

    void (*unused_retaddr);
    thread_func* function;
    void* func_args;
};
// // 进程内存地址结构
// struct mm_struct {
// 	pgd_t *pgd_dir; 	// 进程页表
// };

// // 进程控制块 PCB, 对于线程，将mm_struct设置为NULL
// struct task_struct {
// 	volatile task_state state; 	// 进程当前状态
// 	pid_t 	 pid; 			// 进程标识符
// 	void  	*stack; 		// 进程的内核栈地址
// 	struct mm_struct *mm; 		// 当前进程的内存地址映像
// 	struct context context; 	// 进程切换需要的上下文信息
// 	struct task_struct *next; 	// 链表指针
// };

/**
 * PCB，进程或线程的控制块.
 */ 
struct task_struct {
    // 内核栈
    uint32_t* self_kstack;
	pid_t pid;
    enum task_status status;
    char name[16];
    uint8_t priority;
    // 当前线程可以占用的CPU嘀嗒数
    uint8_t ticks;
    // 此任务占用的总嘀嗒数
    uint32_t elaspsed_ticks;
    // 可执行队列节点
    struct list_elem general_tag;
    // 所有不可运行线程队列节点
    struct list_elem all_list_tag;
    uint32_t* pgdir;
    uint32_t stack_magic;
};



// 全局 pid 值
// extern pid_t now_pid;

// // 内核线程创建
// int32_t kernel_thread(int (*fn)(void *), void *arg);

// // 线程退出函数
// void kthread_exit();

struct task_struct* running_thread();
void thread_create(struct task_struct* pthread, thread_func function, void* func_args);
void init_thread(struct task_struct* pthread, char* name, int prio);
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_args);
void schedule();
void thread_init();

#endif 	// INCLUDE_TASK_H_