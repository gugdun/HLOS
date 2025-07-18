#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#define USER_STACK_TOP   0x00007FFFFFFFE000ULL
#define USER_STACK_SIZE  (8 * 1024 * 1024) // 8 MiB

#include <stdint.h>

#include <xencore/hazardous/xenloader.h>

struct HazardousContext {
    uint64_t *page_table;
    uint64_t entry_point;
    uint64_t stack_top;
};

struct HazardousContext *setup_hazardous_environment(Elf64 *elf);
void enter_hazardous_environment(struct HazardousContext *ctx);

#endif
