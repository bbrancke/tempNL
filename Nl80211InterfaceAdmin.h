// Nl80211InterfaceAdmin.h
// Includes convenience funcs bet InterfaceManagerNl80211 class and Nl80211Base class.

#ifndef NL80211INTERFACEADMIN_H_
#define NL80211INTERFACEADMIN_H_

#include <iostream>
#include <string>
#include <sstream>
#include <cstdio>

#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <errno.h>

#include "Log.h"
#include "Nl80211Base.h"

using namespace std;

class Nl80211InterfaceAdmin : public Nl80211Base
{
public:
	Nl80211InterfaceAdmin(const char *name);
	bool GetInterfaceList();
	void LogInterfaceList(const char *caller);
protected:
//	bool GetInterfaceList();
	bool CreateApInterface(const char *newInterfaceName, uint32_t phyId);
	bool CreateStationInterface(const char *newInterfaceName, uint32_t phyId);
	bool CreateMonitorInterface(const char *newInterfaceName, uint32_t phyId);
	bool DeleteInterface(const char *interfaceName);
private:
	void IfTypeToString(uint32_t iftype, string& strType);
	bool _createInterface(const char *newInterfaceName, 
		uint32_t phyId, enum nl80211_iftype type);
};

#endif  // NL80211INTERFACEADMIN_H_

