/**
 * Created by Tim Lauv (for FWF51E)
 *
 * see build.pl (in a u-boot repo with marvell support)
 */
#ifndef __BOARD_DEF_H
#define __BOARD_DEF_H

//#define DEBUG

#define MV_INCLUDE_SPI
#define MV_SPI_BOOT

#define CONFIG_DDR3
#define CONFIG_USB_STORAGE
#define CONFIG_CMD_SCSI

//notes - override in armada_38x.h
//#define CONFIG_BOOTDELAY	-1 //autoboot disabled
//#define CONFIG_SYS_PROMPT   "FWF51ECUSTOM>> "
//#define CONFIG_SERVERIP		192.168.0.1
//#define ENV_ETH_PRIME		"egiga1" //wan1 as primary nic

#endif	/* __BOARD_DEF_H */