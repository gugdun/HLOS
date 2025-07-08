#ifndef _IDT_H
#define _IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256
#define IRQ_BASE    32

struct __attribute__((packed)) interrupt_frame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
};

typedef void (*isr_t)(struct interrupt_frame*);
typedef void (*isr_err_t)(struct interrupt_frame*, uint64_t);

struct __attribute__((packed)) IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
};

struct __attribute__((packed)) IDTPtr {
    uint16_t limit;
    uint64_t base;
};

void set_idt_entry(int vector, void* isr, uint8_t ist);
void setup_idt(void);

#endif
