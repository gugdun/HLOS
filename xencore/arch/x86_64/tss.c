#include <xencore/arch/x86_64/tss.h>

#include <xencore/xenio/tty.h>

__attribute__((aligned(16))) struct TSS tss;
__attribute__((aligned(16))) uint8_t kernel_stack[8192];  // main stack for kernel mode
__attribute__((aligned(16))) uint8_t df_stack[4096];      // IST1 (double fault)

void setup_tss() {
    tss.rsp0 = (uint64_t)(kernel_stack + sizeof(kernel_stack));
    tss.ist[0] = (uint64_t)(df_stack + sizeof(df_stack));
    tss.iopb_offset = sizeof(struct TSS);
    tty_printf("[TSS] Base: 0x%x\n", (uint64_t)&tss);
}
