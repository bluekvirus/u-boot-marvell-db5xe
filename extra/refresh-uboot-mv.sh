#!/bin/bash
#
make uboot-dirclean
make uboot-rebuild all
cd ./output/build/uboot-custom
./tools/marvell/doimage -T uart -D 0 -E 0 -G ./tools/marvell/bin_hdr/bin_hdr.uart.bin u-boot.bin ../../images/u-boot-a38x-custom-spi-fwf51e-uart.bin
./tools/marvell/doimage -T flash -b 115200 -D 0x0 -E 0x0 -G ./tools/marvell/bin_hdr/bin_hdr.bin u-boot.bin ../../images/u-boot-a38x-custom-spi-fwf51e.bin
./tools/mkimage -A arm -T ramdisk -n 'FWF51E Custom Ramdisk' -a 0x0 -e 0x0 -d ../../images/rootfs.cpio.gz ../../images/uRamdisk
cd -