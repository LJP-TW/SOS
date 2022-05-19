CROSS_COMPILE ?= aarch64-linux-gnu-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

BUILD_DIR   = build
SRC_DIR     = src
INCLUDE_DIR = include
INITRAMFS_DIR = initramfs
IMG_DIR     = img

BOOTLOADER_DIR = bootloader
KERNEL_DIR     = kernel
LIB_DIR        = lib

CFLAGS  = -Wall -nostdlib -nostartfiles -ffreestanding -mgeneral-regs-only \
		  -I$(INCLUDE_DIR) -I$(INCLUDE_DIR)/$(LIB_DIR)
LDFLAGS = -I$(INCLUDE_DIR) -I$(INCLUDE_DIR)/$(LIB_DIR)

BOOTLOADER_IMG := $(BUILD_DIR)/bootloader.img
BOOTLOADER_ELF := $(BUILD_DIR)/bootloader.elf
KERNEL_IMG := $(BUILD_DIR)/kernel8.img
KERNEL_ELF := $(BUILD_DIR)/kernel8.elf
IMG_NAME   := $(BUILD_DIR)/myos.img
INITRAMFS_CPIO := $(BUILD_DIR)/initramfs.cpio

RPI3_DTB   := $(IMG_DIR)/bcm2710-rpi-3-b-plus.dtb

IMG_FILES             = $(wildcard $(IMG_DIR)/*)
INITRAMFS_FILES       = $(wildcard $(INITRAMFS_DIR)/*)

BOOTLOADER_C_FILES    = $(shell find "$(SRC_DIR)/$(BOOTLOADER_DIR)" -name "*.c")
BOOTLOADER_ASM_FILES += $(shell find "$(SRC_DIR)/$(BOOTLOADER_DIR)" -name "*.S")
BOOTLOADER_OBJ_FILES  = $(patsubst $(SRC_DIR)/$(BOOTLOADER_DIR)/%.c,$(BUILD_DIR)/$(BOOTLOADER_DIR)/%_c.o,$(BOOTLOADER_C_FILES))
BOOTLOADER_OBJ_FILES += $(patsubst $(SRC_DIR)/$(BOOTLOADER_DIR)/%.S,$(BUILD_DIR)/$(BOOTLOADER_DIR)/%_s.o,$(BOOTLOADER_ASM_FILES))

KERNEL_C_FILES    = $(shell find "$(SRC_DIR)/$(KERNEL_DIR)" -name "*.c")
KERNEL_ASM_FILES += $(shell find "$(SRC_DIR)/$(KERNEL_DIR)" -name "*.S")
KERNEL_OBJ_FILES  = $(patsubst $(SRC_DIR)/$(KERNEL_DIR)/%.c,$(BUILD_DIR)/$(KERNEL_DIR)/%_c.o,$(KERNEL_C_FILES))
KERNEL_OBJ_FILES += $(patsubst $(SRC_DIR)/$(KERNEL_DIR)/%.S,$(BUILD_DIR)/$(KERNEL_DIR)/%_s.o,$(KERNEL_ASM_FILES))

LIB_C_FILES    = $(shell find "$(SRC_DIR)/$(LIB_DIR)" -name "*.c")
LIB_ASM_FILES += $(shell find "$(SRC_DIR)/$(LIB_DIR)" -name "*.S")
LIB_OBJ_FILES  = $(patsubst $(SRC_DIR)/$(LIB_DIR)/%.c,$(BUILD_DIR)/$(LIB_DIR)/%_c.o,$(LIB_C_FILES))
LIB_OBJ_FILES += $(patsubst $(SRC_DIR)/$(LIB_DIR)/%.S,$(BUILD_DIR)/$(LIB_DIR)/%_s.o,$(LIB_ASM_FILES))

ifdef MM_DEBUG
	CFLAGS += -g -DMM_DEBUG
endif

ifdef DEMANDING_PAGE_DEBUG
	CFLAGS += -g -DDEMANDING_PAGE_DEBUG
endif

all: $(KERNEL_IMG) $(BOOTLOADER_IMG)

$(BOOTLOADER_IMG): $(BOOTLOADER_ELF)
	$(CROSS_COMPILE)objcopy -O binary $^ $(BOOTLOADER_IMG)

$(BOOTLOADER_ELF): $(SRC_DIR)/$(BOOTLOADER_DIR)/linker.ld $(BOOTLOADER_OBJ_FILES) $(LIB_OBJ_FILES)
	$(LD) $(LDFLAGS) -I$(INCLUDE_DIR)/$(BOOTLOADER_DIR) -T $< -o $(BOOTLOADER_ELF) $(BOOTLOADER_OBJ_FILES) $(LIB_OBJ_FILES)

$(KERNEL_IMG): $(KERNEL_ELF)
	$(CROSS_COMPILE)objcopy -O binary $^ $(KERNEL_IMG)

$(KERNEL_ELF): $(SRC_DIR)/$(KERNEL_DIR)/linker.ld $(KERNEL_OBJ_FILES) $(LIB_OBJ_FILES)
	$(LD) $(LDFLAGS) -I$(INCLUDE_DIR)/$(KERNEL_DIR) -T $< -o $(KERNEL_ELF) $(KERNEL_OBJ_FILES) $(LIB_OBJ_FILES)

$(BUILD_DIR)/$(BOOTLOADER_DIR)/%_s.o: $(SRC_DIR)/$(BOOTLOADER_DIR)/%.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR)/$(BOOTLOADER_DIR) -c $< -o $@

$(BUILD_DIR)/$(BOOTLOADER_DIR)/%_c.o: $(SRC_DIR)/$(BOOTLOADER_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR)/$(BOOTLOADER_DIR) -c $< -o $@

$(BUILD_DIR)/$(KERNEL_DIR)/%_s.o: $(SRC_DIR)/$(KERNEL_DIR)/%.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR)/$(KERNEL_DIR) -c $< -o $@

$(BUILD_DIR)/$(KERNEL_DIR)/%_c.o: $(SRC_DIR)/$(KERNEL_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR)/$(KERNEL_DIR) -c $< -o $@

$(BUILD_DIR)/$(LIB_DIR)/%_s.o: $(SRC_DIR)/$(LIB_DIR)/%.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR)/$(LIB_DIR) -c $< -o $@

$(BUILD_DIR)/$(LIB_DIR)/%_c.o: $(SRC_DIR)/$(LIB_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR)/$(LIB_DIR) -c $< -o $@

$(INITRAMFS_CPIO): $(INITRAMFS_FILES)
	cd initramfs; find . | cpio -o -H newc > ../$(INITRAMFS_CPIO)

qemu: all $(INITRAMFS_CPIO) $(RPI3_DTB)
	qemu-system-aarch64 -M raspi3 -kernel $(BOOTLOADER_IMG) \
						-initrd $(INITRAMFS_CPIO) \
						-dtb $(RPI3_DTB) \
						-chardev pty,id=pty0,logfile=pty.log,signal=off \
					    -serial null -serial chardev:pty0 -s -S

image: $(BOOTLOADER_IMG) $(IMG_NAME) $(INITRAMFS_CPIO) $(IMG_FILES)
	./tools/buildimg.sh $^

# Empty Target
# Ref: https://www.gnu.org/software/make/manual/make.html#Empty-Targets
	@sudo touch image

$(IMG_NAME):
	./tools/createimg.sh $(IMG_NAME)

.PHONY: clean
clean:
	rm -f $(KERNEL_OBJ_FILES) $(BOOTLOADER_OBJ_FILES) $(KERNEL_ELF) \
		  $(BOOTLOADER_ELF) $(LIB_OBJ_FILES)

.PHONY: clean-all
clean-all: clean
	rm -f $(KERNEL_IMG) $(BOOTLOADER_IMG) $(IMG_NAME) $(INITRAMFS_CPIO)
