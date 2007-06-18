//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2007 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef __PREFS_H__
#define __PREFS_H__

#include "../utils/UInt128.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CPrefs
{
public:
	CPrefs();
	~CPrefs();

	void	SetKadID(const CUInt128 &id)		{m_clientID = id;}
	const CUInt128& GetKadID() const					{return m_clientID;}

	void	SetClientHash(const CUInt128 &id)	{m_clientHash = id;}
	const CUInt128& GetClientHash() const				{return m_clientHash;}

	uint32	GetIPAddress() const				{return m_ip;}
	void	SetIPAddress(uint32 val);

	bool	GetRecheckIP() const				{return (m_recheckip<4);}
	void	SetRecheckIP()						{m_recheckip = 0; SetFirewalled();}
	void	IncRecheckIP()						{m_recheckip++;}

	bool	HasHadContact() const;
	void	SetLastContact()					{m_lastContact = time(NULL);}
	bool	HasLostConnection() const;
	uint32	GetLastContact() const				{return m_lastContact;}

	bool	GetFirewalled() const;
	void	SetFirewalled();
	void	IncFirewalled();

	uint8	GetTotalFile() const				{return m_totalFile;}
	void	SetTotalFile(uint8 val)				{m_totalFile = val;}

	uint8	GetTotalStoreSrc() const			{return m_totalStoreSrc;}
	void	SetTotalStoreSrc(uint8 val)			{m_totalStoreSrc = val;}

	uint8	GetTotalStoreKey() const			{return m_totalStoreKey;}
	void	SetTotalStoreKey(uint8 val)			{m_totalStoreKey = val;}

	uint8	GetTotalSource() const				{return m_totalSource;}
	void	SetTotalSource(uint8 val)			{m_totalSource = val;}

	uint8	GetTotalNotes() const				{return m_totalNotes;}
	void	SetTotalNotes(uint8 val)			{m_totalNotes = val;}

	uint8	GetTotalStoreNotes() const			{return m_totalStoreNotes;}
	void	SetTotalStoreNotes(uint8 val)		{m_totalStoreNotes = val;}

	uint32	GetKademliaUsers() const			{return m_kademliaUsers;}
	void	SetKademliaUsers(uint32 val)		{m_kademliaUsers = val;}

	uint32	GetKademliaFiles() const			{return m_kademliaFiles;}
	void	SetKademliaFiles();

	bool	GetPublish() const					{return m_Publish;}
	void	SetPublish(bool val)				{m_Publish = val;}

	bool	GetFindBuddy();
	void	SetFindBuddy(bool val = true)		{m_findBuddy = val;}

private:
	wxString	m_filename;

	time_t		m_lastContact;
	CUInt128	m_clientID;
	CUInt128	m_clientHash;
	uint32		m_ip;
	uint32		m_ipLast;
	uint32		m_recheckip;
	uint32		m_firewalled;
	uint32		m_kademliaUsers;
	uint32		m_kademliaFiles;
	uint8		m_totalFile;
	uint8		m_totalStoreSrc;
	uint8		m_totalStoreKey;
	uint8		m_totalSource;
	uint8		m_totalNotes;
	uint8		m_totalStoreNotes;
	bool		m_Publish;
	bool		m_findBuddy;
	bool		m_lastFirewallState;

	void Init(const wxString& filename);
	void Reset();
	void SetDefaults();
	void ReadFile();
	void WriteFile();
};

} // End namespace

#endif //__PREFS_H__
// File_checked_for_headers
