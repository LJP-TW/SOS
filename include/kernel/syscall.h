#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <trapframe.h>

#define SCNUM_GETPID        0
#define SCNUM_UART_READ     1
#define SCNUM_UART_WRITE    2
#define SCNUM_EXEC          3
#define SCNUM_FORK          4
#define SCNUM_EXIT          5
#define SCNUM_MBOX_CALL     6
#define SCNUM_KILL_PID      7
#define SCNUM_SIGNAL        8
#define SCNUM_KILL          9
#define SCNUM_MMAP          10
#define SCNUM_OPEN          11
#define SCNUM_CLOSE         12
#define SCNUM_WRITE         13
#define SCNUM_READ          14
#define SCNUM_MKDIR         15
#define SCNUM_MOUNT         16
#define SCNUM_CHDIR         17
#define SCNUM_LSEEK64       18
#define SCNUM_IOCTL         19
#define SCNUM_SYNC          20
#define SCNUM_SIGRETURN     21
#define SCNUM_SHOW_INFO     22

void syscall_handler(trapframe *regs);

#endif /* _SYSCALL_H */