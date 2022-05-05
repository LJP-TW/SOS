CROSS_COMPILE ?= aarch64-linux-gnu-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

CFLAGS := -Wall -nostdlib -nostartfiles -ffreestanding -mgeneral-regs-only

all: userprog1 userprog2

userprog1: userprog1.elf
	$(CROSS_COMPILE)objcopy -O binary $^ $@

userprog1.elf: linker.ld userprog1.o
	$(LD) $(LDFLAGS) -T $< -o $@ userprog1.o

userprog1.o: userprog1.s
	$(CC) $(CFLAGS) -c $< -o $@

userprog2: userprog2.elf
	$(CROSS_COMPILE)objcopy -O binary $^ $@

userprog2.elf: linker.ld userprog2.o
	$(LD) $(LDFLAGS) -T $< -o $@ userprog2.o

userprog2.o: userprog2.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.elf

.PHONY: clean-all
clean-all: clean
	rm -f userprog1 userprog2