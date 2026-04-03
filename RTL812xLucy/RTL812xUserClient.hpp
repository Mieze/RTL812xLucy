//
//  RTL812xLucyUserClient.hpp
//  RTL812xLucy
//
//  Created by Laura Müller on 14.03.26.
//

#ifndef RTL812xUserClient_hpp
#define RTL812xUserClient_hpp

#include <IOKit/IOUserClient.h>
#include "rtl812x_hw.h"
#include "RTL812xLucy.hpp"

#define kExternalMethodCount    2

class RTL812xUserClient : public IOUserClient
{
    OSDeclareDefaultStructors(RTL812xUserClient);


public:
    static RTL812xUserClient*    withTask( task_t owningTask );    // Constructor

    virtual IOReturn    clientClose() override;
    virtual IOReturn    clientDied() override;

    virtual IOReturn    registerNotificationPort(mach_port_t port, UInt32 type);
    virtual IOReturn    connectClient(IOUserClient *client) override;

    virtual bool        start(IOService *provider) override;
                                    
    virtual IOReturn    externalMethod(uint32_t selector, IOExternalMethodArguments *arguments,
                                    IOExternalMethodDispatch *dispatch,
                                    OSObject *target, void *reference) override;

private:
    static IOReturn getTemp(OSObject *target, void *reference,
                                   IOExternalMethodArguments *arguments);
    static IOReturn getStats(OSObject *target, void *reference,
                                   IOExternalMethodArguments *arguments);

private:
    RTL8125 *rtl812x;
    task_t  task;
    
    const IOExternalMethodDispatch methods[kExternalMethodCount] = {
        {getTemp, 0, 0, 1, 0},
        {getStats, 0, 0, 0, sizeof(RtlStatData)},
    };

};

#endif /* RTL812xLucyUserClient_hpp */
