/* RTL8125.hpp -- RTL812x driver class implementation.
*
* Copyright (c) 2025 Laura Müller <laura-mueller@uni-duesseldorf.de>
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
* Driver for Realtek RTL812x PCIe 2.5/5/10Gbit Ethernet controllers.
*
* This driver is based on version 9.016.01 of Realtek's r8125 driver.
*/

#include "RTL8125Lucy.hpp"
#include "RTL8125LucyRxPool.hpp"

#pragma mark --- static data ---

#define _R(NAME,SNAME,MAC,RCR,MASK,JumFrameSz) \
    { .name = NAME, .speed_name = SNAME, .mcfg = MAC, .RCR_Cfg = RCR, .RxConfigMask = MASK, .jumbo_frame_sz = JumFrameSz }

const struct RtlChipInfo rtlChipInfo[NUM_CHIPS] {
    _R("RTL8125A",
    "2.5",
    CFG_METHOD_2,
    Rx_Fetch_Number_8 | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_256 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8125A",
    "2.5",
    CFG_METHOD_3,
    Rx_Fetch_Number_8 | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_256 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8125B",
    "2.5",
    CFG_METHOD_4,
    Rx_Fetch_Number_8 | RxCfg_pause_slot_en | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_256 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8125B",
    "2.5",
    CFG_METHOD_5,
    Rx_Fetch_Number_8 | RxCfg_pause_slot_en | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_256 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8168KB",
    "2.5",
    CFG_METHOD_6,
    Rx_Fetch_Number_8 | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_256 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8168KB",
    "2.5",
    CFG_METHOD_7,
    Rx_Fetch_Number_8 | RxCfg_pause_slot_en | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_256 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8125BP",
    "2.5",
    CFG_METHOD_8,
    Rx_Fetch_Number_8 | Rx_Close_Multiple | RxCfg_pause_slot_en | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_256 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8125BP",
    "2.5",
    CFG_METHOD_9,
    Rx_Fetch_Number_8 | Rx_Close_Multiple | RxCfg_pause_slot_en | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_256 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8125D",
    "2.5",
    CFG_METHOD_10,
    Rx_Fetch_Number_8 | Rx_Close_Multiple | RxCfg_pause_slot_en | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_256 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8125D",
    "2.5",
    CFG_METHOD_11,
    Rx_Fetch_Number_8 | Rx_Close_Multiple | RxCfg_pause_slot_en | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_256 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8125CP",
    "2.5",
    CFG_METHOD_12,
    Rx_Fetch_Number_8 | Rx_Close_Multiple | RxCfg_pause_slot_en | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_256 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8168KD",
    "2.5",
    CFG_METHOD_13,
    Rx_Fetch_Number_8 | Rx_Close_Multiple | RxCfg_pause_slot_en | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_256 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),
    
    _R("RTL8126A",
    "5",
    CFG_METHOD_31,
    Rx_Fetch_Number_8 | RxCfg_pause_slot_en | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_512 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8126A",
    "5",
    CFG_METHOD_32,
    Rx_Fetch_Number_8 | Rx_Close_Multiple | RxCfg_pause_slot_en | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_512 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("RTL8126A",
    "5",
    CFG_METHOD_33,
    Rx_Fetch_Number_8 | Rx_Close_Multiple | RxCfg_pause_slot_en | EnableInnerVlan | EnableOuterVlan | (RX_DMA_BURST_512 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_9k),

    _R("Unknown",
    "2.5",
    CFG_METHOD_DEFAULT,
    (RX_DMA_BURST_512 << RxCfgDMAShift),
    0xff7e5880,
    Jumbo_Frame_1k)
};
#undef _R

/* Power Management Support */
static IOPMPowerState powerStateArray[kPowerStateCount] =
{
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMDeviceUsable, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

static unsigned const ethernet_polynomial = 0x04c11db7U;

#pragma mark --- function prototypes ---

static inline void prepareTSO4(mbuf_t m, UInt32 *tcpOffset, UInt32 *mss);
static inline void prepareTSO6(mbuf_t m, UInt32 *tcpOffset, UInt32 *mss);

static inline u32 ether_crc(int length, unsigned char *data);

#pragma mark --- public methods ---

OSDefineMetaClassAndStructors(RTL8125, super)

/* IOService (or its superclass) methods. */

bool RTL8125::init(OSDictionary *properties)
{
    bool result;
    
    result = super::init(properties);
    
    if (result) {
        workLoop = NULL;
        commandGate = NULL;
        pciDevice = NULL;
        mediumDict = NULL;
        txQueue = NULL;
        interruptSource = NULL;
        timerSource = NULL;
        netif = NULL;
        netStats = NULL;
        etherStats = NULL;
        baseMap = NULL;
        rxPool = NULL;
        txMbufCursor = NULL;
        rxBufArrayMem = NULL;
        txBufArrayMem = NULL;
        statBufDesc = NULL;
        statPhyAddr = (IOPhysicalAddress64)NULL;
        statData = NULL;
        rxPacketHead = NULL;
        rxPacketTail = NULL;
        rxPacketSize = 0;
        
        /* Initialize state flags. */
        stateFlags = 0;
        
        mtu = ETH_DATA_LEN;
        powerState = 0;
        pciDeviceData.vendor = 0;
        pciDeviceData.device = 0;
        pciDeviceData.subsystem_vendor = 0;
        pciDeviceData.subsystem_device = 0;
        memset(&linuxData, 0, sizeof(struct rtl8125_private));
        linuxData.pci_dev = &pciDeviceData;
        
#ifdef ENABLE_USE_FIRMWARE_FILE
        linuxData.fw_name = NULL;
        linuxData.rtl_fw = NULL;
        
        firmware.fw.size = 0;
        firmware.fw.data = NULL;
        firmware.fw.priv = NULL;
#endif  /* ENABLE_USE_FIRMWARE_FILE */

        rtlChipInfos = &rtlChipInfo[0];
        timerValue = 0;
        enableTSO4 = false;
        enableTSO6 = false;
        wolCapable = false;
        enableGigaLite = false;
        pciPMCtrlOffset = 0;
        pcieCapOffset = 0;

        memset(fallBackMacAddr.bytes, 0, kIOEthernetAddressSize);
        nanoseconds_to_absolutetime(kStatDelayTime, &statDelay);
        nanoseconds_to_absolutetime(kTimespan4ms, &updatePeriod);

#ifdef DEBUG_INTR
        lastRxIntrupts = lastTxIntrupts = lastTmrIntrupts = tmrInterrupts = 0;
        maxRxPkt = 0;
        maxTxPkt = 0;
#endif
    }
    
done:
    return result;
}

void RTL8125::free()
{
    UInt32 i;
    
    DebugLog("free() ===>\n");
    
    if (workLoop) {
        if (interruptSource) {
            workLoop->removeEventSource(interruptSource);
            RELEASE(interruptSource);
        }
        if (timerSource) {
            workLoop->removeEventSource(timerSource);
            RELEASE(timerSource);
        }
        workLoop->release();
        workLoop = NULL;
    }
    RELEASE(commandGate);
    RELEASE(txQueue);
    RELEASE(mediumDict);
    
    for (i = MIDX_AUTO; i < MIDX_COUNT; i++)
        mediumTable[i] = NULL;
    
    RELEASE(baseMap);
    linuxData.mmio_addr = NULL;
    
    RELEASE(pciDevice);
    freeTxResources();
    freeRxResources();
    freeStatResources();
    
    DebugLog("free() <===\n");
    
    super::free();
}

bool RTL8125::start(IOService *provider)
{
    bool result;
    
    result = super::start(provider);
    
    if (!result) {
        IOLog("IOEthernetController::start failed.\n");
        goto done;
    }
    clear_mask((__M_CAST_M | __PROMISC_M), &stateFlags);
    multicastFilter = 0;

    pciDevice = OSDynamicCast(IOPCIDevice, provider);
    
    if (!pciDevice) {
        IOLog("No provider.\n");
        goto done;
    }
    pciDevice->retain();
    
    if (!pciDevice->open(this)) {
        IOLog("Failed to open provider.\n");
        goto error_open;
    }
    mapper = IOMapper::copyMapperForDevice(pciDevice);

    getParams();
        
    if (!initPCIConfigSpace(pciDevice)) {
        goto error_cfg;
    }
    
    if (!rtl812xInit()) {
        IOLog("Failed to initialize chip.\n");
        goto error_cfg;
    }
    
    if (!setupMediumDict()) {
        IOLog("Failed to setup medium dictionary.\n");
        goto error_cfg;
    }
    commandGate = getCommandGate();
    
    if (!commandGate) {
        IOLog("getCommandGate() failed.\n");
        goto error_gate;
    }
    commandGate->retain();
    
    if (!setupTxResources()) {
        IOLog("Error allocating Tx resources.\n");
        goto error_dma1;
    }

    if (!setupRxResources()) {
        IOLog("Error allocating Rx resources.\n");
        goto error_dma2;
    }

    if (!setupStatResources()) {
        IOLog("Error allocating Stat resources.\n");
        goto error_dma3;
    }

    if (!initEventSources(provider)) {
        IOLog("initEventSources() failed.\n");
        goto error_src;
    }
    
    result = attachInterface(reinterpret_cast<IONetworkInterface**>(&netif));

    if (!result) {
        IOLog("attachInterface() failed.\n");
        goto error_src;
    }
    pciDevice->close(this);
    result = true;
    
done:
    return result;

error_src:
    freeStatResources();

error_dma3:
    freeRxResources();

error_dma2:
    freeTxResources();
    
error_dma1:
    RELEASE(commandGate);
        
error_gate:
    RELEASE(mediumDict);

error_cfg:
    pciDevice->close(this);

error_open:
    pciDevice->release();
    pciDevice = NULL;
    goto done;
}

void RTL8125::stop(IOService *provider)
{
    UInt32 i;
    
    if (netif) {
        detachInterface(netif);
        netif = NULL;
    }
    if (workLoop) {
        if (interruptSource) {
            workLoop->removeEventSource(interruptSource);
            RELEASE(interruptSource);
        }
        if (timerSource) {
            workLoop->removeEventSource(timerSource);
            RELEASE(timerSource);
        }
        workLoop->release();
        workLoop = NULL;
    }
    RELEASE(commandGate);
    RELEASE(txQueue);
    RELEASE(mediumDict);
    
    for (i = MIDX_AUTO; i < MIDX_COUNT; i++)
        mediumTable[i] = NULL;

    freeStatResources();
    freeRxResources();
    freeTxResources();

    RELEASE(baseMap);
    linuxData.mmio_addr = NULL;

    RELEASE(pciDevice);
    
    super::stop(provider);
}

IOReturn RTL8125::registerWithPolicyMaker(IOService *policyMaker)
{
    DebugLog("registerWithPolicyMaker() ===>\n");
    
    powerState = kPowerStateOn;
    
    DebugLog("registerWithPolicyMaker() <===\n");

    return policyMaker->registerPowerDriver(this, powerStateArray, kPowerStateCount);
}

IOReturn RTL8125::setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker)
{
    IOReturn result = IOPMAckImplied;
    
    DebugLog("setPowerState() ===>\n");
        
    if (powerStateOrdinal == powerState) {
        DebugLog("Already in power state %lu.\n", powerStateOrdinal);
        goto done;
    }
    DebugLog("switching to power state %lu.\n", powerStateOrdinal);
    
    if (powerStateOrdinal == kPowerStateOff)
        commandGate->runAction(setPowerStateSleepAction);
    else
        commandGate->runAction(setPowerStateWakeAction);

    powerState = powerStateOrdinal;
    
done:
    DebugLog("setPowerState() <===\n");

    return result;
}

void RTL8125::systemWillShutdown(IOOptionBits specifier)
{
    DebugLog("systemWillShutdown() ===>\n");
    
    if ((kIOMessageSystemWillPowerOff | kIOMessageSystemWillRestart) & specifier) {
        disable(netif);
        
        /* Restore the original MAC address. */
        rtl812x_rar_set(&linuxData, (UInt8 *)&origMacAddr.bytes);
    }
    
    DebugLog("systemWillShutdown() <===\n");

    /* Must call super shutdown or system will stall. */
    super::systemWillShutdown(specifier);
}

/* IONetworkController methods. */
IOReturn RTL8125::enable(IONetworkInterface *netif)
{
    const IONetworkMedium *selectedMedium;
    IOReturn result = kIOReturnError;
    
    DebugLog("enable() ===>\n");

    if (test_bit(__ENABLED, &stateFlags)) {
        DebugLog("Interface already enabled.\n");
        result = kIOReturnSuccess;
        goto done;
    }
    if (!pciDevice || pciDevice->isOpen()) {
        IOLog("Unable to open PCI device.\n");
        goto done;
    }
    pciDevice->open(this);
    
    selectedMedium = getSelectedMedium();
    
    if (!selectedMedium) {
        DebugLog("No medium selected. Falling back to autonegotiation.\n");
        selectedMedium = mediumTable[MIDX_AUTO];
    }
    selectMedium(selectedMedium);
    rtl812xEnable();
    
    /* We have to enable the interrupt because we are using a msi interrupt. */
    interruptSource->enable();

    rxPacketHead = rxPacketTail = NULL;
    rxPacketSize = 0;
    txDescDoneCount = txDescDoneLast = 0;
    deadlockWarn = 0;
    set_bit(__ENABLED, &stateFlags);
    clear_bit(__POLL_MODE, &stateFlags);

    result = kIOReturnSuccess;
    
    DebugLog("enable() <===\n");

done:
    return result;
}

IOReturn RTL8125::disable(IONetworkInterface *netif)
{
    UInt64 timeout;
    UInt64 delay;
    UInt64 now;
    UInt64 t;

    DebugLog("disable() ===>\n");
    
    if (!test_bit(__ENABLED, &stateFlags))
        goto done;
    
    netif->stopOutputThread();
    netif->flushOutputQueue();
    
    if (test_bit(__POLLING, &stateFlags)) {
        nanoseconds_to_absolutetime(5000, &delay);
        clock_get_uptime(&now);
        timeout = delay * 10;
        t = delay;

        while (test_bit(__POLLING, &stateFlags) && (t < timeout)) {
            clock_delay_until(now + t);
            t += delay;
        }
    }
    clear_mask((__ENABLED_M | __LINK_UP_M | __POLL_MODE_M | __POLLING_M), &stateFlags);

    timerSource->cancelTimeout();
    txDescDoneCount = txDescDoneLast = 0;

    /* Disable interrupt as we are using msi. */
    interruptSource->disable();

    rtl812xDisable();
    
    clearRxTxRings();
    
    if (pciDevice && pciDevice->isOpen())
        pciDevice->close(this);
        
    DebugLog("disable() <===\n");
    
done:
    return kIOReturnSuccess;
}

IOReturn RTL8125::outputStart(IONetworkInterface *interface, IOOptionBits options )
{
    IOPhysicalSegment txSegments[kMaxSegs];
    mbuf_t m;
    RtlTxDesc *desc;
    UInt64 pktBytes;
    IOReturn result = kIOReturnNoResources;
    UInt32 cmd;
    UInt32 opts2;
    UInt32 offloadFlags;
    UInt32 mss;
    UInt32 len;
    UInt32 tcpOff;
    UInt32 opts1;
    UInt32 vlanTag;
    UInt32 numSegs;
    UInt32 lastSeg;
    UInt32 index;
    UInt32 i;
    
    //DebugLog("outputStart() ===>\n");
    
    if (!(test_mask((__ENABLED_M | __LINK_UP_M), &stateFlags)))  {
        DebugLog("Interface down. Dropping packets.\n");
        goto done;
    }
    while ((txNumFreeDesc > kMinFreeDescs) && (interface->dequeueOutputPackets(1, &m, NULL, NULL, &pktBytes) == kIOReturnSuccess)) {
        cmd = 0;
        opts2 = 0;

        /* Get the packet length. */
        len = (UInt32)mbuf_pkthdr_len(m);

        if (mbuf_get_tso_requested(m, &offloadFlags, &mss)) {
            DebugLog("mbuf_get_tso_requested() failed. Dropping packet.\n");
            mbuf_freem_list(m);
            continue;
        }
        if (offloadFlags & (MBUF_TSO_IPV4 | MBUF_TSO_IPV6)) {
            if (offloadFlags & MBUF_TSO_IPV4) {
                if ((len - ETH_HLEN) > mtu) {
                    /*
                     * Fix the pseudo header checksum, get the
                     * TCP header offset and set paylen.
                     */
                    prepareTSO4(m, &tcpOff, &mss);
                    
                    cmd = (GiantSendv4 | (tcpOff << GTTCPHO_SHIFT));
                    opts2 = ((mss & MSSMask) << MSSShift);
                } else {
                    /*
                     * There is no need for a TSO4 operation as the packet
                     * can be sent in one frame.
                     */
                    offloadFlags = kChecksumTCP;
                    opts2 = (TxIPCS_C | TxTCPCS_C);
                }
            } else {
                if ((len - ETH_HLEN) > mtu) {
                    /* The pseudoheader checksum has to be adjusted first. */
                    prepareTSO6(m, &tcpOff, &mss);
                    
                    cmd = (GiantSendv6 | (tcpOff << GTTCPHO_SHIFT));
                    opts2 = ((mss & MSSMask) << MSSShift);
                } else {
                    /*
                     * There is no need for a TSO6 operation as the packet
                     * can be sent in one frame.
                     */
                    offloadFlags = kChecksumTCPIPv6;
                    opts2 = (TxTCPCS_C | TxIPV6F_C | (((ETH_HLEN + kIPv6HdrLen) & TCPHO_MAX) << TCPHO_SHIFT));
                }
            }
        } else {
            /* We use mss as a dummy here because it isn't needed anymore. */
            mbuf_get_csum_requested(m, &offloadFlags, &mss);
            
            if (offloadFlags & kChecksumTCP)
                opts2 = (TxIPCS_C | TxTCPCS_C);
            else if (offloadFlags & kChecksumTCPIPv6)
                opts2 = (TxTCPCS_C | TxIPV6F_C | (((ETH_HLEN + kIPv6HdrLen) & TCPHO_MAX) << TCPHO_SHIFT));
            else if (offloadFlags & kChecksumUDP)
                opts2 = (TxIPCS_C | TxUDPCS_C);
            else if (offloadFlags & kChecksumUDPIPv6)
                opts2 = (TxUDPCS_C | TxIPV6F_C | (((ETH_HLEN + kIPv6HdrLen) & TCPHO_MAX) << TCPHO_SHIFT));
            else if (offloadFlags & kChecksumIP)
                opts2 = TxIPCS_C;
        }
        /* Finally get the physical segments. */
        if (useAppleVTD)
            numSegs = txMapPacket(m, txSegments, kMaxSegs);
        else
            numSegs = txMbufCursor->getPhysicalSegmentsWithCoalesce(m, txSegments, kMaxSegs);

        /* Alloc required number of descriptors. As the descriptor
         * which has been freed last must be considered to be still
         * in use we never fill the ring completely but leave at
         * least one unused.
         */
        if (!numSegs) {
            DebugLog("getPhysicalSegmentsWithCoalesce() failed. Dropping packet.\n");
            mbuf_freem_list(m);
            continue;
        }
        OSAddAtomic(-numSegs, &txNumFreeDesc);
        index = txNextDescIndex;
        txNextDescIndex = (txNextDescIndex + numSegs) & kTxDescMask;
        
#ifdef ENABLE_TX_NO_CLOSE
        txTailPtr0 += numSegs;
#endif  /* ENABLE_TX_NO_CLOSE */

        lastSeg = numSegs - 1;
        
        /* Next fill in the VLAN tag. */
        opts2 |= (getVlanTagDemand(m, &vlanTag)) ? (OSSwapInt16(vlanTag) | TxVlanTag) : 0;
        
        /* And finally fill in the descriptors. */
        for (i = 0; i < numSegs; i++) {
            desc = &txDescArray[index];
            opts1 = (((UInt32)txSegments[i].length) | cmd | DescOwn);
            
            if (i == 0)
                opts1 |= FirstFrag;

            //opts1 |= (i == 0) ? (FirstFrag | DescOwn) : DescOwn;
            
            if (i == lastSeg) {
                opts1 |= LastFrag;
                txBufArray[index].mbuf = m;
                txBufArray[index].numDescs = numSegs;
                txBufArray[index].packetBytes = (UInt32)pktBytes;
            } else {
                txBufArray[index].mbuf = NULL;
                txBufArray[index].numDescs = 0;
                txBufArray[index].packetBytes = 0;
            }
            if (index == kTxLastDesc)
                opts1 |= RingEnd;
            
            desc->addr = OSSwapHostToLittleInt64(txSegments[i].location);
            desc->opts2 = OSSwapHostToLittleInt32(opts2);
            
#ifndef ENABLE_TX_NO_CLOSE
            wmb();
#endif  /* ENABLE_TX_NO_CLOSE */

            desc->opts1 = OSSwapHostToLittleInt32(opts1);
            
            //DebugLog("opts1=0x%x, opts2=0x%x, addr=0x%llx, len=0x%llx\n", opts1, opts2, txSegments[i].location, txSegments[i].length);
            ++index &= kTxDescMask;
        }
    }
    wmb();
    
#ifdef ENABLE_TX_NO_CLOSE
    /* Update tail pointer. */
    rtl812xDoorbell(&linuxData, txTailPtr0);
#else
    RTL_W16(&linuxData, TPPOLL_8125, BIT_0);
#endif  /* ENABLE_TX_NO_CLOSE */

    result = (txNumFreeDesc > kMinFreeDescs) ? kIOReturnSuccess : kIOReturnNoResources;
    
done:
    //DebugLog("outputStart() <===\n");
    
    return result;
}

void RTL8125::getPacketBufferConstraints(IOPacketBufferConstraints *constraints) const
{
    DebugLog("getPacketBufferConstraints() ===>\n");

    constraints->alignStart = kIOPacketBufferAlign1;
    constraints->alignLength = kIOPacketBufferAlign1;
    
    DebugLog("getPacketBufferConstraints() <===\n");
}

IOOutputQueue* RTL8125::createOutputQueue()
{
    DebugLog("createOutputQueue() ===>\n");
    
    DebugLog("createOutputQueue() <===\n");

    return IOBasicOutputQueue::withTarget(this);
}

const OSString* RTL8125::newVendorString() const
{
    DebugLog("newVendorString() ===>\n");
    
    DebugLog("newVendorString() <===\n");

    return OSString::withCString("Realtek");
}

const OSString* RTL8125::newModelString() const
{
    DebugLog("newModelString() ===>\n");
    DebugLog("newModelString() <===\n");
    
    return OSString::withCString(rtlChipInfo[linuxData.chipset].name);
}

bool RTL8125::configureInterface(IONetworkInterface *interface)
{
    char modelName[kNameLenght];
    IONetworkData *data;
    IOReturn error;
    bool result;

    DebugLog("configureInterface() ===>\n");

    result = super::configureInterface(interface);
    
    if (!result)
        goto done;
    
    /* Get the generic network statistics structure. */
    data = interface->getParameter(kIONetworkStatsKey);
    
    if (data) {
        netStats = (IONetworkStats *)data->getBuffer();
        
        if (!netStats) {
            IOLog("Error getting IONetworkStats\n.");
            result = false;
            goto done;
        }
    }
    /* Get the Ethernet statistics structure. */
    data = interface->getParameter(kIOEthernetStatsKey);
    
    if (data) {
        etherStats = (IOEthernetStats *)data->getBuffer();
        
        if (!etherStats) {
            IOLog("Error getting IOEthernetStats\n.");
            result = false;
            goto done;
        }
    }
    error = interface->configureOutputPullModel(kNumTxDesc, 0, 0, IONetworkInterface::kOutputPacketSchedulingModelNormal);
    
    if (error != kIOReturnSuccess) {
        IOLog("configureOutputPullModel() failed\n.");
        result = false;
        goto done;
    }
    error = interface->configureInputPacketPolling(kNumRxDesc, 0);
    
    if (error != kIOReturnSuccess) {
        IOLog("configureInputPacketPolling() failed\n.");
        result = false;
        goto done;
    }
    snprintf(modelName, kNameLenght, "Realtek %s PCIe %sGbit Ethernet", rtlChipInfo[linuxData.chipset].name, rtlChipInfo[linuxData.chipset].speed_name);
    setProperty("model", modelName);
    
    DebugLog("configureInterface() <===\n");

done:
    return result;
}

bool RTL8125::createWorkLoop()
{
    DebugLog("createWorkLoop() ===>\n");
    
    workLoop = IOWorkLoop::workLoop();
    
    DebugLog("createWorkLoop() <===\n");

    return workLoop ? true : false;
}

IOWorkLoop* RTL8125::getWorkLoop() const
{
    DebugLog("getWorkLoop() ===>\n");
    
    DebugLog("getWorkLoop() <===\n");

    return workLoop;
}

IOReturn RTL8125::setPromiscuousMode(bool active)
{
    struct rtl8125_private *tp = &linuxData;
    UInt32 *filterAddr = (UInt32 *)&multicastFilter;
    UInt32 mcFilter[2];
    UInt32 rxMode;

    DebugLog("setPromiscuousMode() ===>\n");
    
    if (active) {
        DebugLog("Promiscuous mode enabled.\n");
        rxMode = (AcceptBroadcast | AcceptMulticast | AcceptMyPhys | AcceptAllPhys);
        mcFilter[1] = mcFilter[0] = 0xffffffff;
    } else {
        DebugLog("Promiscuous mode disabled.\n");
        rxMode = (AcceptBroadcast | AcceptMulticast | AcceptMyPhys);
        mcFilter[0] = *filterAddr++;
        mcFilter[1] = *filterAddr;
    }
    rxMode |= tp->rtl8125_rx_config | (RTL_R32(&linuxData, RxConfig) & rtlChipInfo[tp->chipset].RxConfigMask);
    RTL_W32(&linuxData, RxConfig, rxMode);
    RTL_W32(&linuxData, MAR1, mcFilter[1]);
    RTL_W32(&linuxData, MAR0, mcFilter[0]);

    if (active)
        set_bit(__PROMISC, &stateFlags);
    else
        clear_bit(__PROMISC, &stateFlags);

    DebugLog("setPromiscuousMode() <===\n");

    return kIOReturnSuccess;
}

IOReturn RTL8125::setMulticastMode(bool active)
{
    struct rtl8125_private *tp = &linuxData;
    UInt32 *filterAddr = (UInt32 *)&multicastFilter;
    UInt32 mcFilter[2];
    UInt32 rxMode;

    DebugLog("setMulticastMode() ===>\n");
    
    if (active) {
        rxMode = (AcceptBroadcast | AcceptMulticast | AcceptMyPhys);
        mcFilter[0] = *filterAddr++;
        mcFilter[1] = *filterAddr;
    } else{
        rxMode = (AcceptBroadcast | AcceptMyPhys);
        mcFilter[1] = mcFilter[0] = 0;
    }
    rxMode |= tp->rtl8125_rx_config | (RTL_R32(&linuxData, RxConfig) & rtlChipInfo[tp->chipset].RxConfigMask);
    RTL_W32(&linuxData, RxConfig, rxMode);
    RTL_W32(&linuxData, MAR1, mcFilter[1]);
    RTL_W32(&linuxData, MAR0, mcFilter[0]);

    if (active)
        set_bit(__M_CAST, &stateFlags);
    else
        clear_bit(__M_CAST, &stateFlags);

    DebugLog("setMulticastMode() <===\n");
    
    return kIOReturnSuccess;
}

IOReturn RTL8125::setMulticastList(IOEthernetAddress *addrs, UInt32 count)
{
    UInt32 *filterAddr = (UInt32 *)&multicastFilter;
    UInt64 filter = 0;
    UInt32 i, bitNumber;
    
    DebugLog("setMulticastList() ===>\n");
    
    if (count <= kMCFilterLimit) {
        for (i = 0; i < count; i++, addrs++) {
            bitNumber = ether_crc(6, reinterpret_cast<unsigned char *>(addrs)) >> 26;
            filter |= (1 << (bitNumber & 0x3f));
        }
        multicastFilter = OSSwapInt64(filter);
    } else {
        multicastFilter = 0xffffffffffffffff;
    }
    RTL_W32(&linuxData, MAR1, *filterAddr);
    RTL_W32(&linuxData, MAR0, *filterAddr++);

    DebugLog("setMulticastList() <===\n");

    return kIOReturnSuccess;
}

IOReturn RTL8125::getChecksumSupport(UInt32 *checksumMask, UInt32 checksumFamily, bool isOutput)
{
    IOReturn result = kIOReturnUnsupported;

    DebugLog("getChecksumSupport() ===>\n");

    if ((checksumFamily == kChecksumFamilyTCPIP) && checksumMask) {
        if (isOutput) {
            *checksumMask = (kChecksumTCP | kChecksumUDP | kChecksumIP | kChecksumTCPIPv6 | kChecksumUDPIPv6);
        } else {
            *checksumMask = (kChecksumTCP | kChecksumUDP | kChecksumIP | kChecksumTCPIPv6 | kChecksumUDPIPv6);
        }
        result = kIOReturnSuccess;
    }
    DebugLog("getChecksumSupport() <===\n");

    return result;
}

UInt32 RTL8125::getFeatures() const
{
    UInt32 features = (kIONetworkFeatureMultiPages | kIONetworkFeatureHardwareVlan);
    
    DebugLog("getFeatures() ===>\n");
    
    if (enableTSO4)
        features |= kIONetworkFeatureTSOIPv4;
    
    if (enableTSO6)
        features |= kIONetworkFeatureTSOIPv6;
    
    DebugLog("getFeatures() <===\n");
    
    return features;
}

IOReturn RTL8125::setWakeOnMagicPacket(bool active)
{
    struct rtl8125_private *tp = &linuxData;
    IOReturn result = kIOReturnUnsupported;

    DebugLog("setWakeOnMagicPacket() ===>\n");

    if (tp->wol_opts && wolCapable) {
        tp->wol_enabled = (active) ? WOL_ENABLED : WOL_DISABLED;
        
        DebugLog("WakeOnMagicPacket %s.\n", active ? "enabled" : "disabled");

        result = kIOReturnSuccess;
    }
    
    DebugLog("setWakeOnMagicPacket() <===\n");

    return result;
}

IOReturn RTL8125::getPacketFilters(const OSSymbol *group, UInt32 *filters) const
{
    IOReturn result = kIOReturnSuccess;

    DebugLog("getPacketFilters() ===>\n");

    if ((group == gIOEthernetWakeOnLANFilterGroup) && linuxData.wol_opts && wolCapable) {
        *filters = kIOEthernetWakeOnMagicPacket;
        DebugLog("kIOEthernetWakeOnMagicPacket added to filters.\n");
    } else {
        result = super::getPacketFilters(group, filters);
    }
    
    DebugLog("getPacketFilters() <===\n");

    return result;
}

/* Methods inherited from IOEthernetController. */
IOReturn RTL8125::getHardwareAddress(IOEthernetAddress *addr)
{
    IOReturn result = kIOReturnError;
    
    DebugLog("getHardwareAddress() ===>\n");
    
    if (addr) {
        bcopy(&currMacAddr.bytes, addr->bytes, kIOEthernetAddressSize);
        result = kIOReturnSuccess;
    }
    
    DebugLog("getHardwareAddress() <===\n");

    return result;
}

IOReturn RTL8125::setHardwareAddress(const IOEthernetAddress *addr)
{
    IOReturn result = kIOReturnError;
    
    DebugLog("setHardwareAddress() ===>\n");
    
    if (addr) {
        bcopy(addr->bytes, &currMacAddr.bytes, kIOEthernetAddressSize);
        rtl812x_rar_set(&linuxData, (UInt8 *)&currMacAddr.bytes);
        result = kIOReturnSuccess;
    }
    
    DebugLog("setHardwareAddress() <===\n");
    
    return result;
}

IOReturn RTL8125::selectMedium(const IONetworkMedium *medium)
{
    struct rtl8125_private *tp = &linuxData;
    IOReturn result = kIOReturnSuccess;
    UInt32 index;
    
    DebugLog("selectMedium() ===>\n");
    
    if (medium) {
        index = medium->getIndex();
        
        rtl812xMedium2Adv(tp, index);
        setCurrentMedium(medium);
        setLinkDown();
    }
    DebugLog("selectMedium() <===\n");
    
done:
    return result;
}

#pragma mark --- jumbo frame support methods ---

IOReturn RTL8125::getMaxPacketSize(UInt32 * maxSize) const
{
    DebugLog("getMaxPacketSize() ===>\n");
        
    *maxSize = kMaxPacketSize;
    DebugLog("Maximum packet size: %u\n", *maxSize);

    DebugLog("getMaxPacketSize() <===\n");
    
    return kIOReturnSuccess;
}

IOReturn RTL8125::setMaxPacketSize(UInt32 maxSize)
{
    struct rtl8125_private *tp = &linuxData;
    IOReturn result = kIOReturnError;

    DebugLog("setMaxPacketSize() ===>\n");
    
    if (maxSize <= kMaxPacketSize) {
        mtu = maxSize - (VLAN_ETH_HLEN);
        DebugLog("maxSize: %u, mtu: %u\n", maxSize, mtu);
        
        /* Adjust maximum rx size. */
        tp->rms = mtu + VLAN_ETH_HLEN + ETH_FCS_LEN;
        
        rtl812xSetOffloadFeatures(mtu <= MSS_MAX);

        /* Force reinitialization. */
        setLinkDown();
        timerSource->cancelTimeout();
        
        tp->eee.tx_lpi_timer = mtu + ETH_HLEN + 0x20;
        rtl812xRestart(tp);
        
        result = kIOReturnSuccess;
    }
    
    DebugLog("setMaxPacketSize() <===\n");
    
    return result;
}

#pragma mark --- common interrupt methods ---

void RTL8125::pciErrorInterrupt()
{
    UInt16 cmdReg = pciDevice->configRead16(kIOPCIConfigCommand);
    UInt16 statusReg = pciDevice->configRead16(kIOPCIConfigStatus);
    
    DebugLog("PCI error: cmdReg=0x%x, statusReg=0x%x\n", cmdReg, statusReg);

    cmdReg |= (kIOPCICommandSERR | kIOPCICommandParityError);
    statusReg &= (kIOPCIStatusParityErrActive | kIOPCIStatusSERRActive | kIOPCIStatusMasterAbortActive | kIOPCIStatusTargetAbortActive | kIOPCIStatusTargetAbortCapable);
    pciDevice->configWrite16(kIOPCIConfigCommand, cmdReg);
    pciDevice->configWrite16(kIOPCIConfigStatus, statusReg);
    
    /* Reset the NIC in order to resume operation. */
    rtl812xRestart(&linuxData);
}

#ifdef ENABLE_TX_NO_CLOSE

void RTL8125::txInterrupt()
{
    struct rtl8125_private *tp = &linuxData;
    mbuf_t m;
    UInt32 nextClosePtr = rtl812xGetHwCloPtr(tp);
    UInt32 oldDirtyIndex = txDirtyDescIndex;
    UInt32 bytes = 0;
    UInt32 descs = 0;
    UInt32 n;

    n = ((nextClosePtr - txClosePtr0) & tp->MaxTxDescPtrMask);
    
    //DebugLog("txInterrupt() txClosePtr0: %u, nextClosePtr: %u, numDone: %u.\n", txClosePtr0, nextClosePtr, numDone);
    
    n = min(txNextDescIndex - txDirtyDescIndex, n);
    txClosePtr0 += n;

    while (n-- > 0) {
        m = txBufArray[txDirtyDescIndex].mbuf;
        txBufArray[txDirtyDescIndex].mbuf = NULL;
        
        if (m) {
            if (useAppleVTD)
                txUnmapPacket();

            descs += txBufArray[txDirtyDescIndex].numDescs;
            bytes += txBufArray[txDirtyDescIndex].packetBytes;
            txBufArray[txDirtyDescIndex].numDescs = 0;
            txBufArray[txDirtyDescIndex].packetBytes = 0;

            freePacket(m, kDelayFree);
        }
        txDescDoneCount++;
        OSIncrementAtomic(&txNumFreeDesc);
        ++txDirtyDescIndex &= kTxDescMask;
    }
    if (oldDirtyIndex != txDirtyDescIndex) {
        if (txNumFreeDesc > kTxQueueWakeTreshhold)
            netif->signalOutputThread();
        
        releaseFreePackets();
        OSAddAtomic(descs, &totalDescs);
        OSAddAtomic(bytes, &totalBytes);
    }
}
#else
void RTL8125::txInterrupt()
{
    mbuf_t m;
    SInt32 numDirty = kNumTxDesc - txNumFreeDesc;
    UInt32 oldDirtyIndex = txDirtyDescIndex;
    UInt32 bytes = 0;
    UInt32 descs = 0;
    UInt32 descStatus;
    
    while (numDirty-- > 0) {
        descStatus = OSSwapLittleToHostInt32(txDescArray[txDirtyDescIndex].opts1);
        
        if (descStatus & DescOwn)
            break;
        
        m = txBufArray[txDirtyDescIndex].mbuf;
        txBufArray[txDirtyDescIndex].mbuf = NULL;
        
        if (m) {
            if (useAppleVTD)
                txUnmapPacket();

            descs += txBufArray[txDirtyDescIndex].numDescs;
            bytes += txBufArray[txDirtyDescIndex].packetBytes;
            txBufArray[txDirtyDescIndex].numDescs = 0;
            txBufArray[txDirtyDescIndex].packetBytes = 0;

            freePacket(m, kDelayFree);
        }
        txDescDoneCount++;
        OSIncrementAtomic(&txNumFreeDesc);
        ++txDirtyDescIndex &= kTxDescMask;
    }
    if (oldDirtyIndex != txDirtyDescIndex) {
        if (txNumFreeDesc > kTxQueueWakeTreshhold)
            netif->signalOutputThread();
        
        releaseFreePackets();
        OSAddAtomic(descs, &totalDescs);
        OSAddAtomic(bytes, &totalBytes);
        
        RTL_W16(&linuxData, TPPOLL_8125, BIT_0);
    }
}
#endif  /* ENABLE_TX_NO_CLOSE */

UInt32 RTL8125::rxInterrupt(IONetworkInterface *interface, uint32_t maxCount, IOMbufQueue *pollQueue, void *context)
{
    RtlRxDesc *desc = &rxDescArray[rxNextDescIndex];
    mbuf_t bufPkt, newPkt;
    UInt64 addr;
    UInt64 word1;
    UInt32 descStatus1, descStatus2;
    SInt32 pktSize;
    UInt32 goodPkts = 0;
    bool replaced;
    
    while (!((descStatus1 = OSSwapLittleToHostInt32(desc->cmd.opts1)) & DescOwn) && (goodPkts < maxCount)) {
        word1 = (rxNextDescIndex == kRxLastDesc) ? (kRxBufferSize | DescOwn | RingEnd) : (kRxBufferSize | DescOwn);
        addr = rxBufArray[rxNextDescIndex].phyAddr;

        /* Drop packets with receive errors. */
        if (unlikely(descStatus1 & RxRES)) {
            DebugLog("Rx error.\n");
            
            if (descStatus1 & (RxRWT | RxRUNT))
                etherStats->dot3StatsEntry.frameTooLongs++;

            if (descStatus1 & RxCRC)
                etherStats->dot3StatsEntry.fcsErrors++;

            discardPacketFragment();
            goto nextDesc;
        }
        
        descStatus2 = OSSwapLittleToHostInt32(desc->cmd.opts2);
        pktSize = (descStatus1 & 0x1fff);
        bufPkt = rxBufArray[rxNextDescIndex].mbuf;
        //DebugLog("rxInterrupt(): descStatus1=0x%x, descStatus2=0x%x, pktSize=%u\n", descStatus1, descStatus2, pktSize);
        
        newPkt = rxPool->replaceOrCopyPacket(&bufPkt, pktSize, &replaced);
        
        if (unlikely(!newPkt)) {
            /*
             * Allocation of a new packet failed so that we must leave the
             * original packet in place.
             */
            DebugLog("replaceOrCopyPacket() failed.\n");
            etherStats->dot3RxExtraEntry.resourceErrors++;
            discardPacketFragment();
            goto nextDesc;
        }
handle_pkt:
        /* If the packet was replaced we have to update the descriptor's buffer address. */
        if (replaced) {
            if (unlikely(mbuf_next(bufPkt) != NULL)) {
                DebugLog("getPhysicalSegment() failed.\n");
                etherStats->dot3RxExtraEntry.resourceErrors++;
                discardPacketFragment();
                mbuf_freem_list(bufPkt);
                goto nextDesc;
            }
            rxBufArray[rxNextDescIndex].mbuf = bufPkt;
            addr = mbuf_data_to_physical(mbuf_datastart(bufPkt));
            rxBufArray[rxNextDescIndex].phyAddr = addr;
        }
        if (descStatus1 & LastFrag) {
            pktSize -= kIOEthernetCRCSize;
            
            if (rxPacketHead) {
                if (pktSize > 0) {
                    /* This is the last buffer of a jumbo frame. */
                    mbuf_setlen(newPkt, pktSize);

                    mbuf_setflags_mask(newPkt, 0, MBUF_PKTHDR);
                    mbuf_setnext(rxPacketTail, newPkt);
                    
                    rxPacketTail = newPkt;
                } else {
                    /*
                     * The last fragment consists only of the FCS or a part
                     * of it, so that we can drop it and adjust the packet
                     * length to exclude the FCS.
                     */
                    DebugLog("Packet size: %d. Dropping!\n", pktSize);
                    mbuf_free(newPkt);
                    mbuf_adjustlen(rxPacketTail, pktSize);
                }
                rxPacketSize += pktSize;
            } else {
                /*
                 * We've got a complete packet in one buffer.
                 * It can be enqueued directly.
                 */
                mbuf_setlen(newPkt, pktSize);

                rxPacketHead = newPkt;
                rxPacketSize = pktSize;
            }
            getChecksumResult(newPkt, descStatus1, descStatus2);
            
            /* Also get the VLAN tag if there is any. */
            if (descStatus2 & RxVlanTag)
                setVlanTag(rxPacketHead, OSSwapInt16(descStatus2 & 0xffff));
            
            mbuf_pkthdr_setlen(rxPacketHead, rxPacketSize);
            interface->enqueueInputPacket(rxPacketHead, pollQueue);
            
            rxPacketHead = rxPacketTail = NULL;
            rxPacketSize = 0;
            
            goodPkts++;
        } else {
            mbuf_setlen(newPkt, pktSize);

            if (rxPacketHead) {
                /* We are in the middle of a jumbo frame. */
                mbuf_setflags_mask(newPkt, 0, MBUF_PKTHDR);
                mbuf_setnext(rxPacketTail, newPkt);
                
                rxPacketTail = newPkt;
                rxPacketSize += pktSize;
            } else {
                /* This is the first buffer of a jumbo frame. */
                rxPacketHead = rxPacketTail = newPkt;
                rxPacketSize = pktSize;
            }
        }

        /* Finally update the descriptor and get the next one to examine. */
    nextDesc:
        desc->buf.addr = OSSwapHostToLittleInt64(addr);
        desc->buf.blen = OSSwapHostToLittleInt64(word1);

        ++rxNextDescIndex &= kRxDescMask;
        desc = &rxDescArray[rxNextDescIndex];
    }
    return goodPkts;
}

void RTL8125::interruptOccurred(OSObject *client, IOInterruptEventSource *src, int count)
{
    struct rtl8125_private *tp = &linuxData;
    UInt32 rxPackets = 0;
    UInt32 status;

    status = RTL_R32(tp, ISR0_8125);
    
    //DebugLog("interruptHandler: status = 0x%x.\n", status);

    /* hotplug/major error/no more work/shared irq */
    if ((status == 0xFFFFFFFF) || !status)
        goto done;
    
    RTL_W32(tp, IMR0_8125, 0x0000);
    RTL_W32(tp, ISR0_8125, (status & ~RxFIFOOver));

    if (status & SYSErr) {
        pciErrorInterrupt();
        goto done;
    }
    if (!test_bit(__POLL_MODE, &stateFlags) &&
        !test_and_set_bit(__POLLING, &stateFlags)) {
        /* Rx interrupt */
        if (status & (RxOK | RxDescUnavail)) {
            rxPackets = rxInterrupt(netif, kNumRxDesc, NULL, NULL);
            
            if (rxPackets)
                netif->flushInputQueue();
            
#ifdef DEBUG_INTR
            if (rxPackets > maxRxPkt)
                maxRxPkt = rxPackets;
#endif  /* DEBUG_INTR */

            etherStats->dot3RxExtraEntry.interrupts++;
        }
        /* Tx interrupt */
        if (status & (TxOK)) {
            txInterrupt();
            
            etherStats->dot3TxExtraEntry.interrupts++;
        }
        if (status & (TxOK | RxOK | PCSTimeout))
            timerValue = updateTimerValue(tp, status);
        
        RTL_W32(tp, TIMER_INT0_8125, timerValue);

        if (timerValue) {
            RTL_W32(tp, TCTR0_8125, timerValue);
            intrMask = intrMaskTimer;
        } else {
            intrMask = intrMaskRxTx;
        }
        clear_bit(__POLLING, &stateFlags);
    }
    if (status & LinkChg) {
        rtl812xCheckLinkStatus(tp);
        timerValue = 0;
        intrMask = intrMaskRxTx;

        RTL_W32(tp, TIMER_INT0_8125, timerValue);
    }
    
done:
    RTL_W32(tp, IMR0_8125, intrMask);
}

bool RTL8125::txHangCheck()
{
    struct rtl8125_private *tp = &linuxData;
    bool deadlock = false;
    
    if ((txDescDoneCount == txDescDoneLast) && (txNumFreeDesc < kNumTxDesc)) {
        if (++deadlockWarn == kTxCheckTreshhold) {
            /* Some members of the RTL8125 family seem to be prone to lose transmitter rinterrupts.
             * In order to avoid false positives when trying to detect transmitter deadlocks, check
             * the transmitter ring once for completed descriptors before we assume a deadlock.
             */
            DebugLog("Warning: Tx timeout, ISR0=0x%x, IMR0=0x%x, polling=%u.\n", RTL_R32(tp, ISR0_8125),
                     RTL_R32(tp, IMR0_8125), test_bit(__POLL_MODE, &stateFlags));
            etherStats->dot3TxExtraEntry.timeouts++;
            txInterrupt();
        } else if (deadlockWarn >= kTxDeadlockTreshhold) {
#ifdef DEBUG
            UInt32 i, index;
            
            for (i = 0; i < 10; i++) {
                index = ((txDirtyDescIndex - 1 + i) & kTxDescMask);
                IOLog("desc[%u]: opts1=0x%x, opts2=0x%x, addr=0x%llx.\n", index,
                      txDescArray[index].opts1, txDescArray[index].opts2, txDescArray[index].addr);
            }
#endif
            IOLog("Tx stalled? Resetting chipset. ISR0=0x%x, IMR0=0x%x.\n", RTL_R32(tp, ISR0_8125),
                  RTL_R32(tp, IMR0_8125));
            etherStats->dot3TxExtraEntry.resets++;
            rtl812xRestart(tp);
            deadlock = true;
        }
    } else {
        deadlockWarn = 0;
    }
    return deadlock;
}

#pragma mark --- rx poll methods ---

IOReturn RTL8125::setInputPacketPollingEnable(IONetworkInterface *interface, bool enabled)
{
    struct rtl8125_private *tp = &linuxData;

    //DebugLog("setInputPacketPollingEnable() ===>\n");

    if (test_bit(__ENABLED, &stateFlags)) {
        if (enabled) {
            set_bit(__POLL_MODE, &stateFlags);

            intrMask = intrMaskPoll;
        } else {
            clear_bit(__POLL_MODE, &stateFlags);

            intrMask = intrMaskRxTx;
            
            /* Clear per interrupt tx counters. */
            totalDescs = 0;
            totalBytes = 0;
        }
        timerValue = 0;
        RTL_W32(tp, IMR0_8125, intrMask);
    }
    DebugLog("Input polling %s.\n", enabled ? "enabled" : "disabled");

    //DebugLog("setInputPacketPollingEnable() <===\n");
    
    return kIOReturnSuccess;
}

void RTL8125::pollInputPackets(IONetworkInterface *interface, uint32_t maxCount, IOMbufQueue *pollQueue, void *context )
{
    //DebugLog("pollInputPackets() ===>\n");
    
    if (test_bit(__POLL_MODE, &stateFlags) &&
        !test_and_set_bit(__POLLING, &stateFlags)) {

        if (useAppleVTD)
            rxInterruptVTD(interface, maxCount, pollQueue, context);
        else
            rxInterrupt(interface, maxCount, pollQueue, context);
        
        /* Finally cleanup the transmitter ring. */
        txInterrupt();
        
        clear_bit(__POLLING, &stateFlags);
    }
    //DebugLog("pollInputPackets() <===\n");
}

void RTL8125::timerAction(IOTimerEventSource *timer)
{
    struct rtl8125_private *tp = &linuxData;

#ifdef DEBUG_INTR
    UInt32 tmrIntr = tmrInterrupts - lastTmrIntrupts;
    UInt32 txIntr = etherStats->dot3TxExtraEntry.interrupts - lastTxIntrupts;
    UInt32 rxIntr = etherStats->dot3RxExtraEntry.interrupts - lastRxIntrupts;

    lastTmrIntrupts = tmrInterrupts;
    lastRxIntrupts = etherStats->dot3RxExtraEntry.interrupts;
    lastTxIntrupts = etherStats->dot3TxExtraEntry.interrupts;

    IOLog("timer: %u, tx: %u, rx: %u, txPkt: %u, rxPkt: %u\n", tmrIntr, txIntr, rxIntr, maxTxPkt, maxRxPkt);
    
    maxRxPkt = 0;
    maxTxPkt = 0;
#endif
    
    if (!test_bit(__LINK_UP, &stateFlags))
        goto done;

    rtl812xDumpTallyCounter(tp);
    thread_call_enter_delayed(statCall, statDelay);

    /* Check for tx deadlock. */
    if (txHangCheck())
        goto done;
    
    timerSource->setTimeoutMS(kTimeoutMS);
        
done:
    txDescDoneLast = txDescDoneCount;
}

#pragma mark --- miscellaneous functions ---

static inline void prepareTSO4(mbuf_t m, UInt32 *tcpOffset, UInt32 *mss)
{
    struct ip *iphdr;
    struct tcphdr *tcphdr;
    UInt16 *addr;
    int i = (int)mbuf_len(m);
    UInt32 csum32 = 6;
    UInt32 il;

    /*
     * VLAN packets have the L3 and L4 headers in the second mbuf.
     * The first one carries only the L2 header.
     */
    if (i == ETH_HLEN)
        iphdr = (struct ip *)mbuf_data(mbuf_next(m));
    else
        iphdr = (struct ip *)((UInt8 *)mbuf_data(m) + ETH_HLEN);

    addr = (UInt16 *)&iphdr->ip_src;
    
    for (i = 0; i < 4; i++) {
        csum32 += ntohs(*addr++);
        csum32 += (csum32 >> 16);
        csum32 &= 0xffff;
    }
    il = ((iphdr->ip_hl & 0x0f) << 2);
    tcphdr = (struct tcphdr *)((UInt8 *)iphdr + il);
    //DebugLog("IPv4 header length: %u\n", il);
    
    /* Fill in the pseudo header checksum for TSOv4. */
    tcphdr->th_sum = htons((UInt16)csum32);

    *tcpOffset = ETH_HLEN + il;
    
    if (*mss > MSS_MAX)
        *mss = MSS_MAX;
}

static inline void prepareTSO6(mbuf_t m, UInt32 *tcpOffset, UInt32 *mss)
{
    struct ip6_hdr *ip6Hdr;
    struct tcphdr *tcpHdr;
    int i = (int)mbuf_len(m);
    UInt32 csum32 = 6;
    
    /*
     * VLAN packets have the L3 and L4 headers in the second mbuf.
     * The first one carries only the L2 header.
     */
    if (i == ETH_HLEN)
        ip6Hdr = (struct ip6_hdr *)mbuf_data(mbuf_next(m));
    else
        ip6Hdr = (struct ip6_hdr *)((UInt8 *)mbuf_data(m) + ETH_HLEN);

    ip6Hdr->ip6_ctlun.ip6_un1.ip6_un1_plen = 0;

    for (i = 0; i < 16; i++) {
        csum32 += ntohs(ip6Hdr->ip6_src.__u6_addr.__u6_addr16[i]);
        csum32 += (csum32 >> 16);
        csum32 &= 0xffff;
    }
    tcpHdr = (struct tcphdr *)((UInt8 *)ip6Hdr + kIPv6HdrLen);
    
    /* Fill in the pseudo header checksum for TSOv6. */
    tcpHdr->th_sum = htons((UInt16)csum32);

    *tcpOffset = ETH_HLEN + kIPv6HdrLen;
    
    if (*mss > MSS_MAX)
        *mss = MSS_MAX;
}

static inline u32 ether_crc(int length, unsigned char *data)
{
    int crc = -1;
    
    while(--length >= 0) {
        unsigned char current_octet = *data++;
        int bit;
        for (bit = 0; bit < 8; bit++, current_octet >>= 1) {
            crc = (crc << 1) ^
            ((crc < 0) ^ (current_octet & 1) ? ethernet_polynomial : 0);
        }
    }
    return crc;
}
