# pydoorlock, the python library for doorlock
#
# Author:
#  Ralf Ramsauer <ralf@binary-kitchen.de>
#
# Copyright (c) Binary Kitchen, 2019
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.
#

ARCHARM = ArchLinuxARM-armv7-latest.tar.gz
#ARCHARM = ArchLinuxARM-rpi-2-latest.tar.gz

DIR_DOWNLOAD = download
OUT = out

SSH_PUB_KEY ?= $(shell realpath ~/.ssh/id_ed25519.pub)

ARCHARM_PKG = $(DIR_DOWNLOAD)/$(ARCHARM)

VEXPRESS_DTB = vexpress-v2p-ca15-tc1.dtb
QEMU_IMAGE = $(OUT)/qemu-vexpress.qcow2
QEMU_KERNEL = $(OUT)/zImage
QEMU_DTB = $(OUT)/$(VEXPRESS_DTB)

SSH_PORT = 5555
IMAGE_SIZE = 5G

MNT_ROOTFS = /tmp/ROOTFS

all: $(QEMU_KERNEL)

qemu: extract_kernel
	qemu-system-arm							\
		-M vexpress-a15 -cpu cortex-a15 -smp 2			\
		-m 1G							\
		-kernel $(QEMU_KERNEL) -serial stdio			\
		-dtb $(QEMU_DTB)					\
		-sd $(QEMU_IMAGE)					\
		-netdev user,id=net,hostfwd=tcp::$(SSH_PORT)-:22	\
		-device virtio-net-device,netdev=net			\
		-append "root=/dev/mmcblk0p2 rootwait rw audit=off"


qemu_img: $(QEMU_IMAGE)

extract_kernel: $(QEMU_IMAGE)
	sudo modprobe nbd
	sudo qemu-nbd -r --connect=/dev/nbd0 $(QEMU_IMAGE)
	mkdir $(MNT_ROOTFS)
	sudo mount /dev/nbd0p1 $(MNT_ROOTFS)

	cp -av $(MNT_ROOTFS)/zImage $(QEMU_KERNEL)
	cp -av $(MNT_ROOTFS)/dtbs/$(VEXPRESS_DTB) $(QEMU_DTB)

	sudo umount $(MNT_ROOTFS)
	rm -rf $(MNT_ROOTFS)
	sudo qemu-nbd --disconnect /dev/nbd0

image_deploy_key: $(QEMU_IMAGE)
	sudo modprobe nbd
	sudo qemu-nbd --connect=/dev/nbd0 $(QEMU_IMAGE)

	sudo mkdir $(MNT_ROOTFS)
	sudo mount /dev/nbd0p2 $(MNT_ROOTFS)

	sudo mkdir -p $(MNT_ROOTFS)/root/.ssh
	sudo cp $(SSH_PUB_KEY) $(MNT_ROOTFS)/root/.ssh/authorized_keys

	sudo umount $(MNT_ROOTFS)
	sudo rm -rf $(MNT_ROOTFS)
	sudo qemu-nbd --disconnect /dev/nbd0

$(QEMU_IMAGE): $(ARCHARM_PKG)
	mkdir -p $(OUT)
	qemu-img create -f qcow2 -o size=$(IMAGE_SIZE) $(QEMU_IMAGE)
	sudo modprobe nbd
	sudo qemu-nbd --connect=/dev/nbd0 $(QEMU_IMAGE)
	#sudo sfdisk /dev/nbd0 < part_layout
	sudo parted --script /dev/nbd0 		\
		mklabel msdos			\
		mkpart primary fat32 1M 128M	\
		mkpart primary ext4 128M 100\%

	sudo mkfs.vfat -F32 /dev/nbd0p1
	sudo mkfs.ext4 -j -E lazy_itable_init=0,lazy_journal_init=0 /dev/nbd0p2
	sync

	sudo mkdir $(MNT_ROOTFS)
	sudo mount /dev/nbd0p2 $(MNT_ROOTFS)
	sudo mkdir $(MNT_ROOTFS)/boot
	sudo mount /dev/nbd0p1 $(MNT_ROOTFS)/boot

	sudo tar -xf $^ -C $(MNT_ROOTFS)

	# Some adjustments. We need to modprobe drm-pl111 on boot.
	echo pl111-drm | sudo tee $(MNT_ROOTFS)/etc/modules-load.d/drm-pl111.conf
	echo /dev/mmcblk0p1 /boot vfat defaults 0 0 | sudo tee -a $(MNT_ROOTFS)/etc/fstab

	sync
	sudo umount $(MNT_ROOTFS)/boot
	sudo umount $(MNT_ROOTFS)
	sudo rm -rf $(MNT_ROOTFS)
	sudo qemu-nbd --disconnect /dev/nbd0


$(ARCHARM_PKG):
	mkdir -p $(DIR_DOWNLOAD)
	wget http://os.archlinuxarm.org/os/$(ARCHARM) -O $@

clean:
	rm -rf $(DIR_DOWNLOAD)
	rm -rf $(OUT)
