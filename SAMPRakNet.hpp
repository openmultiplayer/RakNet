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

#include "../../Server/Components/LegacyNetwork/Query/query.hpp"

#define MAX_UNVERIFIED_RPCS (5)

class SAMPRakNet
{
public:
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

	static uint8_t * Decrypt(uint8_t const * src, int len);
	static uint8_t * Encrypt(uint8_t const * src, int len);

	static uint16_t GetPort();
	static void SetPort(uint16_t value);

	static uint32_t GetToken() { return token_; }
	static void SeedToken() { token_ = rand(); }

	static void HandleQuery(SOCKET instance, int size, const sockaddr_in & client, char const * buf);

	static Pair<uint8_t, StringView> GenerateAuth();
	static bool CheckAuth(uint8_t index, StringView auth);

	static void SeedCookie();
	static uint16_t GetCookie(unsigned int address);

	static void SetTimeout(unsigned int timeout) { timeout_ = timeout; }
	static unsigned int GetTimeout() { return timeout_; }

	static void SetQuery(Query* query) { query_ = query; }

	static void SetLogCookies(bool log) { logCookies_ = log; }
	static bool ShouldLogCookies() { return logCookies_; }

	static ICore* GetCore() { return core_; }

private:
	static uint8_t buffer_[MAXIMUM_MTU_SIZE];
	static uint32_t token_;
	static uint16_t portNumber;
	static Query *query_;
	static unsigned int timeout_;
	static bool logCookies_;
	static ICore* core_;
};

class SAMPRakNetChecksumException : public std::exception
{
public:
	explicit
		SAMPRakNetChecksumException(uint8_t expected, uint8_t got);

	char const *
		what() const noexcept override;

	const uint8_t
		Expected,
		Got;
};
