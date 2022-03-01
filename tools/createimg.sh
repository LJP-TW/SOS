#!/bin/sh

truncate -s 50M $1

# fdisk test.img
(
echo o # Create a new empty DOS partition table
echo n # Add a new partition
echo p # Primary partition
echo 1 # Partition number
echo   # First sector (Accept default: 1)
echo   # Last sector (Accept default: varies)
echo t # Change partition type
echo c # Master Boot Record primary partitions type:LBA
echo w # Write changes
) | sudo fdisk $1

# sudo losetup --partscan --show --find test.img
LOOPBACK=`sudo losetup --partscan --show --find $1`

# "$LOOPBACK" is some string like "/dev/loop0"
echo ${LOOPBACK} | grep --quiet "/dev/loop"

if [ $? = 1 ]
then
    echo "[!] losetup failed!"
    exit 1
fi

# sudo mkfs.vfat -F 32 /dev/loop0p1
sudo mkfs.vfat -F 32 ${LOOPBACK}p1

# sudo losetup -d /dev/loop0
sudo losetup -d ${LOOPBACK}