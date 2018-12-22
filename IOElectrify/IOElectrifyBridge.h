/*
 *  Released under "The GNU General Public License (GPL-2.0)"
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef IOElectrifyBridge_h
#define IOElectrifyBridge_h

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/pci/IOPCIBridge.h>

#ifdef DEBUG
#define DebugLog(args...) do { IOLog("IOElectrifyBridge: " args); } while (0)
#else
#define DebugLog(args...) do { } while (0)
#endif
#define AlwaysLog(args...) do { IOLog("IOElectrifyBridge: " args); } while (0)


// External client methods
enum
{
    kClientExecuteCMD = 0,
    kClientNumMethods
};

class IOElectrifyBridge : public IOService
{
    OSDeclareDefaultStructors(IOElectrifyBridge);
    typedef IOService super;
    
protected:
    IOPCI2PCIBridge* mProvider;
    
public:
    virtual bool init(OSDictionary *propTable);
    virtual bool attach(IOService *provider);
    virtual bool start(IOService *provider);
    virtual void stop(IOService *provider);
    UInt32 probeDev(UInt32 options);
    virtual void free();
	bool mEnablePowerHook = false;
	char parentName[128];
#ifdef DEBUG
    virtual void detach(IOService *provider);
#endif
	virtual IOReturn setPowerState(unsigned long powerState, IOService *service);
};

class IOElectrifyBridgeUserClient : public IOUserClient
{
    OSDeclareDefaultStructors(IOElectrifyBridgeUserClient)
private:
    IOElectrifyBridge* providertarget;
    task_t mTask;
    SInt32 mOpenCount;
    static const IOExternalMethodDispatch sMethods[kClientNumMethods];
public:
    virtual bool start(IOService* provider);
    virtual void stop(IOService* provider);
    virtual bool initWithTask(task_t owningTask, void * securityID, UInt32 type, OSDictionary* properties);
    virtual IOReturn clientClose(void);
    virtual IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments *arguments, IOExternalMethodDispatch* dispatch = 0,
                                    OSObject* target = 0, void* reference = 0);
    static IOReturn executeCMD(IOElectrifyBridge* target, void* reference, IOExternalMethodArguments* arguments);
};

#endif

