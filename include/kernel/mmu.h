#ifndef _MMU_H
#define _MMU_H

#include <types.h>
#include <list.h>
#include <arm.h>
#include <trapframe.h>

#define PAGE_TABLE_SIZE 0x1000

#define PT_R    0x0001
#define PT_W    0x0002
#define PT_X    0x0004

#define VMA_R       PT_R
#define VMA_W       PT_W
#define VMA_X       PT_X
// PA
#define VMA_PA      0x0008
// Kernel VA
#define VMA_KVA     0x0010
// Anonymous
#define VMA_ANON    0x0020

typedef uint64 pd_t;

/* TODO: The vm_area_t linked list is not sorted, making it an ordered list. */
typedef struct _vm_area_t {
    /* @list links to next vm_area_t */
    struct list_head list;
    uint64 va_begin;
    uint64 va_end;
    uint64 flag;
    /* @kva: Mapped kernel virtual address */
    uint64 kva;
} vm_area_t;

typedef struct {
    /* @vma links all vm_area_t */
    struct list_head vma;
} vm_area_meta_t;

/*
 * Set identity paging, enable MMU
 */
void mmu_init(void);

pd_t *pt_create(void);
void pt_free(pd_t *pt);

/*
 * Create a @size mapping of @va -> @pa.
 * @pt is PGD.
 */
void pt_map(pd_t *pt, void *va, uint64 size, void *pa, uint64 flag);

vm_area_meta_t *vma_meta_create(void);
void vma_meta_free(vm_area_meta_t *vma_meta, pd_t *page_table);
void vma_meta_copy(vm_area_meta_t *to, vm_area_meta_t *from, pd_t *page_table);

void vma_map(vm_area_meta_t *vma_meta, void *va, uint64 size,
             uint64 flag, void *addr);

void mem_abort(esr_el1_t *esr);

/* syscalls */
#define PROT_NONE   0
#define PROT_READ   1
#define PROT_WRITE  2
#define PROT_EXEC   4

#define MAP_ANONYMOUS       0x0020
#define MAP_POPULATE        0x8000

/*
 * TODO: @fd, @file_offset are ignored currently.
 */
void syscall_mmap(trapframe *frame, void *addr, size_t len, int prot,
                  int flags, int fd, int file_offset);

#endif /* _MMU_H */