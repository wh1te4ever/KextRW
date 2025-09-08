#ifndef KEXTRWUSERCLIENT_H
#define KEXTRWUSERCLIENT_H

#include <IOKit/IOUserClient.h>

class KextRWUserClient: public IOUserClient {
    OSDeclareFinalStructors(KextRWUserClient);
public:
    virtual bool initWithTask(task_t owningTask, void *securityID, uint32_t type) override;
    virtual IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments *args, IOExternalMethodDispatch *dispatch, OSObject *target, void *reference) override;
private:
    static IOReturn kread(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args);
    static IOReturn kwrite(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args);
    static IOReturn physread(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args);
    static IOReturn physwrite(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args);
    static IOReturn getResetVector(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args);
    static IOReturn kvtophys(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args);
    static IOReturn phystokv(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args);
    static IOReturn callKernelFunction(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args);
    static IOReturn kallocBuffer(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args);
    static IOReturn kfreeBuffer(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args);
    static IOReturn readActlrEL1(KextRWUserClient *client, void *reference, IOExternalMethodArguments *args);
};

#endif // KEXTRWUSERCLIENT_H