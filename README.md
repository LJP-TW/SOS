# My OSC 2022

## Author

| 學號 | GitHub 帳號 | 姓名 | Email |
| --- | ----------- | --- | --- |
|`310555003`| `LJP-TW` | `張書銘` | ljp.cs10@nycu.edu.tw |

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
make DEBUG=1
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
dd if=./build/myos.img of=/dev/<your SD card device>
```
