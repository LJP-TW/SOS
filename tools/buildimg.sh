#!/bin/sh

echo "[*] Mount $2 to ./mnt ..."
mkdir -p mnt

# sudo losetup --partscan --show --find test.img
LOOPBACK=`sudo losetup --partscan --show --find $2`

echo "[*] Create loopback" ${LOOPBACK}

# "$LOOPBACK" is some string like "/dev/loop0"
echo ${LOOPBACK} | grep --quiet "/dev/loop"

if [ $? = 1 ]
then
    echo "[!] losetup failed!"
    exit 1
fi

# sudo mkfs.vfat -F 32 /dev/loop0p1
sudo mkfs.vfat -F 32 ${LOOPBACK}p1

# sudo mount /dev/loop0p1 mnt
sudo mount ${LOOPBACK}p1 mnt

echo "[*] Copy the necessary files to $2 ..."
sudo cp img/* mnt

echo "[*] Copy kernel image $1 to $2 ..."
sudo cp $1 mnt/kernel8.img

# echo "[*] Copy kernel image prebuild/kernel8.img to $2 ..."
# sudo cp prebuild/kernel8.img mnt/kernel8.img

echo "[*] Umount $2 ..."
sudo umount mnt

# sudo losetup -d /dev/loop0
sudo losetup -d ${LOOPBACK}

echo "[*] Use following command to write $2 to your SD card:"
echo "\t" "dd if=./$2 of=/dev/<your SD card device>"
echo "[*] Use following command to check your SD card device:"
echo "\t" "lsblk"