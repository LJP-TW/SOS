#include <mem.h>
#include <mini_uart.h>

char _smem;
char _emem;

#define SMEM (&_smem)
#define EMEM (&_emem)

static char *cur = SMEM;

void *simple_malloc(size_t size)
{
    char *tmp;

    if ((uint64)cur + size > (uint64)EMEM) {
        uart_printf("[!] No enough space!\r\n");
        return 0;
    }

    tmp = cur;
    cur += size;

    return tmp;
}