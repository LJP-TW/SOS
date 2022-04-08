CROSS_COMPILE ?= aarch64-linux-gnu-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

userprog1: userprog1.elf
	$(CROSS_COMPILE)objcopy -O binary $^ $@

userprog1.elf: linker.ld userprog1.o
	$(LD) $(LDFLAGS) -T $< -o $@ userprog1.o

userprog1.o: userprog1.s
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.elf