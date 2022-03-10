#ifndef _BCM2837_H
#define _BCM2837_H
// Ref: https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf

#define PERIPHERALS_BASE 0x3f000000
#define BUS_BASE         0x7e000000

#define BUS_TO_PERIPHERALS(addr) (addr - BUS_BASE + PERIPHERALS_BASE)

// Auxiliaries: UART1 & SPI1, SPI2
#define AUX_IRQ             BUS_TO_PERIPHERALS(0x7E215000)
#define AUX_ENABLES         BUS_TO_PERIPHERALS(0x7E215004)
#define AUX_MU_IO_REG       BUS_TO_PERIPHERALS(0x7E215040)
#define AUX_MU_IER_REG      BUS_TO_PERIPHERALS(0x7E215044)
#define AUX_MU_IIR_REG      BUS_TO_PERIPHERALS(0x7E215048)
#define AUX_MU_LCR_REG      BUS_TO_PERIPHERALS(0x7E21504c)
#define AUX_MU_MCR_REG      BUS_TO_PERIPHERALS(0x7E215050)
#define AUX_MU_LSR_REG      BUS_TO_PERIPHERALS(0x7E215054)
#define AUX_MU_MSR_REG      BUS_TO_PERIPHERALS(0x7E215058)
#define AUX_MU_SCRATCH      BUS_TO_PERIPHERALS(0x7E21505c)
#define AUX_MU_CNTL_REG     BUS_TO_PERIPHERALS(0x7E215060)
#define AUX_MU_STAT_REG     BUS_TO_PERIPHERALS(0x7E215064)
#define AUX_MU_BAUD_REG     BUS_TO_PERIPHERALS(0x7E215068)
#define AUX_SPI0_CNTL0_REG  BUS_TO_PERIPHERALS(0x7E215080)
#define AUX_SPI0_CNTL1_REG  BUS_TO_PERIPHERALS(0x7E215084)
#define AUX_SPI0_STAT_REG   BUS_TO_PERIPHERALS(0x7E215088)
#define AUX_SPI0_IO_REG     BUS_TO_PERIPHERALS(0x7E215090)
#define AUX_SPI0_PEEK_REG   BUS_TO_PERIPHERALS(0x7E215094)
#define AUX_SPI1_CNTL0_REG  BUS_TO_PERIPHERALS(0x7E2150c0)
#define AUX_SPI1_CNTL1_REG  BUS_TO_PERIPHERALS(0x7E2150c4)
#define AUX_SPI1_STAT_REG   BUS_TO_PERIPHERALS(0x7E2150c8)
#define AUX_SPI1_IO_REG     BUS_TO_PERIPHERALS(0x7E2150d0)
#define AUX_SPI1_PEEK_REG   BUS_TO_PERIPHERALS(0x7E2150d4)

// GPIO
#define GPFSEL0 BUS_TO_PERIPHERALS(0x7E200000)
#define GPFSEL1 BUS_TO_PERIPHERALS(0x7E200004)
#define GPFSEL2 BUS_TO_PERIPHERALS(0x7E200008)
#define GPFSEL3 BUS_TO_PERIPHERALS(0x7E20000c)
#define GPFSEL4 BUS_TO_PERIPHERALS(0x7E200010)
#define GPFSEL5 BUS_TO_PERIPHERALS(0x7E200014)

#define GPPUD     BUS_TO_PERIPHERALS(0x7E200094)
#define GPPUDCLK0 BUS_TO_PERIPHERALS(0x7E200098)
#define GPPUDCLK1 BUS_TO_PERIPHERALS(0x7E20009c)

void BCM2837_reset(int tick);
void BCM2837_cancel_reset();


#endif /* _BCM2837_H */