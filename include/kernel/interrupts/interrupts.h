#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <kernel/io/print.h>

static inline void disable_interrupts()
{
    __asm__ volatile ("cli");
#ifdef HLOS_DEBUG
    kprintf("[Interrupts] Disabled\n");
#endif
}

static inline void enable_interrupts()
{
    __asm__ volatile ("sti");
#ifdef HLOS_DEBUG
    kprintf("[Interrupts] Enabled\n");
#endif
}

#endif
