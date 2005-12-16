//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef __CONTACT_H__
#define __CONTACT_H__

#include "../utils/UInt128.h"
#include "../kademlia/Prefs.h"
#include "../kademlia/Kademlia.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CContact
{
	
public:

	~CContact();
	CContact();
	CContact(const CUInt128 &clientID, uint32 ip, uint16 udpPort, uint16 tcpPort, const CUInt128 &target = CKademlia::getPrefs()->getKadID());

	const CUInt128& getClientID(void) const { return m_clientID; };
	void setClientID(const CUInt128 &clientID);

	const wxString getClientIDString(void) const;

	const CUInt128& getDistance(void) const { return m_distance; }
	const wxString getDistanceString(void) const;

	uint32 getIPAddress(void) const;
	void setIPAddress(uint32 ip);

	uint16 getTCPPort(void) const;
	void setTCPPort(uint16 port);

	uint16 getUDPPort(void) const;
	void setUDPPort(uint16 port);

	byte getType(void) const;
	
	void updateType();
	void checkingType();
	
	bool inUse(void) {return (m_inUse>0); }
	void incUse(void) {m_inUse++;}
	void decUse(void) 
	{
		if (m_inUse) {
			m_inUse--;
		}else {
			wxASSERT(0);
		}
	}

	const time_t getCreatedTime() const {return m_created;}

	void setExpireTime(time_t value) { m_expires = value; };	
	const time_t getExpireTime() const {return m_expires;}

	const time_t getLastTypeSet() const {return m_lastTypeSet;}	
	
	
private:

	void initContact(); // Common initialization goes here

	CUInt128	m_clientID;
	CUInt128	m_distance;
	uint32		m_ip;
	uint16		m_tcpPort;
	uint16		m_udpPort;
	byte		m_type;
	time_t		m_lastTypeSet;
	time_t		m_expires;
	time_t		m_created;
	uint32		m_inUse;
};

} // End namespace

#endif // __CONTACT_H__
