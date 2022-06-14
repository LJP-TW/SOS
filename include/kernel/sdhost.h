#ifndef _SDHOST_H
#define _SDHOST_H

#define BLOCK_SIZE 512

void sd_init(void);

void sd_readblock(int block_idx, void *buf);
void sd_writeblock(int block_idx, const void *buf);

#endif /* _SDHOST_H */