#!/usr/bin/env python3
import sys
import argparse
import struct
import serial
import time

def pty_mode(args):
    with open(args.filepath, 'rb') as f:
        kernel = f.read()

    with open(args.devpath, 'wb', buffering = 0) as f:
        f.write(struct.pack('<I', len(kernel)))
        f.write(kernel)

def serial_mode(args):
    with open(args.filepath, 'rb') as f:
        kernel = f.read()

    with serial.Serial(args.devpath, 115200, timeout=2) as ser:
        time.sleep(1)
        ser.write(struct.pack('<I', len(kernel)))
        time.sleep(1)
        ser.write(kernel)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('devpath',
                    help='The device path (e.g. /dev/pty/1)')
    parser.add_argument('filepath',
                    help='The kernel image path (e.g. ./kernel8.img)')
    args = parser.parse_args()

    if 'pty' in args.filepath:
        pty_mode(args)
    else:
        serial_mode(args)

    
