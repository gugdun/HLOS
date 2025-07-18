#include <xencore/arch/x86_64/isrs.h>
#include <xencore/arch/x86_64/ports.h>

#include <xencore/graphics/framebuffer.h>
#include <xencore/xenio/tty.h>
#include <xencore/common.h>
#include <xencore/gman/gman.h>

// ==== Exception Handlers ====

__attribute__((interrupt)) void isr_divide_by_zero(struct interrupt_frame* frame)
{
    disable_interrupts();
    tty_printf("[#DE] Divide by zero at RIP=0x%x\n", frame->rip);
    if (fb_is_double_buffered()) fb_present();
    while (1) __asm__ volatile ("hlt");
}

__attribute__((interrupt)) void isr_general_protection(struct interrupt_frame* frame, uint64_t error_code)
{
    disable_interrupts();
    tty_printf("[#GP] General Protection Fault at RIP=0x%x, error=0x%x\n", frame->rip, error_code);
    if (fb_is_double_buffered()) fb_present();
    while (1) __asm__ volatile ("hlt");
}

__attribute__((interrupt)) void isr_page_fault(struct interrupt_frame* frame, uint64_t error_code)
{
    disable_interrupts();
    uint64_t cr2;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
    tty_printf("[#PF] Page Fault at RIP=0x%x, CR2=0x%x, error=0x%x\n", frame->rip, cr2, error_code);
    if (fb_is_double_buffered()) fb_present();
    while (1) __asm__ volatile ("hlt");
}

__attribute__((interrupt)) void isr_double_fault(struct interrupt_frame* frame, uint64_t error_code)
{
    disable_interrupts();
    tty_printf("[#DF] Double Fault at RIP=0x%x, error=0x%x\n", frame->rip, error_code);
    if (fb_is_double_buffered()) fb_present();
    while (1) __asm__ volatile ("hlt");
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
    disable_interrupts();
    tty_printf("[Unhandled] Interrupt at RIP=0x%x\n", frame->rip);
    if (fb_is_double_buffered()) fb_present();
    while (1) __asm__ volatile ("hlt");
}

// ==== Default Handler (with error code) ====

__attribute__((interrupt)) void isr_default_err(struct interrupt_frame* frame, uint64_t error_code)
{
    disable_interrupts();
    tty_printf("[Unhandled] Interrupt at RIP=0x%x, error=0x%x\n", frame->rip, error_code);
    if (fb_is_double_buffered()) fb_present();
    while (1) __asm__ volatile ("hlt");
}
