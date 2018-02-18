// InterfaceManagerNl80211.h
// InterfaceManager using NL80211 subsystem.

#ifndef INTERFACEMANAGERNL80211_H_
#define INTERFACEMANAGERNL80211_H_

#include <iostream>
#include <string>
#include <sstream>

#include "Log.h"
#include "OneInterface.h"
#include "Nl80211InterfaceAdmin.h"

// This is no longer based upon Interface Manager Interface.
// The Interface class was mostly empty, and the whole idea
// of ShadowX parts (main(), etc) being "isolated" from the
// actual implentation of wifi manipulaton started
// to make less sense
//  #include "IInterfaceManager.h"  no longer

using namespace std;

class InterfaceManagerNl80211 : public Nl80211InterfaceAdmin
{
public:
	static InterfaceManagerNl80211* GetInstance();
	// Not copiable and not assignable:
	InterfaceManagerNl80211(InterfaceManagerNl80211 const&) = delete;
	InterfaceManagerNl80211& operator=(InterfaceManagerNl80211 const&) = delete;

private:
	InterfaceManagerNl80211();  // Private so that ctor can't be called
	static InterfaceManagerNl80211* m_pInstance;
};


#endif  // INTERFACEMANAGERNL80211_H_

