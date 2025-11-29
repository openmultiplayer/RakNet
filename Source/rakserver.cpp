/// \file
///
/// This file is part of RakNet Copyright 2003 Kevin Jenkins.
///
/// Usage of RakNet is subject to the appropriate license agreement.
/// Creative Commons Licensees are subject to the
/// license found at
/// http://creativecommons.org/licenses/by-nc/2.5/
/// Single application licensees are subject to the license found at
/// http://www.rakkarsoft.com/SingleApplicationLicense.html
/// Custom license users are subject to the terms therein.
/// GPL license users are subject to the GNU General Public
/// License as published by the Free
/// Software Foundation; either version 2 of the License, or (at your
/// option) any later version.

#include "RakServer.h"
#include "PacketEnumerations.h"
#include "GetTime.h"
#include "Rand.h"

using namespace RakNet;

#ifdef _MSC_VER
#pragma warning( push )
#endif

RakServer::RakServer()
{
	nextSeedUpdate = 0;
	synchronizedRandomInteger = false;
	relayStaticClientData = false;
	broadcastPingsTime = 0;
}

RakServer::~RakServer()
{}

void RakServer::InitializeSecurity( const char *pubKeyE, const char *pubKeyN )
{
	RakPeer::InitializeSecurity( 0, 0, pubKeyE, pubKeyN );
}

void RakServer::DisableSecurity( void )
{
	RakPeer::DisableSecurity();
}

#ifdef _MSC_VER
#pragma warning( disable : 4100 ) // warning C4100: 'depreciated' : unreferenced formal parameter
#endif
bool RakServer::Start( unsigned short AllowedPlayers, unsigned int depreciated, int threadSleepTimer, unsigned short port, const char *forceHostAddress )
{
	bool init;

	RakPeer::Disconnect( 30 );

	init = RakPeer::Initialize( AllowedPlayers, port, threadSleepTimer, forceHostAddress );
	RakPeer::SetMaximumIncomingConnections( AllowedPlayers );

	// Random number seed
	RakNetTime time = RakNet::GetTime();
	seedMT( (unsigned int) time );
	seed = randomMT();

	if ( seed % 2 == 0 )   // Even
		seed--; // make odd

	nextSeed = randomMT();

	if ( nextSeed % 2 == 0 )   // Even
		nextSeed--; // make odd

	return init;
}

void RakServer::SetPassword( const char *_password )
{
	if ( _password )
	{
		RakPeer::SetIncomingPassword( _password, ( int ) strlen( _password ) + 1 );
	}

	else
	{
		RakPeer::SetIncomingPassword( 0, 0 );
	}
}

bool RakServer::HasPassword( void )
{
	int passwordLength;
	GetIncomingPassword(0, &passwordLength);
	return passwordLength > 0;
}

void RakServer::Disconnect( unsigned int blockDuration, unsigned char orderingChannel )
{
	RakPeer::Disconnect( blockDuration, orderingChannel );
}

bool RakServer::Send( const char *data, const int length, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast )
{
	return RakPeer::Send( data, length, priority, reliability, orderingChannel, playerId, broadcast );
}

bool RakServer::Send( RakNet::BitStream const *bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast )
{
	return RakPeer::Send( bitStream, priority, reliability, orderingChannel, playerId, broadcast );
}

Packet* RakServer::Receive( void )
{
	Packet * packet = RakPeer::Receive();

	// This is just a regular time based update.  Nowhere else good to put it
	if ( RakPeer::IsActive() && synchronizedRandomInteger )
	{
		RakNetTime time = RakNet::GetTime();

		if ( time > nextSeedUpdate || ( packet && packet->data[ 0 ] == ID_NEW_INCOMING_CONNECTION ) )
		{
			if ( time > nextSeedUpdate )
				nextSeedUpdate = time + 9000; // Seeds are updated every 9 seconds

			seed = nextSeed;

			nextSeed = randomMT();

			if ( nextSeed % 2 == 0 )   // Even
				nextSeed--; // make odd

			/*		SetRandomNumberSeedStruct s;

			s.ts = ID_TIMESTAMP;
			s.timeStamp = RakNet::GetTime();
			s.typeId = ID_SET_RANDOM_NUMBER_SEED;
			s.seed = seed;
			s.nextSeed = nextSeed;
			RakNet::BitStream s_BitS( SetRandomNumberSeedStruct_Size );
			s.Serialize( s_BitS );
			*/

			RakNet::BitStream outBitStream(sizeof(unsigned char)+sizeof(unsigned int)+sizeof(unsigned char)+sizeof(unsigned int)+sizeof(unsigned int));
			outBitStream.Write((unsigned char) ID_TIMESTAMP);
			outBitStream.Write((unsigned int) RakNet::GetTime());
			outBitStream.Write((unsigned char) ID_SET_RANDOM_NUMBER_SEED);
			outBitStream.Write(seed);
			outBitStream.Write(nextSeed);

			if ( packet && packet->data[ 0 ] == ID_NEW_INCOMING_CONNECTION )
				Send( &outBitStream, SYSTEM_PRIORITY, RELIABLE, 0, packet->playerId, false );
			else
				Send( &outBitStream, SYSTEM_PRIORITY, RELIABLE, 0, UNASSIGNED_PLAYER_ID, true );
		}
	}

	return packet;
}

void RakServer::Kick(const PlayerID playerId)
{
    RakPeer::NotifyAndFlagForDisconnect(playerId, false, 0);
}

void RakServer::DeallocatePacket( Packet *packet )
{
	RakPeer::DeallocatePacket( packet );
}

void RakServer::SetAllowedPlayers( unsigned short AllowedPlayers )
{
	RakPeer::SetMaximumIncomingConnections( AllowedPlayers );
}

unsigned short RakServer::GetAllowedPlayers( void ) const
{
	return RakPeer::GetMaximumIncomingConnections();
}

unsigned short RakServer::GetConnectedPlayers( void )
{
	unsigned short numberOfSystems;

	RakPeer::GetConnectionList( 0, &numberOfSystems );
	return numberOfSystems;
}

void RakServer::GetPlayerIPFromID( const PlayerID playerId, char returnValue[ 22 ], unsigned short *port )
{
	*port = playerId.port;
	strcpy( returnValue, RakPeer::PlayerIDToDottedIP( playerId ) );
}

void RakServer::PingPlayer( const PlayerID playerId )
{
	RakPeer::Ping( playerId );
}

int RakServer::GetAveragePing( const PlayerID playerId )
{
	return RakPeer::GetAveragePing( playerId );
}

int RakServer::GetLastPing( const PlayerID playerId )
{
	return RakPeer::GetLastPing( playerId );
}

int RakServer::GetLowestPing( const PlayerID playerId )
{
	return RakPeer::GetLowestPing( playerId );
}

void RakServer::StartOccasionalPing( void )
{
	RakPeer::SetOccasionalPing( true );
}

void RakServer::StopOccasionalPing( void )
{
	RakPeer::SetOccasionalPing( false );
}

bool RakServer::IsActive( void ) const
{
	return RakPeer::IsActive();
}

unsigned int RakServer::GetSynchronizedRandomInteger( void ) const
{
	return seed;
}

void RakServer::StartSynchronizedRandomInteger( void )
{
	synchronizedRandomInteger = true;
}

void RakServer::StopSynchronizedRandomInteger( void )
{
	synchronizedRandomInteger = false;
}

bool RakServer::GenerateCompressionLayer( unsigned int inputFrequencyTable[ 256 ], bool inputLayer )
{
	return RakPeer::GenerateCompressionLayer( inputFrequencyTable, inputLayer );
}

bool RakServer::DeleteCompressionLayer( bool inputLayer )
{
	return RakPeer::DeleteCompressionLayer( inputLayer );
}

void RakServer::RegisterAsRemoteProcedureCall( RPCID  uniqueID, void ( *functionPointer ) ( RPCParameters *rpcParms, void* extra ), void* extra)
{
	RakPeer::RegisterAsRemoteProcedureCall( uniqueID, functionPointer, extra );
}

void RakServer::RegisterClassMemberRPC( RPCID  uniqueID, void *functionPointer )
{
	RakPeer::RegisterClassMemberRPC( uniqueID, functionPointer );
}

void RakServer::UnregisterAsRemoteProcedureCall( RPCID  uniqueID )
{
	RakPeer::UnregisterAsRemoteProcedureCall( uniqueID );
}

bool RakServer::RPC( RPCID  uniqueID, const char *data, unsigned int bitLength, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, bool shiftTimestamp, NetworkID networkID, RakNet::BitStream *replyFromTarget )
{
	return RakPeer::RPC( uniqueID, data, bitLength, priority, reliability, orderingChannel, playerId, broadcast, shiftTimestamp, networkID, replyFromTarget );
}

bool RakServer::RPC( RPCID  uniqueID, RakNet::BitStream const *parameters, PacketPriority priority, PacketReliability reliability, char orderingChannel, PlayerID playerId, bool broadcast, bool shiftTimestamp, NetworkID networkID, RakNet::BitStream *replyFromTarget )
{
	return RakPeer::RPC( uniqueID, parameters, priority, reliability, orderingChannel, playerId, broadcast, shiftTimestamp, networkID, replyFromTarget );
}

void RakServer::SetTrackFrequencyTable( bool b )
{
	RakPeer::SetCompileFrequencyTable( b );
}

bool RakServer::GetSendFrequencyTable( unsigned int outputFrequencyTable[ 256 ] )
{
	return RakPeer::GetOutgoingFrequencyTable( outputFrequencyTable );
}

float RakServer::GetCompressionRatio( void ) const
{
	return RakPeer::GetCompressionRatio();
}

float RakServer::GetDecompressionRatio( void ) const
{
	return RakPeer::GetDecompressionRatio();
}

void RakServer::AttachPlugin( PluginInterface *messageHandler )
{
	RakPeer::AttachPlugin(messageHandler);
}

void RakServer::DetachPlugin( PluginInterface *messageHandler )
{
	RakPeer::DetachPlugin(messageHandler);
}

RakNet::BitStream * RakServer::GetStaticServerData( void )
{
	return RakPeer::GetRemoteStaticData( myPlayerId );
}

void RakServer::SetStaticServerData( const char *data, const int length )
{
	RakPeer::SetRemoteStaticData( myPlayerId, data, length );
}

void RakServer::SetRelayStaticClientData( bool b )
{
	relayStaticClientData = b;
}

void RakServer::SendStaticServerDataToClient( const PlayerID playerId )
{
	RakPeer::SendStaticData( playerId );
}

void RakServer::SetOfflinePingResponse( const char *data, const unsigned int length )
{
	RakPeer::SetOfflinePingResponse( data, length );
}

RakNet::BitStream * RakServer::GetStaticClientData( const PlayerID playerId )
{
	return RakPeer::GetRemoteStaticData( playerId );
}

void RakServer::SetStaticClientData( const PlayerID playerId, const char *data, const int length )
{
	RakPeer::SetRemoteStaticData( playerId, data, length );
}

// This will read the data from playerChangedId and send it to playerToSendToId
void RakServer::ChangeStaticClientData( const PlayerID playerChangedId, PlayerID playerToSendToId )
{
	RemoteSystemStruct * remoteSystem = RakPeer::GetRemoteSystemFromPlayerID( playerChangedId, false, true );

	if ( remoteSystem == 0 )
		return ; // No such playerChangedId

	// Relay static data to the other systems but the sender
	RakNet::BitStream bitStream;

	unsigned char typeId = ID_REMOTE_STATIC_DATA;

	bitStream.Write( typeId );

	bitStream.Write( playerChangedId.binaryAddress );

	bitStream.Write( playerChangedId.port );

	bitStream.Write( ( char* ) remoteSystem->staticData.GetData(), remoteSystem->staticData.GetNumberOfBytesUsed() );

	Send( &bitStream, SYSTEM_PRIORITY, RELIABLE, 0, playerToSendToId, true );
}

unsigned int RakServer::GetNumberOfAddresses( void )
{
	return RakPeer::GetNumberOfAddresses();
}

PlayerID RakServer::GetInternalID( void ) const
{
	return RakPeer::GetInternalID();
}

void RakServer::PushBackPacket( Packet *packet, bool pushAtHead )
{
	RakPeer::PushBackPacket(packet, pushAtHead);
}
void RakServer::SetRouterInterface( RouterInterface *routerInterface )
{
	RakPeer::SetRouterInterface(routerInterface);
}
void RakServer::RemoveRouterInterface( RouterInterface *routerInterface )
{
	RakPeer::RemoveRouterInterface(routerInterface);
}

const char* RakServer::GetLocalIP( unsigned int index )
{
	return RakPeer::GetLocalIP( index );
}

int RakServer::GetIndexFromPlayerID( const PlayerID playerId )
{
	return RakPeer::GetIndexFromPlayerID( playerId );
}

PlayerID RakServer::GetPlayerIDFromIndex( int index )
{
	return RakPeer::GetPlayerIDFromIndex( index );
}

void RakServer::AddToBanList( const char *IP, RakNetTime milliseconds )
{
	RakPeer::AddToBanList( IP, milliseconds );
}

void RakServer::RemoveFromBanList( const char *IP )
{
	RakPeer::RemoveFromBanList( IP );
}

void RakServer::ClearBanList( void )
{
	RakPeer::ClearBanList();
}

bool RakServer::IsBanned( const char *IP )
{
	return RakPeer::IsBanned( IP );
}

bool RakServer::IsActivePlayerID( const PlayerID playerId )
{
	return RakPeer::GetRemoteSystemFromPlayerID( playerId, false, true ) != 0;
}

void RakServer::SetTimeoutTime( RakNetTime timeMS, const PlayerID target )
{
	RakPeer::SetTimeoutTime(timeMS, target);
}

bool RakServer::SetMTUSize( int size )
{
	return RakPeer::SetMTUSize( size );
}

int RakServer::GetMTUSize( void ) const
{
	return RakPeer::GetMTUSize();
}

void RakServer::AdvertiseSystem( const char *host, unsigned short remotePort, const char *data, int dataLength )
{
	RakPeer::AdvertiseSystem( host, remotePort, data, dataLength );
}

RakNetStatisticsStruct * RakServer::GetStatistics( const PlayerID playerId )
{
	return RakPeer::GetStatistics( playerId );
}

RakPeer::RemoteSystemStruct* RakServer::GetRemoteSystemFromPlayerID(const PlayerID playerId)
{
	return RakPeer::GetRemoteSystemFromPlayerID(playerId, false, false);
}

void RakServer::ReserveSlots(unsigned short count)
{
		RakPeer::ReserveSlots(count);
}

SAMPRakNet::RemoteSystemData RakServer::GetSAMPDataFromPlayerID(const PlayerID playerId)
{
	RemoteSystemStruct* remoteSystem = RakPeer::GetRemoteSystemFromPlayerID(playerId, false, false);

	if (remoteSystem == nullptr)
		return SAMPRakNet::RemoteSystemData();

	return remoteSystem->sampData;
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif
