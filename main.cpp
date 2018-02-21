// main.cpp - test code.
// To build:
// NO NO NO g++ -g -Wall -std=c++11 -I /usr/include/libnl3/ main.cpp InterfaceManagerNl80211.cpp Nl80211InterfaceAdmin.cpp ChannelSetterNl80211.cpp IfIoctls.cpp Nl80211Base.cpp Log.cpp -lnl-genl-3 -lnl-3 -o test
//
// Note: We do not have /usr/include/libnl3/ folder on ShadowX device.
//   I have added local copies of the netlink/*.h headers since
//   ShadowX does not have these headers and so can't build this
//   on the device; perhaps we need libnl3-dev /genl3-*dev* to the
//   main shadowx-console-image recipe??)
// Now use:
//  g++ -g -Wall -std=c++11 main.cpp InterfaceManagerNl80211.cpp Nl80211InterfaceAdmin.cpp ChannelSetterNl80211.cpp IfIoctls.cpp Nl80211Base.cpp Log.cpp -lnl-genl-3 -lnl-3 -o test
//
// "netlink/netlink.h: No such file or directory":
// Had to install:
//		sudo apt-get install libnl-3-dev
// and:
//		sudo apt-get install libnl-genl-3-dev
// and:  libnl-route-3-dev and libnfnetlink-dev
// and: sudo apt-get install libnl-dev libnl1
// WARNING: One of the without the '3' resets everything BACK!
// FINALLY, it is compiling...
// sudo apt-get install libnl-genl-3-dev
//
/****
Base class opens nl80211 socket, derived classes needed to implement.
Need InterfaceManagerNl80211 (SINGLETON, created by main(),
    somebody to detect platform "Y" or not (by OID?) (which
    main() should instantiate (only ONE for now, *no* WEXT/ioctl version.
---    by OID? Supports Virtual iface? LO band and HI band? )NOT IMPLEMENTED)
- setup at start, return iface for email, AP, surveys
-  MAC addresses to ignore for survey
- Init() get list of physical devices, ensures that we have TWO (not one,
   three is right out...)
-- CreateInterfaces(): Create virtual interfaces (VIFs)
    "sta0" and "ap0" for the TI (built-in) chip and
    "mon0" MONITOR VIF for the USB radio,
    del the 'base' interfaces (wlan0 and wlan1).

main() should use its Terminator class to kill any hostapd
   and wpa_supplicant's running BEFORE calling CreateInterfaces()
   and then start hostapd (on ap0) AFTER CreateInterfaces().
   Alert emails can then be sent using wpa_supplicant (on sta0).
InterfaceManagerNl80211 is a singleton and has methods that
   return the Interface Name that Survey's "Pcap Open" and
   "Channel Changer->ChannelSetter" *and* EmailManager should
   call in case the VIF names change.

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
	bool rv;
	// Here we need to choose InterfaceManagerNl80211 or InterfaceManagerWext
	// (the latter is not implemented, so no real choice...)
	//   IInterfaceManager im;  <== A ptr in real use,
	//   this is a Singleton class; main instantiates
	// Need a GetInstance() function here.
	//InterfaceManagerNl80211 im("InterfaceManagerNl80211");
	//im.GetInterfaceList();
	InterfaceManagerNl80211 *im = InterfaceManagerNl80211::GetInstance();
	cout << "main(): calling Init()..." << endl;
	rv = im->Init();
	if (!rv)
	{
		cout << "main(): Init() failed!" << endl;
		return 0;
	}
	cout << "main(): Init() OK, calling CreateInterfaces()..." << endl;
	rv = im->CreateInterfaces();
	if (!rv)
	{
		cout << "main(): CreateInterfaces() failed!" << endl;
		return 0;
	}
	cout << "main(): CreateInterfaces() success, complete!" << endl << endl;
}
