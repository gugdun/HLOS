#ifndef _ISRS_H
#define _ISRS_H

#include <stdint.h>

struct __attribute__((packed)) interrupt_frame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
};

typedef void (*isr_t)(struct interrupt_frame*);
typedef void (*isr_err_t)(struct interrupt_frame*, uint64_t);

void isr_divide_by_zero(struct interrupt_frame* frame);
void isr_general_protection(struct interrupt_frame* frame, uint64_t error_code);
void isr_page_fault(struct interrupt_frame* frame, uint64_t error_code);
void isr_double_fault(struct interrupt_frame* frame, uint64_t error_code);
void isr_timer(__attribute__((unused)) struct interrupt_frame* frame);
void isr_default(struct interrupt_frame* frame);
void isr_default_err(struct interrupt_frame* frame, uint64_t error_code);

#endif
