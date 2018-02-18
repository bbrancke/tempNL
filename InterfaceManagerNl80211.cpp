// InterfaceManagerNl80211.cpp

#include "InterfaceManagerNl80211.h"

// Global static pointer used to ensure a single instance of the class:
InterfaceManagerNl80211* InterfaceManagerNl80211::m_pInstance = NULL; 

InterfaceManagerNl80211::InterfaceManagerNl80211() : Nl80211InterfaceAdmin("InterfaceManagerNl80211")
{ }

InterfaceManagerNl80211* InterfaceManagerNl80211::GetInstance()
{
	if (m_pInstance == nullptr)
	{
		m_pInstance = new InterfaceManagerNl80211;
	}
	return m_pInstance;
}

