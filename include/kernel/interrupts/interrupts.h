#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <kernel/io/tty.h>

static inline void disable_interrupts()
{
    __asm__ volatile ("cli");
#ifdef HLOS_DEBUG
    tty_printf("[Interrupts] Disabled\n");
#endif
}

static inline void enable_interrupts()
{
    __asm__ volatile ("sti");
#ifdef HLOS_DEBUG
    tty_printf("[Interrupts] Enabled\n");
#endif
}

#endif
