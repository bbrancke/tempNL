// IfIoctls.cpp
// Handle things like Set Mac Address, Bring Interface Up / Down
// that aren't done in Nl80211.

#include "IfIoctls.h"

IfIoctls::IfIoctls() : Log("IfIoctls") { }

IfIoctls::~IfIoctls()
{
	Close();
}

bool IfIoctls::Open()
{
	Close();
	m_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_fd < 0)
	{
		int myErr = errno;
		string s("Open(): Can't get socket: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}
	return true;
}

bool IfIoctls::Close()
{
	if (m_fd > 0)
	{
		close(m_fd);
		m_fd = -1;
	}
	return true;
}

bool IfIoctls::GetFlags(const char *interfaceName, int& flags)
{
	struct ifreq ifr;
	if (!Open())
	{
		return false;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ);
	if (ioctl(m_fd, SIOCGIFFLAGS, &ifr) < 0)
	{
		int myErr = errno;
		Close();
		string s("Get interface flags failed: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}

	flags = ifr.ifr_flags;
	Close();
	return true;
}

bool IfIoctls::SetFlags(const char *interfaceName, int flags)
{
	struct ifreq ifr;
	if (!Open())
	{
		return false;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ);
	ifr.ifr_flags = (short)flags;
	if (ioctl(m_fd, SIOCSIFFLAGS, &ifr) < 0)
	{
		int myErr = errno;
		Close();
		string s("Set interface flags failed: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}

	Close();
	return true;
}

bool IfIoctls::BringInterfaceUp(const char *interfaceName)
{
	int flags;
	if (!GetFlags(interfaceName, flags))
	{
		// Failure reason already logged.
		return false;
	}
	flags |= IFF_UP;
	return SetFlags(interfaceName, flags);
}

bool IfIoctls::BringInterfaceDown(const char *interfaceName)
{
	int flags;
	if (!GetFlags(interfaceName, flags))
	{
		// Failure reason already logged.
		return false;
	}
	flags &= ~IFF_UP;
	return SetFlags(interfaceName, flags);
}

bool IfIoctls::SetMacAddress(const char *ifaceName, const uint8_t *mac, bool isMonitorMode)
{
	struct ifreq ifr;
	if (!Open())
	{
		return false;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	memcpy(&ifr.ifr_hwaddr.sa_data, mac, 6);
	strncpy(ifr.ifr_name, ifaceName, IFNAMSIZ);
	// For a normal 80211 interface (STA / AP)
	// sa_family is ONE (Ethernet 10/100Mbps), this is ARPHRD_ETHER (== 1)
	// (see /usr/include/net/if_arp.h)
	// For interface in MONITOR mode (with Radiotap headers),
	// address family is ARPHRD_IEEE80211_RADIOTAP (== 803)
	ifr.ifr_hwaddr.sa_family =
		isMonitorMode
			?
			ARPHRD_IEEE80211_RADIOTAP
			:
			ARPHRD_ETHER;

	if (ioctl(m_fd, SIOCSIFHWADDR, &ifr) < 0)
	{
		int myErr = errno;
		Close();
		string s("SetMacAddress: Can't set SIOCSIHWADDR: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}

	Close();
	return true;
}

