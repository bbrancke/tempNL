// ChannelSetterNl80211.h
// Set Channel using NL80211 subsystem.
// Replaces (deprecated) Wireless Extensions (WEXT) IOCTLs.

#ifndef CHANNELSETTERNL80211_H_
#define CHANNELSETTERNL80211_H_

#include <iostream>
#include <string>
#include <sstream>

#include "Log.h"
#include "Nl80211Base.h"
#include "IChannelSetter.h"
#include "InterfaceManagerNl80211.h"

using namespace std;

class ChannelSetterNl80211 : public IChannelSetter, public Nl80211Base
{
public:
	ChannelSetterNl80211();
	virtual bool OpenConnection();
	virtual bool SetChannel(uint32_t channel);
	virtual bool CloseConnection();
	virtual ~ChannelSetterNl80211();
private:
	uint32_t ChannelToFrequency(uint32_t channel);
	uint32_t m_interfaceIndex;
};

#endif  // CHANNELSETTERNL80211_H_

