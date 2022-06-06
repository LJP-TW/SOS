#include <cpio.h>
#include <mini_uart.h>
#include <string.h>
#include <utils.h>
#include <fdt.h>

void *initramfs_base;
void *initramfs_end;

static int initramfs_fdt_parser(int level, char *cur, char *dt_strings)
{
    struct fdt_node_header *nodehdr = (struct fdt_node_header *)cur;
    struct fdt_property *prop;

    uint32 ok = 0;
    uint32 tag = fdtn_tag(nodehdr);

    switch (tag) {
    case FDT_PROP:
        prop = (struct fdt_property *)nodehdr;
        if (!strcmp("linux,initrd-start", dt_strings + fdtp_nameoff(prop))) {
            initramfs_base = TO_CHAR_PTR(PA2VA(fdt32_ld((fdt32_t *)&prop->data)));
            uart_printf("[*] initrd addr base: %x\r\n", initramfs_base);
            ok += 1;
        } else if (!strcmp("linux,initrd-end", dt_strings + fdtp_nameoff(prop))) {
            initramfs_end = TO_CHAR_PTR(PA2VA(fdt32_ld((fdt32_t *)&prop->data)));
            uart_printf("[*] initrd addr end : %x\r\n", initramfs_end);
            ok += 1;
        }
        if (ok == 2) {
            return 1;
        }
        break;
    case FDT_BEGIN_NODE:
    case FDT_END_NODE:
    case FDT_NOP:
    case FDT_END:
        break;
    }

    return 0;
}

void initramfs_init(void)
{
    // Get initramfs address from devicetree
    parse_dtb(fdt_base, initramfs_fdt_parser);
    if (!initramfs_base) {
        uart_printf("[x] Cannot find initrd address!!!\r\n");
    }
}