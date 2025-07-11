#include <kernel/interrupts/isrs.h>
#include <kernel/io/ports.h>
#include <kernel/io/tty.h>

// ==== Exception Handlers ====

__attribute__((interrupt)) void isr_divide_by_zero(struct interrupt_frame* frame)
{
    tty_printf("[#DE] Divide by zero at RIP=0x%x\n", frame->rip);
    while (1) __asm__ volatile ("cli; hlt");
}

__attribute__((interrupt)) void isr_general_protection(struct interrupt_frame* frame, uint64_t error_code)
{
    tty_printf("[#GP] General Protection Fault at RIP=0x%x, error=0x%x\n", frame->rip, error_code);
    while (1) __asm__ volatile ("cli; hlt");
}

__attribute__((interrupt)) void isr_page_fault(struct interrupt_frame* frame, uint64_t error_code)
{
    uint64_t cr2;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
    tty_printf("[#PF] Page Fault at RIP=0x%x, CR2=0x%x, error=0x%x\n", frame->rip, cr2, error_code);
    while (1) __asm__ volatile ("cli; hlt");
}

__attribute__((interrupt)) void isr_double_fault(struct interrupt_frame* frame, uint64_t error_code)
{
    tty_printf("[#DF] Double Fault at RIP=0x%x, error=0x%x\n", frame->rip, error_code);
    while (1) __asm__ volatile ("cli; hlt");
}

// ==== IRQ Handlers ====

volatile uint64_t sleep_countdown = 0;

__attribute__((interrupt)) void isr_timer(__attribute__((unused)) struct interrupt_frame* frame)
{
    if (sleep_countdown > 0) {
        sleep_countdown--;
    }
    outb(0x20, 0x20); // EOI to PIC
}

// ==== Default Handler (no error code) ====

__attribute__((interrupt)) void isr_default(struct interrupt_frame* frame)
{
    tty_printf("[Unhandled] Interrupt at RIP=0x%x\n", frame->rip);
    while (1) __asm__ volatile ("cli; hlt");
}

// ==== Default Handler (with error code) ====

__attribute__((interrupt)) void isr_default_err(struct interrupt_frame* frame, uint64_t error_code)
{
    tty_printf("[Unhandled] Interrupt at RIP=0x%x, error=0x%x\n", frame->rip, error_code);
    while (1) __asm__ volatile ("cli; hlt");
}
