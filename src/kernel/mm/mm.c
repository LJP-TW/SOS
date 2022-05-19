#include <mm/mm.h>
#include <fdt.h>
#include <string.h>
#include <mini_uart.h>
#include <utils.h>
#include <head.h>
#include <cpio.h>

/* From linker.ld */
extern char _stack_top;

static uint64 memory_end;
static uint32 memory_node;

// Load 64-bit number (big-endian)
static uint64 ld64(char *start)
{
    uint64 ret = 0;
    uint32 i = 8;

    while (i--) {
        ret <<= 8;
        ret += (uint8)(*start++);
    }

    return ret;
}

static int memory_fdt_parser(int level, char *cur, char *dt_strings)
{
    struct fdt_node_header *nodehdr = (struct fdt_node_header *)cur;
    struct fdt_property *prop;

    uint32 tag = fdtn_tag(nodehdr);

    switch (tag) {
    case FDT_PROP:
        if (!memory_node) {
            break;
        }
        prop = (struct fdt_property *)nodehdr;
        if (!strcmp("reg", dt_strings + fdtp_nameoff(prop))) {
            memory_end = ld64(prop->data);
            return 1;
        }
        break;
    case FDT_BEGIN_NODE:
        if (!strcmp("memory@0", nodehdr->name)) {
            memory_node = 1;
        }
        break;
    case FDT_END_NODE:
        memory_node = 0;
        break;
    case FDT_NOP:
    case FDT_END:
        break;
    }

    return 0;
}

void mm_init(void)
{
    parse_dtb(fdt_base, memory_fdt_parser);

    page_allocator_early_init((void *)PA2VA(0), (void *)PA2VA(memory_end));
    sc_early_init();

    // Spin tables for multicore boot & Booting page tables
    mem_reserve((void *)PA2VA(0), (void *)PA2VA(0x4000));

    // Kernel image in the physical memory
    mem_reserve(_start, &_stack_top);

    // Initramfs
    mem_reserve(initramfs_base, initramfs_end);

    // TODO: Devicetree

    // early_malloc
    mem_reserve(EARLY_MEM_BASE, EARLY_MEM_END);

    /* 
     * page_allocator_init() might output some debug information via mini-uart.
     * uart_init() needs to be called before calling page_allocator_init().
     */
    page_allocator_init();
    sc_init();

#ifdef MM_DEBUG
    page_allocator_test();
    sc_test();
#endif
}

void *kmalloc(int size)
{
    uint32 daif;
    void *ret;

    daif = save_and_disable_interrupt();

    if (size <= PAGE_SIZE) {
        // Use the Small Chunk allocator
        ret = sc_alloc(size);
    } else {
        // Use the Buddy System allocator
        int page_cnt = ALIGN(size, PAGE_SIZE) / PAGE_SIZE;

        ret = alloc_pages(page_cnt);
    }

    restore_interrupt(daif);

    return ret;
}

void kfree(void *ptr)
{
    uint32 daif;

    daif = save_and_disable_interrupt();

    if (!sc_free(ptr)) {
        /*
         * The chunk pointed to by ptr is managed by the Small Chunk
         * allocator.
         */
        goto _KFREE_END;
    }

    free_page(ptr);

_KFREE_END:

    restore_interrupt(daif);
}