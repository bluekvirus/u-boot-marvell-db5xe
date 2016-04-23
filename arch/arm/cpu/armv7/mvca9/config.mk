#
# Give correct cc-options for armv7 cortex-a9 cpu (by Tim Lauv)
# 
# also see 23 ../../config.mk -msoft-float
# also see 230 /tools/marvell/bin_hdr/base.mk -msoft-float
#
# -msoft-float cannot be used with -mfloat-abi=hard (VFP) on this U-Boot
#

PLATFORM_RELFLAGS += -mtune=cortex-a9 #-mfloat-abi=hard
PLATFORM_CPPFLAGS += -mtune=cortex-a9 #-mfloat-abi=hard