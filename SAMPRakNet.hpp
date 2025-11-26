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

#include "Include/raknet/NetworkTypes.h"
#include "Include/raknet/GetTime.h"

#define MAX_UNVERIFIED_RPCS (5)

#define LOCALHOST (0x0100007fu)

#include <shared_mutex>

enum class OmpVersion
{
	None = 0,
	v0_0_1 = 0x000001,
	v0_1_4 = 0x000104,
};

using PlayerAddressHash = uint64_t;

class SAMPRakNet
{
public:
	static constexpr int ENCRYPTION_KEY_SIZE = 4;
	static constexpr int MAGIC_OMP_IDENTIFICATION_NUMBER = 0x006F6D70; // sent to client on connect request and handled there so we know it's a real packet from us. it's basically 'omp' in 32bit btw
	static constexpr int OMP_PETARDED = 0x6D70; // it's basically 'mp' in 16bit
	static constexpr int SAMP_PETARDED = 0x6969; // it's from default SAMP... Petarded [S04E06]
	static constexpr OmpVersion CURRENT_OMP_CLIENT_MOD_VERSION = OmpVersion::v0_1_4;

	enum AuthType
	{
		AuthType_Invalid,
		AuthType_Player,
		AuthType_NPC
	};

	struct RemoteSystemData
	{
		uint8_t authIndex;
		AuthType authType;
		uint8_t unverifiedRPCs;

		RemoteSystemData()
			: authIndex(0)
			, authType(AuthType_Invalid)
			, unverifiedRPCs(0)
		{
		}
	};

	struct OmpPlayerEncryptionData
	{
		uint32_t key;
		OmpVersion version;

		OmpPlayerEncryptionData()
			: key(0)
			, version(OmpVersion::None)
		{
		}

		OmpPlayerEncryptionData(uint32_t _key, OmpVersion _version)
			: key(_key)
			, version(_version)
		{
		}
	};

	static void Init(ICore* core)
	{
		core_ = core;
		srand(time(nullptr));
	}

	static PlayerAddressHash HashPlayerID(const RakNet::PlayerID& player)
	{
		return (static_cast<PlayerAddressHash>(player.binaryAddress) << 16) | player.port;
	}

	static uint8_t* Decrypt(uint8_t const* src, int len);
	static uint8_t* Encrypt(const OmpPlayerEncryptionData* encryptionData, uint8_t const* src, int len);

	static uint16_t GetPort();
	static void SetPort(uint16_t value);

	static uint32_t GetToken() { return token_; }
	static void SeedToken() { token_ = rand(); }

	static void HandleQuery(SOCKET instance, int outsize, const sockaddr_in& client, char const* buf, int insize);

	static Pair<uint8_t, StringView> GenerateAuth();
	static bool CheckAuth(uint8_t index, StringView auth);

	static void SeedCookie();
	static uint16_t GetCookie(unsigned int address);

	static void SetTimeout(unsigned int timeout) { timeout_ = timeout; }
	static unsigned int GetTimeout() { return timeout_; }

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

	static void SetMinimumSendBitsPerSecond(float bps) { minimumSendBitsPerSecond_ = bps; }
	static inline float GetMinimumSendBitsPerSecond() { return minimumSendBitsPerSecond_; }

	static ICore* GetCore() { return core_; }

	static void ReplyToOmpClientAccessRequest(SOCKET connectionSocket, const RakNet::PlayerID& playerId, uint32_t encryptionKey);

	static bool IsPlayerUsingOmp(PlayerAddressHash hash)
	{
		return ompPlayers_.find(hash) != ompPlayers_.end();
	}

	static bool IsPlayerUsingOmp(const RakNet::PlayerID& player)
	{
		auto hash = HashPlayerID(player);
		return ompPlayers_.find(hash) != ompPlayers_.end();
	}

	static void ResetOmpPlayerConfiguration(const RakNet::PlayerID& player)
	{
		auto hash = HashPlayerID(player);
		ompPlayers_.erase(hash);
	}

	static void ConfigurePlayerUsingOmp(const RakNet::PlayerID& player, uint32_t key)
	{
		auto hash = HashPlayerID(player);
		ompPlayers_[hash] = OmpPlayerEncryptionData(key, OmpVersion::None);
	}

	static void SetPlayerOmpVersion(const RakNet::PlayerID& player, uint32_t ompVersion)
	{
		auto hash = HashPlayerID(player);
		auto data = ompPlayers_.find(hash);
		if (data != ompPlayers_.end())
		{
			data->second.version = OmpVersion(ompVersion);
		}
	}

	static const OmpPlayerEncryptionData* GetOmpPlayerEncryptionData(PlayerAddressHash hash)
	{
		auto it = ompPlayers_.find(hash);
		if (it != ompPlayers_.end())
		{
			return &it->second;
		}
		return nullptr;
	}

	static const OmpPlayerEncryptionData* GetOmpPlayerEncryptionData(const RakNet::PlayerID& player)
	{
		auto hash = HashPlayerID(player);
		auto it = ompPlayers_.find(hash);
		if (it != ompPlayers_.end())
		{
			return &it->second;
		}
		return nullptr;
	}

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

	static bool IsOmpEncryptionEnabled()
	{
		static bool* isEnabled = core_->getConfig().getBool("network.use_omp_encryption");
		return isEnabled ? *isEnabled : false;
	}

	static bool OnConnectionRequest(
		SOCKET connectionSocket,
		RakNet::PlayerID& playerId,
		const char* data,
		RakNet::RakNetTime& minConnectionTick,
		RakNet::RakNetTime& minConnectionLogTick);

private:
	static uint8_t decryptBuffer_[MAXIMUM_MTU_SIZE];
	static uint8_t encryptBuffer_[MAXIMUM_MTU_SIZE];
	static uint32_t token_;
	static uint16_t portNumber;
	static Query* query_;
	static unsigned int timeout_;
	static bool logCookies_;
	static unsigned int minConnectionTime_;
	static unsigned int messagesLimit_;
	static unsigned int messageHoleLimit_;
	static unsigned int acksLimit_;
	static unsigned int networkLimitsBanTime_;
	static float minimumSendBitsPerSecond_;
	static ICore* core_;
	static FlatHashSet<uint32_t> incomingConnections_;
	static RakNet::RakNetTime gracePeriod_;
	static FlatHashMap<PlayerAddressHash, OmpPlayerEncryptionData> ompPlayers_;
};
