//
//  RTL812xUserClient.cpp
//  RTL812xLucy
//
//  Created by Laura Müller on 14.03.26.
//

#include "RTL812xUserClient.hpp"

#undef  super
#define super IOUserClient

OSDefineMetaClassAndStructors(RTL812xUserClient, IOUserClient);

RTL812xUserClient* RTL812xUserClient::withTask(task_t owningTask)
{
    RTL812xUserClient* me = new RTL812xUserClient;

    if (me && me->init() == false) {
        me->release();
        return NULL;
    }
    me->task = owningTask;

    return me;
}

bool RTL812xUserClient::start(IOService *provider)
{
    bool result;
    
    result = super::start(provider);
    
    if (!result) {
        IOLog("IOUserClient::start failed.\n");
        goto done;
    }
    
    result = provider->open(this);
    
    if (!result) {
        IOLog("RTL812xUserClient failed to open provider.\n");
        goto done;
    }
    rtl812x = OSDynamicCast(RTL8125, provider);
    
    /* Initialize the call structure:    */
        
done:
    return result;
}

IOReturn RTL812xUserClient::clientClose()
{
    if (rtl812x) {
        if (rtl812x->isOpen(this))
            rtl812x->close(this);

        detach(rtl812x);
        rtl812x = NULL;
    }
    return kIOReturnSuccess;
}

IOReturn RTL812xUserClient::clientDied()
{
    return clientClose();
}

IOReturn RTL812xUserClient::connectClient(IOUserClient *client)
{
    return kIOReturnSuccess;
}

IOReturn RTL812xUserClient::registerNotificationPort(mach_port_t port, UInt32 type)
{
    return kIOReturnUnsupported;
}

IOReturn RTL812xUserClient::getTemp(OSObject *target, void *reference, IOExternalMethodArguments *arguments)
{
    UInt32 temp = 0xfff;
    
    RTL812xUserClient *me = OSDynamicCast(RTL812xUserClient, target);
    
    if (me) {
        temp = me->rtl812x->getTemperature();
    }
    *arguments->scalarOutput = temp;
    
    return (temp == kInvalidTemp) ? kIOReturnUnsupported : kIOReturnSuccess;
}

IOReturn RTL812xUserClient::getStats(OSObject *target, void *reference, IOExternalMethodArguments *arguments)
{
    RTL812xUserClient *me = OSDynamicCast(RTL812xUserClient, target);
    
    if (me) {
        me->rtl812x->getHwStatistics((RtlStatData *)arguments->structureOutput);
    }
    return kIOReturnSuccess;
}

IOReturn RTL812xUserClient::externalMethod(uint32_t selector, IOExternalMethodArguments *arguments,
                                IOExternalMethodDispatch *dispatch,
                                OSObject *target, void *reference)
{
    IOReturn result = kIOReturnUnsupported;

    switch (selector) {
        case kUCMethodGetTemp:
            dispatch = (IOExternalMethodDispatch *)&methods[kUCMethodGetTemp];
            break;
            
        case kUCMethodGetStats:
            dispatch = (IOExternalMethodDispatch *)&methods[kUCMethodGetStats];
            break;
            
        default:
            dispatch = NULL;
            break;
    }
    if (dispatch) {
        target = this;
        reference = NULL;
        
        result = super::externalMethod(selector, arguments, dispatch, target, reference);
    }
    return result;
}
