#include <fdt.h>
#include <string.h>
#include <mini_uart.h>

#define ALIGN(num, base) ((num + base - 1) & ~(base - 1))

static void _print_tab(int level)
{
    while (level--) {
        uart_printf("\t");
    }
}

static void _dump(char *start, int len)
{
    while (len--) {
        char c = *start++;
        if ((0x20 <= c && c <= 0x7e)) {
            uart_printf("%c", c);
        } else {
            uart_printf("%x", c);
        }
    }
}

static int fdt_traversal_parser(int level, char *cur, char *dt_strings)
{
    struct fdt_node_header *nodehdr = cur;
    struct fdt_property *prop;

    uint32 tag = fdtn_tag(nodehdr);

    switch (tag) {
    case FDT_BEGIN_NODE:
        _print_tab(level);
        uart_printf("[*] Node: %s\n", nodehdr->name);
        break;
    case FDT_END_NODE:
        _print_tab(level);
        uart_printf("[*] Node end\r\n");
        break;
    case FDT_PROP:
        prop = nodehdr;
        _print_tab(level);
        uart_printf("[*] %s:", dt_strings + fdtp_nameoff(prop));
        _dump(prop->data, fdtp_len(prop));
        uart_printf("\r\n");
        break;
    case FDT_NOP:
        break;
    case FDT_END:
        uart_printf("[*] END!\r\n");
    }

    return 0;
}

static void parse_dt_struct(
    char *dt_struct, 
    char *dt_strings, 
    fdt_parser parser)
{
    char *cur = dt_struct;
    int level = 0;

    while (1) {
        cur = ALIGN((uint64)cur, 4);
        struct fdt_node_header *nodehdr = cur;
        struct fdt_property *prop;

        uint32 tag = fdtn_tag(nodehdr);

        switch (tag) {
        case FDT_BEGIN_NODE:
            if (parser(level, cur, dt_strings)) {
                return;
            }
            level++;
            cur += sizeof(struct fdt_node_header) + strlen(nodehdr->name) + 1;
            while (*cur != 0) { 
                cur++;
            }
            break;
        case FDT_END_NODE:
            level--;
            if (parser(level, cur, dt_strings)) {
                return;
            }
            cur += sizeof(struct fdt_node_header);
            break;
        case FDT_PROP:
            prop = nodehdr;
            if (parser(level, cur, dt_strings)) {
                return;
            }
            cur += sizeof(struct fdt_property);
            cur += fdtp_len(prop);
            break;
        case FDT_NOP:
            if (parser(level, cur, dt_strings)) {
                return;
            }
            cur += sizeof(struct fdt_node_header);
            break;
        case FDT_END:
            parser(level, cur, dt_strings);
            return;
        }
    }
}

void parse_dtb(char *dtb, fdt_parser parser)
{
    struct fdt_header *hdr = dtb;

    if (fdt_magic(hdr) != FDT_MAGIC) {
        uart_printf("[x] Not valid fdt_header\r\n");
    }

    if (fdt_last_comp_version(hdr) > 17) {
        uart_printf("[x] Only support v17 dtb\r\n");
    }

    char *dt_struct = dtb + fdt_off_dt_struct(hdr);
    char *dt_strings = dtb + fdt_off_dt_strings(hdr);
    // TODO: Parse memory reservation block

    parse_dt_struct(dt_struct, dt_strings, parser);
}

void fdt_traversal(char *dtb)
{
    parse_dtb(dtb, fdt_traversal_parser);
}