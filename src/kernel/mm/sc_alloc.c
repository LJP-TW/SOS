/*
 * Implementation of Small Chunk allocator.
 */

// TODO: Consider critical section

#include <mm/early_alloc.h>
#include <mm/page_alloc.h>
#include <mm/sc_alloc.h>
#include <list.h>
#include <utils.h>
#include <mini_uart.h>

uint32 sc_sizes[] = {
    0x10, // Minimum size cannot be less than 0x10 (sizeof(sc_hdr))
    0x20,
    0x30,
    0x40,
    0x60,
    0x80,
    0xc0,
    0x100,
    0x400,
    0x1000 // Maximum size cannot be larger than 0x1000 (PAGE_SIZE)
};

typedef struct {
    uint8 size_idx:7;
    uint8 splitted:1;
} sc_frame_ent;

typedef struct {
    /* Link to next free chunk */
    struct list_head list;
} sc_hdr;

sc_frame_ent *sc_frame_ents;

struct list_head sc_freelists[ARRAY_SIZE(sc_sizes)];

static uint8 find_size_idx(int size)
{
    if (size <=   0x10) return 0;
    if (size <=   0x20) return 1;
    if (size <=   0x30) return 2;
    if (size <=   0x40) return 3;
    if (size <=   0x60) return 4;
    if (size <=   0x80) return 5;
    if (size <=   0xc0) return 6;
    if (size <=  0x100) return 7;
    if (size <=  0x400) return 8;
    if (size <= 0x1000) return 9;
    /* Never reach */
    return -1;
}

void sc_early_init(void)
{
    sc_frame_ents = early_malloc(sizeof(sc_frame_ent) * frame_ents_size);

    for (int i = 0; i < frame_ents_size; ++i) {
        sc_frame_ents[i].splitted = 0;
    }
}

void sc_init(void)
{
    for (int i = 0; i < ARRAY_SIZE(sc_sizes); ++i) {
        INIT_LIST_HEAD(&sc_freelists[i]);
    }
}

void *sc_alloc(int size)
{
    sc_hdr *hdr;
    uint8 size_idx;
    
    size_idx = find_size_idx(size);

    if (list_empty(&sc_freelists[size_idx])) {
        // Allocate a page and split it into small chunks
        void *page;
        int frame_idx;

        page = alloc_page();

        if (!page) {
            return NULL;
        }

        frame_idx = addr2idx(page);
        sc_frame_ents[frame_idx].size_idx = size_idx;
        sc_frame_ents[frame_idx].splitted = 1;

        for (int i = 0;
             i + sc_sizes[size_idx] <= PAGE_SIZE;
             i += sc_sizes[size_idx]) {
            hdr = (sc_hdr *)((char *)page + i);
            list_add_tail(&hdr->list, &sc_freelists[size_idx]);
        }

#ifdef DEBUG
        uart_sync_printf("[sc] Create chunks (page: %d; size: %d)\r\n", 
                    frame_idx, sc_sizes[size_idx]);
#endif
    }

    hdr = list_first_entry(&sc_freelists[size_idx], sc_hdr, list);
    list_del(&hdr->list);

#ifdef DEBUG
    uart_sync_printf("[sc] Allocate chunks %llx (request: %d; chunksize: %d)\r\n", 
                hdr,
                size,
                sc_sizes[size_idx]);
#endif

    return hdr;
}

int sc_free(void *sc)
{
    sc_hdr *hdr;
    int frame_idx, size_idx;

    frame_idx = addr2idx(sc);

    if (!sc_frame_ents[frame_idx].splitted) {
        /* This frame isn't managed by the Small Chunk allocator */
        return -1;
    }

    size_idx = sc_frame_ents[frame_idx].size_idx;

    hdr = (sc_hdr *)sc;
    list_add(&hdr->list, &sc_freelists[size_idx]);

#ifdef DEBUG
    uart_sync_printf("[sc] Free chunks %llx(size: %d)\r\n", 
                sc,
                sc_sizes[size_idx]);
#endif

    return 0;
}

#ifdef DEBUG
void sc_test(void)
{
    char *ptr1 = sc_alloc(0x18); // A; Allocate a page and create 0x20 chunks
    char *ptr2 = sc_alloc(0x19); // B
    char *ptr3 = sc_alloc(0x1a); // C

    sc_free(ptr1); // 0x20 freelist: A
    sc_free(ptr3); // 0x20 freelist: C -> A

    char *ptr4 = sc_alloc(0x17); // C; 0x20 freelist: A

    sc_free(ptr2); // 0x20 freelist: B -> A

    char *ptr5 = sc_alloc(0x17); // B; 0x20 freelist: A
    char *ptr6 = sc_alloc(0x17); // A; 0x20 freelist: <NULL>

    sc_free(ptr4);
    sc_free(ptr5);
    sc_free(ptr6);

    ptr1 = sc_alloc(0x3f0); // A; Allocate a page and create 0x400 chunks
    ptr2 = sc_alloc(0x3f0); // B
    ptr3 = sc_alloc(0x3f0); // C
    ptr4 = sc_alloc(0x3f0); // D
    ptr5 = sc_alloc(0x3f0); // E; Allocate a page and create 0x400 chunks
    
    sc_free(ptr1);
    sc_free(ptr2);
    sc_free(ptr3);
    sc_free(ptr5);
    sc_free(ptr4);

    ptr1 = sc_alloc(0x3f0); // D
    ptr2 = sc_alloc(0x3f0); // E
    ptr3 = sc_alloc(0x3f0); // C
    ptr4 = sc_alloc(0x3f0); // B
    ptr5 = sc_alloc(0x3f0); // A

    sc_free(ptr1);
    sc_free(ptr2);
    sc_free(ptr3);
    sc_free(ptr5);
    sc_free(ptr4);
}
#endif