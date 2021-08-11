#include "console.h"
# include "debug.h"

int kern_entry()
{
    console_clear();

    console_write_color("Hello, this is BaGua OS kernel!\n", rc_black, rc_green);
    printk("******* Kernel-level printk function utility had supported. *******\n");
    panic("test");
    return 0;
}