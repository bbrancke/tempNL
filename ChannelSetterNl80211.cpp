// ChannelSetterNl80211.cpp
// Implements Channel Set using Nl80211,
// replaces (now deprecated) Wireless Extensions (WEXT) IOCTLs.

#include "ChannelSetterNl80211.h"

ChannelSetterNl80211::ChannelSetterNl80211() : Nl80211Base("ChannelSetterNl80211")
{ }

bool ChannelSetterNl80211::OpenConnection()
{
	if (!Open())
	{
		LogErr(AT, "Can't connect to NL80211.");
		return false;
	}
	// TODO: Get Survey Interface Name from InterfaceManager (const char *):
	GetInterfaceIndex("wlan0", m_interfaceIndex);
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
/***
Shouldn't this use (newer):
516: * @NL80211_CMD_SET_CHANNEL: Set the channel (using %NL80211_ATTR_WIPHY_FREQ
 *	and the attributes determining channel width) the given interface
 *	(identifed by %NL80211_ATTR_IFINDEX) shall operate on.
160: * @NL80211_CMD_SET_WIPHY: set wiphy parameters, needs %NL80211_ATTR_WIPHY or
 *	%NL80211_ATTR_IFINDEX; can be used to set %NL80211_ATTR_WIPHY_NAME,
 *	%NL80211_ATTR_WIPHY_TXQ_PARAMS, %NL80211_ATTR_WIPHY_FREQ (and the
 *	attributes determining the channel width; this is used for setting
 *	monitor mode channel),  %NL80211_ATTR_WIPHY_RETRY_SHORT,
 *	%NL80211_ATTR_WIPHY_RETRY_LONG, %NL80211_ATTR_WIPHY_FRAG_THRESHOLD,
 *	and/or %NL80211_ATTR_WIPHY_RTS_THRESHOLD.
 *	However, for setting the channel, see %NL80211_CMD_SET_CHANNEL  <=== THIS
 *	instead, the support here is for backward compatibility only.
See /usr/include/linux/nl80211.h
****/
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

