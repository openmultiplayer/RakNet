#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define closesocket close
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

namespace
{
	const unsigned char ID_OPEN_CONNECTION_REQUEST = 24;
	const unsigned char ID_OPEN_CONNECTION_REPLY = 25;
	const unsigned char ID_OPEN_CONNECTION_COOKIE = 26;
	const unsigned char ID_CONNECTION_ATTEMPT_FAILED = 29;
	const unsigned char ID_NO_FREE_INCOMING_CONNECTIONS = 31;
	const unsigned short SAMP_PETARDED = 0x6969;

	void PrintUsage()
	{
		std::fprintf(stderr, "Usage: omp_connect_probe <family> <host> <port>\n");
		std::fprintf(stderr, "  family: ipv4 | ipv6\n");
	}

	int ParseFamily(const char* value)
	{
		if (std::strcmp(value, "ipv4") == 0)
			return AF_INET;
		if (std::strcmp(value, "ipv6") == 0)
			return AF_INET6;
		return AF_UNSPEC;
	}

	bool ResolveAddress(const char* host, const char* port, int family, sockaddr_storage* output, socklen_t* outputLen)
	{
		addrinfo hints;
		addrinfo* result = 0;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = family;
		hints.ai_socktype = SOCK_DGRAM;

		const int rc = getaddrinfo(host, port, &hints, &result);
		if (rc != 0)
		{
			std::fprintf(stderr, "getaddrinfo(%s,%s) failed: %s\n", host, port, gai_strerror(rc));
			return false;
		}

		bool ok = false;
		for (addrinfo* it = result; it; it = it->ai_next)
		{
			if (it->ai_family == family)
			{
				std::memset(output, 0, sizeof(sockaddr_storage));
				std::memcpy(output, it->ai_addr, (size_t)it->ai_addrlen);
				*outputLen = (socklen_t)it->ai_addrlen;
				ok = true;
				break;
			}
		}

		freeaddrinfo(result);
		return ok;
	}

	unsigned short ParsePort(const char* value)
	{
		char* end = 0;
		const unsigned long parsed = std::strtoul(value, &end, 10);
		if (end == value || *end != '\0' || parsed > 65535)
			return 0;
		return (unsigned short)parsed;
	}

	bool SendRequest(SOCKET socketFd, const sockaddr_storage& target, socklen_t targetLen, unsigned short cookieXor)
	{
		unsigned char request[3];
		request[0] = ID_OPEN_CONNECTION_REQUEST;
		request[1] = (unsigned char)(cookieXor & 0xFF);
		request[2] = (unsigned char)((cookieXor >> 8) & 0xFF);

		if (sendto(socketFd, reinterpret_cast<const char*>(request), sizeof(request), 0, reinterpret_cast<const sockaddr*>(&target), targetLen) != SOCKET_ERROR)
			return true;

#ifdef _WIN32
		std::fprintf(stderr, "sendto failed: %d\n", WSAGetLastError());
#else
		std::fprintf(stderr, "sendto failed: errno=%d (%s)\n", errno, std::strerror(errno));
#endif
		return false;
	}
}

int main(int argc, char** argv)
{
#ifdef _WIN32
	WSADATA winsockInfo;
	if (WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0)
		return 1;
#endif

	if (argc != 4)
	{
		PrintUsage();
		return 1;
	}

	const int family = ParseFamily(argv[1]);
	const char* host = argv[2];
	const char* portString = argv[3];
	if (family == AF_UNSPEC)
	{
		std::fprintf(stderr, "invalid family: %s\n", argv[1]);
		return 1;
	}

	const unsigned short port = ParsePort(portString);
	if (port == 0)
	{
		std::fprintf(stderr, "invalid port: %s\n", portString);
		return 1;
	}

	sockaddr_storage target;
	socklen_t targetLen = 0;
	if (!ResolveAddress(host, portString, family, &target, &targetLen))
		return 1;

	SOCKET socketFd = socket(family, SOCK_DGRAM, 0);
	if (socketFd == INVALID_SOCKET)
	{
#ifdef _WIN32
		std::fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
#else
		std::fprintf(stderr, "socket failed: errno=%d (%s)\n", errno, std::strerror(errno));
#endif
		return 1;
	}

#ifdef _WIN32
	DWORD timeout = 2000;
	setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
	timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif

	if (!SendRequest(socketFd, target, targetLen, 0))
	{
		closesocket(socketFd);
		return 1;
	}

	unsigned char response[128];
	int received = recvfrom(socketFd, reinterpret_cast<char*>(response), sizeof(response), 0, 0, 0);
	if (received == SOCKET_ERROR)
	{
#ifdef _WIN32
		std::fprintf(stderr, "recvfrom failed: %d\n", WSAGetLastError());
#else
		std::fprintf(stderr, "recvfrom failed: errno=%d (%s)\n", errno, std::strerror(errno));
#endif
		closesocket(socketFd);
		return 1;
	}

	unsigned char packetId = response[0];
	if (packetId == ID_OPEN_CONNECTION_COOKIE && received >= 3)
	{
		const unsigned short cookie = (unsigned short)(response[1] | (response[2] << 8));
		const unsigned short cookieXor = cookie ^ SAMP_PETARDED;
		std::printf("received %d bytes\n", received);
		std::printf("packet_id=%u\n", packetId);
		std::printf("result=open_connection_cookie\n");

		if (!SendRequest(socketFd, target, targetLen, cookieXor))
		{
			closesocket(socketFd);
			return 1;
		}

		received = recvfrom(socketFd, reinterpret_cast<char*>(response), sizeof(response), 0, 0, 0);
		if (received == SOCKET_ERROR)
		{
#ifdef _WIN32
			std::fprintf(stderr, "recvfrom failed after cookie exchange: %d\n", WSAGetLastError());
#else
			std::fprintf(stderr, "recvfrom failed after cookie exchange: errno=%d (%s)\n", errno, std::strerror(errno));
#endif
			closesocket(socketFd);
			return 1;
		}

		packetId = response[0];
	}

	std::printf("received %d bytes\n", received);
	std::printf("packet_id=%u\n", packetId);

	switch (packetId)
	{
		case ID_OPEN_CONNECTION_REPLY:
			std::printf("result=open_connection_reply\n");
			break;
		case ID_CONNECTION_ATTEMPT_FAILED:
			std::printf("result=connection_attempt_failed\n");
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			std::printf("result=no_free_incoming_connections\n");
			break;
		default:
			std::printf("result=unexpected\n");
			closesocket(socketFd);
#ifdef _WIN32
			WSACleanup();
#endif
			return 2;
	}

	for (int i = 0; i < received; ++i)
	{
		std::printf("%02x", response[i]);
		if (i + 1 < received)
			std::printf(" ");
	}
	std::printf("\n");

	closesocket(socketFd);
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}
