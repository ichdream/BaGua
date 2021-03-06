#include "console.h"
#include "string.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "task.h"
// #include "sched.h"
#include "common.h"

// 内核初始化函数
void kern_init();

// 开启分页机制之后的 Multiboot 数据指针
multiboot_t *glb_mboot_ptr;

// 开启分页机制之后的内核栈
char kern_stack[STACK_SIZE]  __attribute__ ((aligned(16)));

// 内核栈的栈顶
uint32_t kern_stack_top;

// 内核使用的临时页表和页目录
// 该地址必须是页对齐的地址，内存 0-640KB 肯定是空闲的
__attribute__((section(".init.data"))) pgd_t *pgd_tmp  = (pgd_t *)0x1000;
__attribute__((section(".init.data"))) pgd_t *pte_low  = (pgd_t *)0x2000;
__attribute__((section(".init.data"))) pgd_t *pte_high = (pgd_t *)0x3000;
__attribute__((section(".init.data"))) pgd_t *pte_hign_last = (pgd_t *)(0x3000 + 1023 * 0x1000);

// 内核入口函数
__attribute__((section(".init.text"))) void kern_entry()
{
	pgd_tmp[0] = (uint32_t)pte_low | PAGE_PRESENT | PAGE_WRITE;
	for(int i = 769; i < 1023; i++) {
		pgd_t *pte_hign_tmp = i * 0x1000 + pte_high;
		pgd_tmp[i] = (uint32_t)pte_hign_tmp | PAGE_PRESENT | PAGE_WRITE;
	}
	pgd_tmp[768] = (uint32_t)pte_high | PAGE_PRESENT | PAGE_WRITE;
	pgd_tmp[1023] = (uint32_t)pgd_tmp | PAGE_PRESENT | PAGE_WRITE;

	// 映射内核虚拟地址 4MB 到物理地址的前 4MB
	int i;
	for (i = 0; i < 1024; i++) {
		pte_low[i] = (i << 12) | PAGE_PRESENT | PAGE_WRITE;
	}

	// 映射 0x00000000-0x00400000 的物理地址到虚拟地址 0xC0000000-0xC0400000
	for (i = 0; i < 1024; i++) {
		pte_high[i] = (i << 12) | PAGE_PRESENT | PAGE_WRITE;
	}
		
	// 设置临时页目录表
	asm volatile ("mov %0, %%cr3" : : "r" (pgd_tmp));

	uint32_t cr0;

	// 启用分页，将 cr0 寄存器的分页位置为 1 就好
	asm volatile ("mov %%cr0, %0" : "=r" (cr0));
	cr0 |= 0x80000000;
	asm volatile ("mov %0, %%cr0" : : "r" (cr0));
	
	// 切换内核栈
	kern_stack_top = ((uint32_t)kern_stack + STACK_SIZE);
	asm volatile ("mov %0, %%esp\n\t"
			"xor %%ebp, %%ebp" : : "r" (kern_stack_top));

	// 更新全局 multiboot_t 指针
	glb_mboot_ptr = mboot_ptr_tmp + PAGE_OFFSET;

	// 调用内核初始化函数
	kern_init();
}


void k_thread_function_a(void*);
void k_thread_function_b(void*);


void k_thread_function_a(void* args) {
    // 这里必须是死循环，否则执行流并不会返回到main函数，所以CPU将会放飞自我，出发6号未知操作码异常
	int cnt = 3;
	while(cnt--) {
		put_str((char*) args);
	}
    while (1) {
		asm volatile ("hlt");
    }
}

void k_thread_function_b(void* args) {
	int cnt = 5;
	while(cnt--) {
		put_str((char*) args);
	}
    while (1) {
		asm volatile ("hlt");
    }
}

void kern_init()
{
	init_debug();
	init_gdt();
	init_idt();

	console_clear();
	printk_color(rc_black, rc_green, "BaGua OS kernel is running...\n\n");

	printk("memory info:\nlower memory amount, in kilobytes:\t0x%08X\nupper memory amount, in kilobytes:\t0x%08X\n", glb_mboot_ptr->mem_lower, glb_mboot_ptr->mem_upper);

	init_timer(200);

	// 开启中断
	// asm volatile ("sti");

	printk("kernel in memory start: 0x%08X\n", kern_start);
	printk("kernel in memory end:   0x%08X\n", kern_end);
	printk("kernel in memory used:   %d KB\n\n", (kern_end - kern_start) / 1024);
	
	show_memory_map();
	// init_pmm();
	// init_vmm();
	// init_heap();

	// printk_color(rc_black, rc_red, "\nThe Count of Physical Memory Page is: %u\n\n", phy_page_count);

	// uint32_t allc_addr = NULL;
	// printk_color(rc_black, rc_light_brown, "Test Physical Memory Alloc :\n");
	// allc_addr = pmm_alloc_page();
	// printk_color(rc_black, rc_light_brown, "Alloc Physical Addr: 0x%08X\n", allc_addr);
	// allc_addr = pmm_alloc_page();
	// printk_color(rc_black, rc_light_brown, "Alloc Physical Addr: 0x%08X\n\n", allc_addr);
	
	// test_heap();

	// init_sched();

	put_str("I am kernel.\n");
    // init_all();
	mem_init();
	thread_init();

	struct task_struct *thread_a = thread_start("k_thread_a", 31, k_thread_function_a, "threadA ");
	printk("thread_a's pid: %d\n", thread_a->pid);
    struct task_struct *thread_b = thread_start("k_thread_b", 8, k_thread_function_b, "threadB ");
	printk("thread_b's pid: %d\n", thread_b->pid);
	intr_enable();
	int cnt = 5;
	while(cnt--) {
		put_str("main ");
	}
	
	while (1) {
		asm volatile ("hlt");
	}
}