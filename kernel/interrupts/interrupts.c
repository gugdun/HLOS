#include <kernel/io/print.h>

void disable_interrupts()
{
    __asm__ volatile ("cli");
    kprintf("[Interrupts] Disabled\n");
}

void enable_interrupts()
{
    __asm__ volatile ("sti");
    kprintf("[Interrupts] Enabled\n");
}
