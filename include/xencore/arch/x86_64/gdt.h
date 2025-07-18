#ifndef _GDT_H
#define _GDT_H

#include <stdint.h>

#define GDT_ENTRIES 7   // null + kcode + kdata + ucode + udata + tss_low + tss_high

enum {
    GDT_NULL = 0,
    GDT_KERNEL_CODE,
    GDT_KERNEL_DATA,
    GDT_USER_CODE,
    GDT_USER_DATA,
    GDT_TSS_LOW,
    GDT_TSS_HIGH
};

struct __attribute__((packed)) GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
};

struct __attribute__((packed)) TSSDescriptorHigh {
    uint32_t base_upper;
    uint32_t reserved;
};

struct __attribute__((packed)) GDTPtr {
    uint16_t limit;
    uint64_t base;
};

void setup_gdt();

#endif
