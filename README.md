BIOS + Bootloader for Marvell 6820-db51e
========================================
This is the modified version of the Marvell 6820 board bring-up package (boot only). The targeted development board has 2 Gigabits Ethernet PHYs, 1 Gigabits Switch (5/7-port), 1 30G SSD (local-config:log) with 1 128M SPI-Flash (3B mode ROM boot, 4B mode R/W, header:u-boot:env:os1:os2:default-config) and 1 Dual-Band Wireless Radio (802.11bgn)

Things supported so far
-----------------------
1. 2 x PHYs (eth1, eth2)
2. 1 x Switch (eth0 and switch ports 1-5)
3. 1 x SSD
4. 1 x SPI-Flash
5. 1 x Wifi

Things needs soft reset
-----------------------
1. SPI-Flash, back to 3B before u-boot `reset` (not yet) and os `reboot` (done);
2. Wifi, pci-E reset in board header before u-boot init; (not yet)
3. PHYs and Switch, reset in u-boot init (done);

Note that, if you unplug the power cord, it is a hard reset. You don't need the above resets.

Hack it open
------------
0. Invalid bit 0-8 on the SPI-Flash (ROM hard wired to read this as valid boot device record)
1. Trigger rescue boot-squence (see `/tools/marvell/doimage_mv/pattern.py`)
2. Use *modified* xmodem over serial port to upload boot firmware (header:u-boot:env).
3. Use `dnsmasq` with TFTP for uploading os(kernel), devicetree and initramfs.

Develop
-------
1. `/tools/marvell/bin_hdr/src_phy/` -- board SERDES Lanes topology
2. `/board/mv_ebu/a38x/armada_38x_family/boardEnv/` -- board init hooks implementation
3. `/drivers/mtd/spi/` -- SPI-Flash 4B mode support (for upgrading *this*)

Debug
-----
###Console 
```
sudo minicom -D /dev/ttyS0 -b 115200
sudo minicom -D /dev/ttyUSB0 -b 115200
```

License
-------
Copyright 2016 Tim Lauv. 
Under the [GPL v2](https://opensource.org/licenses/GPL-2.0) License.
