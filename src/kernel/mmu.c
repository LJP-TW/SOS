#include <mmu.h>
#include <utils.h>

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define PD_TABLE 0b11
#define PD_BLOCK 0b01
#define PD_ACCESS (1 << 10)
#define PD_NSTABLE ((uint64)1 << 63)
#define PD_UXNTABLE ((uint64)1 << 60)
#define PD_MAIR_DEVICE_IDX (MAIR_IDX_DEVICE_nGnRnE << 2)
#define PD_MAIR_NORMAL_IDX (MAIR_IDX_NORMAL_NOCACHE << 2)
// Block Entry
#define PD_BE PD_ACCESS | PD_BLOCK

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
    /* TODO: Support run user program in EL0 */
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