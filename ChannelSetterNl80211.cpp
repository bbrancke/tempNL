// ChannelSetterNl80211.cpp
// Implements Channel Set using Nl80211,
// replaces (now deprecated) Wireless Extensions (WEXT) IOCTLs.

#include "ChannelSetterNl80211.h"

ChannelSetterNl80211::ChannelSetterNl80211() : Nl80211Base("InterfaceManagerNl80211")
{ }

bool ChannelSetterNl80211::OpenConnection()
{
	if (!Open())
	{
		LogErr(AT, "Can't connect to NL80211.");
		return false;
	}
	// TODO: Get Survey Interface Name from InterfaceManager:
	GetDeviceId("wlan0");
	return true;
}

uint32_t ChannelSetterNl80211::ChannelToFrequency(uint32_t channel)
{
	if (channel < 14)
	{
		return 2407 + channel * 5;
	}
	if (channel == 14)
	{
		return 2484;
	}
	return (channel + 1000) * 5;
}

bool ChannelSetterNl80211::SetChannel(uint32_t channel)
{
	uint32_t freq = ChannelToFrequency(channel);
	uint32_t htval = NL80211_CHAN_NO_HT;

	if (!SetupMessage(0, NL80211_CMD_SET_WIPHY))
	{
		return false;
	}
	// NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devid);
	// NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, freq);
	// NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, htval);
	if (!AddMessageParameterU32(NL80211_ATTR_IFINDEX, m_deviceId)
		||
		!AddMessageParameterU32(NL80211_ATTR_WIPHY_FREQ, freq)
		||
		!AddMessageParameterU32(NL80211_ATTR_WIPHY_CHANNEL_TYPE, htval))
		{
			LogErr(AT, "SetChannel() aborted, AddParam() failed.");
			return false;
		}

	return SendAndFreeMessage();
}

bool ChannelSetterNl80211::CloseConnection()
{
	return true;
}

ChannelSetterNl80211::~ChannelSetterNl80211()
{
}

