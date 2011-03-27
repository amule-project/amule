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

#ifndef __PREFS_H__
#define __PREFS_H__

#include "../utils/UInt128.h"
#include "../../Preferences.h"
#include <protocol/kad/Constants.h>
#include <time.h>
#include <vector>

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CPrefs
{
public:
	CPrefs();
	~CPrefs();

	void	SetKadID(const CUInt128 &id) throw()		{ m_clientID = id; }
	const CUInt128&	GetKadID() const throw()		{ return m_clientID; }

	void	SetClientHash(const CUInt128 &id) throw()	{ m_clientHash = id; }
	const CUInt128& GetClientHash() const throw()		{ return m_clientHash; }

	uint32_t GetIPAddress() const throw()			{ return m_ip; }
	void	 SetIPAddress(uint32_t val) throw();

	bool	GetRecheckIP() const throw()			{ return (m_recheckip < KADEMLIAFIREWALLCHECKS); }
	void	SetRecheckIP()					{ m_recheckip = 0; SetFirewalled(); }
	void	IncRecheckIP() throw()				{ m_recheckip++; }

	bool	HasHadContact() const throw()			{ return m_lastContact ? ((time(NULL) - m_lastContact) < KADEMLIADISCONNECTDELAY) : false; }
	void	SetLastContact() throw()			{ m_lastContact = time(NULL); }
	bool	HasLostConnection() const throw()		{ return m_lastContact ? !((time(NULL) - m_lastContact) < KADEMLIADISCONNECTDELAY) : false; }
	uint32_t GetLastContact() const throw()			{ return m_lastContact; }

	bool	GetFirewalled() const throw();
	void	SetFirewalled();
	void	IncFirewalled();

	uint8_t	GetTotalFile() const throw()			{ return m_totalFile; }
	void	SetTotalFile(uint8_t val) throw()		{ m_totalFile = val; }

	uint8_t	GetTotalStoreSrc() const throw()		{ return m_totalStoreSrc; }
	void	SetTotalStoreSrc(uint8_t val) throw()		{ m_totalStoreSrc = val; }

	uint8_t	GetTotalStoreKey() const throw()		{ return m_totalStoreKey; }
	void	SetTotalStoreKey(uint8_t val) throw()		{ m_totalStoreKey = val; }

	uint8_t	GetTotalSource() const throw()			{ return m_totalSource; }
	void	SetTotalSource(uint8_t val) throw()		{ m_totalSource = val; }

	uint8_t	GetTotalNotes() const throw()			{ return m_totalNotes; }
	void	SetTotalNotes(uint8_t val) throw()		{ m_totalNotes = val; }

	uint8_t	GetTotalStoreNotes() const throw()		{ return m_totalStoreNotes; }
	void	SetTotalStoreNotes(uint8_t val) throw()		{ m_totalStoreNotes = val; }

	uint32_t GetKademliaUsers() const throw()		{ return m_kademliaUsers; }
	void	 SetKademliaUsers(uint32_t val) throw()		{ m_kademliaUsers = val; }

	uint32_t GetKademliaFiles() const throw()		{ return m_kademliaFiles; }
	void	 SetKademliaFiles();

	bool	GetPublish() const throw()			{ return m_Publish; }
	void	SetPublish(bool val) throw()			{ m_Publish = val; }

	bool	GetFindBuddy() throw()				{ return m_findBuddy ? m_findBuddy = false, true : false; }
	void	SetFindBuddy(bool val = true) throw()		{ m_findBuddy = val; }

	bool	GetUseExternKadPort() const;
	void	SetUseExternKadPort(bool val) throw()		{ m_useExternKadPort = val; }

	uint16_t GetExternalKadPort() const throw()		{ return m_externKadPort; }
	uint16_t GetInternKadPort() const throw()		{ return thePrefs::GetUDPPort(); }
	void	 SetExternKadPort(uint16_t port, uint32_t fromIP);
	bool	 FindExternKadPort(bool reset = false);

	static uint8_t	GetMyConnectOptions(bool encryption = true, bool callback = true);
	static uint32_t GetUDPVerifyKey(uint32_t targetIP);

	// Statistics
	void	StatsIncUDPFirewalledNodes(bool firewalled) throw()	{ firewalled ? ++m_statsUDPFirewalledNodes : ++m_statsUDPOpenNodes; }
	void	StatsIncTCPFirewalledNodes(bool firewalled) throw()	{ firewalled ? ++m_statsTCPFirewalledNodes : ++m_statsTCPOpenNodes; }
	float	StatsGetFirewalledRatio(bool udp) const throw();
	float	StatsGetKadV8Ratio();

private:
	wxString	m_filename;

	time_t		m_lastContact;
	CUInt128	m_clientID;
	CUInt128	m_clientHash;
	uint32_t	m_ip;
	uint32_t	m_ipLast;
	uint32_t	m_recheckip;
	uint32_t	m_firewalled;
	uint32_t	m_kademliaUsers;
	uint32_t	m_kademliaFiles;
	uint8_t		m_totalFile;
	uint8_t		m_totalStoreSrc;
	uint8_t		m_totalStoreKey;
	uint8_t		m_totalSource;
	uint8_t		m_totalNotes;
	uint8_t		m_totalStoreNotes;
	bool		m_Publish;
	bool		m_findBuddy;
	bool		m_lastFirewallState;
	bool		m_useExternKadPort;
	uint16_t	m_externKadPort;
	std::vector<uint32_t>	m_externPortIPs;
	std::vector<uint16_t>	m_externPorts;

	// Statistics
	uint32_t	m_statsUDPOpenNodes;
	uint32_t	m_statsUDPFirewalledNodes;
	uint32_t	m_statsTCPOpenNodes;
	uint32_t	m_statsTCPFirewalledNodes;
	time_t		m_statsKadV8LastChecked;
	float		m_statsKadV8Ratio;

	void Init(const wxString& filename);
	//	void Reset();
	//	void SetDefaults();
	void ReadFile();
	void WriteFile();
};

} // End namespace

#include "Kademlia.h"

inline bool Kademlia::CPrefs::GetUseExternKadPort() const
{
	return m_useExternKadPort && !Kademlia::CKademlia::IsRunningInLANMode();
}

#endif //__PREFS_H__
// File_checked_for_headers
