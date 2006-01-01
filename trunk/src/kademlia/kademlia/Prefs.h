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

#ifndef __PREFS_H__
#define __PREFS_H__

#include "../utils/UInt128.h"
#include "Types.h"
#include <ctime>

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CPrefs
{
public:
	CPrefs();
	~CPrefs();

	void	setKadID(const CUInt128 &id)		{m_clientID = id;}
	const CUInt128& getKadID() const					{return m_clientID;}

	void	setClientHash(const CUInt128 &id)	{m_clientHash = id;}
	const CUInt128& getClientHash() const				{return m_clientHash;}

	uint32	getIPAddress() const				{return m_ip;}
	void	setIPAddress(uint32 val);

	bool	getRecheckIP() const				{return (m_recheckip<4);}
	void	setRecheckIP()						{m_recheckip = 0; setFirewalled();}
	void	incRecheckIP()						{m_recheckip++;}

	bool	hasHadContact() const;
	void	setLastContact()					{m_lastContact = time(NULL);}
	bool	hasLostConnection() const;
	uint32	getLastContact() const				{return m_lastContact;}

	bool	getFirewalled() const;
	void	setFirewalled();
	void	incFirewalled();

	uint8	getTotalFile() const				{return m_totalFile;}
	void	setTotalFile(uint8 val)				{m_totalFile = val;}

	uint8	getTotalStoreSrc() const			{return m_totalStoreSrc;}
	void	setTotalStoreSrc(uint8 val)			{m_totalStoreSrc = val;}

	uint8	getTotalStoreKey() const			{return m_totalStoreKey;}
	void	setTotalStoreKey(uint8 val)			{m_totalStoreKey = val;}

	uint8	getTotalSource() const				{return m_totalSource;}
	void	setTotalSource(uint8 val)			{m_totalSource = val;}

	uint8	getTotalNotes() const				{return m_totalNotes;}
	void	setTotalNotes(uint8 val)			{m_totalNotes = val;}

	uint8	getTotalStoreNotes() const			{return m_totalStoreNotes;}
	void	setTotalStoreNotes(uint8 val)		{m_totalStoreNotes = val;}

	uint32	getKademliaUsers() const			{return m_kademliaUsers;}
	void	setKademliaUsers(uint32 val)		{m_kademliaUsers = val;}

	uint32	getKademliaFiles() const			{return m_kademliaFiles;}
	void	setKademliaFiles();

	bool	getPublish() const					{return m_Publish;}
	void	setPublish(bool val)				{m_Publish = val;}

	bool	getFindBuddy();
	void	setFindBuddy(bool val = true)		{m_findBuddy = val;}

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

	void init(const wxString& filename);
	void reset();
	void setDefaults();
	void readFile();
	void writeFile();
};

} // End namespace

#endif //__PREFS_H__
