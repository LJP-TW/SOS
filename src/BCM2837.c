#include <utils.h>

#define PM_PASSWORD 0x5a000000
#define PM_RSTC     0x3F10001c
#define PM_WDOG     0x3F100024

void BCM2837_reset(int tick) {          // reboot after watchdog timer expire
    put32(PM_RSTC, PM_PASSWORD | 0x20); // full reset
    put32(PM_WDOG, PM_PASSWORD | tick); // number of watchdog tick
}

void BCM2837_cancel_reset() {
    put32(PM_RSTC, PM_PASSWORD | 0); // full reset
    put32(PM_WDOG, PM_PASSWORD | 0); // number of watchdog tick
}
