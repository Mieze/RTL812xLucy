/* RTL812xHardware.cpp -- RTL8125 hardware initialzation methods.
*
* Copyright (c) 2020 Laura Müller <laura-mueller@uni-duesseldorf.de>
* All rights reserved.
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation; either version 2 of the License, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*
* Driver for Realtek RTL8125 PCIe 2.5GB ethernet controllers.
*
* This driver is based on Realtek's r8125 Linux driver (9.003.04).
*/

#include "RTL812xLucy.hpp"
#include "linux/mdio.h"
#include "rtl812x_eeprom.h"

#pragma mark --- hardware initialization methods ---

bool RTL8125::initPCIConfigSpace(IOPCIDevice *provider)
{
    IOByteCount pmCapOffset;
    UInt32 pcieLinkCap;
    UInt16 cmdReg;
    UInt16 pmCap;
    bool result = false;
    
    /* Get vendor and device info. */
    pciDeviceData.vendor = provider->configRead16(kIOPCIConfigVendorID);
    pciDeviceData.device = provider->configRead16(kIOPCIConfigDeviceID);
    pciDeviceData.subsystem_vendor = provider->configRead16(kIOPCIConfigSubSystemVendorID);
    pciDeviceData.subsystem_device = provider->configRead16(kIOPCIConfigSubSystemID);
    
    /* Setup power management. */
    if (provider->extendedFindPCICapability(kIOPCIPowerManagementCapability, &pmCapOffset)) {
        pmCap = provider->extendedConfigRead16(pmCapOffset + kIOPCIPMCapability);
        DebugLog("PCI power management capabilities: 0x%x.\n", pmCap);
        
        if (pmCap & kPCIPMCPMESupportFromD3Cold) {
            wolCapable = true;
            DebugLog("PME# from D3 (cold) supported.\n");
        }
        pciPMCtrlOffset = pmCapOffset + kIOPCIPMControl;
    } else {
        IOLog("PCI power management unsupported.\n");
    }
    provider->enablePCIPowerManagement(kPCIPMCSPowerStateD0);
    
    /* Get PCIe link information. */
    if (provider->extendedFindPCICapability(kIOPCIPCIExpressCapability, &pcieCapOffset)) {
        pcieLinkCap = provider->extendedConfigRead32(pcieCapOffset + kIOPCIELinkCapability);
        DebugLog("PCIe link capability: 0x%08x.\n", pcieLinkCap);
    }
    /* Enable the device. */
    cmdReg = provider->configRead16(kIOPCIConfigCommand);
    cmdReg &= ~kIOPCICommandIOSpace;
    cmdReg |= (kIOPCICommandBusMaster | kIOPCICommandMemorySpace | kIOPCICommandMemWrInvalidate);
    provider->configWrite16(kIOPCIConfigCommand, cmdReg);
    
    baseMap = provider->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress2, kIOMapInhibitCache);
    
    if (!baseMap) {
        IOLog("region #2 not an MMIO resource, aborting.\n");
        goto done;
    }
    linuxData.mmio_addr = reinterpret_cast<volatile void *>(baseMap->getVirtualAddress());
    
    linuxData.org_pci_offset_80 = provider->extendedConfigRead8(0x80);
    linuxData.org_pci_offset_81 = provider->extendedConfigRead8(0x81);

    result = true;
    
done:
    return result;
}

void RTL8125::setupASPM(IOPCIDevice *provider, bool allowL0s, bool allowL1)
{
    IOOptionBits aspmState = 0;
    UInt32 pcieLinkCap = 0;

    if (pcieCapOffset) {
        pcieLinkCap = provider->extendedConfigRead32(pcieCapOffset + kIOPCIELinkCapability);
        DebugLog("PCIe link capability: 0x%08x.\n", pcieLinkCap);

        if (pcieLinkCap & kIOPCIELinkCapASPMCompl) {
            if ((pcieLinkCap & kIOPCIELinkCapL0sSup) && allowL0s)
                aspmState |= kIOPCILinkControlASPMBitsL0s;
            
            if ((pcieLinkCap & kIOPCIELinkCapL1Sup) && allowL1)
                aspmState |= kIOPCILinkControlASPMBitsL1;
            
            IOLog("Enable PCIe ASPM: 0x%08x.\n", aspmState);
        } else {
            IOLog("Disable PCIe ASPM.\n");
        }
        provider->setASPMState(this, aspmState);
    }
}

IOReturn RTL8125::setPowerStateWakeAction(OSObject *owner, void *arg1, void *arg2, void *arg3, void *arg4)
{
    RTL8125 *ethCtlr = OSDynamicCast(RTL8125, owner);
    IOPCIDevice *dev;
    UInt16 val16;
    UInt8 offset;
    
    if (ethCtlr && ethCtlr->pciPMCtrlOffset) {
        dev = ethCtlr->pciDevice;
        offset = ethCtlr->pciPMCtrlOffset;
        
        val16 = dev->extendedConfigRead16(offset);
        
        val16 &= ~(kPCIPMCSPowerStateMask | kPCIPMCSPMEStatus | kPCIPMCSPMEEnable);
        val16 |= kPCIPMCSPowerStateD0;
        
        dev->extendedConfigWrite16(offset, val16);
    }
    return kIOReturnSuccess;
}

IOReturn RTL8125::setPowerStateSleepAction(OSObject *owner, void *arg1, void *arg2, void *arg3, void *arg4)
{
    RTL8125 *ethCtlr = OSDynamicCast(RTL8125, owner);
    IOPCIDevice *dev;
    UInt16 val16;
    UInt8 offset;

    if (ethCtlr && ethCtlr->pciPMCtrlOffset) {
        dev = ethCtlr->pciDevice;
        offset = ethCtlr->pciPMCtrlOffset;
        
        val16 = dev->extendedConfigRead16(offset);
        
        val16 &= ~(kPCIPMCSPowerStateMask | kPCIPMCSPMEStatus | kPCIPMCSPMEEnable);

        if (ethCtlr->linuxData.wol_enabled)
            val16 |= (kPCIPMCSPMEStatus | kPCIPMCSPMEEnable | kPCIPMCSPowerStateD3);
        else
            val16 |= kPCIPMCSPowerStateD3;
        
        dev->extendedConfigWrite16(offset, val16);
    }
    return kIOReturnSuccess;
}

/*
 * These functions have to be rewritten after every update
 * of the underlying Linux sources.
 */

bool RTL8125::rtl812xIdentifyChip(struct rtl8125_private *tp)
{
    UInt32 reg, val32;
    UInt32 ICVerID;
    
    val32 = RTL_R32(tp, TxConfig);
    reg = val32 & 0x7c800000;
    ICVerID = val32 & 0x00700000;

    tp->mcfg = CFG_METHOD_DEFAULT;
    tp->HwIcVerUnknown = false;

    switch (reg) {
        case 0x60800000:
            if (ICVerID == 0x00000000) {
                tp->mcfg = CFG_METHOD_2;
                tp->chipset = 0;
            } else if (ICVerID == 0x100000) {
                tp->mcfg = CFG_METHOD_3;
                tp->chipset = 1;
            } else {
                tp->mcfg = CFG_METHOD_3;
                tp->chipset = 1;

                tp->HwIcVerUnknown = TRUE;
            }

            //tp->efuse_ver = EFUSE_SUPPORT_V4;
            break;
            
        case 0x64000000:
            if (ICVerID == 0x00000000) {
                tp->mcfg = CFG_METHOD_4;
                tp->chipset = 2;
            } else if (ICVerID == 0x100000) {
                tp->mcfg = CFG_METHOD_5;
                tp->chipset = 3;
            } else {
                tp->mcfg = CFG_METHOD_5;
                tp->chipset = 3;
                tp->HwIcVerUnknown = TRUE;
            }

            //tp->efuse_ver = EFUSE_SUPPORT_V4;
            break;
            
        case 0x68000000:
            if (ICVerID == 0x00000000) {
                tp->mcfg = CFG_METHOD_8;
                tp->chipset = 6;
            } else if (ICVerID == 0x100000) {
                tp->mcfg = CFG_METHOD_9;
                tp->chipset = 7;
            } else {
                tp->mcfg = CFG_METHOD_9;
                tp->chipset = 7;
                tp->HwIcVerUnknown = TRUE;
            }
            //tp->efuse_ver = EFUSE_SUPPORT_V4;
            break;
            
        case 0x68800000:
            if (ICVerID == 0x00000000) {
                tp->mcfg = CFG_METHOD_10;
                tp->chipset = 8;
            } else if (ICVerID == 0x100000) {
                tp->mcfg = CFG_METHOD_11;
                tp->chipset = 9;
            } else {
                tp->mcfg = CFG_METHOD_11;
                tp->chipset = 9;
                tp->HwIcVerUnknown = TRUE;
            }
            //tp->efuse_ver = EFUSE_SUPPORT_V4;
            break;
            
        case 0x70800000:
            if (ICVerID == 0x00000000) {
                tp->mcfg = CFG_METHOD_12;
                tp->chipset = 10;
            } else {
                tp->mcfg = CFG_METHOD_12;
                tp->chipset = 10;
                tp->HwIcVerUnknown = TRUE;
            }
            //tp->efuse_ver = EFUSE_SUPPORT_V4;
            break;
            
        case 0x64800000:
            if (ICVerID == 0x00000000) {
                tp->mcfg = CFG_METHOD_31;
                tp->chipset = 12;
            } else if (ICVerID == 0x100000) {
                tp->mcfg = CFG_METHOD_32;
                tp->chipset = 13;
            } else if (ICVerID == 0x200000) {
                tp->mcfg = CFG_METHOD_33;
                tp->chipset = 14;
            } else {
                tp->mcfg = CFG_METHOD_33;
                tp->chipset = 14;
                tp->HwIcVerUnknown = TRUE;
            }
            //tp->efuse_ver = EFUSE_SUPPORT_V4;
            break;
            
        default:
            DebugLog("Unknown chip version (%x)\n", reg);
            tp->mcfg = CFG_METHOD_DEFAULT;
            tp->HwIcVerUnknown = TRUE;
            //tp->efuse_ver = EFUSE_NOT_SUPPORT;
            break;
    }

    if (pciDeviceData.device == 0x8162) {
        if (tp->mcfg == CFG_METHOD_3) {
            tp->mcfg = CFG_METHOD_6;
            tp->chipset = 4;
        } else if (tp->mcfg == CFG_METHOD_5) {
            tp->mcfg = CFG_METHOD_7;
            tp->chipset = 5;
        } else if (tp->mcfg == CFG_METHOD_11) {
            tp->mcfg = CFG_METHOD_13;
            tp->chipset = 11;
        }
    }
    
    tp->rtl8125_rx_config = rtlChipInfos[tp->chipset].RCR_Cfg;

#ifdef ENABLE_USE_FIRMWARE_FILE
    tp->fw_name = rtlChipFwInfos[tp->mcfg].fw_name;
#else
    tp->fw_name = NULL;
#endif  /* ENABLE_USE_FIRMWARE_FILE */
    
    return (tp->mcfg == CFG_METHOD_DEFAULT) ? false : true;
}

void RTL8125::initMacAddr(struct rtl8125_private *tp)
{
    struct IOEthernetAddress macAddr;
    int i;
    
    for (i = 0; i < kIOEthernetAddressSize; i++)
        macAddr.bytes[i] = RTL_R8(tp, MAC0 + i);

    DebugLog("Current MAC: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n",
             macAddr.bytes[0], macAddr.bytes[1],
             macAddr.bytes[2], macAddr.bytes[3],
             macAddr.bytes[4], macAddr.bytes[5]);

    *(u32*)&origMacAddr.bytes[0] = RTL_R32(tp, BACKUP_ADDR0_8125);
    *(u16*)&origMacAddr.bytes[4] = RTL_R16(tp, BACKUP_ADDR1_8125);
    
    DebugLog("Backup MAC: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n",
          origMacAddr.bytes[0], origMacAddr.bytes[1],
          origMacAddr.bytes[2], origMacAddr.bytes[3],
          origMacAddr.bytes[4], origMacAddr.bytes[5]);

    if (is_valid_ether_addr((UInt8 *)&macAddr.bytes))
        goto done;

    if (is_valid_ether_addr((UInt8 *)&fallBackMacAddr.bytes)) {
        memcpy(&macAddr.bytes, &fallBackMacAddr.bytes, sizeof(struct IOEthernetAddress));
        goto done;
    }
    if (is_valid_ether_addr((UInt8 *)&origMacAddr.bytes)) {
        memcpy(&macAddr.bytes, &origMacAddr.bytes, sizeof(struct IOEthernetAddress));
        goto done;
    }
    /* Create a random Ethernet address. */
    random_buf(&macAddr.bytes, kIOEthernetAddressSize);
    macAddr.bytes[0] &= 0xfe;   /* clear multicast bit */
    macAddr.bytes[0] |= 0x02;   /* set local assignment bit (IEEE802) */
    DebugLog("Using random MAC address.\n");
    
done:
    //memcpy(&origMacAddr.bytes, &macAddr.bytes, sizeof(struct IOEthernetAddress));
    memcpy(&currMacAddr.bytes, &macAddr.bytes, sizeof(struct IOEthernetAddress));

    rtl8125_rar_set(&linuxData, (UInt8 *)&currMacAddr.bytes);
}

bool RTL8125::rtl812xInit()
{
    struct rtl8125_private *tp = &linuxData;
    bool result = false;
    
    /* Identify chip attached to board. */
    if (!rtl812xIdentifyChip(tp)) {
        IOLog("Unknown chipset. Aborting...\n");
        goto done;
    }

    tp->phy_reset_enable = rtl8125_xmii_reset_enable;
    tp->phy_reset_pending = rtl8125_xmii_reset_pending;
    tp->link_ok = rtl8125_xmii_link_ok;

    rtl8125_get_bios_setting(tp);
    
    if (!rtl8125_aspm_is_safe(tp)) {
        IOLog("Hardware doesn't support ASPM properly. Disable it!\n");
        tp->aspm = false;
    }
    setupASPM(pciDevice, true, tp->aspm);

    rtl8125_init_software_variable(tp);
    
    /* Setup lpi timer. */
    tp->eee.tx_lpi_timer = mtu + ETH_HLEN + 0x20;

    /* Setup maximum receive size. */
    tp->rms = mtu + VLAN_ETH_HLEN + ETH_FCS_LEN;
    
    //tp->NicCustLedValue = RTL_R16(tp, CustomLED);

    tp->wol_opts = rtl8125_get_hw_wol(tp);
    tp->wol_enabled = (tp->wol_opts) ? WOL_ENABLED : WOL_DISABLED;

    /* Set wake on LAN support. */
    wolCapable = (tp->wol_enabled == WOL_ENABLED);
    
    rtl8125_set_link_option(tp, AUTONEG_ENABLE, HW_SUPP_PHY_LINK_SPEED_5000M(tp) ?
                            SPEED_5000 : SPEED_2500, DUPLEX_FULL,
                            rtl8125_fc_none);
    
    if (tp->mcfg != CFG_METHOD_DEFAULT) {
        struct ethtool_keee *eee = &tp->eee;

        eee->eee_enabled = 1;
        linkmode_set_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT, &eee->supported);
        linkmode_set_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT, &eee->supported);
        linkmode_set_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT, &eee->advertised);
        linkmode_set_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT, &eee->advertised);

        if (tp->mcfg > CFG_METHOD_3) {
            if (HW_SUPP_PHY_LINK_SPEED_2500M(tp)) {
                linkmode_set_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, &eee->supported);
                linkmode_set_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, &eee->advertised);
            }
            if (HW_SUPP_PHY_LINK_SPEED_5000M(tp)) {
                linkmode_set_bit(ETHTOOL_LINK_MODE_5000baseT_Full_BIT, &eee->supported);
                linkmode_set_bit(ETHTOOL_LINK_MODE_5000baseT_Full_BIT, &eee->advertised);
            }
        }
        eee->tx_lpi_enabled = 1;
        eee->tx_lpi_timer = ETH_DATA_LEN + ETH_HLEN + 0x20;
    }

    rtl8125_exit_oob(tp);
    rtl812xHwInit(tp);

    rtl8125_nic_reset(tp);
    
    /* Get production from EEPROM */
    rtl8125_eeprom_type(tp);

    if (tp->eeprom_type == EEPROM_TYPE_93C46 || tp->eeprom_type == EEPROM_TYPE_93C56)
            rtl8125_set_eeprom_sel_low(tp);

    initMacAddr(tp);
    
    tp->cp_cmd = (RTL_R16(tp, CPlusCmd) | RxChkSum);
    
    intrMaskRxTx = (SYSErr | LinkChg | RxDescUnavail | TxOK | RxOK);
    intrMaskTimer = (SYSErr | LinkChg | RxDescUnavail | PCSTimeout | RxOK);
    intrMaskPoll = (SYSErr | LinkChg);
    intrMask = intrMaskRxTx;
    
    /* Get the RxConfig parameters. */
    tp->rtl8125_rx_config = rtlChipInfos[tp->chipset].RCR_Cfg;
  
    /* Reset the tally counter. */
    RTL_W32(tp, CounterAddrHigh, (statPhyAddr >> 32));
    RTL_W32(tp, CounterAddrLow, (statPhyAddr & 0x00000000ffffffff) | CounterReset);

    rtl8125_disable_rxdvgate(tp);
    
#ifdef DEBUG
    
    if (wolCapable)
        IOLog("Device is WoL capable.\n");
    
#endif
    
    result = true;
    
done:
    return result;
}

void RTL8125::rtl812xEnable()
{
    struct rtl8125_private *tp = &linuxData;
    
    setLinkStatus(kIONetworkLinkValid);
    
    intrMask = intrMaskRxTx;
    clear_bit(__POLL_MODE, &stateFlags);
    
    rtl8125_exit_oob(tp);
    rtl812xHwInit(tp);
    rtl8125_hw_reset(tp);
    rtl8125_powerup_pll(tp);
    rtl8125_hw_ephy_config(tp);
    rtl8125_hw_phy_config(tp);
    rtl812xHwConfig(tp);
    
    rtl812xSetPhyMedium(tp, tp->autoneg, tp->speed, tp->duplex, tp->advertising);
}

void RTL8125::rtl812xDisable()
{
    struct rtl8125_private *tp = &linuxData;
    
    rtl8125_irq_mask_and_ack(tp);
    rtl8125_hw_reset(tp);
    
    clearRxTxRings();
    
    rtl8125_hw_d3_para(tp);
    rtl8125_powerdown_pll(tp);
    
    if (HW_DASH_SUPPORT_DASH(tp))
        rtl8125_driver_stop(tp);

    if (test_and_clear_bit(__LINK_UP, &stateFlags)) {
        setLinkStatus(kIONetworkLinkValid);
        IOLog("Link down on en%u\n", netif->getUnitNumber());
    }
}

/* Reset the NIC in case a tx deadlock or a pci error occurred. timerSource and txQueue
 * are stopped immediately but will be restarted by checkLinkStatus() when the link has
 * been reestablished.
 */

void RTL8125::rtl812xRestart()
{
    /* Stop output thread and flush txQueue */
    netif->stopOutputThread();
    netif->flushOutputQueue();
    
    clear_bit(__LINK_UP, &stateFlags);
    setLinkStatus(kIONetworkLinkValid);
    
    /* Reset NIC and cleanup both descriptor rings. */
    rtl8125_nic_reset(&linuxData);
/*
    if (rxInterrupt(netif, kNumRxDesc, NULL, NULL))
        netif->flushInputQueue();
*/
    clearRxTxRings();

    /* Reinitialize NIC. */
    rtl812xEnable();
}

void RTL8125::rtl812xHwInit(struct rtl8125_private *tp)
{
    u32 csi_tmp;

    rtl8125_enable_aspm_clkreq_lock(tp, 0);
    rtl8125_enable_force_clkreq(tp, 0);

    rtl8125_set_reg_oobs_en_sel(tp, true);

    //Disable UPS
    rtl8125_mac_ocp_write(tp, 0xD40A, rtl8125_mac_ocp_read(tp, 0xD40A) & ~(BIT_4));

#ifndef ENABLE_USE_FIRMWARE_FILE
    rtl8125_hw_mac_mcu_config(tp);
#endif

    /*disable ocp phy power saving*/
    if (tp->mcfg == CFG_METHOD_2 ||
        tp->mcfg == CFG_METHOD_3 ||
        tp->mcfg == CFG_METHOD_6)
            rtl8125_disable_ocp_phy_power_saving(tp);

    //Set PCIE uncorrectable error status mask pcie 0x108
    csi_tmp = rtl8125_csi_read(tp, 0x108);
    csi_tmp |= BIT_20;
    rtl8125_csi_write(tp, 0x108, csi_tmp);

    rtl8125_enable_cfg9346_write(tp);
    rtl8125_disable_linkchg_wakeup(tp);
    rtl8125_disable_cfg9346_write(tp);
    rtl8125_disable_magic_packet(tp);
    rtl8125_disable_d0_speedup(tp);
    
    rtl8125_enable_magic_packet(tp);

#ifdef ENABLE_USE_FIRMWARE_FILE
    if (tp->rtl_fw && !tp->resume_not_chg_speed)
        rtl8125_apply_firmware(tp);
#endif
}

void RTL8125::rtl812xHwConfig(struct rtl8125_private *tp)
{
    //UInt32 i;
    int timeout;
    UInt16 mac_ocp_data;
    
    rtl8125_disable_rx_packet_filter(tp);
    
    rtl8125_nic_reset(tp);
    
    RTL_W8(tp, Cfg9346, RTL_R8(tp, Cfg9346) | Cfg9346_Unlock);
    
    rtl8125_enable_force_clkreq(tp, 0);
    rtl8125_enable_aspm_clkreq_lock(tp, 0);

    //clear io_rdy_l23
    switch (tp->mcfg) {
        case CFG_METHOD_2:
        case CFG_METHOD_3:
        case CFG_METHOD_4:
        case CFG_METHOD_5:
            RTL_W8(tp, Config3, RTL_R8(tp, Config3) & ~BIT_1);
            break;
    }
/*
    switch (tp->mcfg) {
        case CFG_METHOD_2:
        case CFG_METHOD_3:
        case CFG_METHOD_4:
        case CFG_METHOD_5:
            //IntMITI_0-IntMITI_31
            for (i=0xA00; i<0xB00; i+=4)
                    RTL_W32(tp, i, 0x00000000);
            break;
    }*/
    RTL_W16(tp, EEE_TXIDLE_TIMER_8125, tp->eee.tx_lpi_timer);

    /* Keep magic packet only. */
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xC0B6);
    mac_ocp_data &= BIT_0;
    rtl8125_mac_ocp_write(tp, 0xC0B6, mac_ocp_data);

    /* Fill tally counter address. */
    RTL_W32(tp, CounterAddrHigh, (statPhyAddr >> 32));
    RTL_W32(tp, CounterAddrLow, (statPhyAddr & 0x00000000ffffffff));

    /* Enable extended tally counter. */
    rtl8125_set_mac_ocp_bit(tp, 0xEA84, (BIT_1 | BIT_0));

    /* Setup the descriptor rings. */
    txTailPtr0 = txClosePtr0 = 0;
    txNextDescIndex = txDirtyDescIndex = 0;
    txNumFreeDesc = kNumTxDesc;
    rxNextDescIndex = 0;
    
    /* Fill in the decriptor ring addresses. */
    RTL_W32(tp, TxDescStartAddrLow, (txPhyAddr & 0x00000000ffffffff));
    RTL_W32(tp, TxDescStartAddrHigh, (txPhyAddr >> 32));
    RTL_W32(tp, RxDescAddrLow, (rxPhyAddr & 0x00000000ffffffff));
    RTL_W32(tp, RxDescAddrHigh, (rxPhyAddr >> 32));

    /* Set DMA burst size and Interframe Gap Time */
    RTL_W32(tp, TxConfig, (TX_DMA_BURST_unlimited << TxDMAShift) |
            (InterFrameGap << TxInterFrameGapShift));

    /* Enable TxNoClose. */
    RTL_W32(tp, TxConfig, (RTL_R32(tp, TxConfig) | BIT_6));
    
    rtl8125_set_l1_l0s_entry_latency(tp);
    rtl812xSetMrrs(tp, 0x50);

    /* Enable TCAM. */
    if (tp->HwSuppTcamVer == 1)
        RTL_W16(tp, 0x382, 0x221B);

    /* Disable RSS. */
    RTL_W8(tp, RSS_CTRL_8125, 0x00);
    RTL_W16(tp, Q_NUM_CTRL_8125, 0x0000);

    /* Meaning unknown. */
    RTL_W8(tp, Config1, RTL_R8(tp, Config1) & ~0x10);

    rtl8125_mac_ocp_write(tp, 0xC140, 0xFFFF);
    rtl8125_mac_ocp_write(tp, 0xC142, 0xFFFF);

    /*
     * Disabling the new tx descriptor format seems to prevent
     * tx timeouts when using TSO.
     */
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xEB58);
    
    if (tp->mcfg == CFG_METHOD_32 || tp->mcfg == CFG_METHOD_33)
        mac_ocp_data &= ~(BIT_0 | BIT_1);
    else
        mac_ocp_data &= ~(BIT_0);

#ifdef USE_NEW_TX_DESC
    mac_ocp_data |= (BIT_0);
#endif  /* USE_NEW_TX_DESC */
    
    rtl8125_mac_ocp_write(tp, 0xEB58, mac_ocp_data);

    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xE614);
    mac_ocp_data &= ~( BIT_10 | BIT_9 | BIT_8);
    
    if (tp->mcfg == CFG_METHOD_31 || tp->mcfg == CFG_METHOD_32 ||
        tp->mcfg == CFG_METHOD_33) {
        /* RTL8126A */
        mac_ocp_data |= 0x0400;
    } else if (tp->mcfg == CFG_METHOD_4 || tp->mcfg == CFG_METHOD_5) {
        /* RTL8125B */
        mac_ocp_data |= 0x0200;
    } else {
        mac_ocp_data |= 0x0300;
    }
    rtl8125_mac_ocp_write(tp, 0xE614, mac_ocp_data);
    
    //rtl8125_set_tx_q_num(tp, tp->HwSuppNumTxQueues);
    
    /* Set tx queue num to one. */
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xE63E);
    mac_ocp_data &= ~(BIT_11 | BIT_10);
    //mac_ocp_data |= ((0 & 0x03) << 10);
    rtl8125_mac_ocp_write(tp, 0xE63E, mac_ocp_data);

    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xE63E);
    mac_ocp_data &= ~(BIT_5 | BIT_4);
    
    if (tp->mcfg == CFG_METHOD_2 || tp->mcfg == CFG_METHOD_3)
        mac_ocp_data |= ((0x02 & 0x03) << 4);
    
    rtl8125_mac_ocp_write(tp, 0xE63E, mac_ocp_data);
    
    /* Disable MCU. */
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xC0B4);
    mac_ocp_data &= ~BIT_0;
    rtl8125_mac_ocp_write(tp, 0xC0B4, mac_ocp_data);
    
    /* Reenable MCU. */
    mac_ocp_data |= BIT_0;
    rtl8125_mac_ocp_write(tp, 0xC0B4, mac_ocp_data);
    
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xC0B4);
    mac_ocp_data |= (BIT_3|BIT_2);
    rtl8125_mac_ocp_write(tp, 0xC0B4, mac_ocp_data);
    
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xEB6A);
    mac_ocp_data &= ~(BIT_7 | BIT_6 | BIT_5 | BIT_4 | BIT_3 | BIT_2 | BIT_1 | BIT_0);
    mac_ocp_data |= (BIT_5 | BIT_4 | BIT_1 | BIT_0);
    rtl8125_mac_ocp_write(tp, 0xEB6A, mac_ocp_data);
    
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xEB50);
    mac_ocp_data &= ~(BIT_9 | BIT_8 | BIT_7 | BIT_6 | BIT_5);
    mac_ocp_data |= (BIT_6);
    rtl8125_mac_ocp_write(tp, 0xEB50, mac_ocp_data);
    
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xE056);
    mac_ocp_data &= ~(BIT_7 | BIT_6 | BIT_5 | BIT_4);
    mac_ocp_data |= (BIT_4 | BIT_5);
    rtl8125_mac_ocp_write(tp, 0xE056, mac_ocp_data);
    
    RTL_W8(tp, TDFNR, 0x10);
    
    RTL_W8(tp, 0xD0, RTL_R8(tp, 0xD0) | BIT_7);
    
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xE040);
    mac_ocp_data &= ~(BIT_12);
    rtl8125_mac_ocp_write(tp, 0xE040, mac_ocp_data);
    
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xEA1C);
    mac_ocp_data &= ~(BIT_1 | BIT_0);
    mac_ocp_data |= (BIT_0);
    rtl8125_mac_ocp_write(tp, 0xEA1C, mac_ocp_data);
    
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xE0C0);
    mac_ocp_data &= ~(BIT_14 | BIT_11 | BIT_10 | BIT_9 | BIT_8 | BIT_3 | BIT_2 | BIT_1 | BIT_0);
    mac_ocp_data |= (BIT_14 | BIT_10 | BIT_1 | BIT_0);
    rtl8125_mac_ocp_write(tp, 0xE0C0, mac_ocp_data);
    
    SetMcuAccessRegBit(tp, 0xE052, (BIT_6|BIT_5|BIT_3));
    ClearMcuAccessRegBit(tp, 0xE052, BIT_7);
    
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xD430);
    mac_ocp_data &= ~(BIT_11 | BIT_10 | BIT_9 | BIT_8 | BIT_7 | BIT_6 | BIT_5 | BIT_4 | BIT_3 | BIT_2 | BIT_1 | BIT_0);
    mac_ocp_data |= 0x47F;
    rtl8125_mac_ocp_write(tp, 0xD430, mac_ocp_data);
    
    //rtl8125_mac_ocp_write(tp, 0xE0C0, 0x4F87);
    RTL_W8(tp, 0xD0, RTL_R8(tp, 0xD0) | BIT_6 | BIT_7);
    
    if (tp->mcfg == CFG_METHOD_2 || tp->mcfg == CFG_METHOD_3)
        RTL_W8(tp, 0xD3, RTL_R8(tp, 0xD3) | BIT_0);
    
    rtl8125_disable_eee_plus(tp);
    
    mac_ocp_data = rtl8125_mac_ocp_read(tp, 0xEA1C);
    mac_ocp_data &= ~(BIT_2);
    
    if (tp->mcfg == CFG_METHOD_32 || tp->mcfg == CFG_METHOD_33)
        mac_ocp_data &= ~(BIT_9 | BIT_8);

    rtl8125_mac_ocp_write(tp, 0xEA1C, mac_ocp_data);

    /* Disable rx decriptor type 4. */
    if (tp->HwSuppRxDescType == RX_DESC_RING_TYPE_4)
        RTL_W8(tp, 0xd8, RTL_R8(tp, 0xd8) & ~EnableRxDescV4_0);

    /* Clear tcam entries. */
    if (HW_SUPPORT_TCAM(tp)) {
        SetMcuAccessRegBit(tp, 0xEB54, BIT_0);
        udelay(1);
        ClearMcuAccessRegBit(tp, 0xEB54, BIT_0);
    }

    RTL_W16(tp, 0x1880, RTL_R16(tp, 0x1880) & ~(BIT_4 | BIT_5));

    //other hw parameters
    rtl8125_hw_clear_timer_int(tp);

    rtl8125_hw_clear_int_miti(tp);

    rtl8125_enable_exit_l1_mask(tp);

    /* Meaning unknown. */
    rtl8125_mac_ocp_write(tp, 0xE098, 0xC302);

    if ((tp->aspm) && (tp->org_pci_offset_99 & (BIT_2 | BIT_5 | BIT_6)))
        rtl8125_init_pci_offset_99(tp);
    else
        rtl8125_disable_pci_offset_99(tp);

    if ((tp->aspm) && (tp->org_pci_offset_180 & rtl8125_get_l1off_cap_bits(tp)))
        rtl8125_init_pci_offset_180(tp);
    else
        rtl8125_disable_pci_offset_180(tp);

    if (tp->RequiredPfmPatch)
        rtl8125_set_pfm_patch(tp, 0);

    tp->cp_cmd &= ~(EnableBist | Macdbgo_oe | Force_halfdup |
                    Force_rxflow_en | Force_txflow_en | Cxpl_dbg_sel |
                    ASF | Macdbgo_sel);

    RTL_W16(tp, CPlusCmd, tp->cp_cmd);

    for (timeout = 0; timeout < 10; timeout++) {
        if ((rtl8125_mac_ocp_read(tp, 0xE00E) & BIT_13) == 0)
            break;
        mdelay(1);
    }
    /* Adjust the maximum receive size to mtu. */
    //RTL_W16(tp, RxMaxSize, mtu + (ETH_HLEN + VLAN_HLEN + ETH_FCS_LEN));
    rtl8125_set_rms(tp, tp->rms);

    rtl8125_disable_rxdvgate(tp);

    /* Set receiver mode. */
    setMulticastMode(test_bit(__M_CAST, &stateFlags));

    #ifdef ENABLE_DASH_SUPPORT
            if (tp->DASH && !tp->dash_printer_enabled)
                    NICChkTypeEnableDashInterrupt(tp);
    #endif

    rtl8125_enable_aspm_clkreq_lock(tp, tp->aspm ? 1 : 0);

    RTL_W8(tp, Cfg9346, RTL_R8(tp, Cfg9346) & ~Cfg9346_Unlock);
    
    /* Enable all known interrupts by setting the interrupt mask. */
    RTL_W32(tp, IMR0_8125, intrMask);

    udelay(10);
}

void RTL8125::rtl812xSetOffloadFeatures(bool active)
{
    ifnet_t ifnet = netif->getIfnet();
    ifnet_offload_t offload;
    UInt32 mask = 0;

    if (enableTSO4)
        mask |= IFNET_TSO_IPV4;
    
    if (enableTSO6)
        mask |= IFNET_TSO_IPV6;

    offload = ifnet_offload(ifnet);
    
    if (active) {
        offload |= mask;
        DebugLog("Enable hardware offload features: %x!\n", mask);
    } else {
        offload &= ~mask;
        DebugLog("Disable hardware offload features: %x!\n", mask);
    }
    
    if (ifnet_set_offload(ifnet, offload))
        IOLog("Error setting hardware offload: %x!\n", offload);
}

void RTL8125::rtl812xSetPhyMedium(struct rtl8125_private *tp, UInt8 autoneg, UInt32 speed, UInt8 duplex, UInt64 adv)
{
    int auto_nego = 0;
    int giga_ctrl = 0;
    int ctrl_2500 = 0;

    DebugLog("speed: %u, duplex: %u, adv: %llx\n", static_cast<unsigned int>(speed), duplex, adv);
    
    if (!rtl8125_is_speed_mode_valid(speed)) {
        if (HW_SUPP_PHY_LINK_SPEED_5000M(tp))
            speed = SPEED_5000;
        else if (HW_SUPP_PHY_LINK_SPEED_2500M(tp))
            speed = SPEED_2500;
        else
            speed = SPEED_1000;

        duplex = DUPLEX_FULL;
        adv |= tp->advertising;
    }
    
    /* Enable or disable EEE support according to selected medium. */
    if (tp->eee.eee_enabled && (autoneg == AUTONEG_ENABLE)) {
        rtl8125_enable_eee(tp);
        DebugLog("Enable EEE support.\n");
    } else {
        rtl8125_disable_eee(tp);
        DebugLog("Disable EEE support.\n");
    }
    rtl8125_disable_giga_lite(tp);

    giga_ctrl = rtl8125_mdio_read(tp, MII_CTRL1000);
    giga_ctrl &= ~(ADVERTISE_1000HALF | ADVERTISE_1000FULL);
    ctrl_2500 = mdio_direct_read_phy_ocp(tp, 0xA5D4);
    ctrl_2500 &= ~(RTK_ADVERTISE_2500FULL | RTK_ADVERTISE_5000FULL);

    if (autoneg == AUTONEG_ENABLE) {
        /*n-way force*/
        auto_nego = rtl8125_mdio_read(tp, MII_ADVERTISE);
        auto_nego &= ~(ADVERTISE_10HALF | ADVERTISE_10FULL |
                       ADVERTISE_100HALF | ADVERTISE_100FULL |
                       ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);

        if (adv & ADVERTISED_10baseT_Half)
                auto_nego |= ADVERTISE_10HALF;
        
        if (adv & ADVERTISED_10baseT_Full)
                auto_nego |= ADVERTISE_10FULL;
        
        if (adv & ADVERTISED_100baseT_Half)
                auto_nego |= ADVERTISE_100HALF;
        
        if (adv & ADVERTISED_100baseT_Full)
                auto_nego |= ADVERTISE_100FULL;
        
        if (adv & ADVERTISED_1000baseT_Half)
                giga_ctrl |= ADVERTISE_1000HALF;
        
        if (adv & ADVERTISED_1000baseT_Full)
                giga_ctrl |= ADVERTISE_1000FULL;
        
        if (adv & ADVERTISED_2500baseX_Full)
                ctrl_2500 |= RTK_ADVERTISE_2500FULL;
        
        if (HW_SUPP_PHY_LINK_SPEED_5000M(tp)) {
            if (adv & RTK_ADVERTISED_5000baseX_Full)
                ctrl_2500 |= RTK_ADVERTISE_5000FULL;
        }

        //flow control
        if (tp->fcpause == rtl8125_fc_full)
            auto_nego |= ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM;

        tp->phy_auto_nego_reg = auto_nego;
        tp->phy_1000_ctrl_reg = giga_ctrl;

        tp->phy_2500_ctrl_reg = ctrl_2500;

        rtl8125_mdio_write(tp, 0x1f, 0x0000);
        rtl8125_mdio_write(tp, MII_ADVERTISE, auto_nego);
        rtl8125_mdio_write(tp, MII_CTRL1000, giga_ctrl);
        mdio_direct_write_phy_ocp(tp, 0xA5D4, ctrl_2500);
        rtl8125_phy_restart_nway(tp);
    } else {
        /*true force*/
        if (speed == SPEED_10 || speed == SPEED_100)
            rtl8125_phy_setup_force_mode(tp, speed, duplex);
        else
            return;
    }
    tp->autoneg = autoneg;
    tp->speed = speed;
    tp->duplex = duplex;
    tp->advertising = adv;

    rtl8125_set_d0_speedup_speed(tp);
}

void RTL8125::rtl812xLinkOnPatch(struct rtl8125_private *tp)
{
    UInt32 status;

    rtl812xHwConfig(tp);

    status = RTL_R32(tp, PHYstatus);

    if (tp->mcfg == CFG_METHOD_2) {
        if (status & FullDup)
            RTL_W32(tp, TxConfig, (RTL_R32(tp, TxConfig) | (BIT_24 | BIT_25)) & ~BIT_19);
        else
            RTL_W32(tp, TxConfig, (RTL_R32(tp, TxConfig) | BIT_25) & ~(BIT_19 | BIT_24));
    }

    switch (tp->mcfg) {
        case CFG_METHOD_2:
        case CFG_METHOD_3:
        case CFG_METHOD_4:
        case CFG_METHOD_5:
        case CFG_METHOD_6:
        case CFG_METHOD_7:
        case CFG_METHOD_8:
        case CFG_METHOD_9:
        case CFG_METHOD_12:
        case CFG_METHOD_31:
        case CFG_METHOD_32:
        case CFG_METHOD_33:
            if (status & _10bps)
                rtl8125_enable_eee_plus(tp);
            break;
            
        default:
            break;
    }

    if (tp->RequiredPfmPatch)
        rtl8125_set_pfm_patch(tp, (status & _10bps) ? 1 : 0);
    
    tp->phy_reg_aner = rtl8125_mdio_read(tp, MII_EXPANSION);
    tp->phy_reg_anlpar = rtl8125_mdio_read(tp, MII_LPA);
    tp->phy_reg_gbsr = rtl8125_mdio_read(tp, MII_STAT1000);
    tp->phy_reg_status_2500 = mdio_direct_read_phy_ocp(tp, 0xA5D6);
}

void RTL8125::rtl812xLinkDownPatch(struct rtl8125_private *tp)
{
    tp->phy_reg_aner = 0;
    tp->phy_reg_anlpar = 0;
    tp->phy_reg_gbsr = 0;
    tp->phy_reg_status_2500 = 0;

    switch (tp->mcfg) {
        case CFG_METHOD_2:
        case CFG_METHOD_3:
        case CFG_METHOD_4:
        case CFG_METHOD_5:
        case CFG_METHOD_6:
        case CFG_METHOD_7:
        case CFG_METHOD_8:
        case CFG_METHOD_9:
        case CFG_METHOD_12:
            rtl8125_disable_eee_plus(tp);
            break;
            
        default:
            break;
    }
    if (tp->RequiredPfmPatch)
        rtl8125_set_pfm_patch(tp, 1);

    rtl8125_hw_reset(tp);
}

void RTL8125::rtl812xGetEEEMode(struct rtl8125_private *tp)
{
    UInt32 adv, lp, sup;
    UInt16 val;
    
    /* Get supported EEE. */
    //val = rtl8125_mdio_direct_read_phy_ocp(tp, 0xA5C4);
    //sup = mmd_eee_cap_to_ethtool_sup_t(val);
    sup = tp->eee.supported;
    DebugLog("EEE supported: 0x%0x\n", sup);

    /* Get advertisement EEE */
    val = mdio_direct_read_phy_ocp(tp, 0xA5D0);
    adv = mmd_eee_adv_to_ethtool_adv_t(val);
    
    val = mdio_direct_read_phy_ocp(tp, 0xA6D4);
    
    if (val & RTK_EEE_ADVERTISE_2500FULL)
        adv |= ADVERTISED_2500baseX_Full;

    DebugLog("EEE advertised: 0x%0x\n", adv);

    /* Get LP advertisement EEE */
    val = mdio_direct_read_phy_ocp(tp, 0xA5D2);
    lp = mmd_eee_adv_to_ethtool_adv_t(val);
    DebugLog("EEE link partner: 0x%0x\n", lp);

    val = mdio_direct_read_phy_ocp(tp, 0xA6D0);
    
    if (val & RTK_LPA_EEE_ADVERTISE_2500FULL)
        lp |= ADVERTISED_2500baseX_Full;
    
    val = rtl8125_mac_ocp_read(tp, 0xE040);
    val &= BIT_1 | BIT_0;

    tp->eee.eee_enabled = !!val;
    tp->eee.eee_active = !!(sup & adv & lp);

}

UInt32 RTL8125::rtl812xGetHwCloPtr(struct rtl8125_private *tp)
{
    UInt32 cloPtr;
    
    if (tp->HwSuppTxNoCloseVer == 3)
        cloPtr = RTL_R16(tp, tp->HwCloPtrReg);
    else
        cloPtr = RTL_R32(tp, tp->HwCloPtrReg);

    return cloPtr;
}

void RTL8125::rtl812xDoorbell(struct rtl8125_private *tp, UInt32 txTailPtr)
{
    if (tp->HwSuppTxNoCloseVer > 3)
        RTL_W32(tp, tp->SwTailPtrReg, txTailPtr);
    else
        RTL_W16(tp, tp->SwTailPtrReg, txTailPtr & 0xffff);
}

/* Set PCI configuration space offset 0x79 to setting. */

void RTL8125::rtl812xSetMrrs(struct rtl8125_private *tp, UInt8 setting)
{
    UInt8 devctl;
    
    devctl = pciDevice->extendedConfigRead8(0x79);
    devctl &= ~0x70;
    devctl |= setting;
    pciDevice->extendedConfigWrite8(0x79, devctl);
}
