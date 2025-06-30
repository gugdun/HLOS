#include <kernel/idt.h>
#include <kernel/isr.h>

__attribute__((aligned(16))) struct IDTEntry idt[IDT_ENTRIES];
struct IDTPtr idt_ptr;

__attribute__((naked)) void isr_divide_by_zero(struct interrupt_frame* frame) {
    (void *)frame;
    while (1) __asm__ volatile ("cli; hlt");
}

__attribute__((naked)) void isr_dummy(struct interrupt_frame* frame) {
    (void *)frame;
    while (1) __asm__ volatile ("cli; hlt");
}

void set_idt_entry(int vector, void (*isr)(), uint8_t ist) {
    uint64_t addr = (uint64_t)isr;
    idt[vector].offset_low = addr & 0xFFFF;
    idt[vector].selector = 0x08;
    idt[vector].ist = ist & 0x07;
    idt[vector].type_attr = 0x8E;
    idt[vector].offset_mid = (addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero = 0;
}

void setup_idt() {
    for (int i = 0; i < IDT_ENTRIES; ++i)
        set_idt_entry(i, (void*)isr_dummy, 0);

    set_idt_entry(0, (void*)isr_divide_by_zero, 0); // #DE
    set_idt_entry(8, (void*)isr_dummy, 1);          // #DF w/ IST

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;

    asm volatile ("lidt %0" : : "m"(idt_ptr));
    kprint("[IDT] Initialized!\n");
}
