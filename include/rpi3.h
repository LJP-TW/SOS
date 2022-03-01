#ifndef _RPI3_H
#define _RPI3_H

struct arm_memory_info {
    unsigned int baseaddr;
    unsigned int size;
};

unsigned int get_board_revision(void);
void get_arm_memory(struct arm_memory_info *);

#endif