// Nl80211Base.h
// Base class sets up connection to nl80211 sub-system.

#ifndef NL80211BASE_H_
#define NL80211BASE_H_

#include <iostream>
#include <string>
#include <sstream>

// Use: "-I /usr/include/libnl3/"
#include <netlink/socket.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

#include <linux/nl80211.h>

#include "Log.h"

using namespace std;

// fwd def:
class Nl80211Base;

// this is the "arg *" we're going to
// pass back to our nl80211 callback
typedef struct
{
	Nl80211Base* m_pInstance;
	int status;
} nl80211CallbackInfo;

class Nl80211Base : protected Log
{
public:
	Nl80211Base(const char* name);
	// The NL80211_CMD_GET_INTERFACE command has two callback functions:
	static int finish_handler(struct nl_msg *msg, void *arg);
	// 'arg' is the last parameter to the xxx call:
	// int nl_cb_set(struct nl_cb *cb, enum nl_cb_type type,
	//      enum nl_cb_kind kind, nl_recvmsg_msg_cb_t func,
	//      void *arg <== this guy);
	// Example:
	//     nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, list_interface_handler, NULL);
	static int list_interface_handler(struct nl_msg *msg, void *arg);
	
	bool Close();
	bool Open();
	bool SetupCallback();
	// flags: 0 or NLM_F_DUMP if repeating responses expected.
	// cmd:  NL80211_CMD_GET_INTERFACE - get List of interfaces
	//       NL80211_CMD_SET_WIPHY - set frequency
	bool SetupMessage(int flags, uint8_t cmd);
	// Call this when expecting multiple responses [e.g., GetInterfaceList()]:
	bool SendWithRepeatingResponses();
	// TODO: Send with no mult [e.g., SetChannel()]
	
protected:
	Nl80211Base() { }
private:
	struct nl_sock *m_sock = nullptr;
	struct nl_msg *m_msg = nullptr;
	struct nl_cb *m_cb = nullptr;
	int m_nl80211Id;
	nl80211CallbackInfo m_cbInfo;
};

/*****
C-style callbacks in C++:
From:
http://tanks4code.blogspot.com/2008/07/c-style-callbacks-in-c-code.html

// this is the "additional information" we're going to
// pass back to our callback
typedef struct
{
  // stuff
} callbackInfo;

// this is the callback prototype
typedef void (*cbfunc)(const callbackInfo*, void*);
===========================================================
User Side:
// BB: Don't we need "CDecl" (or something like that) so the
//   Calling Convention is right? Does the called method or
//   the Caller do the stack cleanup? Apparently not...
//
// this is the "callback side" (user) class
class CallMe
{
public:
  CallMe()
  {
    globalCaller.registerCB(&myCallback, this);
  }
  ~CallMe()
  {
    globalCaller.deregisterCB(&myCallback, this);
  }
  static void myCallback(const callbackInfo* info, void* userData)
  {
    if (!userData) return;
    ((CallMe*)userData)->localCallback(info);
  }
  void localCallback(const callbackInfo* info)
  {
    // do stuff
  }
}
//
// The NL80211_CMD_GET_INTERFACE command has two callback functions:
//    static int finish_handler(struct nl_msg *msg, void *arg)
// and:
//    static int list_interface_handler(struct nl_msg *msg, void *arg)
******/

#endif  // NL80211BASE_H_

