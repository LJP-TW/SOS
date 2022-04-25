.section ".text"
.global _start
_start:
    mov x0, 0
1:
    add x0, x0, 1
    
    // syscall_show_info
    mov x8, 10
    svc 0

    cmp x0, 5
    blt 1b
1:
    // syscall_exit
    mov x8, 5
    svc 0

    b 1b
