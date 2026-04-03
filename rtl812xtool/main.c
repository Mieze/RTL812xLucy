//
//  main.c
//  rtl812xtool
//
//  Created by Laura Müller on 14.03.26.
//

#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFSerialize.h>
#include <IOKit/network/IONetworkLib.h>
#include <IOKit/IOTypes.h>

#include <mach/mach.h>
#include <mach/mach_interface.h>

#include <sys/time.h>
#include <sys/file.h>

#include "rtl812x_hw.h"

/* Invalid temperature value */
#define kInvalidTemp    0xffff

enum {
    noCmd = 0,
    getTempCmd = 1,
    getStatsCmd = 2,
};

static void print_usage(void)
{
    printf("Usage: sudo rtl812xtool [-t] [-s] interface\n");
}

/* Search the registry for an IONetworkInterface object with    */
/* the given name. If a match is found, the object is returned.    */

io_object_t getInterfaceWithName(const char *devName)
{
    CFDictionaryRef matching_dict = NULL;
    CFStringRef class_name;
    io_iterator_t iter = 0;
    io_service_t service = 0;
    io_service_t parent = 0;
    kern_return_t result;
        
    // Create a matching dictionary that will find any USB device.
    matching_dict = IOBSDNameMatching(kIOMasterPortDefault, 0, devName);
    
    // Create an iterator for all I/O Registry objects that match the dictionary.
    result = IOServiceGetMatchingServices(kIOMasterPortDefault, matching_dict, &iter);
    
    if (result == KERN_SUCCESS) {
        // Iterate over all matching objects.
        if ((service = IOIteratorNext(iter)) != 0) {
            result = IORegistryEntryGetParentEntry(service, kIOServicePlane, &parent);

            if (result == KERN_SUCCESS) {
                class_name = IOObjectCopyClass(parent);

                if (CFEqual(class_name, CFSTR("RTL8125")) == false) {
                    // Object is not an instance of RTL812xLucy
                    IOObjectRelease(parent);
                    parent = 0;
                }
                CFRelease(class_name);
            }
            IOObjectRelease(service);
        }
        // Release the iterator.
        IOObjectRelease(iter);
    }
    return parent;
}

static void get_temperature(io_connect_t conObj, const char *devName)
{
    uint64_t output_scalar = 0;
    uint32_t output_count = 1;
    kern_return_t result;

    result = IOConnectCallScalarMethod(conObj, kUCMethodGetTemp, NULL, 0, &output_scalar, &output_count);
    
    if ((result == kIOReturnSuccess) && (output_scalar != kInvalidTemp))
        printf("Temperature of device %s: %llu°C\n", devName, output_scalar);
    else
        printf("Device %s has no thermal sensor.\n", devName);
}

static void get_statistics(io_connect_t conObj, const char *devName)
{
    RtlStatData *stats = malloc(sizeof(RtlStatData));
    size_t size = sizeof(RtlStatData);
    kern_return_t result;

    if (stats) {
        result = IOConnectCallStructMethod(conObj, kUCMethodGetStats, NULL, 0, stats, &size);

        if (result == kIOReturnSuccess) {
            printf("    txPackets: %llu\n"
                   "    rxPackets: %llu\n"
                   "    txErrors: %llu\n"
                   "    rxErrors: %u\n"
                   "    rxMissed: %u\n"
                   "    alignErrors: %u\n"
                   "    txOneCollision: %u\n"
                   "    txMultiCollision: %u\n"
                   "    rxUnicast: %llu\n"
                   "    rxBroadcast: %llu\n"
                   "    rxMulticast: %u\n"
                   "    txAborted: %u\n"
                   "    txUnderun: %u\n"
                   "    txOctets: %llu\n"
                   "    rxOctets: %llu\n"
                   "    rxMulticast64: %llu\n"
                   "    txUnicast64: %llu\n"
                   "    txBroadcast64: %llu\n"
                   "    txMulticast64: %llu\n"
                   "    txPauseOn: %u\n"
                   "    txPauseOff: %u\n"
                   "    txPauseAll: %u\n"
                   "    txDeferred: %u\n"
                   "    txLateCollision: %u\n"
                   "    txAllCollision: %u\n"
                   "    txAborted32: %u\n"
                   "    alignErrors32: %u\n"
                   "    rxFrame2Long: %u\n"
                   "    rxRunt: %u\n"
                   "    rxPauseOn: %u\n"
                   "    rxPauseOff: %u\n"
                   "    rxPauseAll: %u\n"
                   "    rxUnknownOpcode: %u\n"
                   "    rxMacError: %u\n"
                   "    txUnderrun32: %u\n"
                   "    rxMacMissed: %u\n"
                   "    rxTcamDropped: %u\n"
                   "    tdu: %u\n"
                   "    rdu: %u\n",
                   stats->txPackets,
                   stats->rxPackets,
                   stats->txErrors,
                   (unsigned int)stats->rxErrors,
                   stats->rxMissed,
                   stats->alignErrors,
                   (unsigned int)stats->txOneCollision,
                   (unsigned int)stats->txMultiCollision,
                   stats->rxUnicast,
                   stats->rxBroadcast,
                   (unsigned int)stats->rxMulticast,
                   stats->txAborted,
                   stats->txUnderun,
                   stats->txOctets,
                   stats->rxOctets,
                   stats->rxMulticast64,
                   stats->txUnicast64,
                   stats->txBroadcast64,
                   stats->txMulticast64,
                   (unsigned int)stats->txPauseOn,
                   (unsigned int)stats->txPauseOff,
                   (unsigned int)stats->txPauseAll,
                   (unsigned int)stats->txDeferred,
                   (unsigned int)stats->txLateCollision,
                   (unsigned int)stats->txAllCollision,
                   (unsigned int)stats->txAborted32,
                   (unsigned int)stats->alignErrors32,
                   (unsigned int)stats->rxFrame2Long,
                   (unsigned int)stats->rxRunt,
                   (unsigned int)stats->rxPauseOn,
                   (unsigned int)stats->rxPauseOff,
                   (unsigned int)stats->rxPauseAll,
                   (unsigned int)stats->rxUnknownOpcode,
                   (unsigned int)stats->rxMacError,
                   (unsigned int)stats->txUnderrun32,
                   (unsigned int)stats->rxMacMissed,
                   (unsigned int)stats->rxTcamDropped,
                   (unsigned int)stats->tdu,
                   (unsigned int)stats->rdu
                   );
        } else {
            printf("Failed to get hw statistics of device %s with error %d.\n", devName, result);
        }
        free(stats);
    }
    
}

int main(int argc, const char * argv[])
{
    const char *ifname = NULL;
    io_object_t netif;        // network interface
    io_connect_t con_obj;        // connection object
    kern_return_t result;

    int c = 0;
    int cmd = noCmd;
    
    while ((c = getopt(argc, (char *const *)argv, "hst")) != EOF) {
        switch(c) {
            case 'h':
                cmd = noCmd;
                break;
                
            case 's':
                cmd = getStatsCmd;
                break;
                
            case 't':
                cmd = getTempCmd;
                break;

            default:
                cmd = noCmd;
                break;
        }
    }
    if (cmd != noCmd) {
        if (optind < argc)
            ifname = (char *)argv[optind++];
        else {
            print_usage();
            goto done;
        }
        if (ifname) {
            netif = getInterfaceWithName(ifname);
            
            if (netif) {
                result = IOServiceOpen(netif, mach_task_self(), kRTL812xMagicNumber, &con_obj);
                
                if (result == kIOReturnSuccess) {
                    switch (cmd) {
                        case getTempCmd:
                            get_temperature(con_obj, ifname);
                            break;
                            
                        case getStatsCmd:
                            get_statistics(con_obj, ifname);
                            break;
                            
                        default:
                            break;
                    }
                    IOServiceClose(con_obj);
                } else {
                    printf("Open device %s failed with error %d.\n", ifname, result);
                }
                IOObjectRelease(netif);
            } else {
                printf("Device %s is not a RTL812x.\n", ifname);
            }
        }
    } else {
        print_usage();
    }
done:
    exit(0);
}
