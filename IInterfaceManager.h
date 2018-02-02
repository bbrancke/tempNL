// IInterfaceManager.h
// Interface for the (radios) interfaces.
// Ideally, all methods in here are all virtual.
//   [ e.g. virtual int DoSomething(...) = 0; ]
// so that this only defines the "what" and not "how";
// each method's IMPLEMENTATION will be in a
//   derived class (e.g. InterfaceNl80211 for
//   nl80211 and InterfaceWext for IOCTLs.
// Why? Implementing ShadowX for the new "Duovero-Y" boards
//   was a royal pain; Channel change time was > 2 seconds:
//   "iwconfig wlan[X] channel 11" e.g. - UNACCEPTABLY SLOW!
//   (iwconfig uses Wireless Extensions)
//   Frank discovered that using "iw ..." (which uses the new
//   Netlink API was much faster / better.
//   -- This attempts to abstract all the low-level
//      Radio Control stuff from ShadowX, so that (future)
//      OS-level changes will be isolated.
// Unfortunately, WEXT, which has served us well for approx.
//   5,000,000 years is now deprecated in favor of Netlink. OK...
// FOR NOW, only implementing the NL80211 version;
//   older ShX versions still have WEXT (Wireless Extensions /
//   IOCTLs) mixed in throughout.
#ifndef IINTERFACEMANAGER_H_
#define IINTERFACEMANAGER_H_

#include <string>
// Instantiate:
// InterfaceManagerNl80211 im("InterfaceManagerNl80211");
// ==> GetInstance(name);
class IInterfaceManager
{
public:
	virtual bool GetInterfaceList() = 0;

};

#endif  // IINTERFACEMANAGER_H_


