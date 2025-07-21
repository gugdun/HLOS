#ifndef _SYSCALL_H
#define _SYSCALL_H

#define SYS_WRITE 1
#define SYS_EXIT  60

#define SYSCALL_STACK_SIZE 0x1000

void setup_syscall(void);

#endif
