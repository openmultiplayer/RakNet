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
	void PrintUsage()
	{
		std::fprintf(stderr, "Usage: omp_query_probe <family> <host> <port> [opcode]\n");
		std::fprintf(stderr, "  family: ipv4 | ipv6\n");
		std::fprintf(stderr, "  opcode: i (server info, default), o, c, r, p\n");
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
		unsigned long parsed = std::strtoul(value, &end, 10);
		if (end == value || *end != '\0' || parsed > 65535)
			return 0;
		return (unsigned short)parsed;
	}
}

int main(int argc, char** argv)
{
#ifdef _WIN32
	WSADATA winsockInfo;
	if (WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0)
		return 1;
#endif

	if (argc < 4 || argc > 5)
	{
		PrintUsage();
		return 1;
	}

	const int family = ParseFamily(argv[1]);
	const char* host = argv[2];
	const char* portString = argv[3];
	const char opcode = argc == 5 ? argv[4][0] : 'i';
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

	unsigned char request[32];
	int requestLength;
	if (family == AF_INET6)
	{
		const sockaddr_in6* ipv6 = reinterpret_cast<const sockaddr_in6*>(&target);
		std::memcpy(request, "SAMP6", 5);
		std::memcpy(request + 5, &ipv6->sin6_addr, 16);
		request[21] = (unsigned char)(port & 0xFF);
		request[22] = (unsigned char)((port >> 8) & 0xFF);
		request[23] = (unsigned char)opcode;
		requestLength = 24;
	}
	else
	{
		const sockaddr_in* ipv4 = reinterpret_cast<const sockaddr_in*>(&target);
		std::memcpy(request, "SAMP", 4);
		std::memcpy(request + 4, &ipv4->sin_addr.s_addr, 4);
		request[8] = (unsigned char)(port & 0xFF);
		request[9] = (unsigned char)((port >> 8) & 0xFF);
		request[10] = (unsigned char)opcode;
		requestLength = 11;
	}

	if (opcode == 'p')
	{
		request[requestLength + 0] = 0x78;
		request[requestLength + 1] = 0x56;
		request[requestLength + 2] = 0x34;
		request[requestLength + 3] = 0x12;
		requestLength += 4;
	}

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

	if (sendto(socketFd, reinterpret_cast<const char*>(request), requestLength, 0, reinterpret_cast<const sockaddr*>(&target), targetLen) == SOCKET_ERROR)
	{
#ifdef _WIN32
		std::fprintf(stderr, "sendto failed: %d\n", WSAGetLastError());
#else
		std::fprintf(stderr, "sendto failed: errno=%d (%s)\n", errno, std::strerror(errno));
#endif
		closesocket(socketFd);
		return 1;
	}

	unsigned char response[2048];
	const int received = recvfrom(socketFd, reinterpret_cast<char*>(response), sizeof(response), 0, 0, 0);
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

	std::printf("received %d bytes for opcode %c\n", received, opcode);
	if (received >= 24 && std::memcmp(response, "SAMP6", 5) == 0)
	{
		std::printf("magic=%.5s opcode=%c\n", response, response[23]);
	}
	else if (received >= 11)
	{
		std::printf("magic=%.4s opcode=%c\n", response, response[10]);
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
