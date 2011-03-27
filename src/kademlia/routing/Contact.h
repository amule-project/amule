//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Barry Dunne (http://www.emule-project.net)
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

#ifndef __CONTACT_H__
#define __CONTACT_H__

#include "../kademlia/Kademlia.h"
#include "../utils/KadUDPKey.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CContact
{
public:
	~CContact();
	CContact(const CUInt128 &clientID,
		uint32_t ip, uint16_t udpPort, uint16_t tcpPort, uint8_t version,
		const CKadUDPKey& kadKey, bool ipVerified,
		const CUInt128 &target = CKademlia::GetPrefs()->GetKadID());

	CContact(const CContact& k1);

	const CUInt128& GetClientID() const throw()		{ return m_clientID; }
	void SetClientID(const CUInt128& clientID) throw()	{ m_clientID = clientID; m_distance = CKademlia::GetPrefs()->GetKadID() ^ clientID; }

	const wxString GetClientIDString() const		{ return m_clientID.ToHexString(); }

	const CUInt128& GetDistance() const throw()		{ return m_distance; }
	const wxString GetDistanceString() const		{ return m_distance.ToBinaryString(); }

	uint32_t GetIPAddress() const throw()			{ return m_ip; }
	void	 SetIPAddress(uint32_t ip) throw()		{ if (m_ip != ip) { SetIPVerified(false); m_ip = ip; } }

	uint16_t GetTCPPort() const throw()			{ return m_tcpPort; }
	void	 SetTCPPort(uint16_t port) throw()		{ m_tcpPort = port; }

	uint16_t GetUDPPort() const throw()			{ return m_udpPort; }
	void	 SetUDPPort(uint16_t port) throw()		{ m_udpPort = port; }

	uint8_t	 GetType() const throw()			{ return m_type; }

	void	 UpdateType() throw();
	void	 CheckingType() throw();

	bool	 InUse() const throw()				{ return m_inUse > 0; }
	void	 IncUse() throw()				{ m_inUse++; }
	void	 DecUse()					{ if (m_inUse) m_inUse--; else { wxFAIL; } }

	time_t	 GetCreatedTime() const throw()			{ return m_created; }

	void	 SetExpireTime(time_t value) throw()		{ m_expires = value; };	
	time_t	 GetExpireTime() const throw()			{ return m_expires; }

	time_t	 GetLastTypeSet() const throw()			{ return m_lastTypeSet; }

	time_t	 GetLastSeen() const throw();

	uint8_t	 GetVersion() const throw()			{ return m_version; }
	void	 SetVersion(uint8_t value) throw()		{ m_version = value; }

	const CKadUDPKey& GetUDPKey() const throw()		{ return m_udpKey; }
	void	 SetUDPKey(const CKadUDPKey& key) throw()	{ m_udpKey = key; }

	bool	 IsIPVerified() const throw()			{ return m_ipVerified; }
	void	 SetIPVerified(bool ipVerified) throw()		{ m_ipVerified = ipVerified; }

	bool	GetReceivedHelloPacket() const throw()		{ return m_receivedHelloPacket; }
	void	SetReceivedHelloPacket() throw()		{ m_receivedHelloPacket = true; }

private:
	CUInt128	m_clientID;
	CUInt128	m_distance;
	uint32_t	m_ip;
	uint16_t	m_tcpPort;
	uint16_t	m_udpPort;
	uint8_t		m_type;
	time_t		m_lastTypeSet;
	time_t		m_expires;
	time_t		m_created;
	uint32_t	m_inUse;
	uint8_t		m_version;
	bool		m_ipVerified;
	bool		m_receivedHelloPacket;
	CKadUDPKey	m_udpKey;
};

} // End namespace

#endif // __CONTACT_H__
// File_checked_for_headers
