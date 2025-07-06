#include <kernel/io/print.h>

void disable_interrupts()
{
    asm volatile ("cli");
    kprint("[Interrupts] Disabled\n");
}

void enable_interrupts()
{
    asm volatile ("sti");
    kprint("[Interrupts] Enabled\n");
}
