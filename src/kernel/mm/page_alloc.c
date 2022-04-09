/*
 * Implementation of Buddy System.
 */

// TODO: Consider critical section

#include <mm/page_alloc.h>
#include <list.h>
#include <utils.h>
#include <bitops.h>
#include <mini_uart.h>

#define FREELIST_CNT 16

typedef struct {
    uint8 exp:7;
    uint8 allocated:1;
} frame_ent;

typedef struct {
    /* Linked to next free frame_hdr */
    struct list_head list;
} frame_hdr;

frame_ent frame_ents[FRAME_ARRAY_SIZE];

struct list_head freelists[FREELIST_CNT];

/*
 * Convert number of pages to the corresponding idx (or say exp) of freelists
 *
 * num     -> size -> exp
 * 0b1     -> 1    -> 0
 * 0b11    -> 4    -> 2
 * 0b110   -> 8    -> 3
 * 0b10000 -> 16   -> 4
 * 0b10001 -> 32   -> 5
 */
static inline int num2exp(int num)
{
    return fls(num - 1);
}

static inline int is_valid_page(void *page)
{
    if ((char *)SBUDDY > (char *)page ||
        (char *)page >= (char *)EBUDDY ||
        (uint64)page & (PAGE_SIZE - 1)) {
            return 0;
    }

    return 1;
}

void page_allocator_init(void)
{
    int idx, exp, size;

    for (int i = 0; i < FREELIST_CNT; ++i) {
        INIT_LIST_HEAD(&freelists[i]);
    }

    idx = 0;
    exp = FREELIST_CNT - 1;
    size = 1 << exp;

    while (idx != FRAME_ARRAY_SIZE) {
        frame_hdr *hdr;

        if (idx + size <= FRAME_ARRAY_SIZE) {
            frame_ents[idx].exp = exp;
            frame_ents[idx].allocated = 0;

            hdr = idx2addr(idx);
            list_add_tail(&hdr->list, &freelists[exp]);

#ifdef DEBUG
    uart_printf("[*] page init, idx %d belong to exp %d\r\n", idx, exp);
#endif

            idx += size;
        } else {
            --exp;
            size >>= 1;
        }
    }
}

void *alloc_pages(int num)
{
    frame_hdr *hdr;
    int idx, topexp, exp;

#ifdef DEBUG
    uart_printf("[*] alloc_pages %d pages\r\n", num);
#endif

    if (!num) {
        return NULL;
    }

    exp = num2exp(num);

    if (exp >= FREELIST_CNT) {
        return NULL;
    }

    for (topexp = exp; topexp < FREELIST_CNT; topexp++) {
        if (!list_empty(&freelists[topexp])) {
            break;
        }
    }

    if (topexp == FREELIST_CNT) {
        return NULL;
    }

    // Allocate
    hdr = list_first_entry(&freelists[topexp], frame_hdr, list);
    idx = addr2idx(hdr);

    list_del(&hdr->list);

    // Expand
    while (topexp != exp) {
        frame_hdr *buddy_hdr;
        int buddy_idx;

        topexp -= 1;
        buddy_idx = idx ^ (1 << topexp);

        frame_ents[buddy_idx].exp = topexp;
        frame_ents[buddy_idx].allocated = 0;

        buddy_hdr = idx2addr(buddy_idx);
        list_add(&buddy_hdr->list, &freelists[topexp]);

#ifdef DEBUG
    uart_printf("[*] Expand to idx (%d, %d) to exp (%d)\r\n", 
        idx, buddy_idx, topexp);
#endif
    }

    frame_ents[idx].exp = exp;
    frame_ents[idx].allocated = 1;

#ifdef DEBUG
    uart_printf("[*] Allocate idx %d exp %d\r\n", 
        idx, exp);
#endif

    return (void *)hdr;
}

void *alloc_page(void)
{
    return alloc_pages(1);
}

static inline void _free_page(frame_hdr *page)
{
    int idx, buddy_idx, exp;

    idx = addr2idx(page);
    exp = frame_ents[idx].exp;

    frame_ents[idx].allocated = 0;
    list_add(&page->list, &freelists[exp]);

    buddy_idx = idx ^ (1 << exp);

    // Merge buddy
    while (exp + 1 < FREELIST_CNT &&
           !frame_ents[buddy_idx].allocated &&
           frame_ents[buddy_idx].exp == exp) {
#ifdef DEBUG
    uart_printf("[*] merge idx (%d, %d) to exp (%d)\r\n", 
                idx, buddy_idx, exp + 1);
#endif

        frame_hdr *hdr;

        exp += 1;

        hdr = idx2addr(idx);
        list_del(&hdr->list);

        hdr = idx2addr(buddy_idx);
        list_del(&hdr->list);

        idx = idx & buddy_idx;
        hdr = idx2addr(idx);

        frame_ents[idx].exp = exp;
        list_add(&hdr->list, &freelists[exp]);

        buddy_idx = idx ^ (1 << exp);
    }
}

void free_page(void *page)
{
    if (!is_valid_page(page)) {
        return;
    }

#ifdef DEBUG
    uart_printf("[*] free_page idx %d\r\n", addr2idx(page));
#endif

    _free_page((frame_hdr *)page);
}

#ifdef DEBUG
void page_allocator_test(void)
{
    char *ptr1 = alloc_pages(2);
    char *ptr2 = alloc_pages(2);
    char *ptr3 = alloc_pages(2);
    char *ptr4 = alloc_pages(2);

    free_page(ptr3);

    char *ptr5 = alloc_pages(1); // Split ptr3
    char *ptr6 = alloc_pages(1);

    free_page(ptr4);
    free_page(ptr2);
    free_page(ptr1); // Merge with ptr2
    free_page(ptr5);
    free_page(ptr6); // Merge with ptr5, then merge with ptr4, then merge with ptr1
}
#endif