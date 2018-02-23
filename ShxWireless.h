// ShxWireless.h
#ifndef SHXWIRELESS_H_
#define SHXWIRELESS_H_
// Problem:
//    #include <net/if.h>
//    #include <linux/wireless.h>
// Compile:
//    "error: redefinition of ‘struct ifconf’... in linux/wireless.h *and* net/if.h
//
// This takes some of the "guts" from linux/wireless.h
//   (in particular, definition of "struct iwreq"
//   so we don't need to include linux/wireless.h
// See for example: What's wrong with linux/if.h and net/if.h?
//   https://stackoverflow.com/questions/8755030/whats-wrong-with-linux-if-h-and-net-if-h
//     (Summary: One is for kernel-space, the other for user-space.)
// This is a (partial) copy of /usr/include/linux/wireless.h

// *******
//   We prefer to avoid using the (kernel-space) headers like linux/if.h and instead
//   use the (user-space) headers such as net/if.h...
// *******

#include <stdint.h>

//#define	IFNAMSIZ	16   // linux/if.h
#define SHX_IFNAMESIZE 16

// ioctl() commands:
// Power saving stuff (power management, unicast and multicast)
// ADD 'SHX' to the beginning to avoid conflict with the
//   "real" value.
#define SHX_SIOCSIWPOWER	0x8B2C		// Set Power Management settings
#define SHX_SIOCGIWPOWER	0x8B2D		// Get Power Management settings

// For SetWirelessPowerSaveOff(), we need to set wrq.u.power.disabled = 1;
// 'u' is type iwreq_data, is a union of approx 5,000,000 structs.
// Only including the relevant ones for me.
// FUTURE USES: MISSING / UNKNOWN element names?
// Look them up in /usr/include/linux/wireless.h and add to 
//  definition of "union shx_iwreq_data
//
// This is 'struct iw_param' in wireless.h (at line 673):
/* Generic format for most parameters that fit in an int */
/**********
struct	iw_param
{
  __s32		value;		// The value of the parameter itself
  __u8		fixed;		// Hardware should not use auto select
  __u8		disabled;	// Disable the feature
  __u16		flags;		// Various specifc flags (if any)
};
***********/
// ShadowX's version is:
struct shx_iw_param
{
	int32_t value;     // The value of the parameter itself.
	uint8_t fixed;     // Hardware should not use auto select.
	uint8_t disabled;  // Disable the feature.
	uint16_t flags;    // Various specific flags (in any).
};

// This is ShadowX's (stripped-down) version of 'iwreq_data':
//   Currently we only need the 'power' struct
// This is wireless.h's "union iwreq_data"
//   Hmmm... Why isn't "union" pronounced like the tasty round rings
//   on top of a burger ("Onion")?
union shx_iwreq_data
{
	/* Config - generic */
	char name[IFNAMSIZ];
	// Name : used to verify the presence of  wireless extensions.
	// Examples of ELIDED stuff in this union:
	// struct iw_freq freq;  // frequency (0->1000) or channel (> 1000)
	// struct iw_param txpower;  // default transmit power
	// ... etc etc etc ...
	// Here is the only one we currently need, it is 'struct iw_param' in wireless.h:
	struct shx_iw_param	power;		/* PM duration/timeout */
};



/*
 * The structure to exchange data for ioctl.
 * This structure is the same as 'struct ifreq', but (re)defined for
 * convenience...
 * Do I need to remind you about structure size (32 octets) ?
 */
 // THIS IS ShadowX version of 'struct iwreq':
struct shx_iwreq 
{
	union
	{
		char ifrn_name[SHX_IFNAMESIZE];	/* if name, e.g. "eth0" */
	} ifr_ifrn;

	/* Data part (defined just above) */
	union shx_iwreq_data u;
};

#endif  // SHXWIRELESS_H_

