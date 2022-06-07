#ifndef _RPI3_H
#define _RPI3_H

/* Channels */
#define MAILBOX_CH_POWER   0
#define MAILBOX_CH_FB      1
#define MAILBOX_CH_VUART   2
#define MAILBOX_CH_VCHIQ   3
#define MAILBOX_CH_LEDS    4
#define MAILBOX_CH_BTNS    5
#define MAILBOX_CH_TOUCH   6
#define MAILBOX_CH_COUNT   7
#define MAILBOX_CH_PROP    8

/* Others */
#define MBOX_REQUEST_CODE       0x00000000
#define MBOX_TAG_REQUEST        0x00000000
#define MBOX_END_TAG            0x00000000

struct arm_memory_info {
    unsigned int baseaddr;
    unsigned int size;
};

void mailbox_call(unsigned char channel, unsigned int *mb);
unsigned int get_board_revision(void);
void get_arm_memory(struct arm_memory_info *);

#endif