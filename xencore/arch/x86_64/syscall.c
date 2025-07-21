#include "xencore/xenmem/mem_entry.h"
#include <string.h>

#include <xencore/arch/x86_64/syscall.h>
#include <xencore/arch/x86_64/paging.h>
#include <xencore/arch/x86_64/segments.h>
#include <xencore/arch/x86_64/msr.h>

#include <xencore/xenmem/xenalloc.h>
#include <xencore/xenio/tty.h>
#include <xencore/common.h>

#define MSR_EFER    0xC0000080
#define MSR_STAR    0xC0000081
#define MSR_LSTAR   0xC0000082
#define MSR_FMASK   0xC0000084
#define EFER_SCE    (1 << 0)    // Enable SYSCALL/SYSRET

uint64_t syscall_stack_top = 0;

uint64_t syscall_dispatch(
    uint64_t num,
    uint64_t a1, uint64_t a2, uint64_t a3,
    uint64_t a4, uint64_t a5, uint64_t a6
) {
#ifdef HLOS_DEBUG
    tty_printf(
        "[Syscall] num=0x%x a1=0x%x a2=0x%x a3=0x%x a4=0x%x a5=0x%x a6=0x%x\n",
        (uint64_t)num, a1, a2, a3, a4, a5, a6
    );
#endif

    switch (num) {
        case SYS_WRITE: {
            int fd = (int)a1;
            const char *buf = (const char*)a2;
            size_t len = (size_t)a3;
            if (fd == 1) {
                for (size_t i = 0; i < len; ++i) tty_putc(buf[i]);
                return len;
            }
            return (uint64_t)-1;
        }

        case SYS_EXIT: {
            int code = (int)a1;
            tty_printf("[Syscall] exit(%d)\n", code);
            while (1) __asm__ volatile("hlt");
        }

        default:
            tty_printf("[Syscall] Unknown num=0x%x\n", (uint64_t)num);
            return (uint64_t)-1;
    }
}

__attribute__((naked)) void syscall_entry(void)
{
    __asm__ volatile (
        "cli\n\t"

        "mov   %rsp, %r15\n\t"                /* r15 = user_rsp */

        "mov   syscall_stack_top(%rip), %rsp\n\t"
        "sub   $144, %rsp\n\t"

        /* Save user state */
        "mov   %r11,   0(%rsp)\n\t"
        "mov   %rcx,   8(%rsp)\n\t"
        "mov   %r15,  16(%rsp)\n\t"           /* user_rsp    */
        "mov   %rax,  24(%rsp)\n\t"           /* user_rax (num) */
        "mov   %rbx,  32(%rsp)\n\t"
        "mov   %rcx,  40(%rsp)\n\t"           /* debug */
        "mov   %rdx,  48(%rsp)\n\t"           /* a3 */
        "mov   %rsi,  56(%rsp)\n\t"           /* a2 */
        "mov   %rdi,  64(%rsp)\n\t"           /* a1 */
        "mov   %rbp,  72(%rsp)\n\t"
        "mov   %r8,   80(%rsp)\n\t"           /* a5 */
        "mov   %r9,   88(%rsp)\n\t"           /* a6 */
        "mov   %r10,  96(%rsp)\n\t"           /* a4 */
        "mov   %r11, 104(%rsp)\n\t"           /* debug */
        "mov   %r12, 112(%rsp)\n\t"
        "mov   %r13, 120(%rsp)\n\t"
        "mov   %r14, 128(%rsp)\n\t"
        "mov   %r15, 136(%rsp)\n\t"           /* user_r15 (actually user_rsp) */

        /* Build C call args. */
        "mov   24(%rsp), %rdi\n\t"            /* num */
        "mov   64(%rsp), %rsi\n\t"            /* a1 */
        "mov   56(%rsp), %rdx\n\t"            /* a2 */
        "mov   48(%rsp), %rcx\n\t"            /* a3 */
        "mov   96(%rsp), %r8\n\t"             /* a4 */
        "mov   80(%rsp), %r9\n\t"             /* a5 */
        "mov   88(%rsp), %rax\n\t"            /* a6 tmp */
        "push  %rax\n\t"                      /* 7th arg */

        "sti\n\t"
        "call  syscall_dispatch\n\t"
        "cli\n\t"

        "add   $8, %rsp\n\t"                  /* drop a6 */

        /* Restore user GPRs (except %rax/%rcx/%r11). */
        "mov   32(%rsp), %rbx\n\t"
        "mov   72(%rsp), %rbp\n\t"
        "mov  112(%rsp), %r12\n\t"
        "mov  120(%rsp), %r13\n\t"
        "mov  128(%rsp), %r14\n\t"
        "mov  136(%rsp), %r15\n\t"
        "mov   64(%rsp), %rdi\n\t"
        "mov   56(%rsp), %rsi\n\t"            /* restore a2 */
        "mov   48(%rsp), %rdx\n\t"
        "mov   96(%rsp), %r10\n\t"
        "mov   80(%rsp), %r8\n\t"
        "mov   88(%rsp), %r9\n\t"

        /* Load user_rflags & user_rip. */
        "mov    0(%rsp), %r11\n\t"
        "mov    8(%rsp), %rcx\n\t"

        /* Switch directly back to the saved user stack */
        "mov   16(%rsp), %rsp\n\t"

        "sysretq\n\t"
    );
}

void setup_syscall(void)
{
    // Allocate syscall stack
    syscall_stack_top = (uint64_t)xen_alloc_aligned(SYSCALL_STACK_SIZE);
    if (!syscall_stack_top) {
        tty_printf("[Syscall] Failed to allocate syscall stack\n");
        while (1) halt();
    }
    memset((void *)syscall_stack_top, 0, SYSCALL_STACK_SIZE);

    // Map syscall stack
    syscall_stack_top = virt_to_phys(syscall_stack_top);
    struct MemoryMapEntry entry = {
        .type = KernelData,
        .pad = 0,
        .physical_start = syscall_stack_top,
        .virtual_start = syscall_stack_top,
        .size_pages = SYSCALL_STACK_SIZE / PAGE_SIZE_4KB,
        .attribute = PAGE_RW | PAGE_PRESENT
    };
    map_identity(&entry);
    syscall_stack_top += SYSCALL_STACK_SIZE; // Point to top of stack

    // Enable syscall in EFER
    uint64_t efer = rdmsr(MSR_EFER);
    efer |= EFER_SCE;
    wrmsr(MSR_EFER, efer);

    // STAR: syscall/user mode segments
    uint64_t star = ((uint64_t)USER_CS << 48) | ((uint64_t)KERNEL_CS << 32);
    wrmsr(MSR_STAR, star);

    // LSTAR: syscall entry point
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

    /* FMASK: clear IF, TF, DF on entry */
    uint64_t fmask = (1ULL << 9) | (1ULL << 8) | (1ULL << 10);
    wrmsr(MSR_FMASK, fmask);

    tty_printf(
        "[Syscall] STAR=0x%x LSTAR=0x%x FMASK=0x%x syscall_stack_top=0x%x\n",
        (uint64_t)star, &syscall_entry, fmask, syscall_stack_top
    );
}
