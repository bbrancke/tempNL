// OneInterface.h
// Create one of these for each GET_INTERFACE callback.

#ifndef ONEINTERFACE_H_
#define ONEINTERFACE_H_

#include <cstring>

#include <stdint.h>

using namespace std;

class OneInterface
{
public:
	uint32_t phy;
	int macLength;  // Reported by GET_INTERFACEs
	char name[17];  // IFNAMSIZE is 16
	uint8_t mac[6];
	OneInterface(uint32_t thePhy, const char *ifaceName, const uint8_t *macAddr, int reportedMacLength)
	{
		phy = thePhy;
		macLength = reportedMacLength;
		strncpy(name, ifaceName, 16);
		name[16] = 0;
		memcpy(mac, macAddr, 6);
	}
};

#endif  // ONEINTERFACE_H_

