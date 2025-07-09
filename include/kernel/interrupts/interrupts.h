#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <kernel/io/print.h>

static inline void disable_interrupts()
{
    __asm__ volatile ("cli");
    kprintf("[Interrupts] Disabled\n");
}

static inline void enable_interrupts()
{
    __asm__ volatile ("sti");
    kprintf("[Interrupts] Enabled\n");
}

#endif
