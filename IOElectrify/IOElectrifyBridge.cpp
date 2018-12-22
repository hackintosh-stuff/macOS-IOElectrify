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

#include <IOKit/IOLib.h>
#include <IOKit/IOUserClient.h>
#include "IOElectrifyBridge.h"

#include <libkern/version.h>
extern kmod_info_t kmod_info;

#define kIOElectrifyBridgePowerHookKey "IOElectrifyBridgePowerHook"
#define kMatchParentNameKey "MatchParentName"

// define & enumerate power states
enum
{
    kPowerStateSleep    = 0,
    kPowerStateDoze     = 1,
    kPowerStateNormal   = 2,
    kPowerStateCount
};

OSDefineMetaClassAndStructors(IOElectrifyBridge, IOService);

bool IOElectrifyBridge::init(OSDictionary *propTable)
{
    DebugLog("IOElectrifyBridge::init() %p\n", this);

	OSBoolean *osBool;
	osBool = OSDynamicCast(OSBoolean, propTable->getObject(kIOElectrifyBridgePowerHookKey));
	if (osBool)
		mEnablePowerHook = (bool)osBool->getValue();
	else
		mEnablePowerHook = false;
	
	OSString *osStr;
	osStr = OSDynamicCast(OSString, propTable->getObject(kMatchParentNameKey));
	strncpy(parentName, osStr->getCStringNoCopy(), osStr->getLength());
	
    // announce version
    IOLog("IOElectrifyBridge: Version %s starting on OS X Darwin %d.%d.\n", kmod_info.version, version_major, version_minor);
    
    bool ret = super::init(propTable);
    if (!ret)
    {
        AlwaysLog("super::init returned false\n");
        return false;
    }
	
    setProperty("PowerHook", mEnablePowerHook);
	
    // place version/build info in ioreg properties DV,Build and DV,Version
    char buf[128];
    snprintf(buf, sizeof(buf), "%s %s", kmod_info.name, kmod_info.version);
    setProperty("DV,Version", buf);
#ifdef DEBUG
    setProperty("DV,Build", "Debug-darkvoid");// LOGNAME);
#else
    setProperty("DV,Build", "Release-darkvoid");// LOGNAME);
#endif
    
    mProvider = NULL;
    
    return true;
}

bool IOElectrifyBridge::attach(IOService* provider)
{
    DebugLog("IOElectrifyBridge::attach() %s\n", provider->getName());
    
    mProvider = OSDynamicCast(IOPCI2PCIBridge, provider);
    
    if (mProvider == NULL) {
        return false;
    }

    DebugLog("Provider -> Provider %s\n", mProvider->getProvider()->getName());

    if (strcmp(mProvider->getProvider()->getName(), parentName)) {
        DebugLog("Mismatch %s vs %s\n", mProvider->getProvider()->getName(), parentName);
        return false;
    }

    return super::attach(provider);
}

bool IOElectrifyBridge::start(IOService *provider)
{
    DebugLog("IOElectrifyBridge::start() %s\n", provider->getName());
    
    if (!super::start(provider))
    {
        AlwaysLog("super::start returned false\n");
        return false;
    }
    probeDev(0);
    
    return true;
}

UInt32 IOElectrifyBridge::probeDev(UInt32 options)
{
    //SInt32 score = 0;
    //mProvider->probe(mProvider, &score);
    //DebugLog("Probe score: %d", score);
    
    //IOOptionBits options = 0;
	DebugLog("probeDev options: 0x%lx\n", options);
    return mProvider->requestProbe(options);
}

void IOElectrifyBridge::stop(IOService *provider)
{
    DebugLog("IOElectrifyBridge::stop() %p\n", this);
    
    super::stop(provider);
}

void IOElectrifyBridge::free()
{
    DebugLog("IOElectrifyBridge::free() %p\n", this);
    
    super::free();
}

#ifdef DEBUG
void IOElectrifyBridge::detach(IOService *provider)
{
    DebugLog("IOElectrifyBridge::detach() %p\n", this);
    
    return super::detach(provider);
}
#endif


IOReturn IOElectrifyBridge::setPowerState(unsigned long powerState, IOService *service)
{
    DebugLog("setPowerState %ld\n", powerState);
    
	if (mEnablePowerHook)
	{
	    switch (powerState)
	    {
	        case kPowerStateSleep:
	            DebugLog("--> sleep(%d)\n", (int)powerState);
				probeDev(kIOPCIProbeOptionEject | kIOPCIProbeOptionDone);
	            break;
	        case kPowerStateDoze:
	        case kPowerStateNormal:
	            DebugLog("--> awake(%d)\n", (int)powerState);
				probeDev(kIOPCIProbeOptionNeedsScan | kIOPCIProbeOptionDone);
	            break;
	    }
	}
	
	
    
    return IOPMAckImplied;
}


//*********************************************************************
// Userspace Client:
//*********************************************************************

const IOExternalMethodDispatch IOElectrifyBridgeUserClient::sMethods[kClientNumMethods] =
{
    { // kClientExecuteCMD
        (IOExternalMethodAction)&IOElectrifyBridgeUserClient::executeCMD,
        1, // One scalar input value
        0, // No struct inputs
        1, // One scalar output value
        0  // No struct outputs
    }
};

// Structure of IOExternalMethodDispatch:
// https://developer.apple.com/library/content/samplecode/SimpleUserClient/Listings/User_Client_Info_txt.html
/*
 struct IOExternalMethodDispatch
 {
 IOExternalMethodAction function;
 uint32_t           checkScalarInputCount;
 uint32_t           checkStructureInputSize;
 uint32_t           checkScalarOutputCount;
 uint32_t           checkStructureOutputSize;
 };
*/

/*
 * Define the metaclass information that is used for runtime
 * typechecking of IOKit objects. We're a subclass of IOUserClient.
 */

OSDefineMetaClassAndStructors(IOElectrifyBridgeUserClient, IOUserClient)

//
// IOUserClient user-kernel boundary interface init (initWithTask)
//

bool IOElectrifyBridgeUserClient::initWithTask(task_t owningTask, void* securityID, UInt32 type, OSDictionary* properties)
{
    IOLog("Client::initWithTask(type %u)\n", (unsigned int)type);
    
    mTask = owningTask;
    
    return IOUserClient::initWithTask(owningTask, securityID, type, properties);
}

//
// IOUserClient user-kernel boundary interface start
//

bool IOElectrifyBridgeUserClient::start(IOService * provider)
{
    bool result = IOUserClient::start(provider);
    IOLog("Client::start\n");
    
    if(!result)
        return(result);
    
    /*
     * Provider is always IOElectrify
     */
    assert(OSDynamicCast(IOElectrifyBridge, provider));
    providertarget = (IOElectrifyBridge*) provider;
    
    mOpenCount = 1;
    
    return result;
}

//
// IOUserClient user-kernel boundary interface client exit behavior
//

IOReturn IOElectrifyBridgeUserClient::clientClose(void)
{
    if (!isInactive())
        terminate();
    
    return kIOReturnSuccess;
}

//
// IOUserClient user-kernel boundary interface stop override 
//

void IOElectrifyBridgeUserClient::stop(IOService * provider)
{
    IOLog("Client::stop\n");
    
    IOUserClient::stop(provider);
}

//
// IOUserClient handle external method
//

IOReturn IOElectrifyBridgeUserClient::externalMethod(uint32_t selector, IOExternalMethodArguments* arguments,
                                              IOExternalMethodDispatch* dispatch, OSObject* target, void* reference)
{
    DebugLog("%s[%p]::%s(%d, %p, %p, %p, %p)\n", getName(), this, __FUNCTION__, selector, arguments, dispatch, target, reference);
    
    if (selector < (uint32_t)kClientNumMethods)
    {
        dispatch = (IOExternalMethodDispatch *)&sMethods[selector];
        
        if (!target)
        {
            if (selector == kClientExecuteCMD)
                target = providertarget;
            else
                target = this;
        }
    }
    return IOUserClient::externalMethod(selector, arguments, dispatch, target, reference);
}

IOReturn IOElectrifyBridgeUserClient::executeCMD(IOElectrifyBridge* target, void* reference, IOExternalMethodArguments* arguments)
{
    arguments->scalarOutput[0] = target->probeDev((UInt32)arguments->scalarInput[0]);
    return kIOReturnSuccess;
}
