#include <mm/early_alloc.h>
#include <mini_uart.h>
#include <utils.h>

static char *cur = EARLY_MEM_BASE;

void *early_malloc(size_t size)
{
    char *tmp;

    size = ALIGN(size, 4);

    if ((uint64)cur + size > (uint64)EARLY_MEM_END) {
        uart_printf("[!] No enough space!\r\n");
        return NULL;
    }

    tmp = cur;
    cur += size;

#ifdef DEBUG
    uart_printf("[*] Early allocate: %llx ~ %llx\r\n", tmp, cur - 1);
#endif

    return tmp;
}