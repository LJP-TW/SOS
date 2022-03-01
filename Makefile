CROSS_COMPILE ?= aarch64-linux-gnu-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

BUILD_DIR   = build
SRC_DIR     = src
INCLUDE_DIR = include

CFLAGS  = -Wall -nostdlib -nostartfiles -ffreestanding -mgeneral-regs-only \
		  -I$(INCLUDE_DIR)
LDFLAGS = -I$(INCLUDE_DIR)

KERNEL_IMG := $(BUILD_DIR)/kernel8.img
KERNEL_ELF := $(BUILD_DIR)/kernel8.elf
IMG_NAME   := $(BUILD_DIR)/myos.img

C_FILES    = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES  = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES  = $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)
OBJ_FILES += $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)

$(KERNEL_IMG): $(KERNEL_ELF)
	$(CROSS_COMPILE)objcopy -O binary $^ $(KERNEL_IMG)

$(KERNEL_ELF): $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(LD) $(LDFLAGS) -T $< -o $(KERNEL_ELF) $(OBJ_FILES)

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

qemu: $(KERNEL_IMG)
	qemu-system-aarch64 -M raspi3 -kernel $(KERNEL_IMG) -display none \
					    -serial null -serial stdio

image: $(KERNEL_IMG) $(IMG_NAME)
	./tools/buildimg.sh $^

# Empty Target
# Ref: https://www.gnu.org/software/make/manual/make.html#Empty-Targets
	@sudo touch image

$(IMG_NAME):
	./tools/createimg.sh $(IMG_NAME)

.PHONY: clean
clean:
	rm -f $(OBJ_FILES) $(KERNEL_ELF)

.PHONY: clean-all
clean-all: clean
	rm -f $(KERNEL_IMG) $(IMG_NAME)
