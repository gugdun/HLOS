#include <stddef.h>

static inline long __syscall(
    long n,
    long a1, long a2, long a3,
    long a4, long a5, long a6
) {
    long ret;
    __asm__ volatile (
        "movq %5, %%r10\n\t"
        "movq %6, %%r8\n\t"
        "movq %7, %%r9\n\t"
        "syscall"
        : "=a"(ret)
        : "a"(n),          /* %1 */
          "D"(a1),         /* %2 */
          "S"(a2),         /* %3 */
          "d"(a3),         /* %4 */
          "r"(a4),         /* %5 -> r10 */
          "r"(a5),         /* %6 -> r8  */
          "r"(a6)          /* %7 -> r9  */
        : "rcx", "r11", "r10", "r8", "r9", "memory"
    );
    return ret;
}

long write(int fd, const void *buf, size_t len) {
    return __syscall(1, fd, (long)buf, len, 0, 0, 0);
}

void exit(int code) {
    __syscall(60, code, 0, 0, 0, 0, 0);
    __builtin_unreachable();
}

void _start() {
    write(1, "Hello from ELF!\n", 16);
    exit(0);
    __builtin_unreachable();
}
