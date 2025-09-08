#include <IOKit/IOReturn.h>
#include <string.h>
#include <kern/task.h>
#include <libkern/copyio.h>
#include <mach/vm_param.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <mach-o/loader.h>
#include "KextRWUserClient.h"
#include "routines.h"
#include "kpi.h"

#define super IOUserClient
OSDefineMetaClassAndFinalStructors(KextRWUserClient, IOUserClient)

bool KextRWUserClient::initWithTask(task_t owningTask, void *securityID, uint32_t type) {
    if (!super::initWithTask(owningTask, securityID, type)) {
        return false;
    }

    bool allow = false;
    OSObject *entitlement = copyClientEntitlement(owningTask, "com.apple.security.alfie.kext-rw");
    if (entitlement) {
        allow = (entitlement == kOSBooleanTrue);
        entitlement->release();
    }
    return allow;
}

IOReturn KextRWUserClient::externalMethod(uint32_t selector, IOExternalMethodArguments *args, IOExternalMethodDispatch *dispatch, OSObject *target, void *reference) {
    static const IOExternalMethodDispatch methods[] =
    {
        /* 0 */ { (IOExternalMethodAction)&KextRWUserClient::kread,  3, 0, 0, 0 },
        /* 1 */ { (IOExternalMethodAction)&KextRWUserClient::kwrite, 3, 0, 0, 0 },
        /* 2 */ { (IOExternalMethodAction)&KextRWUserClient::physread,  4, 0, 0, 0 },
        /* 3 */ { (IOExternalMethodAction)&KextRWUserClient::physwrite, 4, 0, 0, 0 },
        /* 4 */ { (IOExternalMethodAction)&KextRWUserClient::getResetVector, 0, 0, 1, 0 },
        /* 5 */ { (IOExternalMethodAction)&KextRWUserClient::kvtophys, 1, 0, 1, 0 },
        /* 6 */ { (IOExternalMethodAction)&KextRWUserClient::phystokv, 1, 0, 1, 0 },
        /* 7 */ { (IOExternalMethodAction)&KextRWUserClient::callKernelFunction, 11, 0, 1, 0 },
        /* 8 */ { (IOExternalMethodAction)&KextRWUserClient::kallocBuffer, 1, 0, 1, 0 },
        /* 9 */ { (IOExternalMethodAction)&KextRWUserClient::kfreeBuffer, 2, 0, 1, 0 },
    };

    if(selector < sizeof(methods)/sizeof(methods[0]))
    {
        dispatch = const_cast<IOExternalMethodDispatch*>(&methods[selector]);
        target = this;
    }

    return super::externalMethod(selector, args, dispatch, target, reference);
}

IOReturn KextRWUserClient::kread(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args)
{
    int r = copyout((const void*)args->scalarInput[0], (user_addr_t)args->scalarInput[1], args->scalarInput[2]);
    return r == 0 ? kIOReturnSuccess : kIOReturnVMError;
}

IOReturn KextRWUserClient::kwrite(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args)
{
    int r = copyin((user_addr_t)args->scalarInput[0], (void*)args->scalarInput[1], args->scalarInput[2]);
    return r == 0 ? kIOReturnSuccess : kIOReturnVMError;
}

IOReturn physcopy(uint64_t src, uint64_t dst, uint64_t len, uint64_t alignment, IODirection direction)
{
    IOReturn retval = kIOReturnError;
    IOReturn ret;
    IOOptionBits mapOptions = 0;

    uint64_t va = direction == kIODirectionIn ? dst : src;
    uint64_t pa = direction == kIODirectionIn ? src : dst;

    switch(alignment)
    {
        case 0:
            break;

        case 4:
        case 8:
            if((pa % alignment) != 0 || (len % alignment) != 0)
            {
                return kIOReturnNotAligned;
            }
            mapOptions |= kIOMapInhibitCache;
            break;

        default:
            return kIOReturnBadArgument;
    }

    uint64_t voff = va & PAGE_MASK;
    uint64_t poff = pa & PAGE_MASK;
    va &= ~(uint64_t)PAGE_MASK;
    pa &= ~(uint64_t)PAGE_MASK;

    IOMemoryDescriptor *vDesc = IOMemoryDescriptor::withAddressRange((mach_vm_address_t)va, (len + voff + PAGE_MASK) & ~(uint64_t)PAGE_MASK, direction == kIODirectionIn ? kIODirectionOut : kIODirectionIn, current_task());
    if(!vDesc)
    {
        retval = iokit_vendor_specific_err(1);
    }
    else
    {
        ret = vDesc->prepare();
        if(ret != kIOReturnSuccess)
        {
            retval = ret;
        }
        else
        {
            IOMemoryMap *vMap = vDesc->map();
            if(!vMap)
            {
                retval = iokit_vendor_specific_err(2);
            }
            else
            {
                IOMemoryDescriptor *pDesc = IOMemoryDescriptor::withPhysicalAddress((IOPhysicalAddress)pa, (len + poff + PAGE_MASK) & ~(uint64_t)PAGE_MASK, direction);
                if(!pDesc)
                {
                    retval = iokit_vendor_specific_err(3);
                }
                else
                {
                    IOMemoryMap *pMap = pDesc->map(mapOptions);
                    if(!pMap)
                    {
                        retval = iokit_vendor_specific_err(4);
                    }
                    else
                    {
                        IOVirtualAddress v = vMap->getVirtualAddress();
                        IOVirtualAddress p = pMap->getVirtualAddress();
                        const void *from = (const void*)(direction == kIODirectionIn ? p + poff : v + voff);
                              void *to   = (      void*)(direction == kIODirectionIn ? v + voff : p + poff);

                        switch(alignment)
                        {
                            case 0:
                                bcopy(from, to, len);
                                break;

                            case 4:
                                for(size_t i = 0; i < len/4; ++i)
                                {
                                    ((volatile uint32_t*)to)[i] = ((const volatile uint32_t*)from)[i];
                                }
                                break;

                            case 8:
                                for(size_t i = 0; i < len/8; ++i)
                                {
                                    ((volatile uint64_t*)to)[i] = ((const volatile uint64_t*)from)[i];
                                }
                                break;
                        }

                        retval = kIOReturnSuccess;

                        pMap->release();
                    }
                    pDesc->release();
                }
                vMap->release();
            }
            vDesc->complete();
        }
        vDesc->release();
    }

    return retval;
}

IOReturn KextRWUserClient::physread(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args)
{
    return physcopy(args->scalarInput[0], args->scalarInput[1], args->scalarInput[2], args->scalarInput[3], kIODirectionIn);
}

IOReturn KextRWUserClient::physwrite(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args)
{
    return physcopy(args->scalarInput[0], args->scalarInput[1], args->scalarInput[2], args->scalarInput[3], kIODirectionOut);
}

IOReturn KextRWUserClient::getResetVector(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args)
{
    uint64_t reset_vector = 0;
    asm volatile("mrs %0, vbar_el1" : "=r"(reset_vector));
    args->scalarOutput[0] = reset_vector;
    return kIOReturnSuccess;
}

IOReturn KextRWUserClient::kvtophys(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args)
{
    uint64_t va = args->scalarInput[0];
    args->scalarOutput[0] = arm_kvtophys(va);
    return kIOReturnSuccess;
}

IOReturn KextRWUserClient::phystokv(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args)
{
    // TODO
    return kIOReturnUnsupported;
}

IOReturn KextRWUserClient::callKernelFunction(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args)
{
    if (args->scalarInputCount > 11) return kIOReturnBadArgument;
    if (args->scalarOutputCount > 1) return kIOReturnBadArgument;
    uint64_t fn = args->scalarInput[0];
    if (!fn) return kIOReturnBadArgument;
    uint64_t a0 = args->scalarInput[1] ? args->scalarInput[1] : 0;
    uint64_t a1 = args->scalarInput[2] ? args->scalarInput[2] : 0;
    uint64_t a2 = args->scalarInput[3] ? args->scalarInput[3] : 0;
    uint64_t a3 = args->scalarInput[4] ? args->scalarInput[4] : 0;
    uint64_t a4 = args->scalarInput[5] ? args->scalarInput[5] : 0;
    uint64_t a5 = args->scalarInput[6] ? args->scalarInput[6] : 0;
    uint64_t a6 = args->scalarInput[7] ? args->scalarInput[7] : 0;
    uint64_t a7 = args->scalarInput[8] ? args->scalarInput[8] : 0;
    uint64_t a8 = args->scalarInput[9] ? args->scalarInput[9] : 0;
    uint64_t a9 = args->scalarInput[10] ? args->scalarInput[10] : 0;

    uint64_t ret = arbitrary_call(fn, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);

    if (args->scalarOutputCount) args->scalarOutput[0] = ret;

    return kIOReturnSuccess;
}

IOReturn KextRWUserClient::kallocBuffer(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args)
{
    uint64_t size = args->scalarInput[0];
    if (!size) return kIOReturnBadArgument;

    uint64_t addr = (uint64_t)kalloc(size);
    if (!addr) return kIOReturnNoMemory;

    args->scalarOutput[0] = addr;
    return kIOReturnSuccess;
}

IOReturn KextRWUserClient::kfreeBuffer(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args)
{
    uint64_t addr = args->scalarInput[0];
    if (!addr) return kIOReturnBadArgument;

    uint64_t size = args->scalarInput[1];
    if (!size) return kIOReturnBadArgument;

    kfree((void*)addr, size);
    return kIOReturnSuccess;
}

IOReturn KextRWUserClient::readActlrEL1(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args)
{
    uint64_t val = 0;
    asm volatile("mrs %0, ACTLR_EL1" : "=r"(val));
    args->scalarOutput[0] = val;
    return kIOReturnSuccess;
}