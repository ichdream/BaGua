#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "console.h"
#include "debug.h"

int kern_entry()
{
    console_clear();
    console_write_color("GDT table init...\n", rc_black, rc_green);
    init_gdt();
    console_write_color("IDT table init...\n", rc_black, rc_green);
    init_idt();

    // asm volatile ("int $0x3");
	// asm volatile ("int $0x4");

    console_write_color("Hello, this is BaGua OS kernel， V0.02!\n", rc_black, rc_green);
    printk("******* Kernel-level printk function utility had supported. *******\n");
    print_cur_status();

    console_write_color("timer interrupt...!\n", rc_black, rc_green);
    init_timer(200);

	// 开启中断
	asm volatile ("sti");

    // printk("kerbel debug:\n");
    // panic("test");
    
    return 0;
}
