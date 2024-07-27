#pragma once

#include <cstdint>
#include <string>
#include <exception>

#if defined _WIN32 || defined WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#if __has_include(<MTUSize.h>) 
#include <MTUSize.h> 
#endif

#ifndef MAXIMUM_MTU_SIZE
#define MAXIMUM_MTU_SIZE 1500
#endif

#define MAX_AUTH_RESPONSE_LEN (64)

#ifdef BUILD_FOR_CLIENT
#define AUTHKEY_RESPONSE_LEN (40)
#endif

#ifndef BUILD_FOR_CLIENT
#include "../../Server/Components/LegacyNetwork/Query/query.hpp"
#endif

#include "Include/raknet/NetworkTypes.h"
#include "Include/raknet/GetTime.h"

#define MAX_UNVERIFIED_RPCS (5)

#define LOCALHOST (0x0100007fu)

class SAMPRakNet
{
public:
#ifndef BUILD_FOR_CLIENT
	enum AuthType {
		AuthType_Invalid,
		AuthType_Player,
		AuthType_NPC
	};

	struct RemoteSystemData {
		uint8_t authIndex;
		AuthType authType;
		uint8_t unverifiedRPCs;

		RemoteSystemData() : authIndex(0), authType(AuthType_Invalid), unverifiedRPCs(0)
		{}
	};

	static void Init(ICore* core) {
		core_ = core;
		srand(time(nullptr));
	}
#endif

	static uint8_t * Decrypt(uint8_t const * src, int len);
	static uint8_t * Encrypt(uint8_t const * src, int len);

	static uint16_t GetPort();
	static void SetPort(uint16_t value);

#ifdef BUILD_FOR_CLIENT
	static char * PrepareAuthkeyResponse(const char* initialKey);
#endif

#ifndef BUILD_FOR_CLIENT
	static uint32_t GetToken() { return token_; }
	static void SeedToken() { token_ = rand(); }

	static void HandleQuery(SOCKET instance, int outsize, const sockaddr_in& client, char const* buf, int insize);

	static Pair<uint8_t, StringView> GenerateAuth();
	static bool CheckAuth(uint8_t index, StringView auth);

	static void SeedCookie();
	static uint16_t GetCookie(unsigned int address);
#endif

	static void SetTimeout(unsigned int timeout) { timeout_ = timeout; }
	static unsigned int GetTimeout() { return timeout_; }

#ifdef BUILD_FOR_CLIENT
	static void SetConnectionAsNpc(bool enabled) { connectAsNpc_ = enabled; }
	static bool ShouldConnectAsNpc() { return connectAsNpc_; }
#endif

#ifndef BUILD_FOR_CLIENT
	static void SetQuery(Query* query) { query_ = query; }

	static void SetLogCookies(bool log) { logCookies_ = log; }
	static bool ShouldLogCookies() { return logCookies_; }

	static void SetMinConnectionTime(unsigned int time) { minConnectionTime_ = time; }
	static unsigned int GetMinConnectionTime() { return minConnectionTime_; }

	static void SetMessagesLimit(unsigned int limit) { messagesLimit_ = limit; }
	static unsigned int GetMessagesLimit() { return messagesLimit_; }

	static void SetMessageHoleLimit(unsigned int limit) { messageHoleLimit_ = limit; }
	static unsigned int GetMessageHoleLimit() { return messageHoleLimit_; }

	static void SetAcksLimit(unsigned int limit) { acksLimit_ = limit; }
	static unsigned int GetAcksLimit() { return acksLimit_; }

	static void SetNetworkLimitsBanTime(unsigned int time) { networkLimitsBanTime_ = time; }
	static unsigned int GetNetworkLimitsBanTime() { return networkLimitsBanTime_; }

	static void SetGracePeriod(unsigned int time) { gracePeriod_ = RakNet::GetTime() + time; }
	static RakNet::RakNetTime GetGracePeriod() { return gracePeriod_; }

	static ICore* GetCore() { return core_; }

	static bool IsAlreadyRequestingConnection(unsigned int binaryAddress)
	{
		return incomingConnections_.find(binaryAddress) != incomingConnections_.end();
	}

	static void SetRequestingConnection(unsigned int binaryAddress, bool status)
	{
		if (status)
			incomingConnections_.insert(binaryAddress);
		else
			incomingConnections_.erase(binaryAddress);
	}

	static bool OnConnectionRequest(
		SOCKET connectionSocket,
		RakNet::PlayerID& playerId,
		const char* data,
		RakNet::RakNetTime& minConnectionTick,
		RakNet::RakNetTime& minConnectionLogTick
	);
#endif

private:
	static uint8_t buffer_[MAXIMUM_MTU_SIZE];
#ifdef BUILD_FOR_CLIENT
	static char authkeyBuffer_[AUTHKEY_RESPONSE_LEN];
	static bool connectAsNpc_;
#endif
#ifndef BUILD_FOR_CLIENT
	static uint32_t token_;
#endif
	static uint16_t portNumber;
#ifndef BUILD_FOR_CLIENT
	static Query *query_;
#endif
	static unsigned int timeout_;
#ifndef BUILD_FOR_CLIENT
	static bool logCookies_;
    static unsigned int minConnectionTime_;
    static unsigned int messagesLimit_;
    static unsigned int messageHoleLimit_;
    static unsigned int acksLimit_;
    static unsigned int networkLimitsBanTime_;
	static ICore* core_;
	static FlatHashSet<uint32_t> incomingConnections_;
	static RakNet::RakNetTime gracePeriod_;
#endif
};
