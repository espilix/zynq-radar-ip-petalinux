#!/bin/bash

BUILD_DIR=$1
if [ -z "$BUILD_DIR" ]; then
    echo "Usage: $0 <build_directory>"
    exit 1
fi

SDCARD_IMG="$BUILD_DIR/sdcard.img"
SDCARD_SIZE="2G"

echo "Creating SD card image..."

# Create empty image
dd if=/dev/zero of=$SDCARD_IMG bs=1M count=2048

# Create partition table
parted $SDCARD_IMG mklabel msdos
parted $SDCARD_IMG mkpart primary fat32 1MiB 100MiB
parted $SDCARD_IMG mkpart primary ext4 100MiB 100%

# Setup loop device
LOOP_DEV=$(losetup -f --show $SDCARD_IMG)
partprobe $LOOP_DEV

# Format partitions
mkfs.fat -F 32 ${LOOP_DEV}p1
mkfs.ext4 ${LOOP_DEV}p2

# Mount partitions
mkdir -p /tmp/sdcard_boot /tmp/sdcard_root
mount ${LOOP_DEV}p1 /tmp/sdcard_boot
mount ${LOOP_DEV}p2 /tmp/sdcard_root

# Copy boot files
cp $BUILD_DIR/BOOT.BIN /tmp/sdcard_boot/
cp $BUILD_DIR/image.ub /tmp/sdcard_boot/

# Extract rootfs
cd /tmp/sdcard_root
tar -xzf $BUILD_DIR/rootfs.tar.gz

# Create device nodes for radar
mkdir -p /tmp/sdcard_root/dev
mknod /tmp/sdcard_root/dev/pulse_radar_ip c 240 0

# Unmount
umount /tmp/sdcard_boot /tmp/sdcard_root
losetup -d $LOOP_DEV
rmdir /tmp/sdcard_boot /tmp/sdcard_root

echo "SD card image created: $SDCARD_IMG"
echo "Write to SD card with: dd if=$SDCARD_IMG of=/dev/sdX bs=4M"
