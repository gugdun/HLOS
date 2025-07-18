#include <xencore/arch/x86_64/syscall.h>
#include <xencore/arch/x86_64/paging.h>
#include <xencore/arch/x86_64/segments.h>
#include <xencore/arch/x86_64/msr.h>

#include <xencore/xenio/tty.h>

#define MSR_EFER    0xC0000080
#define MSR_STAR    0xC0000081
#define MSR_LSTAR   0xC0000082
#define MSR_FMASK   0xC0000084
#define EFER_SCE    (1 << 0)    // Enable SYSCALL/SYSRET

extern uint64_t kernel_stack_top;

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

__attribute__((naked)) void syscall_entry(void) {
    __asm__ volatile (
        /* INPUT (SYSCALL, 64-bit):
         *  rax = num
         *  rdi = a1
         *  rsi = a2
         *  rdx = a3
         *  r10 = a4
         *  r8  = a5
         *  r9  = a6
         *  rcx = user RIP
         *  r11 = user RFLAGS
         *  rsp = user RSP
         */

        /* Save user_rsp */
        "mov %rsp, %r12\n\t"

        /* Switch to kernel_stack_top (value, not address) */
        "lea kernel_stack_top(%rip), %rsp\n\t"
        "mov (%rsp), %rsp\n\t"

        /* user-state frame: 3 * 8 */
        "sub $24, %rsp\n\t"
        "mov %r11, 0(%rsp)\n\t"  /* user_rflags */
        "mov %rcx, 8(%rsp)\n\t"  /* user_rip */
        "mov %r12,16(%rsp)\n\t"  /* user_rsp */

        /* Save all syscall-registers to stack (caller-saved): */
        "push %r9\n\t"           /* [a6] */
        "push %r8\n\t"           /* [a5] */
        "push %r10\n\t"          /* [a4] */
        "push %rdx\n\t"          /* [a3] */
        "push %rsi\n\t"          /* [a2] */
        "push %rdi\n\t"          /* [a1] */
        "push %rax\n\t"          /* [num]   rsp -> num */

        /* Load ABI-compatible arguments for C:
           rdi=num, rsi=a1, rdx=a2, rcx=a3, r8=a4, r9=a5
           a6 = *(rsp + 48) → put on stack before call
        */
        "mov 0(%rsp),  %rdi\n\t"   /* num */
        "mov 8(%rsp),  %rsi\n\t"   /* a1 */
        "mov 16(%rsp), %rdx\n\t"   /* a2 */
        "mov 24(%rsp), %rcx\n\t"   /* a3 */
        "mov 32(%rsp), %r8\n\t"    /* a4 */
        "mov 40(%rsp), %r9\n\t"    /* a5 */
        "mov 48(%rsp), %rax\n\t"   /* a6 -> rax temp */
        "push %rax\n\t"            /* push a6 (7-th arg) */

        "call syscall_dispatch\n\t"

        "add $8, %rsp\n\t"         /* drop a6 arg */

        /* Remove saved registers num..a6 (7*8) */
        "add $56, %rsp\n\t"

        /* Restore user-state and return */
        "mov 0(%rsp), %r11\n\t"    /* user_rflags */
        "mov 8(%rsp), %rcx\n\t"    /* user_rip */
        "mov 16(%rsp), %rsp\n\t"   /* user_rsp */
        "sysretq\n\t"
    );
}

void setup_syscall(void)
{
    // Enable syscall in EFER
    uint64_t efer = rdmsr(MSR_EFER);
    efer |= EFER_SCE;
    wrmsr(MSR_EFER, efer);

    // STAR: user CS=0x1B, kernel CS=0x08
    uint64_t star = ((uint64_t)0x001B << 48) | ((uint64_t)0x0008 << 32);
    wrmsr(MSR_STAR, star);

    // LSTAR: syscall entry point
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

    // FMASK: clear IF when entering kernel
    wrmsr(MSR_FMASK, (1 << 9));

    tty_printf(
        "[Syscall] STAR=0x%x LSTAR=0x%x FMASK=0x%x\n",
        (uint64_t)star, &syscall_entry, (uint64_t)(1ULL << 9)
    );
}
