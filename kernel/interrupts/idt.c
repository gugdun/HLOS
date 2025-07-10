#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/isrs.h>
#include <kernel/io/tty.h>
#include <kernel/io/ports.h>
#include <kernel/timer/pit.h>

__attribute__((aligned(16))) struct IDTEntry idt[IDT_ENTRIES];
struct IDTPtr idt_ptr;

void set_idt_entry(int vector, void* isr, uint8_t ist) {
    uint64_t addr = (uint64_t)isr;
    idt[vector].offset_low  = addr & 0xFFFF;
    idt[vector].selector    = 0x08; // Kernel code segment
    idt[vector].ist         = ist & 0x7;
    idt[vector].type_attr   = 0x8E; // Present, DPL=0, interrupt gate
    idt[vector].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;
}

void setup_idt() {
    for (int i = 0; i < IDT_ENTRIES; ++i) {
        // Exceptions with error codes
        if (i == 8 || i == 10 || i == 11 || i == 12 || i == 13 || i == 14 || i == 17 || i == 21) {
            set_idt_entry(i, (void*)isr_default_err, 0);
        } else {
            set_idt_entry(i, (void*)isr_default, 0);
        }
    }

    // Known exception overrides
    set_idt_entry(0, (void*)isr_divide_by_zero, 0);       // #DE
    set_idt_entry(8, (void*)isr_double_fault, 1);         // #DF (with IST)
    set_idt_entry(13, (void*)isr_general_protection, 0);  // #GP
    set_idt_entry(14, (void*)isr_page_fault, 0);          // #PF

    // IRQ0 (PIT)
    set_idt_entry(IRQ_BASE + 0, (void*)isr_timer, 0);

    // Fill the rest with default handler
    for (int i = IRQ_BASE + 1; i < IDT_ENTRIES; ++i) {
        set_idt_entry(i, (void*)isr_default, 0);
    }

    // Load IDT
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint64_t)&idt;

    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
    tty_printf("[IDT] Base: 0x%x\n", idt_ptr.base);
}
