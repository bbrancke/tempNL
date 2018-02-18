// Nl80211InterfaceAdmin.cpp
// Includes convenience "translator" funcs between InterfaceManagerNl80211
// class and Nl80211Base class, as well as Interface IOCTLs not provided
// by NL80211 API (e.g., SetMacAddress). [At least, I don't see how...]

#include "Nl80211InterfaceAdmin.h"

Nl80211InterfaceAdmin::Nl80211InterfaceAdmin(const char *name) : Nl80211Base(name) { }

bool Nl80211InterfaceAdmin::GetInterfaceList()
{
	LogInfo("Get InterfaceList() entry.");

	if (!Open())
	{
		Close();
		LogErr(AT, "Nl80211 open failed.");
		return false;
	}
	if (!SetupCallback())
	{
		Close();
		LogErr(AT, "Nl80211 SetupCallback failed.");
		return false;
	}
	// flags: 0 or NLM_F_DUMP if repeating responses expected.
	// cmd:  NL80211_CMD_GET_INTERFACE - get List of interfaces
	//       NL80211_CMD_SET_WIPHY - set frequency
	if (!SetupMessage(NLM_F_DUMP, NL80211_CMD_GET_INTERFACE))
	{
		Close();
		LogErr(AT, "Nl80211 SetupMessage failed.");
		return false;
	}
	// Call this when expecting multiple responses [e.g., GetInterfaceList()]:
	if (!SendWithRepeatingResponses())
	{
		Close();
		LogErr(AT, "Nl80211 SendWithRepeatingResponses failed.");
		return false;
	}
	// All done, close everything:
	Close();
	LogInfo("GetInterfaceList() complete, success");
	return true;
}

bool Nl80211InterfaceAdmin::SetMacAddress(const char *ifaceName, const uint8_t *mac)
{
	int fd;
	struct ifreq dev;

	fd = socket (AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		int myErr = errno;
		string s("SetMacAddress: Can't get socket: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}

	memset(&dev, 0, sizeof(struct ifreq));
	memcpy(&dev.ifr_hwaddr.sa_data, mac, 6);
	snprintf(dev.ifr_name, IFNAMSIZ, "%s", ifaceName);
	dev.ifr_hwaddr.sa_family = AF_INET;

	if (ioctl(fd, SIOCSIFHWADDR, &dev) < 0)
	{
		int myErr = errno;
		close(fd);
		string s("SetMacAddress: Can't set SIOCSIHWADDR: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}
	// else OK...
	close(fd);
	return true;
}

