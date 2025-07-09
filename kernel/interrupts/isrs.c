#include <kernel/interrupts/isrs.h>
#include <kernel/io/ports.h>
#include <kernel/io/print.h>

// ==== Exception Handlers ====

__attribute__((interrupt)) void isr_divide_by_zero(struct interrupt_frame* frame) {
    kprintf("[#DE] Divide by zero at RIP=0x%x\n", frame->rip);
    while (1) __asm__ volatile ("cli; hlt");
}

__attribute__((interrupt)) void isr_general_protection(struct interrupt_frame* frame, uint64_t error_code) {
    kprintf("[#GP] General Protection Fault at RIP=0x%x, error=0x%x\n", frame->rip, error_code);
    while (1) __asm__ volatile ("cli; hlt");
}

__attribute__((interrupt)) void isr_page_fault(struct interrupt_frame* frame, uint64_t error_code) {
    uint64_t cr2;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
    kprintf("[#PF] Page Fault at RIP=0x%x, CR2=0x%x, error=0x%x\n", frame->rip, cr2, error_code);
    while (1) __asm__ volatile ("cli; hlt");
}

__attribute__((interrupt)) void isr_double_fault(struct interrupt_frame* frame, uint64_t error_code) {
    kprintf("[#DF] Double Fault at RIP=0x%x, error=0x%x\n", frame->rip, error_code);
    while (1) __asm__ volatile ("cli; hlt");
}

// ==== IRQ Handlers ====

__attribute__((interrupt)) void isr_timer(__attribute__((unused)) struct interrupt_frame* frame) {
    static uint64_t ticks = 0;
    ticks++;
    if (ticks % 100 == 0)
        kprintf("[Timer] Ticks: %u\n", ticks);
    outb(0x20, 0x20); // EOI to PIC
}

// ==== Default Handler (no error code) ====

__attribute__((interrupt)) void isr_default(struct interrupt_frame* frame) {
    kprintf("[Unhandled] Interrupt at RIP=0x%x\n", frame->rip);
    while (1) __asm__ volatile ("cli; hlt");
}

// ==== Default Handler (with error code) ====

__attribute__((interrupt)) void isr_default_err(struct interrupt_frame* frame, uint64_t error_code) {
    kprintf("[Unhandled] Interrupt at RIP=0x%x, error=0x%x\n", frame->rip, error_code);
    while (1) __asm__ volatile ("cli; hlt");
}
