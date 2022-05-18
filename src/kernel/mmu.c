#include <mmu.h>
#include <utils.h>
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