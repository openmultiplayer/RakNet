/// \file
/// \brief \b [Internal] Memory pool for InternalPacket*
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

#ifndef __INTERNAL_PACKET_POOL
#define __INTERNAL_PACKET_POOL
#include "DS_Queue.h"
#include "InternalPacket.h"

namespace RakNet
{
	/// open.mp's InternalPacketPool reimplementation
	/// Handles of a pool of InternalPacket pointers.  This is only here for efficiency.
	/// \sa InternalPacket.h
	class InternalPacketPool
	{
	public:
		InternalPacket* GetPointer();
		void ReleasePointer(InternalPacket* p);
	};
}

#endif

