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

## How to burn it into pi3

```
dd if=./build/myos.img of=/dev/<your SD card device>
```
