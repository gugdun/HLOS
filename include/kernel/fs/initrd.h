#ifndef _INITRD_H
#define _INITRD_H

#include <stdint.h>

struct InitrdParams {
    uint64_t addr;
    uint64_t size;
};

void parse_initrd(struct InitrdParams *params);

#endif
