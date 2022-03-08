CROSS_COMPILE ?= aarch64-linux-gnu-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

BUILD_DIR   = build
SRC_DIR     = src
INCLUDE_DIR = include

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

BOOTLOADER_C_FILES    = $(wildcard $(SRC_DIR)/$(BOOTLOADER_DIR)/*.c)
BOOTLOADER_ASM_FILES  = $(wildcard $(SRC_DIR)/$(BOOTLOADER_DIR)/*.S)
BOOTLOADER_OBJ_FILES  = $(BOOTLOADER_ASM_FILES:$(SRC_DIR)/$(BOOTLOADER_DIR)/%.S=$(BUILD_DIR)/$(BOOTLOADER_DIR)/%_s.o)
BOOTLOADER_OBJ_FILES += $(BOOTLOADER_C_FILES:$(SRC_DIR)/$(BOOTLOADER_DIR)/%.c=$(BUILD_DIR)/$(BOOTLOADER_DIR)/%_c.o)
KERNEL_C_FILES    = $(wildcard $(SRC_DIR)/$(KERNEL_DIR)/*.c)
KERNEL_ASM_FILES  = $(wildcard $(SRC_DIR)/$(KERNEL_DIR)/*.S)
KERNEL_OBJ_FILES  = $(KERNEL_ASM_FILES:$(SRC_DIR)/$(KERNEL_DIR)/%.S=$(BUILD_DIR)/$(KERNEL_DIR)/%_s.o)
KERNEL_OBJ_FILES += $(KERNEL_C_FILES:$(SRC_DIR)/$(KERNEL_DIR)/%.c=$(BUILD_DIR)/$(KERNEL_DIR)/%_c.o)
LIB_C_FILES    = $(wildcard $(SRC_DIR)/$(LIB_DIR)/*.c)
LIB_ASM_FILES  = $(wildcard $(SRC_DIR)/$(LIB_DIR)/*.S)
LIB_OBJ_FILES  = $(LIB_ASM_FILES:$(SRC_DIR)/$(LIB_DIR)/%.S=$(BUILD_DIR)/$(LIB_DIR)/%_s.o)
LIB_OBJ_FILES += $(LIB_C_FILES:$(SRC_DIR)/$(LIB_DIR)/%.c=$(BUILD_DIR)/$(LIB_DIR)/%_c.o)

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

qemu: $(KERNEL_IMG)
	qemu-system-aarch64 -M raspi3 -kernel $(KERNEL_IMG) -display none \
					    -serial null -serial pty

image: $(BOOTLOADER_IMG) $(IMG_NAME) 
	./tools/buildimg.sh $^

# Empty Target
# Ref: https://www.gnu.org/software/make/manual/make.html#Empty-Targets
	@sudo touch image

$(IMG_NAME):
	./tools/createimg.sh $(IMG_NAME)

.PHONY: clean
clean:
	rm -f $(KERNEL_OBJ_FILES) $(BOOTLOADER_OBJ_FILES) $(KERNEL_ELF) $(BOOTLOADER_ELF)

.PHONY: clean-all
clean-all: clean
	rm -f $(KERNEL_IMG) $(BOOTLOADER_IMG) $(IMG_NAME)
