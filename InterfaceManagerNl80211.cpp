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

bool InterfaceManagerNl80211::GetInterfaceList()
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
		LogErr(AT, "Nl80211 SetupMessagw failed.");
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

