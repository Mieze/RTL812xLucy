//
//  RTL8125Lucy.c
//  RTL8125Lucy
//
//  Created by Laura Müller on 08.03.26.
//

#include <mach/mach_types.h>

kern_return_t RTL8125Lucy_start(kmod_info_t * ki, void *d);
kern_return_t RTL8125Lucy_stop(kmod_info_t *ki, void *d);

kern_return_t RTL8125Lucy_start(kmod_info_t * ki, void *d)
{
    return KERN_SUCCESS;
}

kern_return_t RTL8125Lucy_stop(kmod_info_t *ki, void *d)
{
    return KERN_SUCCESS;
}
