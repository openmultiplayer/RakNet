/**
* @file
* @brief SocketLayer class implementation 
*
 * This file is part of RakNet Copyright 2003 Rakkarsoft LLC and Kevin Jenkins.
 *
 * Usage of Raknet is subject to the appropriate licence agreement.
 * "Shareware" Licensees with Rakkarsoft LLC are subject to the
 * shareware license found at
 * http://www.rakkarsoft.com/shareWareLicense.html which you agreed to
 * upon purchase of a "Shareware license" "Commercial" Licensees with
 * Rakkarsoft LLC are subject to the commercial license found at
 * http://www.rakkarsoft.com/sourceCodeLicense.html which you agreed
 * to upon purchase of a "Commercial license"
 * Custom license users are subject to the terms therein.
 * All other users are
 * subject to the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Refer to the appropriate license agreement for distribution,
 * modification, and warranty rights.
*/
#include "SocketLayer.h"
#include <assert.h>
#include <stdio.h>
#include "MTUSize.h"
#include "RakAssert.h"
#include "PacketEnumerations.h"
#include "RakPeer.h"

#include "../../SAMPRakNet.hpp"

#ifdef _WIN32
#include <process.h>
#define COMPATIBILITY_2_RECV_FROM_FLAGS 0
typedef int socklen_t;
#elif defined(_COMPATIBILITY_2)
#include "Compatibility2Includes.h"
#else
#define COMPATIBILITY_2_RECV_FROM_FLAGS 0
#define closesocket close
#include <string.h> // memcpy
#include <unistd.h>
#include <fcntl.h>
#endif

using namespace RakNet;

#ifdef _MSC_VER
#pragma warning( push )
#endif

bool SocketLayer::socketLayerStarted = false;
#ifdef _WIN32
WSADATA SocketLayer::winsockInfo;
#endif
SocketLayer *SocketLayer::_instance = nullptr;

namespace RakNet
{
	#ifdef _WIN32
	extern void __stdcall ProcessNetworkPacket( const unsigned int binaryAddress, const unsigned short port, const char *data, const int length, RakPeer *rakPeer );
	extern void __stdcall ProcessNetworkPacket( const TransportAddress &transportAddress, const char *data, const int length, RakPeer *rakPeer );
	#else
	extern void ProcessNetworkPacket( const unsigned int binaryAddress, const unsigned short port, const char *data, const int length, RakPeer *rakPeer );
	extern void ProcessNetworkPacket( const TransportAddress &transportAddress, const char *data, const int length, RakPeer *rakPeer );
	#endif

	#ifdef _WIN32
	extern void __stdcall ProcessPortUnreachable( const unsigned int binaryAddress, const unsigned short port, RakPeer *rakPeer );
	#else
	extern void ProcessPortUnreachable( const unsigned int binaryAddress, const unsigned short port, RakPeer *rakPeer );
	#endif
}

#ifdef _DEBUG
#include <stdio.h>
#endif

SocketLayer::SocketLayer()
{
	if ( socketLayerStarted == false )
	{
#ifdef _WIN32

		if ( WSAStartup( MAKEWORD( 2, 2 ), &winsockInfo ) != 0 )
		{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
			DWORD dwIOError = GetLastError();
			LPVOID messageBuffer;
			FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
				( LPTSTR ) & messageBuffer, 0, NULL );
			// something has gone wrong here...
			printf( "WSAStartup failed:Error code - %lu\n%s", dwIOError, (char *)messageBuffer );
			//Free the buffer.
			LocalFree( messageBuffer );
#endif
		}

#endif
		socketLayerStarted = true;
	}
}

SocketLayer::~SocketLayer()
{
	if ( socketLayerStarted == true )
	{
#ifdef _WIN32
		WSACleanup();
#endif

		socketLayerStarted = false;
	}
}

TransportAddress::TransportAddress()
{
	addressFamily = AF_UNSPEC;
	port = 0;
	scopeId = 0;
	memset(address, 0, sizeof(address));
}

bool TransportAddress::IsValid(void) const
{
	return addressFamily == AF_INET || addressFamily == AF_INET6;
}

bool TransportAddress::IsIPv4(void) const
{
	return addressFamily == AF_INET;
}

bool TransportAddress::IsIPv6(void) const
{
	return addressFamily == AF_INET6;
}

bool TransportAddress::operator==(const TransportAddress &right) const
{
	if (addressFamily != right.addressFamily || port != right.port || scopeId != right.scopeId)
		return false;

	if (IsIPv4())
		return memcmp(address, right.address, 4) == 0;
	if (IsIPv6())
		return memcmp(address, right.address, 16) == 0;
	return IsValid() == false && right.IsValid() == false;
}

bool TransportAddress::operator!=(const TransportAddress &right) const
{
	return (*this == right) == false;
}

unsigned int TransportAddress::ToIPv4Binary(void) const
{
	unsigned int binaryAddress;

	if (IsIPv4() == false)
		return 0;

	memcpy(&binaryAddress, address, sizeof(binaryAddress));
	return binaryAddress;
}

bool TransportAddress::ToSockaddr(sockaddr_storage *storage, socklen_t *len) const
{
	if (storage == 0 || len == 0 || IsValid() == false)
		return false;

	memset(storage, 0, sizeof(sockaddr_storage));
	if (addressFamily == AF_INET)
	{
		sockaddr_in *ipv4 = reinterpret_cast<sockaddr_in *>(storage);
		ipv4->sin_family = AF_INET;
		ipv4->sin_port = htons(port);
		memcpy(&ipv4->sin_addr.s_addr, address, sizeof(ipv4->sin_addr.s_addr));
		*len = sizeof(sockaddr_in);
		return true;
	}

	sockaddr_in6 *ipv6 = reinterpret_cast<sockaddr_in6 *>(storage);
	ipv6->sin6_family = AF_INET6;
	ipv6->sin6_port = htons(port);
	ipv6->sin6_scope_id = scopeId;
	memcpy(&ipv6->sin6_addr, address, sizeof(ipv6->sin6_addr));
	*len = sizeof(sockaddr_in6);
	return true;
}

TransportAddress TransportAddress::FromSockaddr(const sockaddr *sa, socklen_t len)
{
	TransportAddress output;

	if (sa == 0)
		return output;

	if (sa->sa_family == AF_INET && len >= (socklen_t) sizeof(sockaddr_in))
	{
		const sockaddr_in *ipv4 = reinterpret_cast<const sockaddr_in *>(sa);
		output.addressFamily = AF_INET;
		output.port = ntohs(ipv4->sin_port);
		memcpy(output.address, &ipv4->sin_addr.s_addr, sizeof(ipv4->sin_addr.s_addr));
	}
	else if (sa->sa_family == AF_INET6 && len >= (socklen_t) sizeof(sockaddr_in6))
	{
		const sockaddr_in6 *ipv6 = reinterpret_cast<const sockaddr_in6 *>(sa);
		output.addressFamily = AF_INET6;
		output.port = ntohs(ipv6->sin6_port);
		output.scopeId = ipv6->sin6_scope_id;
		memcpy(output.address, &ipv6->sin6_addr, sizeof(ipv6->sin6_addr));
	}

	return output;
}

SOCKET SocketLayer::Connect( SOCKET writeSocket, unsigned int binaryAddress, unsigned short port )
{
	RakAssert( writeSocket != INVALID_SOCKET );
	sockaddr_in connectSocketAddress;

	connectSocketAddress.sin_family = AF_INET;
	connectSocketAddress.sin_port = htons( port );
	connectSocketAddress.sin_addr.s_addr = binaryAddress;

	if ( connect( writeSocket, ( struct sockaddr * ) & connectSocketAddress, sizeof( struct sockaddr ) ) != 0 )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) &messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "WSAConnect failed:Error code - %lu\n%s", dwIOError, (char *)messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif
	}

	return writeSocket;
}

SOCKET SocketLayer::Connect( SOCKET writeSocket, const TransportAddress &address )
{
	sockaddr_storage connectSocketAddress;
	socklen_t connectSocketAddressLen;

	RakAssert( writeSocket != INVALID_SOCKET );
	if (address.ToSockaddr(&connectSocketAddress, &connectSocketAddressLen) == false)
		return writeSocket;

	if ( connect( writeSocket, reinterpret_cast<struct sockaddr *>(&connectSocketAddress), connectSocketAddressLen ) != 0 )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
			( LPTSTR ) &messageBuffer, 0, NULL );
		printf( "WSAConnect failed:Error code - %lu\n%s", dwIOError, (char *)messageBuffer );
		LocalFree( messageBuffer );
#endif
	}

	return writeSocket;
}

#ifdef _MSC_VER
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
#endif
SOCKET SocketLayer::CreateBoundSocket( unsigned short port, bool blockingSocket, const char *forceHostAddress )
{
	return CreateBoundSocket(SocketBindParameters(port, blockingSocket, AF_INET, false, forceHostAddress));
}

SOCKET SocketLayer::CreateBoundSocket( const SocketBindParameters &parameters )
{
	SOCKET listenSocket;
	int ret;

#ifdef __USE_IO_COMPLETION_PORTS

	if ( parameters.blockingSocket == false ) 
		listenSocket = WSASocket( parameters.addressFamily, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED );
	else
#endif

		listenSocket = socket( parameters.addressFamily, SOCK_DGRAM, 0 );

	if ( listenSocket == INVALID_SOCKET )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "socket(...) failed:Error code - %lu\n%s", dwIOError, (char *)messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif

		return INVALID_SOCKET;
	}

	int sock_opt = 1;

#if defined(_WIN32)
    if (setsockopt(listenSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&sock_opt, sizeof(sock_opt)) == -1)
#else
	if (setsockopt( listenSocket, SOL_SOCKET, SO_REUSEADDR, ( char * ) & sock_opt, sizeof ( sock_opt ) ) == -1)
#endif
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "setsockopt(SO_EXCLUSIVEADDRUSE) failed:Error code - %lu\n%s", dwIOError, (char *)messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif
	}

	if (parameters.addressFamily == AF_INET6)
	{
		int ipv6Only = parameters.ipv6Only ? 1 : 0;
		setsockopt(listenSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &ipv6Only, sizeof(ipv6Only));
	}

	// This doubles the max throughput rate
	sock_opt=1024*256;
	setsockopt(listenSocket, SOL_SOCKET, SO_RCVBUF, ( char * ) & sock_opt, sizeof ( sock_opt ) );
	
	// This doesn't make much difference: 10% maybe
	sock_opt=1024*128;
	setsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, ( char * ) & sock_opt, sizeof ( sock_opt ) );

	#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
	// If this assert hit you improperly linked against WSock32.h
	RakAssert(IP_DONTFRAGMENT==14);
	#endif

	// TODO - I need someone on dialup to test this with :(
	// Path MTU Detection
	/*
	if ( setsockopt( listenSocket, IPPROTO_IP, IP_DONTFRAGMENT, ( char * ) & sock_opt, sizeof ( sock_opt ) ) == -1 )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "setsockopt(IP_DONTFRAGMENT) failed:Error code - %d\n%s", dwIOError, (char *)messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif
	}
	*/

#ifndef _COMPATIBILITY_2
	//Set non-blocking
#ifdef _WIN32
	unsigned long nonblocking = 1;
// http://www.die.net/doc/linux/man/man7/ip.7.html
	if ( ioctlsocket( listenSocket, FIONBIO, &nonblocking ) != 0 )
	{
		RakAssert( 0 );
		return INVALID_SOCKET;
	}
#else
	if ( fcntl( listenSocket, F_SETFL, O_NONBLOCK ) != 0 )
	{
		RakAssert( 0 );
		return INVALID_SOCKET;
	}
#endif
#endif

	// Set broadcast capable
	if ( setsockopt( listenSocket, SOL_SOCKET, SO_BROADCAST, ( char * ) & sock_opt, sizeof( sock_opt ) ) == -1 )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "setsockopt(SO_BROADCAST) failed:Error code - %lu\n%s", dwIOError, (char *)messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif

	}

	TransportAddress bindAddress;
	sockaddr_storage listenerSocketAddress;
	socklen_t listenerSocketAddressLen;

	bindAddress.addressFamily = (unsigned short) parameters.addressFamily;
	bindAddress.port = parameters.port;
	if (parameters.addressFamily == AF_INET)
	{
		unsigned int anyAddress = INADDR_ANY;
		memcpy(bindAddress.address, &anyAddress, sizeof(anyAddress));
	}

	if (parameters.forceHostAddress && parameters.forceHostAddress[0])
	{
		if (DomainNameToAddress(parameters.forceHostAddress, parameters.port, parameters.addressFamily, &bindAddress) == false)
		{
			closesocket(listenSocket);
			return INVALID_SOCKET;
		}
	}

	if (bindAddress.ToSockaddr(&listenerSocketAddress, &listenerSocketAddressLen) == false)
	{
		closesocket(listenSocket);
		return INVALID_SOCKET;
	}

	ret = bind( listenSocket, reinterpret_cast<struct sockaddr *>(&listenerSocketAddress), listenerSocketAddressLen );

	if ( ret == SOCKET_ERROR )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "bind(...) failed:Error code - %lu\n%s", dwIOError, (char *)messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif

		return INVALID_SOCKET;
	}

	return listenSocket;
}

#if !defined(_COMPATIBILITY_1) && !defined(_COMPATIBILITY_2)
const char* SocketLayer::DomainNameToIP( const char *domainName )
{
	static char ipString[INET_ADDRSTRLEN];
	TransportAddress address;

	if (DomainNameToAddress(domainName, 0, AF_INET, &address) == false)
		return 0;

	if (inet_ntop(AF_INET, address.address, ipString, sizeof(ipString)) == 0)
		return 0;

	return ipString;
}

bool SocketLayer::DomainNameToAddress( const char *domainName, unsigned short port, int addressFamily, TransportAddress *output )
{
	addrinfo hints;
	addrinfo *result;
	addrinfo *it;
	char service[16];

	if (domainName == 0 || output == 0)
		return false;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_family = addressFamily;
	hints.ai_flags = AI_ADDRCONFIG;
#ifdef _WIN32
	_snprintf(service, sizeof(service), "%u", port);
#else
	snprintf(service, sizeof(service), "%u", port);
#endif

	result = 0;
	if (getaddrinfo(domainName, service, &hints, &result) != 0)
		return false;

	for (it = result; it; it = it->ai_next)
	{
		*output = TransportAddress::FromSockaddr(it->ai_addr, (socklen_t) it->ai_addrlen);
		if (output->IsValid())
		{
			freeaddrinfo(result);
			return true;
		}
	}

	freeaddrinfo(result);
	return false;
}
#endif

void SocketLayer::Write( const SOCKET writeSocket, const char* data, const int length )
{

	RakAssert( writeSocket != INVALID_SOCKET );

	send( writeSocket, data, length, 0 );
}

// Start an asynchronous read using the specified socket.
#ifdef _MSC_VER
#pragma warning( disable : 4100 ) // warning C4100: <variable name> : unreferenced formal parameter
#endif
bool SocketLayer::AssociateSocketWithCompletionPortAndRead( SOCKET readSocket, unsigned int binaryAddress, unsigned short port, RakPeer *rakPeer )
{
	return true;
}

int SocketLayer::RecvFrom( const SOCKET s, RakPeer *rakPeer, int *errorCode )
{
	return RecvFrom(s, rakPeer, errorCode, 0);
}

int SocketLayer::RecvFrom( const SOCKET s, RakPeer *rakPeer, int *errorCode, TransportAddress *sender )
{
	int len;
	char data[ MAXIMUM_MTU_SIZE ];
	sockaddr_storage sa;
	socklen_t len2 = sizeof( sa );
	memset(&sa, 0, sizeof(sa));

#ifdef _DEBUG
	data[ 0 ] = 0;
	len = 0;
#endif

	if ( s == INVALID_SOCKET )
	{
		*errorCode = SOCKET_ERROR;
		return SOCKET_ERROR;
	}

	len = recvfrom( s, data, MAXIMUM_MTU_SIZE, COMPATIBILITY_2_RECV_FROM_FLAGS, reinterpret_cast<sockaddr*>(&sa), &len2 );

	// if (len>0)
	//  printf("Got packet on port %i\n",ntohs(sa.sin_port));

	if (len < 1 && len != -1) 
	{
        return 1;
	}

	if ( len != SOCKET_ERROR )
	{
		TransportAddress packetSender = TransportAddress::FromSockaddr(reinterpret_cast<sockaddr*>(&sa), len2);
		if (sender)
			*sender = packetSender;

		if ((len > 10 && data[0] == 'S' && data[1] == 'A' && data[2] == 'M' && data[3] == 'P' && data[4] != '6') ||
			(len > 23 && data[0] == 'S' && data[1] == 'A' && data[2] == 'M' && data[3] == 'P' && data[4] == '6'))
		{
			SAMPRakNet::HandleQuery(s, len2, sa, data, len);
			return 1;
		}

		uint8_t* decrypted = SAMPRakNet::Decrypt((uint8_t*)data, len);
		if (decrypted) {
			ProcessNetworkPacket(packetSender, (char*)decrypted, len - 1, rakPeer);
		}
#ifdef _DEBUG
		else {
			if (packetSender.IsIPv4())
			{
				uint8_t* const addr = reinterpret_cast<uint8_t*>(packetSender.address);
				SAMPRakNet::GetCore()->printLn("Dropping bad packet from %u.%u.%u.%u:%u!", addr[0], addr[1], addr[2], addr[3], packetSender.port);
			}
			else if (packetSender.IsIPv6())
			{
				char ipString[INET6_ADDRSTRLEN];
				if (inet_ntop(AF_INET6, packetSender.address, ipString, sizeof(ipString)))
					SAMPRakNet::GetCore()->printLn("Dropping bad packet from [%s]:%u!", ipString, packetSender.port);
			}
		}
#endif
		return 1;
	}
	else
	{
		*errorCode = 0;

#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)

		DWORD dwIOError = WSAGetLastError();

		if ( dwIOError == WSAEWOULDBLOCK )
		{
			return SOCKET_ERROR;
		}
		if ( dwIOError == WSAECONNRESET )
		{
#if defined(_DEBUG)
//			printf( "A previous send operation resulted in an ICMP Port Unreachable message.\n" );
#endif


			unsigned short portnum=0;
			ProcessPortUnreachable(0, portnum, rakPeer);
			// *errorCode = dwIOError;
			return SOCKET_ERROR;
		}
		else
		{
#if defined(_DEBUG)
			if ( dwIOError != WSAEINTR )
			{
				LPVOID messageBuffer;
				FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
					( LPTSTR ) & messageBuffer, 0, NULL );
				// something has gone wrong here...
				printf( "recvfrom failed:Error code - %lu\n%s", dwIOError, (char *)messageBuffer );

				//Free the buffer.
				LocalFree( messageBuffer );
			}
#endif
		}
#endif
	}

	return 0; // no data
}

#ifdef _MSC_VER
#pragma warning( disable : 4702 ) // warning C4702: unreachable code
#endif
int SocketLayer::SendTo( SOCKET s, const char *data, int length, unsigned int binaryAddress, unsigned short port )
{
	TransportAddress address;
	address.addressFamily = AF_INET;
	address.port = port;
	memcpy(address.address, &binaryAddress, sizeof(binaryAddress));
	return SendTo(s, data, length, address);
}

int SocketLayer::SendTo( SOCKET s, const char *data, int length, const TransportAddress &address )
{
	if ( s == INVALID_SOCKET )
	{
		return -1;
	}

	int len;
	sockaddr_storage sa;
	socklen_t saLen;
	if (address.ToSockaddr(&sa, &saLen) == false)
		return -1;

	do
	{
		// TODO - use WSASendTo which is faster.
		auto encrypted = (uint8_t*)data;
		if (address.IsIPv4() && SAMPRakNet::IsOmpEncryptionEnabled())
		{
			auto encryptionData = SAMPRakNet::GetOmpPlayerEncryptionData(PlayerID { address.ToIPv4Binary(), address.port });
			if (encryptionData)
			{
				encrypted = SAMPRakNet::Encrypt(encryptionData, (uint8_t*)data, length);
				len = sendto(s, (char*)encrypted, length + 1, 0, reinterpret_cast<const sockaddr*>(&sa), saLen);
			}
			else
			{
				len = sendto(s, (char*)encrypted, length, 0, reinterpret_cast<const sockaddr*>(&sa), saLen);
			}
		}
		else
		{
			len = sendto(s, (char*)encrypted, length, 0, reinterpret_cast<const sockaddr*>(&sa), saLen);
		}
	}
	while ( len == 0 );

	if ( len != SOCKET_ERROR )
		return 0;

#if defined(_WIN32)

	DWORD dwIOError = WSAGetLastError();

	if ( dwIOError == WSAECONNRESET )
	{
#if defined(_DEBUG)
		printf( "A previous send operation resulted in an ICMP Port Unreachable message.\n" );
#endif

	}
	else if ( dwIOError != WSAEWOULDBLOCK )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "sendto failed:Error code - %lu\n%s", dwIOError, (char *)messageBuffer );

		//Free the buffer.
		LocalFree( messageBuffer );
#endif

	}

	return dwIOError;
#endif

	return 1; // error
}

int SocketLayer::SendTo( SOCKET s, const char *data, int length, char ip[ 16 ], unsigned short port )
{
	TransportAddress address;
	if (DomainNameToAddress(ip, port, AF_UNSPEC, &address) == false)
		return 1;

	return SendTo( s, data, length, address );
}

#if !defined(_COMPATIBILITY_1) && !defined(_COMPATIBILITY_2)
void SocketLayer::GetMyIP( char ipList[ 10 ][ 16 ] )
{
	char ac[ 80 ];

	if ( gethostname( ac, sizeof( ac ) ) == SOCKET_ERROR )
	{
	#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "gethostname failed:Error code - %lu\n%s", dwIOError, (char *)messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
	#endif

		return ;
	}

	struct hostent *phe = gethostbyname( ac );

	if ( phe == 0 )
	{
#if defined(_WIN32) && !defined(_COMPATIBILITY_1) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		printf( "gethostbyname failed:Error code - %lu\n%s", dwIOError, (char *)messageBuffer );

		//Free the buffer.
		LocalFree( messageBuffer );
#endif

		return ;
	}

	for ( int i = 0; phe->h_addr_list[ i ] != 0 && i < 10; ++i )
	{

		struct in_addr addr;

		memcpy( &addr, phe->h_addr_list[ i ], sizeof( struct in_addr ) );
		//cout << "Address " << i << ": " << inet_ntoa(addr) << endl;
		strcpy( ipList[ i ], inet_ntoa( addr ) );
	}
}

unsigned SocketLayer::GetMyAddresses( TransportAddress *addresses, unsigned maxAddresses, int addressFamily )
{
	char hostname[80];
	addrinfo hints;
	addrinfo *result;
	addrinfo *it;
	unsigned count;

	if (addresses == 0 || maxAddresses == 0)
		return 0;

	if ( gethostname( hostname, sizeof( hostname ) ) == SOCKET_ERROR )
		return 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = addressFamily;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_ADDRCONFIG;

	result = 0;
	if (getaddrinfo(hostname, 0, &hints, &result) != 0)
		return 0;

	count = 0;
	for (it = result; it && count < maxAddresses; it = it->ai_next)
	{
		TransportAddress address = TransportAddress::FromSockaddr(it->ai_addr, (socklen_t) it->ai_addrlen);
		if (address.IsValid())
			addresses[count++] = address;
	}

	freeaddrinfo(result);
	return count;
}
#endif

unsigned short SocketLayer::GetLocalPort ( SOCKET s )
{
	sockaddr_storage sa;
	socklen_t len = sizeof(sa);
	if (getsockname(s, (sockaddr*)&sa, &len)!=0)
		return 0;
	return TransportAddress::FromSockaddr(reinterpret_cast<sockaddr*>(&sa), len).port;
}


#ifdef _MSC_VER
#pragma warning( pop )
#endif
