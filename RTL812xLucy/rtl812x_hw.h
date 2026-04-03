//
//  rtl812x_hw.h
//  RTL812xLucy
//
//  Created by Laura Müller on 24.03.26.
//

#ifndef rtl812x_hw_h
#define rtl812x_hw_h

#define kRTL812xMagicNumber     0x10ec8125

/* Invalid temperature value */
#define kInvalidTemp    0xffff

enum {
    kUCMethodGetTemp = 0,
    kUCMethodGetStats = 1,
    kUCMethodCount
};

/* RTL8125's statistics dump data structure */
typedef struct RtlStatData {
    UInt64 txPackets;
    UInt64 rxPackets;
    UInt64 txErrors;
    UInt32 rxErrors;
    UInt16 rxMissed;
    UInt16 alignErrors;
    UInt32 txOneCollision;
    UInt32 txMultiCollision;
    UInt64 rxUnicast;
    UInt64 rxBroadcast;
    UInt32 rxMulticast;
    UInt16 txAborted;
    UInt16 txUnderun;
    /* new since RTL8125 */
    UInt64 txOctets;
    UInt64 rxOctets;
    UInt64 rxMulticast64;
    UInt64 txUnicast64;
    UInt64 txBroadcast64;
    UInt64 txMulticast64;
    UInt32 txPauseOn;
    UInt32 txPauseOff;
    UInt32 txPauseAll;
    UInt32 txDeferred;
    UInt32 txLateCollision;
    UInt32 txAllCollision;
    UInt32 txAborted32;
    UInt32 alignErrors32;
    UInt32 rxFrame2Long;
    UInt32 rxRunt;
    UInt32 rxPauseOn;
    UInt32 rxPauseOff;
    UInt32 rxPauseAll;
    UInt32 rxUnknownOpcode;
    UInt32 rxMacError;
    UInt32 txUnderrun32;
    UInt32 rxMacMissed;
    UInt32 rxTcamDropped;
    UInt32 tdu;
    UInt32 rdu;
} RtlStatData;

#endif /* rtl812x_hw_h */
