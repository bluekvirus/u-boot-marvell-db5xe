#!/bin/bash
#
make linux-rebuild
sudo cp -v output/images/uImage /var/ftpd/
sudo cp -v output/images/rootfs.cpio.uboot /var/ftpd/
sudo cp -v output/images/fwf51e.dtb /var/ftpd/

