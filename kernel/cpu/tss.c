#include <kernel/cpu/tss.h>
#include <kernel/io/print.h>

__attribute__((aligned(16))) struct TSS tss;
__attribute__((aligned(16))) uint8_t ist_stack[4096];
__attribute__((aligned(16))) uint8_t df_stack[4096]; // double fault IST

void setup_tss() {
    tss.rsp0 = (uint64_t)(ist_stack + sizeof(ist_stack));
    tss.ist[0] = (uint64_t)(df_stack + sizeof(df_stack)); // IST1 = double fault
    tss.iopb_offset = sizeof(struct TSS);
    kprintf("[TSS] Base: 0x%x\n", (uint64_t)&tss);
}
