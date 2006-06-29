//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003 Barry Dunne (http://www.emule-project.net)
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#ifndef __KAD_UDP_LISTENER_H__
#define __KAD_UDP_LISTENER_H__

#include "../utils/UInt128.h"
#include "../../Tag.h"

class CKnownFile;
class CMemFile;
struct SSearchTerm;

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CSearch;

class CKademliaUDPListener
{
	friend class CSearch;

public:
	virtual ~CKademliaUDPListener() {};
	void Bootstrap(uint32 ip, uint16 port);
	void FirewalledCheck(uint32 ip, uint16 port);
	void SendMyDetails(byte opcode, uint32 ip, uint16 port);
	void PublishPacket(uint32 ip, uint16 port, const CUInt128& targetID, const CUInt128& contactID, const TagPtrList& tags);
	void SendNullPacket(byte opcode, uint32 ip, uint16 port);
	virtual void ProcessPacket(const byte* data, uint32 lenData, uint32 ip, uint16 port);
	void SendPacket(const CMemFile& data, byte opcode, uint32 destinationHost, uint16 destinationPort);

private:
	void AddContact (const byte* data, uint32 lenData, uint32 ip, uint16 port, uint16 tport = 0);
	void AddContacts(const byte* data, uint32 lenData, uint16 numContacts);
	static SSearchTerm* CreateSearchExpressionTree(CMemFile& bio, int iLevel);
	static void Free(SSearchTerm* pSearchTerms);

	void ProcessBootstrapRequest		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessBootstrapResponse		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessHelloRequest			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessHelloResponse			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessKademliaRequest			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessKademliaResponse		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessSearchRequest			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessSearchResponse			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessPublishRequest			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessPublishResponse			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessSearchNotesRequest		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessSearchNotesResponse		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessPublishNotesRequest		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessPublishNotesResponse	(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessFirewalledRequest		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessFirewalledResponse		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessFirewalledResponse2		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessFindBuddyRequest		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessFindBuddyResponse		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void ProcessCallbackRequest			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	
	void DebugClientOutput(const wxString& place, uint32 kad_ip, uint32 port, const byte* data = NULL, int len = 0);
};

} // End namespace

#endif //__KAD_UDP_LISTENER_H__
// File_checked_for_headers
