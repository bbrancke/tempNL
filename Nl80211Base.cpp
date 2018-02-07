// Nl80211Base.cpp

// Base class sets up connection to nl80211 sub-system.

#include "Nl80211Base.h"

Nl80211Base::Nl80211Base(const char* name) : Log(name)
{ }

int Nl80211Base::list_interface_handler(struct nl_msg *msg, void *arg)
{
	nl80211CallbackInfo* info = (nl80211CallbackInfo *)arg;
	// arg->m_pInstance == [this *]
	Nl80211Base* instance = info->m_pInstance;
	instance->LogInfo("list_interface_handler() called");
	return NL_SKIP;
}

int Nl80211Base::finish_handler(struct nl_msg *msg, void *arg)
{
	nl80211CallbackInfo* info = (nl80211CallbackInfo *)arg;
	info->status = 0;
	Nl80211Base* instance = info->m_pInstance;
	instance->LogInfo("finish_handler() called");
	// Caller checks:
	//   while (info.status > 0)
	//   {
	//       nl_recvmsgs(wifi.nls, cb);
	//   }
	return NL_SKIP;
}

bool Nl80211Base::FreeMessage()
{
	if (m_msg != nullptr)
	{
		nlmsg_free(m_msg);
		m_msg = nullptr;
	}
	return true;
}

bool Nl80211Base::Close()
{
	if (m_sock != nullptr)
	{
		nl_socket_free(m_sock);
		m_sock = nullptr;
	}
	if (m_cb != nullptr)
	{
		nl_cb_put(m_cb);
		m_cb = nullptr;
	}
	return FreeMessage();
}

bool Nl80211Base::Open()
{
	m_sock = nl_socket_alloc();
	if (m_sock == nullptr)
	{
		LogErr(AT, "Can't alloc netlink socket.");
		return false;
	}
	nl_socket_set_buffer_size(m_sock, 8192, 8192);
	if (genl_connect(m_sock))
	{
		LogErr(AT, "Can't connect to generic netlink.");
		Close();
		return false;
	}
// /usr/include/linux\nl80211.h:
// #define NL80211_GENL_NAME "nl80211"
	m_nl80211Id = genl_ctrl_resolve(m_sock, NL80211_GENL_NAME);
	if (m_nl80211Id < 0)
	{
		LogErr(AT, "nl80211: Not found.");
		Close();
		return false;
	}

	return true;
}

bool Nl80211Base::GetDeviceId(const char* ifaceName)
{
	m_deviceId = if_nametoindex(ifaceName);
	return true;
}

bool Nl80211Base::SetupCallback()
{
	m_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (m_cb == nullptr)
	{
		LogErr(AT, "Can't allocate netlink callback.");
		return false;
	}

	m_cbInfo.m_pInstance = this;
	m_cbInfo.status = 1;

	nl_cb_set(m_cb, NL_CB_VALID, NL_CB_CUSTOM, list_interface_handler, &m_cbInfo);
	return true;
}

// flags: 0 or NLM_F_DUMP if repeating responses expected.
// cmd:  NL80211_CMD_GET_INTERFACE - get List of interfaces
//       NL80211_CMD_SET_WIPHY - set frequency
bool Nl80211Base::SetupMessage(int flags, uint8_t cmd)
{
	m_msg = nlmsg_alloc();
	if (m_msg == nullptr)
	{
		LogErr(AT, "Can't allocate NL message");
		return false;
	}

	// Add Generic Netlink headers to Netlink message
	// void *genlmsg_put(struct nl_msg *msg, uint32_t port, uint32_t seq,
	//     int family, int hdrlen, int flags, uint8_t cmd, uint8_t version)
	//
	// Here is msg setup for SET FREQ:
	// genlmsg_put(msg, 0, 0, genl_family_get_id(state.nl80211), 0,
	//       0, NL80211_CMD_SET_WIPHY, 0);
	
 	// Get list of interfaces:
	//genlmsg_put(m_msg, 0, 0, m_nl80211Id, 0,
	//	NLM_F_DUMP, NL80211_CMD_GET_INTERFACE, 0);

	genlmsg_put(m_msg, 0, 0, m_nl80211Id, 0, flags, cmd, 0);
	
	return true;
}

// Add (unsigned) 32-bit parameter to the message.
// Used, e.g., "NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devid);" ==>
//    "AddMessageParameterU32(NL80211_ATTR_IFINDEX, deviceId);"
bool Nl80211Base::AddMessageParameterU32(enum nl80211_attrs parameterName, uint32_t value)
{
	NLA_PUT_U32(m_msg, parameterName, value);
	return true;
nla_put_failure:
	LogErr(AT, "Can't Add Parameter");
	return false;
}

bool Nl80211Base::SendWithRepeatingResponses()
{
	nl_send_auto_complete(m_sock, m_msg);
	// m_cbInfo.status = 1;
	// m_cb (an nl_cb *) can hold > 1 callback, calls finish_handler()
	//   when FINISHed, and list_interface_handler() foreach interface
	nl_cb_set(m_cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &m_cbInfo);
	// finish_handler() method sets m_cbInfo->status to zero when complete.
	while (m_cbInfo.status > 0)
	{
		nl_recvmsgs(m_sock, m_cb);
	}
	return true;
}

bool Nl80211Base::SendAndFreeMessage()
{
	nl_send_auto_complete(m_sock, m_msg);
	return FreeMessage();
}
/***
 struct nl_sock *nls;
        int nl80211_id;
    } wifi;

    wifi.nls = nl_socket_alloc();
    if (!wifi.nls) {
        fprintf(stderr, "Failed to allocate netlink socket.\n");
        return EXIT_FAILURE;
    }

    int ret = EXIT_SUCCESS;

    nl_socket_set_buffer_size(wifi.nls, 8192, 8192);

    if (genl_connect(wifi.nls)) {
        fprintf(stderr, "Failed to connect to generic netlink.\n");
        ret = EXIT_FAILURE;
        goto out_connect;
    }

    wifi.nl80211_id = genl_ctrl_resolve(wifi.nls, "nl80211");
    if (wifi.nl80211_id < 0) {
        fprintf(stderr, "nl80211 not found.\n");
        ret = EXIT_FAILURE;
        goto out_connect;
    }

    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Failed to allocate netlink message.\n");
        ret = EXIT_FAILURE;
        goto out_connect;
    }

    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        fprintf(stderr, "Failed to allocate netlink callback.\n");
        ret = EXIT_FAILURE;
        goto out_alloc;
    }

    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, list_interface_handler, NULL);

    genlmsg_put(msg, 0, 0, wifi.nl80211_id, 0,
            NLM_F_DUMP, NL80211_CMD_GET_INTERFACE, 0);

    nl_send_auto_complete(wifi.nls, msg);

    int err = 1;

    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);

    while (err > 0)
        nl_recvmsgs(wifi.nls, cb);

    nl_cb_put(cb);

out_alloc:
    nlmsg_free(msg);
out_connect:
nl_socket_free(wifi.nls);
****/
