// main.cpp - test code.
// To build:
// g++ -g -std=c++11 -I /usr/include/libnl3/ main.cpp InterfaceManagerNl80211.cpp Nl80211Base.cpp Log.cpp -lnl-genl-3 -lnl-3 -o test
//
// "netlink/netlink.h: No such file or directory":
// Had to install:
//		sudo apt-get install libnl-3-dev
// and:
//		sudo apt-get install libnl-genl-3-dev
// and:  libnl-route-3-dev and libnfnetlink-dev
// and: sudo apt-get install libnl-dev libnl1
// FINALLY, it is compiling...
// sudo apt-get install libnl-genl-3-dev
/****
Base class opens nl80211 socket, derived classes to implement
Need InterfaceManager (SINGLETON, created by main(),
    somebody to detect platform "Y" or not (by OID?) (which I.M.
    main() should instantiate (only ONE for now
---    by OID? Supports Virtual iface? LO band and HI band?
- setup at start, return iface for email, AP, surveys
-  MAC addresses to ignore for survey
- Init() get list of interfaces, create virtual
    MONITOR mon0 iface, del the 'base' one
    Start wpasupplicant, then hostapd (still necessary?)
-- Lookup syntax for multiple inheritance. IIM is pure virtual,
     derived will derive from NL80211Base, Log (protected in header)
ChannelChanger using nl80211 sted IOCTLs
Radiotap parser class for ShadowX *and* Cardinal.

NL80211:
https://github.com/yurovsky/utils/blob/master/nl-genl/list-80211.c
http://yurovsky.github.io/2015/01/06/linux-netlink-list-wifi-interfaces/
https://github.com/bmegli/wifi-scan
https://wireless.wiki.kernel.org/en/developers/documentation/nl80211
http://elixir.free-electrons.com/linux/latest/source/include/uapi/linux/nl80211.h

****/
#include <iostream>
#include <string>
#include <sstream>

using namespace std;
#include "Nl80211Base.h"
#include "Log.h"

#include "InterfaceManagerNl80211.h"

int main(int argc, char* argv[])
{
	Log l;
	// Here we need to choose InterfaceManagerNl80211 or InterfaceManagerWext
	// (the latter is not implemented, so no real choice...)
	//   IInterfaceManager im;  <== A ptr in real use,
	//   this is a Singleton class; main instantiates
	// Need a GetInstance() function here.
	//InterfaceManagerNl80211 im("InterfaceManagerNl80211");
	//im.GetInterfaceList();
	InterfaceManagerNl80211 *im = InterfaceManagerNl80211::GetInstance();
	im->GetInterfaceList();
}