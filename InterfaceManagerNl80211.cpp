// InterfaceManagerNl80211.cpp

#include "InterfaceManagerNl80211.h"

// Global static pointer used to ensure a single instance of the class:
InterfaceManagerNl80211* InterfaceManagerNl80211::m_pInstance = NULL; 

InterfaceManagerNl80211::InterfaceManagerNl80211() : Nl80211InterfaceAdmin("InterfaceManagerNl80211")
{
	// Random seed (for random MAC addresses):
	srand(time(nullptr));
}

InterfaceManagerNl80211* InterfaceManagerNl80211::GetInstance()
{
	if (m_pInstance == nullptr)
	{
		m_pInstance = new InterfaceManagerNl80211;
	}
	return m_pInstance;
}

// bool InterfaceManagerNl80211::Init() implemented already; gets interface list
//   and ensures exactly TWO phy Ids (two physical wi-fi devices).

bool InterfaceManagerNl80211::CreateInterfaces()
{
	// Typically we have two entries in m_interfaces at startup:
	// Entries (type: OneInterface *) in m_interfaces:
	// OneInterface members: phy (uint32_t), name[IFNAMSIZE] (char), mac[6] (uint8_t)
	//    The device:		Name:	phy	mac
	//  Built-in TI chip	wlan0	0	D0:B5:C2:CB:90:CA
	//  Realtek USB radio	wlan1	1	EC:F0:0E:67:79:0A
	// If here, then Init() guaranteed we have two physical devices
	// I have seen name and phy reversed (Realtek came up BEFORE the TI chip)
	// and the name and phy can differ if something weird/bad happened before
	// ShadowX started (or was re-started).
	//
	// When this method completes, we will have:
	//	TI chip			apX		0	For hostapd --------+-- TWO Virtual
	//					staY	0	For wpa_supplicant -+-- Interfaces
	//													+-- on TI chip
	//	Realtek USB		monZ	1	For survey / pcap
	// Get Interface names from me -- MONITOR only!:
	// FOR NOW, X, Y and Z are all 0 (ap0, sta0, mon0)
	//   so we don't have to update hostapd.conf, wpa_supp.conf, etc.
	//   I think this will be OK. May change if can't recreate
	//     an interface name that prev. existed and had an error.
	//
	// IMPORTANT:
	// main() has a Terminator class and must ensure that hostapd and
	// wpa_supplicant are no longer running; main() should start hostapd
	// AFTER this method creates interfaces. (wpa_supplicant is run
	// only when sending alert emails, and is started and stopped by
	// the email manager).
	//
	// Get the phy ID for the TI chip, look for OUI (first 3 bytes)
	// of "D0-B5-C2"
	// If ShadowX is restarted, we longer have "wlan0" but will have
	// "sta0" and "ap0" whose MAC addresses begin with D0-B5-C2.
	uint32_t phyId = 0;
	bool found = false;
	for (OneInterface* i : m_interfaces)
	{
		if (memcmp(m_TiChipsetOui, i->mac, 3) == 0)
		{
			found = true;
			phyId = i->phy;
			break;
		}
	}
	if (!found)
	{
		LogErr(AT, "CreateInterfaces(): Can't find TI OUI, assuming phyId = 0");
		// Continue anyway...
	}
	// Delete all interfaces associated with this phy Id:
	//  Delete..() and Create...() are in Nl80211InterfaceAdmin class
	//  and Open() / Close() the NL80211 connection for us automatically.
	for (OneInterface* i : m_interfaces)
	{
		if (i->phy == phyId)
		{
			m_ifIoctls.BringInterfaceDown(i->name);
			if (!DeleteInterface(i->name))
			{
				string s("CreateInterfaces(): Can't delete iface [");
				s += i->name;
				s += "], continuing...";
				LogErr(AT, s);
			}
		}
	}
	// Note: Need to re-read InterfaceList here, it is no longer valid...

	if (!CreateApInterface("ap0", phyId))
	{
		LogErr("CreateInterfaces(): Can't create AP Interface ap0!");
		// NL80211 connection is closed and cleaned up at this point.
		return false;
	}
	if (!CreateStationInterface("sta0", phyId))
	{
		LogErr("CreateInterfaces(): Can't create STA Interface sta0!");
		return false;
	}
	
	if (!m_ifIoctls.BringInterfaceDown("ap0")
		|| !m_ifIoctls.BringInterfaceDown("sta0"))
	{
		LogErr("CreateInterfaces(): Can't bring Interface down.");
		return false;
	}

	// Frank thinks that ALL the TI chips will have the SAME MAC address
	// across all ShadowX devices, so randomize the AP and STA interface
	// MAC addresses:
	uint8_t mac[6];
	memcpy(mac, m_TiChipsetOui, 3);
	mac[3] = rand() % 256;
	mac[4] = rand() % 256;
	mac[5] = rand() % 256;
	if (!m_ifIoctls.SetMacAddress("ap0", mac)
		||
		!m_ifIoctls.SetMacAddress("sta0", mac))
	{
		LogErr("CreateInterfaces(): Can't set Interface MAC address, continuing...");
	}
	// Bring UP the two new TI interfaces:
	if (!m_ifIoctls.BringInterfaceUp("ap0")
		|| !m_ifIoctls.BringInterfaceUp("sta0"))
	{
		LogErr("CreateInterfaces(): Can't bring Interface up.");
		return false;
	}
	
	// --------------------------------------------------------
	// Set up mon0 interface on the USB radio:
	uint32_t tiPhy = phyId;
	phyId = 1;
	found = false;
	// Get the OTHER Phy ID:
	for (OneInterface* i : m_interfaces)
	{
		if (i->phy != tiPhy)
		{
			found = true;
			phyId = i->phy;
			break;
		}
	}
	if (!found)
	{
		LogErr(AT, "CreateInterfaces(): Can't find USB Phy ID, assuming phyId = 1");
		// Continue anyway...
	}
	for (OneInterface* i : m_interfaces)
	{
		if (i->phy == phyId)
		{
			m_ifIoctls.BringInterfaceDown(i->name);
			if (!DeleteInterface(i->name))
			{
				string s("CreateInterfaces(): Can't delete iface [");
				s += i->name;
				s += "], continuing...";
				LogErr(AT, s);
			}
		}
	}
	
	// Create mon0 interface:
	if (!CreateMonitorInterface("mon0", phyId))
	{
		LogErr(AT, "CreateInterfaces(): Can't create mon0 interface.");
		return false;
	}

	if (!m_ifIoctls.BringInterfaceUp("mon0"))
	{
		LogErr("CreateInterfaces(): Can't bring mon0 up.");
		return false;
	}

	// Finally re-read the new interface list:
	if (!GetInterfaceList())
	{
		LogErr("CreateInterfaces(): Can't re-read Interface List.");
		return false;
	}
	return true;
}

const char *InterfaceManagerNl80211::GetMonitorInterfaceName()
{
	// Called by Survey's ChannelChange->ChannelSetter class.
	// Currently this ALWAYS "mon0" but this may change if re-creating
	// a troubled iface name does not succeed.
	return "mon0";
}

