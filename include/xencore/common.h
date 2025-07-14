#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <xencore/io/tty.h>

static inline void disable_interrupts()
{
#ifdef ARCH_x86_64
    __asm__ volatile ("cli");
#endif
#ifdef HLOS_DEBUG
    tty_printf("[Interrupts] Disabled\n");
#endif
}

static inline void enable_interrupts()
{
#ifdef ARCH_x86_64
    __asm__ volatile ("sti");
#endif
#ifdef HLOS_DEBUG
    tty_printf("[Interrupts] Enabled\n");
#endif
}

static inline void halt()
{
#ifdef ARCH_x86_64
    __asm__ volatile ("hlt");
#endif
}

#endif
