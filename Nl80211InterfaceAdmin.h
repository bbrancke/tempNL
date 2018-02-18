// Nl80211InterfaceAdmin.h
// Includes convenience funcs bet InterfaceManagerNl80211 class and Nl80211Base class
// as well as Interface IOCTLs not provided by NL80211 API (e.g., SetMacAddress).

#ifndef NL80211INTERFACEADMIN_H_
#define NL80211INTERFACEADMIN_H_

#include <iostream>
#include <string>
#include <sstream>

#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>

#include "Log.h"
#include "Nl80211Base.h"

using namespace std;

class Nl80211InterfaceAdmin : public Nl80211Base
{
public:
	Nl80211InterfaceAdmin(const char *name);
protected:
	bool SetMacAddress(const char *ifaceName, const uint8_t *mac);
	bool GetInterfaceList();
};

#endif  // NL80211INTERFACEADMIN_H_

