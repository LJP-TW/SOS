#include <mmu.h>
#include <arm.h>
#include <utils.h>
#include <mini_uart.h>
#include <exec.h>
#include <panic.h>
#include <preempt.h>
#include <task.h>
#include <current.h>
#include <mm/mm.h>

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB          ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT      (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE  0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE  0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define PD_TABLE    0b11
#define PD_BLOCK    0b01
#define PD_ACCESS       (1 << 10)
#define PD_PXN          ((uint64)1 << 53)
#define PD_NSTABLE      ((uint64)1 << 63)
#define PD_UXNTABLE     ((uint64)1 << 60)
#define PD_MAIR_DEVICE_IDX (MAIR_IDX_DEVICE_nGnRnE << 2)
#define PD_MAIR_NORMAL_IDX (MAIR_IDX_NORMAL_NOCACHE << 2)
// Block Entry
#define PD_BE PD_ACCESS | PD_BLOCK
// Level 3 Block Entry
#define PD_L3BE PD_ACCESS | PD_TABLE

#define BOOT_PGD ((pd_t *)0x1000)
#define BOOT_PUD ((pd_t *)0x2000)
#define BOOT_PMD ((pd_t *)0x3000)

static void segmentation_fault(void)
{
    uart_sync_printf("[Segmentation fault]: Kill Process\r\n");
    exit_user_prog();

    // Never reach
}

static vm_area_t *vma_create(void *va, uint64 size, uint64 flag, void *addr)
{
    vm_area_t *vma;

    vma = kmalloc(sizeof(vm_area_t));
    size = ALIGN(size, PAGE_SIZE);

    vma->va_begin = (uint64)va;
    vma->va_end = (uint64)va + size;
    vma->flag = flag;

    if (vma->flag & VMA_ANON) {
        vma->kva = 0;
    } else if (vma->flag & VMA_PA) {
        vma->kva = PA2VA(addr);
    } else if (vma->flag & VMA_KVA) {
        vma->kva = (uint64)addr;
    } else {
        // Unexpected
        panic("vma_create flag error");
    }

    return vma;
}

static void clone_uva_region(uint64 uva_begin, uint64 uva_end,
                             pd_t *pt, uint64 flag)
{
    for (uint64 addr = uva_begin; addr < uva_end; addr += PAGE_SIZE) {
        void *new_kva;
        uint64 par;

        // try to get the PA of UVA
        asm volatile (
            "at s1e0r, %0"
            :: "r" (addr)
        );

        par = read_sysreg(PAR_EL1);

        if (PAR_FAILED(par)) {
            // VA to PA conversion aborted
            continue;
        }

        // convert PA to KVA
        par = PA2VA(PAR_PA(par));

        // Allocate a page and copy content to this page
        new_kva = kmalloc(PAGE_SIZE);
        memncpy(new_kva, (void *)par, PAGE_SIZE);

        // map @new_kva into @pt
        pt_map(pt, (void *)addr, PAGE_SIZE, (void *)VA2PA(new_kva), flag);
    }
}

static vm_area_t *vma_clone(vm_area_t *vma, pd_t *page_table)
{
    vm_area_t *new_vma;

    new_vma = kmalloc(sizeof(vm_area_t));

    new_vma->va_begin = vma->va_begin;
    new_vma->va_end = vma->va_end;
    new_vma->flag = vma->flag;

    if (vma->flag & VMA_ANON) {
        clone_uva_region(vma->va_begin, vma->va_end, page_table, vma->flag);
        new_vma->kva = 0;
    } else if (vma->flag & VMA_PA) {
        new_vma->kva = vma->kva;
    } else if (vma->flag & VMA_KVA) {
        void *new_kva;
        
        new_kva = kmalloc(vma->va_end - vma->va_begin);
        memncpy(new_kva, (void *)vma->kva, vma->va_end - vma->va_begin);

        new_vma->kva = (uint64)new_kva;
    } else {
        // Unexpected
        panic("vma_clone flag error");
    }

    return new_vma;
}

static void free_uva_region(uint64 uva_begin, uint64 uva_end)
{
    for (uint64 addr = uva_begin; addr < uva_end; addr += PAGE_SIZE) {
        uint64 par;

        // try to get the PA of UVA
        asm volatile (
            "at s1e0r, %0"
            :: "r" (addr)
        );

        par = read_sysreg(PAR_EL1);

        if (PAR_FAILED(par)) {
            // VA to PA conversion aborted
            continue;
        }

        // convert PA to KVA
        par = PA2VA(PAR_PA(par));

        // free KVA
        kfree((void *)par);
    }
}

static void vma_free(vm_area_t *vma)
{
    if (vma->kva && vma->flag & VMA_KVA) {
        kfree((void *)vma->kva);
    } else if (vma->flag & VMA_ANON) {
        free_uva_region(vma->va_begin, vma->va_end);
    } else if (!(vma->flag & VMA_PA)){
        // Unexpected
        panic("vma_free flag error");
    }

    kfree(vma);
}

static vm_area_t *vma_find(vm_area_meta_t *vma_meta, uint64 addr)
{
    vm_area_t *vma;

    list_for_each_entry(vma, &vma_meta->vma, list) {
        if (vma->va_begin <= addr && addr < vma->va_end) {
            return vma;
        }
    }

    return NULL;
}

void mmu_init(void)
{
    uint32 sctlr_el1;

    // Set Translation Control Register
    write_sysreg(TCR_EL1, TCR_CONFIG_DEFAULT);

    // Set Memory Attribute Indirection Register
    write_sysreg(MAIR_EL1,
                (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) |
                (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)));
    
    // Set Identity Paging
    // 0x00000000 ~ 0x3f000000: Normal
    // 0x3f000000 ~ 0x40000000: Device
    // 0x40000000 ~ 0x80000000: Device
    BOOT_PGD[0] = (uint64)BOOT_PUD | PD_NSTABLE | PD_UXNTABLE | PD_TABLE;

    BOOT_PUD[0] = (uint64)BOOT_PMD | PD_TABLE;
    BOOT_PUD[1] = 0x40000000 | PD_MAIR_DEVICE_IDX | PD_BE;

    for (int i = 0; i < 504; ++i) {
        BOOT_PMD[i] = (i * (1 << 21)) | PD_MAIR_NORMAL_IDX | PD_BE;
    }

    for (int i = 504; i < 512; ++i) {
        BOOT_PMD[i] = (i * (1 << 21)) | PD_MAIR_DEVICE_IDX | PD_BE;
    }

    write_sysreg(TTBR0_EL1, BOOT_PGD);
    write_sysreg(TTBR1_EL1, BOOT_PGD);

    // Enable MMU
    sctlr_el1 = read_sysreg(SCTLR_EL1);
    write_sysreg(SCTLR_EL1, sctlr_el1 | 1);
}

pd_t *pt_create(void)
{
    pd_t *pt = kmalloc(PAGE_TABLE_SIZE);

    for (int i = 0; i < PAGE_TABLE_SIZE / sizeof(pt[0]); ++i) {
        pt[i] = 0;
    }

    return pt;
}

void pt_free(pd_t *pt)
{
    // TODO
}

static void _pt_map(pd_t *pt, void *va, void *pa, uint64 flag)
{
    pd_t pd;
    int idx;

    // 47 ~ 39, 38 ~ 30, 29 ~ 21, 20 ~ 12
    for (int layer = 3; layer > 0; --layer) {    
        idx = ((uint64)va >> (12 + 9 * layer)) & 0b111111111;
        pd = pt[idx];

        if (!(pd & 1)) {
            // Invalid entry
            pd_t *tmp = pt_create();
            pt[idx] = VA2PA(tmp) | PD_TABLE;
            pt = tmp;
            continue;
        }

        // Must be a table entry
        pt = (pd_t *)PA2VA(pd & ~((uint64)0xfff));
    }

    idx = ((uint64)va >> 12) & 0b111111111;
    pd = pt[idx];

    if (!(pd & 1)) {
        // Invalid entry
        // Access permissions
        uint64 ap;
        uint64 uxn;

        if (flag & PT_R) {
            if (flag & PT_W) {
                ap = 0b01;
            } else {
                ap = 0b11;
            }
        } else {
            ap = 0b00;
        }

        if (flag & PT_X) {
            uxn = 0;
        } else {
            uxn = 1;
        }

        pt[idx] = (uint64)pa | (uxn << 54) | PD_PXN |
                  PD_MAIR_NORMAL_IDX | (ap << 6) | PD_L3BE;
    }

    // TODO: Already mapping, do nothing?
}

void pt_map(pd_t *pt, void *va, uint64 size, void *pa, uint64 flag)
{
    if ((uint64)va & (PAGE_SIZE - 1)) {
        return;
    }

    if ((uint64)pa & (PAGE_SIZE - 1)) {
        return;
    }

    size = ALIGN(size, PAGE_SIZE);
    
    for (uint64 i = 0; i < size; i += PAGE_SIZE) {
        _pt_map(pt, (void *)((uint64)va + i), (void *)((uint64)pa + i), flag);
    }
}

vm_area_meta_t *vma_meta_create(void)
{
    vm_area_meta_t *vma_meta;

    vma_meta = kmalloc(sizeof(vm_area_meta_t));
    INIT_LIST_HEAD(&vma_meta->vma);

    return vma_meta;
}

void vma_meta_free(vm_area_meta_t *vma_meta, pd_t *page_table)
{
    uint64 old_page_table;
    vm_area_t *vma, *safe;

    old_page_table = get_page_table();
    set_page_table(page_table);

    preempt_disable();

    list_for_each_entry_safe(vma, safe, &vma_meta->vma, list) {
        vma_free(vma);
    }

    preempt_enable();

    kfree(vma_meta);

    set_page_table(old_page_table);
}

void vma_meta_copy(vm_area_meta_t *to, vm_area_meta_t *from, pd_t *page_table)
{
    uint64 old_page_table;
    vm_area_t *vma, *new_vma;

    old_page_table = get_page_table();
    set_page_table(page_table);

    preempt_disable();

    list_for_each_entry(vma, &from->vma, list) {
        new_vma = vma_clone(vma, (pd_t *)old_page_table);

        list_add_tail(&new_vma->list, &to->vma);
    }

    preempt_enable();

    set_page_table(old_page_table);
}

void vma_map(vm_area_meta_t *vma_meta, void *va, uint64 size,
             uint64 flag, void *addr)
{
    vm_area_t *vma;
    int cnt = 0;

    if (flag & VMA_PA) cnt++;
    if (flag & VMA_KVA) cnt++;
    if (flag & VMA_ANON) cnt++;

    if (cnt != 1) {
        return;
    }

    if ((uint64)va & (PAGE_SIZE - 1)) {
        return;
    }

    vma = vma_find(vma_meta, (uint64)va);
    if (vma) {
        return;
    }

    vma = vma_find(vma_meta, (uint64)va + size - 1);
    if (vma) {
        return;
    }

    vma = vma_create(va, size, flag, addr);

    list_add_tail(&vma->list, &vma_meta->vma);
}

static void do_page_fault(esr_el1_t *esr)
{
    uint64 far;
    uint64 va;
    uint64 fault_perm;
    vm_area_t *vma;

    far = read_sysreg(FAR_EL1);

    vma = vma_find(current->address_space, far);

    if (!vma) {
        segmentation_fault();
        // Never reach
    }

    // Check permission
    if (esr->ec == EC_IA_LE) {
        // Execute abort
        fault_perm = VMA_X;
    } else if (ISS_WnR(esr)) {
        // Write abort
        fault_perm = VMA_W;
    } else {
        // Read abort
        fault_perm = VMA_R;
    }
    
    if (!(vma->flag & fault_perm)) {
        goto PAGE_FAULT_INVALID;
    }

    va = far & ~(PAGE_SIZE - 1);

    if (vma->kva) {
        uint64 offset;

        offset = va - vma->va_begin;
        
        pt_map(current->page_table, (void *)va, PAGE_SIZE, 
               (void *)VA2PA(vma->kva + offset), vma->flag);
    } else if (vma->flag & VMA_ANON) {
        void *kva = kmalloc(PAGE_SIZE);

        memzero(kva, PAGE_SIZE);

        pt_map(current->page_table, (void *)va, PAGE_SIZE, 
               (void *)VA2PA(kva), vma->flag);
    } else {
        // Unexpected result
        goto PAGE_FAULT_INVALID;
    }

    return;

PAGE_FAULT_INVALID:
    segmentation_fault();

    // Never reach
}

void mem_abort(esr_el1_t *esr)
{
    int fsc;
#ifdef DEMANDING_PAGE_DEBUG
    uint64 addr;

    addr = read_sysreg(FAR_EL1);
#endif

    fsc = ISS_FSC(esr);

    switch (fsc) {
    case FSC_TF_L0:
    case FSC_TF_L1:
    case FSC_TF_L2:
    case FSC_TF_L3:
#ifdef DEMANDING_PAGE_DEBUG
        uart_sync_printf("[Translation fault]: 0x%llx\r\n", addr);
#endif
        do_page_fault(esr);
        break;
    default:
        segmentation_fault();

        // Never reach
    }
}

void syscall_mmap(trapframe *frame, void *addr, size_t len, int prot,
                  int flags, int fd, int file_offset)
{
    vm_area_t *vma;
    int mapflag;

    // do some initial work
    len = ALIGN(len, PAGE_SIZE);

    if (addr == NULL) {
        addr = (void *)0x550000000000;
    }

    while (1) {
        if ((uint64)addr > 0x0000ffffffffffff) {
            frame->x0 = 0;
            return;
        }

        vma = vma_find(current->address_space, (uint64)addr);
        if (vma) {
            addr = (void *)((uint64)addr + 0x10000000);
            continue;
        }

        vma = vma_find(current->address_space, (uint64)addr + len - 1);
        if (vma) {
            addr = (void *)((uint64)addr + 0x10000000);
            continue;
        }

        break;
    }

    mapflag = 0;

    if (prot & PROT_READ)  mapflag |= VMA_R;
    if (prot & PROT_WRITE) mapflag |= VMA_W;
    if (prot & PROT_EXEC)  mapflag |= VMA_X;

    if (flags & MAP_POPULATE) {
        void *kva;
        
        mapflag |= VMA_KVA;

        kva = kmalloc(len);
        memzero(kva, len);

        vma_map(current->address_space, addr, len, mapflag, kva);

        pt_map(current->page_table, addr, len, (void *)VA2PA(kva), mapflag);
    } else if (flags & MAP_ANONYMOUS) {
        mapflag |= VMA_ANON;

        vma_map(current->address_space, addr, len, mapflag, NULL);
    } else {
        // Unexpected.
        frame->x0 = 0;
        return;
    }

    frame->x0 = (uint64)addr;
}