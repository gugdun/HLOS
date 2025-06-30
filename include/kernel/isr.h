#ifndef _ISR_H
#define _ISR_H

struct __attribute__((packed)) interrupt_frame {
    uint64_t rip, cs, flags, rsp, ss;
};

#endif
