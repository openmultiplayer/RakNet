#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

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
		std::fprintf(stderr,
			"Usage:\n"
			"  udp_probe listen <family> <bind_host> <port> [timeout_ms]\n"
			"  udp_probe send <family> <host> <port> <message>\n"
			"  udp_probe selftest <family> <bind_host> <port> <message>\n"
			"  family: ipv4 | ipv6 | any\n");
	}

	int ParseFamily(const char *value)
	{
		if (std::strcmp(value, "ipv4") == 0)
			return AF_INET;
		if (std::strcmp(value, "ipv6") == 0)
			return AF_INET6;
		if (std::strcmp(value, "any") == 0)
			return AF_UNSPEC;
		return -1;
	}

	bool ResolveAddress(const char *host, const char *port, int family, addrinfo **result, int flags)
	{
		addrinfo hints;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = family;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_flags = flags;
		const int rc = getaddrinfo(host, port, &hints, result);
		if (rc != 0)
			std::fprintf(stderr, "getaddrinfo(%s,%s) failed: %s\n", host ? host : "<null>", port ? port : "<null>", gai_strerror(rc));
		return rc == 0;
	}

	std::string DescribeSockaddr(const sockaddr *sa, socklen_t len)
	{
		char host[INET6_ADDRSTRLEN];
		char service[32];
		if (getnameinfo(sa, len, host, sizeof(host), service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV) != 0)
			return "<unknown>";

		if (sa->sa_family == AF_INET6)
			return std::string("[") + host + "]:" + service;
		return std::string(host) + ":" + service;
	}

	int Listen(const char *familyValue, const char *bindHost, const char *port, int timeoutMs)
	{
		addrinfo *result;
		addrinfo *it;
		SOCKET socketFd;
		char buffer[2048];
		sockaddr_storage from;
		socklen_t fromLen;

		if (ResolveAddress(bindHost, port, ParseFamily(familyValue), &result, AI_PASSIVE) == false)
		{
			std::fprintf(stderr, "resolve failed for %s:%s\n", bindHost, port);
			return 1;
		}

		socketFd = INVALID_SOCKET;
		for (it = result; it; it = it->ai_next)
		{
			socketFd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
			if (socketFd == INVALID_SOCKET)
			{
#ifdef _WIN32
				std::fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
#else
				std::perror("socket");
#endif
				continue;
			}

			if (it->ai_family == AF_INET6)
			{
				int ipv6Only = 1;
				setsockopt(socketFd, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &ipv6Only, sizeof(ipv6Only));
			}

			if (bind(socketFd, it->ai_addr, (socklen_t) it->ai_addrlen) == 0)
				break;

			std::fprintf(stderr, "bind attempt failed for %s\n", DescribeSockaddr(it->ai_addr, (socklen_t) it->ai_addrlen).c_str());
#ifdef _WIN32
			std::fprintf(stderr, "winsock error: %d\n", WSAGetLastError());
#else
			std::perror("bind");
#endif
			closesocket(socketFd);
			socketFd = INVALID_SOCKET;
		}
		freeaddrinfo(result);

		if (socketFd == INVALID_SOCKET)
		{
			std::fprintf(stderr, "bind failed\n");
			return 1;
		}

#ifdef _WIN32
		DWORD timeout = timeoutMs;
		setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
#else
		timeval timeout;
		timeout.tv_sec = timeoutMs / 1000;
		timeout.tv_usec = (timeoutMs % 1000) * 1000;
		setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif

		fromLen = sizeof(from);
		const int received = recvfrom(socketFd, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr *>(&from), &fromLen);
		if (received == SOCKET_ERROR)
		{
			std::fprintf(stderr, "recvfrom failed or timed out\n");
			closesocket(socketFd);
			return 1;
		}

		buffer[received] = '\0';
		std::printf("received %d bytes from %s\n", received, DescribeSockaddr(reinterpret_cast<sockaddr *>(&from), fromLen).c_str());
		std::printf("%s\n", buffer);
		closesocket(socketFd);
		return 0;
	}

	int Send(const char *familyValue, const char *host, const char *port, const char *message)
	{
		addrinfo *result;
		addrinfo *it;
		SOCKET socketFd;

		if (ResolveAddress(host, port, ParseFamily(familyValue), &result, 0) == false)
		{
			std::fprintf(stderr, "resolve failed for %s:%s\n", host, port);
			return 1;
		}

		socketFd = INVALID_SOCKET;
		for (it = result; it; it = it->ai_next)
		{
			socketFd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
			if (socketFd == INVALID_SOCKET)
				continue;

			if (sendto(socketFd, message, (int) std::strlen(message), 0, it->ai_addr, (socklen_t) it->ai_addrlen) != SOCKET_ERROR)
				break;

			closesocket(socketFd);
			socketFd = INVALID_SOCKET;
		}
		freeaddrinfo(result);

		if (socketFd == INVALID_SOCKET)
		{
			std::fprintf(stderr, "send failed\n");
			return 1;
		}

		std::printf("sent message to %s:%s\n", host, port);
		closesocket(socketFd);
		return 0;
	}

	int SelfTest(const char *familyValue, const char *bindHost, const char *port, const char *message)
	{
		addrinfo *result;
		addrinfo *it;
		SOCKET listenSocket;
		SOCKET sendSocket;
		sockaddr_storage from;
		socklen_t fromLen;
		char buffer[2048];

		if (ResolveAddress(bindHost, port, ParseFamily(familyValue), &result, AI_PASSIVE) == false)
		{
			std::fprintf(stderr, "resolve failed for %s:%s\n", bindHost, port);
			return 1;
		}

		listenSocket = INVALID_SOCKET;
		for (it = result; it; it = it->ai_next)
		{
			listenSocket = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
			if (listenSocket == INVALID_SOCKET)
				continue;

			if (it->ai_family == AF_INET6)
			{
				int ipv6Only = 1;
				setsockopt(listenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &ipv6Only, sizeof(ipv6Only));
			}

			if (bind(listenSocket, it->ai_addr, (socklen_t) it->ai_addrlen) == 0)
				break;

			closesocket(listenSocket);
			listenSocket = INVALID_SOCKET;
		}

		if (listenSocket == INVALID_SOCKET)
		{
			freeaddrinfo(result);
			std::fprintf(stderr, "selftest bind failed\n");
			return 1;
		}

		sendSocket = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (sendSocket == INVALID_SOCKET)
		{
			freeaddrinfo(result);
			closesocket(listenSocket);
			std::fprintf(stderr, "selftest sender socket failed\n");
			return 1;
		}

		if (sendto(sendSocket, message, (int) std::strlen(message), 0, it->ai_addr, (socklen_t) it->ai_addrlen) == SOCKET_ERROR)
		{
			freeaddrinfo(result);
			closesocket(sendSocket);
			closesocket(listenSocket);
			std::fprintf(stderr, "selftest send failed\n");
			return 1;
		}

		freeaddrinfo(result);
		fromLen = sizeof(from);
		const int received = recvfrom(listenSocket, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr *>(&from), &fromLen);
		closesocket(sendSocket);
		closesocket(listenSocket);

		if (received == SOCKET_ERROR)
		{
			std::fprintf(stderr, "selftest receive failed\n");
			return 1;
		}

		buffer[received] = '\0';
		std::printf("selftest received %d bytes from %s\n", received, DescribeSockaddr(reinterpret_cast<sockaddr *>(&from), fromLen).c_str());
		std::printf("%s\n", buffer);
		return 0;
	}
}

int main(int argc, char **argv)
{
#ifdef _WIN32
	WSADATA winsockInfo;
	if (WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0)
		return 1;
#endif

	if (argc < 2)
	{
		PrintUsage();
		return 1;
	}

	const std::string mode = argv[1];
	int result = 1;
	if (mode == "listen")
	{
		if (argc < 5 || argc > 6)
		{
			PrintUsage();
			return 1;
		}

		const int timeoutMs = argc == 6 ? std::atoi(argv[5]) : 3000;
		result = Listen(argv[2], argv[3], argv[4], timeoutMs);
	}
	else if (mode == "send")
	{
		if (argc != 6)
		{
			PrintUsage();
			return 1;
		}

		result = Send(argv[2], argv[3], argv[4], argv[5]);
	}
	else if (mode == "selftest")
	{
		if (argc != 6)
		{
			PrintUsage();
			return 1;
		}

		result = SelfTest(argv[2], argv[3], argv[4], argv[5]);
	}
	else
	{
		PrintUsage();
	}

#ifdef _WIN32
	WSACleanup();
#endif
	return result;
}
