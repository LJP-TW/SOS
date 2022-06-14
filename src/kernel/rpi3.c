// Ref: 
//    https://github.com/bztsrc/raspi3-tutorial/blob/master/04_mailboxes/mbox.h
//    https://github.com/raspberrypi/firmware/wiki/Mailboxes
//    https://github.com/raspberrypi/firmware/wiki/Accessing-mailboxes 
#include <rpi3.h>
#include <BCM2837.h>
#include <utils.h>

/* Mailbox registers */
#define MAILBOX_BASE    PA2VA(PERIPHERALS_BASE + 0xb880)
#define MAILBOX_READ    MAILBOX_BASE
#define MAILBOX_STATUS  MAILBOX_BASE + 0x18
#define MAILBOX_WRITE   MAILBOX_BASE + 0x20

/* Tags */
#define GET_BOARD_REVISION  0x00010002
#define GET_ARM_MEMORY      0x00010005

/* Others */
#define MAILBOX_EMPTY   0x40000000
#define MAILBOX_FULL    0x80000000

#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001

// Aligned buffer
static unsigned int __attribute__((aligned(0x10))) mailbox[16];

void mailbox_call(unsigned char channel, unsigned int *mb)
{
    // Write the data (shifted into the upper 28 bits) combined with 
    // the channel (in the lower four bits) to the write register.
    unsigned int r = (((unsigned long)mb) & ~0xf) | channel;

    // Check if Mailbox 0 status register’s full flag is set.
    while ((get32(MAILBOX_STATUS) & MAILBOX_FULL)) {};

    // If not, then you can write to Mailbox 1 Read/Write register.
    put32(MAILBOX_WRITE, r);

    while (1) {
        // Check if Mailbox 0 status register’s empty flag is set.
        while ((get32(MAILBOX_STATUS) & MAILBOX_EMPTY)) {};

        // If not, then you can read from Mailbox 0 Read/Write register.
        // Check if the value is the same as you wrote in step 1.
        if (r == get32(MAILBOX_READ))
            return;
    }
}

// It should be 0xa020d3 for rpi3 b+
unsigned int get_board_revision(void) 
{
    mailbox[0] = 7 * 4;              // Buffer size in bytes
    mailbox[1] = MBOX_REQUEST_CODE;
    // Tags begin
    mailbox[2] = GET_BOARD_REVISION; // Tag identifier
    mailbox[3] = 4;                  // Value buffer size in bytes
    mailbox[4] = MBOX_TAG_REQUEST;   // Request/response codes
    mailbox[5] = 0;                  // Optional value buffer
    // Tags end
    mailbox[6] = MBOX_END_TAG;

    mailbox_call(MAILBOX_CH_PROP, mailbox); // Message passing procedure call

    return mailbox[5]; 
}

void get_arm_memory(struct arm_memory_info *info)
{
    mailbox[0] = 8 * 4;              // Buffer size in bytes
    mailbox[1] = MBOX_REQUEST_CODE;
    // Tags begin
    mailbox[2] = GET_ARM_MEMORY;     // Tag identifier
    mailbox[3] = 8;                  // Value buffer size in bytes
    mailbox[4] = MBOX_TAG_REQUEST;   // Request/response codes
    mailbox[5] = 0;                  // Optional value buffer
    mailbox[6] = 0;                  // Optional value buffer
    // Tags end
    mailbox[7] = MBOX_END_TAG;

    mailbox_call(MAILBOX_CH_PROP, mailbox); // Message passing procedure call

    info->baseaddr = mailbox[5];
    info->size     = mailbox[6];
}
