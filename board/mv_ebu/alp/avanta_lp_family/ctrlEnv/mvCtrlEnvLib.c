/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

/* includes */
#include "mvCommon.h"
#include "mvCtrlEnvLib.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "gpp/mvGpp.h"
#include "gpp/mvGppRegs.h"
#include "mvSysEthConfig.h"

#include "pex/mvPex.h"
#include "pex/mvPexRegs.h"

#if defined(MV_INCLUDE_GIG_ETH)
#if defined(MV_ETH_LEGACY)
#include "eth/mvEth.h"
#else
#include "neta/gbe/mvNeta.h"
#endif /* MV_ETH_LEGACY or MV_ETH_NETA */
#endif

#if defined(MV_INCLUDE_XOR)
#include "xor/mvXor.h"
#endif

#if defined(MV_INCLUDE_SATA)
#include "sata/CoreDriver/mvSata.h"
#endif
#if defined(MV_INCLUDE_USB)
#include "usb/mvUsb.h"
#endif

#if defined(MV_INCLUDE_TDM)
#include "mvSysTdmConfig.h"
#endif

#include "ddr2_3/mvDramIfRegs.h"

/* defines  */
#undef MV_DEBUG
#ifdef MV_DEBUG
#define DB(x)	x
#else
#define DB(x)
#endif

/*
 * Control Environment internal data structure
 * Note: it should be initialized dynamically only once.
 */
#define MV_INVALID_CTRL_REV	0xff

typedef struct _ctrlEnvInfo {
	MV_U16	ctrlModel;
	MV_U8	ctrlRev;
} CTRL_ENV_INFO;

CTRL_ENV_INFO ctrlEnvInfo = {};

MV_U32 mvCtrlGetCpuNum(MV_VOID)
{
	return 0; /* kostaz: fix from cider */
}

MV_U32 mvCtrlGetQuadNum(MV_VOID)
{
	return 0; /* kostaz: fix */
}

MV_BOOL mvCtrlIsValidSatR(MV_VOID)
{
	return MV_FALSE;
}

#ifdef MV_INCLUDE_PEX
MV_STATUS mvCtrlUpdatePexId(MV_VOID)
{
	return MV_ERROR;
}
#endif

/*******************************************************************************
* mvCtrlEnvInit - Initialize Marvell controller environment.
*
* DESCRIPTION:
*       This function get environment information and initialize controller
*       internal/external environment. For example
*       1) MPP settings according to board MPP macros.
*		NOTE: It is the user responsibility to shut down all DMA channels
*		in device and disable controller sub units interrupts during
*		boot process.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_STATUS mvCtrlEnvInit(MV_VOID)
{
#if !defined(CONFIG_MACH_AVANTA_LP_FPGA_FPGA)
	MV_U32 mppGroup;
	MV_U32 mppVal;
	MV_U32 i, gppMask;

	/* Disable MBus Error Propagation */
	MV_REG_BIT_RESET(SOC_COHERENCY_FABRIC_CTRL_REG, BIT8);

	/* Use Background sync barrier (polling) for I/O cache coherency */
	MV_REG_BIT_SET(SOC_CIB_CTRL_CFG_REG, BIT8);

	/* MPP Init - scan which modeule is connected */
	mvBoardMppModulesScan();

	/* Read MPP config values from board level and write MPP options to HW */
	for (mppGroup = 0; mppGroup < MV_MPP_MAX_GROUP; mppGroup++) {
		mppVal = mvBoardMppGet(mppGroup);	/* get pre-defined values */
		MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), mppVal);
	}

	/* disable all GPIO interrupts */
	for (i = 0; i < MV_GPP_MAX_GROUP; i++) {
		MV_REG_WRITE(GPP_INT_MASK_REG(i), 0x0);
		MV_REG_WRITE(GPP_INT_LVL_REG(i), 0x0);
	}

	/* clear all int */
	for (i = 0; i < MV_GPP_MAX_GROUP; i++)
		MV_REG_WRITE(GPP_INT_CAUSE_REG(i), 0x0);

	/* Set gpp interrupts as needed */
	for (i = 0; i < MV_GPP_MAX_GROUP; i++) {
		gppMask = mvBoardGpioIntMaskGet(i);
		mvGppTypeSet(i, gppMask , (MV_GPP_IN & gppMask));
		mvGppPolaritySet(i, gppMask , (MV_GPP_IN_INVERT & gppMask));
	}

	/* Scan for other modules (SERDES/LVDS/...) */
	mvBoardOtherModulesScan();

	/* Update interfaces configuration based on above scan */
	if (MV_OK != mvCtrlSerdesPhyConfig())
		mvOsPrintf("%s: Can't init some or all SERDES lanes\n", __func__);

	mvOsDelay(100);
#endif

	return MV_OK;
}

/*******************************************************************************
* mvCtrlMppRegGet - return reg address of mpp group
*
* DESCRIPTION:
*
* INPUT:
*       mppGroup - MPP group.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_U32 - Register address.
*
*******************************************************************************/
MV_U32 mvCtrlMppRegGet(MV_U32 mppGroup)
{
	MV_U32 ret;

	if (mppGroup >= MV_MPP_MAX_GROUP)
		mppGroup = 0;

	ret = MPP_CONTROL_REG(mppGroup);

	return ret;
}

#if defined(MV_INCLUDE_PEX)
/*******************************************************************************
* mvCtrlPexMaxIfGet
*
* DESCRIPTION:
*       This function returns Marvell controller number of PEX interfaces.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Marvell controller number of PEX interfaces. If controller
*		ID is undefined the function returns '0'.
*
*******************************************************************************/
MV_U32 mvCtrlPexMaxIfGet(MV_VOID)
{
	return 1;
}
#endif

/*******************************************************************************
* mvCtrlPexMaxUnitGet
*
* DESCRIPTION:
*       This function returns Marvell controller number of PEX units.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Marvell controller number of PEX units. If controller
*		ID is undefined the function returns '0'.
*
*******************************************************************************/
MV_U32 mvCtrlPexMaxUnitGet(MV_VOID)
{
	return 1;
}

#if defined(MV_INCLUDE_PCI)
/*******************************************************************************
* mvCtrlPciMaxIfGet
*
* DESCRIPTION:
*       This function returns Marvell controller number of PEX interfaces.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Marvell controller number of PEX interfaces. If controller
*		ID is undefined the function returns '0'.
*
*******************************************************************************/
#ifndef mvCtrlPciMaxIfGet
MV_U32 mvCtrlPciMaxIfGet(MV_VOID)
{
	return 1;
}
#endif
#endif

/*******************************************************************************
* mvCtrlEthMaxPortGet - Get Marvell controller number of etherent ports.
*
* DESCRIPTION:
*       This function returns Marvell controller number of etherent port.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Marvell controller number of etherent port.
*
*******************************************************************************/
MV_U32 mvCtrlEthMaxPortGet(MV_VOID)
{
	MV_U32 devId;

	devId = mvCtrlModelGet();
	switch (devId) {
	case MV_FPGA_DEV_ID:
		return MV_FPGA_ETH_MAX_PORT;
	default:
		return 0;
	}
}

/*******************************************************************************
* mvCtrlEthMaxCPUsGet - Get Marvell controller number of CPUs.
*
* DESCRIPTION:
*       This function returns Marvell controller number of CPUs.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Marvell controller number of CPUs.
*
*******************************************************************************/
MV_U8 mvCtrlEthMaxCPUsGet(MV_VOID)
{
	return 1;
}

#if defined(MV_INCLUDE_SATA)
/*******************************************************************************
* mvCtrlSataMaxPortGet - Get Marvell controller number of Sata ports.
*
* DESCRIPTION:
*       This function returns Marvell controller number of Sata ports.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Marvell controller number of Sata ports.
*
*******************************************************************************/
MV_U32 mvCtrlSataMaxPortGet(MV_VOID)
{
	return 0;
}
#endif

#if defined(MV_INCLUDE_XOR)
/*******************************************************************************
* mvCtrlXorMaxChanGet - Get Marvell controller number of XOR channels.
*
* DESCRIPTION:
*       This function returns Marvell controller number of XOR channels.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Marvell controller number of XOR channels.
*
*******************************************************************************/
MV_U32 mvCtrlXorMaxChanGet(MV_VOID)
{
	return 0;
}

/*******************************************************************************
* mvCtrlXorMaxUnitGet - Get Marvell controller number of XOR units.
*
* DESCRIPTION:
*       This function returns Marvell controller number of XOR units.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Marvell controller number of XOR units.
*
*******************************************************************************/
MV_U32 mvCtrlXorMaxUnitGet(MV_VOID)
{
	return 0;
}

#endif

#if defined(MV_INCLUDE_USB)
/*******************************************************************************
* mvCtrlUsbHostMaxGet - Get number of Marvell Usb  controllers
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       returns number of Marvell USB  controllers.
*
*******************************************************************************/
MV_U32 mvCtrlUsbMaxGet(void)
{
	return 0;
}
#endif

#if defined(MV_INCLUDE_LEGACY_NAND)
/*******************************************************************************
* mvCtrlNandSupport - Return if this controller has integrated NAND flash support
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if NAND is supported and MV_FALSE otherwise
*
*******************************************************************************/
MV_U32 mvCtrlNandSupport(MV_VOID)
{
	return 0; /* kostaz: what's that ??? */
}
#endif

#if defined(MV_INCLUDE_SDIO)
/*******************************************************************************
* mvCtrlSdioSupport - Return if this controller has integrated SDIO flash support
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if SDIO is supported and MV_FALSE otherwise
*
*******************************************************************************/
MV_U32 mvCtrlSdioSupport(MV_VOID)
{
	return 0; /* kostaz: what's that ??? */
}
#endif

#if defined(MV_INCLUDE_TDM)
/*******************************************************************************
* mvCtrlTdmSupport - Return if this controller has integrated TDM flash support
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if TDM is supported and MV_FALSE otherwise
*
*******************************************************************************/
MV_U32 mvCtrlTdmSupport(MV_VOID)
{
	return 0; /* kostaz: what's that ??? */
}

/*******************************************************************************
* mvCtrlTdmMaxGet - Return the maximum number of TDM ports.
*
* DESCRIPTION:
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       The number of TDM ports in device.
*
*******************************************************************************/
MV_U32 mvCtrlTdmMaxGet(MV_VOID)
{
	return 0; /* kostaz: what's that ??? */
}

/*******************************************************************************
* mvCtrlTdmUnitTypeGet
*
* DESCRIPTION:
*	Return the TDM unit type being compiled in.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*	The TDM unit type.
*
*******************************************************************************/
MV_UNIT_ID mvCtrlTdmUnitTypeGet(MV_VOID)
{
	return TDM_UNIT_32CH; /* kostaz: FIXME */
}

/*******************************************************************************
* mvCtrlTdmUnitIrqGet
*
* DESCRIPTION:
*	Return the TDM unit IRQ number depending on the TDM unit compilation
*	options.
*
* INPUT:
*	None.
*
* OUTPUT:
*	None.
*
* RETURN:
*	None.
******************************************************************************/
MV_U32 mvCtrlTdmUnitIrqGet(MV_VOID)
{
	return MV_TDM_IRQ_NUM;
}
#endif /* MV_INCLUDE_TDM */

/*******************************************************************************
* mvCtrlModelGet - Get Marvell controller device model (Id)
*
* DESCRIPTION:
*       This function returns 16bit describing the device model (ID) as defined
*       in PCI Device and Vendor ID configuration register offset 0x0.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       16bit desscribing Marvell controller ID
*
*******************************************************************************/
MV_U16 mvCtrlModelGet(MV_VOID)
{
	MV_U32 devId, reg, reg2;
	CTRL_ENV_INFO *ci = &ctrlEnvInfo;

	if (ci->ctrlModel != MV_INVALID_DEV_ID)
		return ci->ctrlModel;

	/* If PEX0 clock is disabled - enable it to read */
	reg = MV_REG_READ(POWER_MNG_CTRL_REG);
	if ((reg & PMC_PEXSTOPCLOCK_MASK(0)) == PMC_PEXSTOPCLOCK_STOP(0)) {
		reg2 = ((reg & ~PMC_PEXSTOPCLOCK_MASK(0)) | PMC_PEXSTOPCLOCK_EN(0));
		MV_REG_WRITE(POWER_MNG_CTRL_REG, reg2);
	}

	devId = MV_REG_READ(PEX_CFG_DIRECT_ACCESS(0, PEX_DEVICE_AND_VENDOR_ID));

	/* Set PEX0 clock to original state */
	if ((reg & PMC_PEXSTOPCLOCK_MASK(0)) == PMC_PEXSTOPCLOCK_STOP(0))
		MV_REG_WRITE(POWER_MNG_CTRL_REG, reg);

	ci->ctrlModel = (MV_U16) ((devId >> 16) & 0xFFFF);
	return ci->ctrlModel;
}

/*******************************************************************************
* mvCtrlRevGet - Get Marvell controller device revision number
*
* DESCRIPTION:
*       This function returns 8bit describing the device revision as defined
*       in PCI Express Class Code and Revision ID Register.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       8bit desscribing Marvell controller revision number
*
*******************************************************************************/
MV_U8 mvCtrlRevGet(MV_VOID)
{
	CTRL_ENV_INFO *ci = &ctrlEnvInfo;
	MV_U8 revNum;

	if (ci->ctrlRev != MV_INVALID_CTRL_REV)
		return ci->ctrlRev;

#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
	/* Check pex power state */
	MV_U32 pexPower;
	pexPower = mvCtrlPwrClckGet(PEX_UNIT_ID, 0);
	if (pexPower == MV_FALSE)
		mvCtrlPwrClckSet(PEX_UNIT_ID, 0, MV_TRUE);
#endif

	revNum = (MV_U8)MV_REG_READ(PEX_CFG_DIRECT_ACCESS(
				0, PCI_CLASS_CODE_AND_REVISION_ID));

#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
	/* Return to power off state */
	if (pexPower == MV_FALSE)
		mvCtrlPwrClckSet(PEX_UNIT_ID, 0, MV_FALSE);
#endif

	ci->ctrlRev = ((revNum & PCCRIR_REVID_MASK) >> PCCRIR_REVID_OFFS);
	return ci->ctrlRev;
}

/*******************************************************************************
* mvCtrlNameGet - Get Marvell controller name
*
* DESCRIPTION:
*       This function returns a string describing the device model and revision.
*
* INPUT:
*       None.
*
* OUTPUT:
*       pNameBuff - Buffer to contain device name string. Minimum size 30 chars.
*
* RETURN:
*
*       MV_ERROR if informantion can not be read.
*******************************************************************************/
MV_STATUS mvCtrlNameGet(char *pNameBuff)
{
	if (mvCtrlModelGet() == 0x7800)
		mvOsSPrintf(pNameBuff, "%s78XX", SOC_NAME_PREFIX);
	else
		mvOsSPrintf(pNameBuff, "%s%x Rev %d", SOC_NAME_PREFIX, mvCtrlModelGet(), mvCtrlRevGet());

	return MV_OK;
}

/*******************************************************************************
* mvCtrlModelRevGet - Get Controller Model (Device ID) and Revision
*
* DESCRIPTION:
*       This function returns 32bit value describing both Device ID and Revision
*       as defined in PCI Express Device and Vendor ID Register and device revision
*	    as defined in PCI Express Class Code and Revision ID Register.

*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit describing both controller device ID and revision number
*
*******************************************************************************/
MV_U32 mvCtrlModelRevGet(MV_VOID)
{
	return ((mvCtrlModelGet() << 16) | mvCtrlRevGet());
}

/*******************************************************************************
* mvCtrlModelRevNameGet - Get Marvell controller name
*
* DESCRIPTION:
*       This function returns a string describing the device model and revision.
*
* INPUT:
*       None.
*
* OUTPUT:
*       pNameBuff - Buffer to contain device name string. Minimum size 30 chars.
*
* RETURN:
*
*       MV_ERROR if informantion can not be read.
*******************************************************************************/
MV_STATUS mvCtrlModelRevNameGet(char *pNameBuff)
{
	switch (mvCtrlModelRevGet()) {
	case MV_78130_Z1_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_78130_Z1_NAME);
		break;
	default:
		mvCtrlNameGet(pNameBuff);
	}

	return MV_OK;
}

static const char *cntrlName[] = TARGETS_NAME_ARRAY;

/*******************************************************************************
* mvCtrlTargetNameGet - Get Marvell controller target name
*
* DESCRIPTION:
*       This function convert the trget enumeration to string.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Target name (const MV_8 *)
*******************************************************************************/
const MV_8 *mvCtrlTargetNameGet(MV_TARGET target)
{
	if (target >= MAX_TARGETS)
		return "target unknown";

	return cntrlName[target];
}

/*******************************************************************************
* mvCtrlPexAddrDecShow - Print the PEX address decode map (BARs and windows).
*
* DESCRIPTION:
*		This function print the PEX address decode map (BARs and windows).
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
#if defined(MV_INCLUDE_PEX)
static MV_VOID mvCtrlPexAddrDecShow(MV_VOID)
{
	/* kostaz: FIXME: will be filled for AvantaLP */
}
#endif

/*******************************************************************************
* mvUnitAddrDecShow - Print the Unit's address decode map.
*
* DESCRIPTION:
*       This is a generic function for printing the different unit's address
*	decode map.
*
* INPUT:
*       unit	- The unit to print the address decode for.
*	name	- The unit's name.
*	winGetFuncPtr - A pointer to the HAL's window get function.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
static void mvUnitAddrDecShow(MV_U8 numUnits, MV_UNIT_ID unitId,
			      const char *name, MV_WIN_GET_FUNC_PTR winGetFuncPtr)
{
	MV_UNIT_WIN_INFO win;
	MV_U32 unit, i;

	for (unit = 0; unit < numUnits; unit++) {

		if (MV_FALSE == mvCtrlPwrClckGet(unitId, unit))
			continue;
		mvOsOutput("\n");
		mvOsOutput("%s %d:\n", name, unit);
		mvOsOutput("----\n");

		for (i = 0; i < 16; i++) {
			memset(&win, 0, sizeof(MV_UNIT_WIN_INFO));

			mvOsOutput("win%d - ", i);

			if (winGetFuncPtr(unit, i, &win) == MV_OK) {
				if (win.enable) {
					mvOsOutput("%s base %08x, ",
						   mvCtrlTargetNameGet(mvCtrlTargetByWinInfoGet(&win)),
						   win.addrWin.baseLow);
					mvOsOutput("....");
					if (win.addrWin.size == 0)
						mvOsOutput("size %3dGB ", 4);
					else
						mvSizePrint(win.addrWin.size);
					mvOsOutput("\n");
				} else
					mvOsOutput("disable\n");
			}
		}
	}
}

/*******************************************************************************
* mvCtrlAddrDecShow - Print the Controller units address decode map.
*
* DESCRIPTION:
*		This function the Controller units address decode map.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID mvCtrlAddrDecShow(MV_VOID)
{
	mvCpuIfAddDecShow();
	mvAhbToMbusAddDecShow();
#if defined(MV_INCLUDE_PEX)
	mvCtrlPexAddrDecShow();
#endif
#if defined(MV_INCLUDE_USB)
	mvUnitAddrDecShow(mvCtrlUsbMaxGet(), USB_UNIT_ID, "USB", mvUsbWinRead);
#endif

#if defined(MV_INCLUDE_GIG_ETH)
#if defined(MV_ETH_LEGACY)
	mvUnitAddrDecShow(mvCtrlEthMaxPortGet(), ETH_GIG_UNIT_ID, "ETH", mvEthWinRead);
#else
	mvUnitAddrDecShow(mvCtrlEthMaxPortGet(), ETH_GIG_UNIT_ID, "ETH", mvNetaWinRead);
#endif /* MV_ETH_LEGACY */
#endif /* MV_INCLUDE_GIG_ETH */

#if defined(MV_INCLUDE_XOR)
	mvUnitAddrDecShow(mvCtrlXorMaxChanGet(), XOR_UNIT_ID, "XOR", mvXorTargetWinRead);
#endif
#if defined(MV_INCLUDE_SATA)
	mvUnitAddrDecShow(mvCtrlSataMaxPortGet(), SATA_UNIT_ID, "Sata", mvSataWinRead);
#endif
}

/*******************************************************************************
* ctrlSizeToReg - Extract size value for register assignment.
*
* DESCRIPTION:
*       Address decode size parameter must be programed from LSB to MSB as
*       sequence of 1's followed by sequence of 0's. The number of 1's
*       specifies the size of the window in 64 KB granularity (e.g. a
*       value of 0x00ff specifies 256x64k = 16 MB).
*       This function extract the size value from the size parameter according
*       to given aligment paramter. For example for size 0x1000000 (16MB) and
*       aligment 0x10000 (64KB) the function will return 0x00FF.
*
* INPUT:
*       size - Size.
*       alignment - Size alignment. Note that alignment must be power of 2!
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit describing size register value correspond to size parameter.
*       If value is '-1' size parameter or aligment are invalid.
*******************************************************************************/
MV_U32 ctrlSizeToReg(MV_U32 size, MV_U32 alignment)
{
	MV_U32 retVal;

	/* Check size parameter alignment               */
	if ((0 == size) || (MV_IS_NOT_ALIGN(size, alignment))) {
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Size is zero or not aligned.\n"));
		return -1;
	}

	/* Take out the "alignment" portion out of the size parameter */
	alignment--;		/* Now the alignmet is a sequance of '1' (e.g. 0xffff)          */
	/* and size is 0x1000000 (16MB) for example     */
	while (alignment & 1) {	/* Check that alignmet LSB is set       */
		size = (size >> 1);	/* If LSB is set, move 'size' one bit to right      */
		alignment = (alignment >> 1);
	}

	/* If after the alignment first '0' was met we still have '1' in                */
	/* it then aligment is invalid (not power of 2)                                 */
	if (alignment) {
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Alignment parameter 0x%x invalid.\n", (MV_U32) alignment));
		return -1;
	}

	/* Now the size is shifted right according to aligment: 0x0100                  */
	size--;			/* Now the size is a sequance of '1': 0x00ff                    */
	retVal = size;

	/* Check that LSB to MSB is sequence of 1's followed by sequence of 0's         */
	while (size & 1)	/* Check that LSB is set    */
		size = (size >> 1);	/* If LSB is set, move one bit to the right         */

	if (size) {		/* Sequance of 1's is over. Check that we have no other 1's         */
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Size parameter 0x%x invalid.\n", size));
		return -1;
	}
	return retVal;
}

/*******************************************************************************
* ctrlRegToSize - Extract size value from register value.
*
* DESCRIPTION:
*       This function extract a size value from the register size parameter
*       according to given aligment paramter. For example for register size
*       value 0xff and aligment 0x10000 the function will return 0x01000000.
*
* INPUT:
*       regSize   - Size as in register format.	See ctrlSizeToReg.
*       alignment - Size alignment.	Note that alignment must be power of 2!
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit describing size.
*       If value is '-1' size parameter or aligment are invalid.
*******************************************************************************/
MV_U32 ctrlRegToSize(MV_U32 regSize, MV_U32 alignment)
{
	MV_U32 temp;

	/* Check that LSB to MSB is sequence of 1's followed by sequence of 0's         */
	temp = regSize;		/* Now the size is a sequance of '1': 0x00ff            */

	while (temp & 1)	/* Check that LSB is set                                    */
		temp = (temp >> 1);	/* If LSB is set, move one bit to the right         */

	if (temp) {		/* Sequance of 1's is over. Check that we have no other 1's         */
		DB(mvOsPrintf("%s: ERR: Size parameter 0x%x invalid.\n", __func__, regSize));
		return -1;
	}

	/* Check that aligment is a power of two                                        */
	temp = alignment - 1;	/* Now the alignmet is a sequance of '1' (0xffff)          */

	while (temp & 1)	/* Check that alignmet LSB is set                           */
		temp = (temp >> 1);	/* If LSB is set, move 'size' one bit to right      */

	/* If after the 'temp' first '0' was met we still have '1' in 'temp'            */
	/* then 'temp' is invalid (not power of 2)                                      */
	if (temp) {
		DB(mvOsPrintf("%s: ERR: Alignment parameter 0x%x invalid.\n", __func__, alignment));
		return -1;
	}

	regSize++;		/* Now the size is 0x0100                                       */

	/* Add in the "alignment" portion to the register size parameter                */
	alignment--;		/* Now the alignmet is a sequance of '1' (e.g. 0xffff)          */

	while (alignment & 1) {	/* Check that alignmet LSB is set                       */
		regSize = (regSize << 1);	/* LSB is set, move 'size' one bit left     */
		alignment = (alignment >> 1);
	}

	return regSize;
}

/*******************************************************************************
* ctrlSizeRegRoundUp - Round up given size
*
* DESCRIPTION:
*       This function round up a given size to a size that fits the
*       restrictions of size format given an aligment parameter.
*		to given aligment paramter. For example for size parameter 0xa1000 and
*		aligment 0x1000 the function will return 0xFF000.
*
* INPUT:
*       size - Size.
*		alignment - Size alignment.	Note that alignment must be power of 2!
*
* OUTPUT:
*       None.
*
* RETURN:
*       32bit describing size value correspond to size in register.
*******************************************************************************/
MV_U32 ctrlSizeRegRoundUp(MV_U32 size, MV_U32 alignment) /* kostaz: FIXME: remove unused */
{
	MV_U32 msbBit = 0;
	MV_U32 retSize;

	/* Check if size parameter is already comply with restriction   */
	if (!(-1 == ctrlSizeToReg(size, alignment)))
		return size;

	while (size) {
		size = (size >> 1);
		msbBit++;
	}

	retSize = (1 << msbBit);

	if (retSize < alignment)
		return alignment;
	else
		return retSize;
}

/*******************************************************************************
* mvCtrlIsBootFromNOR
*
* DESCRIPTION:
*       Check if device is configured to boot from NOR flash according to the
*	SAR registers.
*
* INPUT:
*	None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if device boot from SPI.
*******************************************************************************/
MV_BOOL mvCtrlIsBootFromNOR(MV_VOID)
{
	return MV_TRUE;
}

/*******************************************************************************
* mvCtrlIsBootFromSPI
*
* DESCRIPTION:
*       Check if device is configured to boot from SPI flash according to the
*	SAR registers.
*
* INPUT:
*	None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if device boot from SPI.
*******************************************************************************/
MV_BOOL mvCtrlIsBootFromSPI(MV_VOID)
{
	return MV_FALSE;
}

/*******************************************************************************
* mvCtrlIsBootFromNAND
*
* DESCRIPTION:
*       Check if device is confiogured to boot from NAND flash according to the SAR
*	registers.
*
* INPUT:
*	None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if device boot from NAND.
*******************************************************************************/
MV_BOOL mvCtrlIsBootFromNAND(MV_VOID)
{
	return MV_FALSE;
}

#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
/*******************************************************************************
* mvCtrlPwrClckSet - Set Power State for specific Unit
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*
* RETURN:
*******************************************************************************/
MV_VOID mvCtrlPwrClckSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable)
{
	/* kostaz: FIXME: verify 0x18220 is not changed */

	/* Clock gating is not supported on FPGA */
	if (mvCtrlModelGet() == MV_FPGA_DEV_ID)
		return;

	switch (unitId) {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_PEXSTOPCLOCK_MASK(index));
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_PEXSTOPCLOCK_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_GESTOPCLOCK_MASK(index));
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_GESTOPCLOCK_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_INTEG_SATA)
	case SATA_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_SATASTOPCLOCK_MASK(index));
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_SATASTOPCLOCK_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_CESASTOPCLOCK_MASK);
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_CESASTOPCLOCK_MASK);

		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_USBSTOPCLOCK_MASK(index));
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_USBSTOPCLOCK_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_SDIO)
	case SDIO_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_SDIOSTOPCLOCK_MASK);
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_SDIOSTOPCLOCK_MASK);

		break;
#endif
	case TDM_32CH_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_TDMSTOPCLOCK_MASK);
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_TDMSTOPCLOCK_MASK);
		break;
	default:
		break;
	}
}

/*******************************************************************************
* mvCtrlPwrClckGet - Get Power State of specific Unit
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*
* RETURN:
******************************************************************************/
MV_BOOL mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index)
{
	MV_U32 reg = MV_REG_READ(POWER_MNG_CTRL_REG);
	MV_BOOL state = MV_TRUE;

	/* Clock gating is not supported on FPGA */
	if (mvCtrlModelGet() == MV_FPGA_DEV_ID)
		return MV_TRUE;

	switch (unitId) {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if ((reg & PMC_PEXSTOPCLOCK_MASK(index)) == PMC_PEXSTOPCLOCK_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if ((reg & PMC_GESTOPCLOCK_MASK(index)) == PMC_GESTOPCLOCK_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SATA)
	case SATA_UNIT_ID:
		if ((reg & PMC_SATASTOPCLOCK_MASK(index)) == PMC_SATASTOPCLOCK_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if ((reg & PMC_CESASTOPCLOCK_MASK) == PMC_CESASTOPCLOCK_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if ((reg & PMC_USBSTOPCLOCK_MASK(index)) == PMC_USBSTOPCLOCK_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SDIO)
	case SDIO_UNIT_ID:
		if ((reg & PMC_SDIOSTOPCLOCK_MASK) == PMC_SDIOSTOPCLOCK_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_TDM)
	case TDM_32CH_UNIT_ID:
		if ((reg & PMC_TDMSTOPCLOCK_MASK) == PMC_TDMSTOPCLOCK_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
	default:
		state = MV_TRUE;
		break;
	}

	return state;
}

/*******************************************************************************
* mvCtrlPwrMemSet - Set Power State for memory on specific Unit
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*
* RETURN:
*******************************************************************************/
MV_VOID mvCtrlPwrMemSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable)
{
	switch (unitId) {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_PEX), PMC_PEXSTOPMEM_STOP(index));
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_PEX), PMC_PEXSTOPMEM_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_GE), PMC_GESTOPMEM_STOP(index));
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_GE), PMC_GESTOPMEM_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_INTEG_SATA)
	case SATA_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_SATA), PMC_SATASTOPMEM_STOP(index));
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_SATA), PMC_SATASTOPMEM_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_CESA), PMC_CESASTOPMEM_STOP);
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_CESA), PMC_CESASTOPMEM_MASK);

		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_USB), PMC_USBSTOPMEM_STOP(index));
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_USB), PMC_USBSTOPMEM_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_XOR)
	case XOR_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_XOR), PMC_XORSTOPMEM_STOP(index));
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_XOR), PMC_XORSTOPMEM_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_BM)
	case BM_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_BM), PMC_BMSTOPMEM_STOP);
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_BM), PMC_BMSTOPMEM_MASK);

		break;
#endif
#if defined(MV_INCLUDE_PNC)
	case PNC_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_PNC), PMC_PNCSTOPMEM_STOP);
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_PNC), PMC_PNCSTOPMEM_MASK);

		break;
#endif
	default:
		break;
	}
}

/*******************************************************************************
* mvCtrlPwrMemGet - Get Power State of memory on specific Unit
*
* DESCRIPTION:
*
* INPUT:
*
* OUTPUT:
*
* RETURN:
******************************************************************************/
MV_BOOL mvCtrlPwrMemGet(MV_UNIT_ID unitId, MV_U32 index)
{
	MV_U32 reg;
	MV_BOOL state = MV_TRUE;

	switch (unitId) {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_PEX));
		if ((reg & PMC_PEXSTOPMEM_MASK(index)) == PMC_PEXSTOPMEM_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_GE));
		if ((reg & PMC_GESTOPMEM_MASK(index)) == PMC_GESTOPMEM_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SATA)
	case SATA_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_SATA));
		if ((reg & PMC_SATASTOPMEM_MASK(index)) == PMC_SATASTOPMEM_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_CESA));
		if ((reg & PMC_CESASTOPMEM_MASK) == PMC_CESASTOPMEM_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_USB));
		if ((reg & PMC_USBSTOPMEM_MASK(index)) == PMC_USBSTOPMEM_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_XOR)
	case XOR_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_XOR));
		if ((reg & PMC_XORSTOPMEM_MASK(index)) == PMC_XORSTOPMEM_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_BM)
	case BM_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_BM));
		if ((reg & PMC_BMSTOPMEM_MASK) == PMC_BMSTOPMEM_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_PNC)
	case PNC_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_PNC));
		if ((reg & PMC_PNCSTOPMEM_MASK) == PMC_PNCSTOPMEM_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
	default:
		state = MV_TRUE;
		break;
	}

	return state;
}
#else
MV_VOID mvCtrlPwrClckSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable)
{
	return;
}

MV_BOOL mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index)
{
	return MV_TRUE;
}
#endif /* #if defined(MV_INCLUDE_CLK_PWR_CNTRL) */

/*******************************************************************************
* mvCtrlSerdesMaxLinesGet - Get Marvell controller number of SERDES lines.
*
* DESCRIPTION:
*       This function returns Marvell controller number of SERDES lines.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Marvell controller number of PEX units. If controller
*		ID is undefined the function returns '0'.
*
*******************************************************************************/
MV_U32 mvCtrlSerdesMaxLinesGet(MV_VOID)
{
	return 0;
}

MV_U32 mvCtrlDDRBudWidth(MV_VOID)
{
	MV_U32 reg;
	reg = MV_REG_READ(0x1400);

	return (reg & 0x8000) ? 64 : 32;
}

MV_BOOL mvCtrlDDRThruXbar(MV_VOID)
{
	MV_U32 reg;
	reg = MV_REG_READ(0x20184);

	return (reg & 0x1) ? MV_FALSE : MV_TRUE;
}

MV_BOOL mvCtrlDDRECC(MV_VOID)
{
	MV_U32 reg;
	reg = MV_REG_READ(REG_SDRAM_CONFIG_ADDR);

	return (reg & (0x1 << REG_SDRAM_CONFIG_ECC_OFFS)) ? MV_TRUE : MV_FALSE;
}

static const MV_U8 serdesCfg[][8] = SERDES_CFG;

/*******************************************************************************
* mvCtrlSerdesPhyConfig
*
* DESCRIPTION:
*	Configure Serdes MUX and init PHYs connected to SERDES lines.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Status
*
*******************************************************************************/
MV_STATUS mvCtrlSerdesPhyConfig(MV_VOID)
{
	return MV_ERROR;
}