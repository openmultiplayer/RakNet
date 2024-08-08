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

#include "InternalPacketPool.h"
#include <assert.h>

using namespace RakNet;

// open.mp's InternalPacketPool reimplementation.
// Amir: Instead of relying on a super slow DataStructures::Queue<InternalPacket*> pool, we allocate and deallocate in time of need.
// This way since we know when deallocation is needed and we do it, we don't have to rely on ClearPool usage in ReliablityLayer::FreeThreadedMemory;
// Because no InternalPacket is left to be cleared/deallocated anyway.

InternalPacket* InternalPacketPool::GetPointer()
{
	return new InternalPacket;
}

void InternalPacketPool::ReleasePointer(InternalPacket* p)
{
	delete p;
}
