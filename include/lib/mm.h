#ifndef _MM_H
#define _MM_H

#ifndef __ASSEMBLER__

void memzero(char *src, unsigned long n);
void memncpy(char *dst, char *src, unsigned long n);

#endif /* __ASSEMBLER__ */

#endif /* _MM_H */