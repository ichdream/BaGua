#include "gdt.h"
#include "console.h"
#include "debug.h"

int kern_entry()
{
    console_clear();
    console_write_color("GDT table init...\n", rc_black, rc_green);
    init_gdt();

    console_write_color("Hello, this is BaGua OS kernel， V0.01!\n", rc_black, rc_green);
    printk("******* Kernel-level printk function utility had supported. *******\n");
    print_cur_status();

    printk("kerbel debug:\n");
    panic("test");
    
    return 0;
}
