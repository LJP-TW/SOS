# SOS

Small Operating System runs on Raspberry Pi 3 Model B+.

NYCU OSDI Spring 2022 Project.

## How to build

* Download dependencies:
```
sudo apt-get install gcc-aarch64-linux-gnu qemu-system-arm
```

* Build kernel8.img:
```
make
```

* Build kernel8.img with some debug information:
```
# Options:
#   MM_DEBUG
#   DEMANDING_PAGE_DEBUG
make MM_DEBUG=1
make DEMANDING_PAGE_DEBUG=1
```

* Build myos.img (which you can dd to SD card):
```
make image
```

## How to run

* Qemu emulation:
```
make qemu
```

* You should either attach gdb to qemu, or just remove the arugments `-s -S` passed to qemu in the Makefile

* Qemu will emulate the bootloader and you can then attach to the bootloader shell with:
```
sudo screen /dev/pts/<pts_idx>
```

* In the bootloader shell, you can use the command `load` to ask the bootloader to load the kernel image. After entering the `load` command, you can send the kernel image to the bootloader with the following command:
```
sudo ./tools/loadkernel.py /dev/pts/<pts_id> build/kernel8.img
```

## How to burn it into pi3

```
sudo dd if=./build/myos.img of=/dev/<your SD card device>
```

## Directory

| Dir | Comment |
| --- | --- |
| build | The compiled object files, images of bootloader, kernel, the initramfs.cpio and the fat32 image |
| img | The files that will be stored to the fat32 image |
| include | Header files |
| initramfs | The files that will be stored to the initramfs.cpio (initramfs.cpio will be stored to the fat32 image) |
| mnt | The tool scripts will use it to create the fat32 image |
| src | Source code files |
| tools | Some helper scripts |