//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef FRIEND_H
#define FRIEND_H


#include <ec/cpp/ECID.h>	// Needed for CECID
#include "MD4Hash.h"
#include "ClientRef.h"		// Needed for CClientRef

class CFile;
class CFileDataIO;

#define	FF_NAME		0x01

class CFriend  : public CECID
{
friend class CFriendListRem;
public:
	CFriend()	{ Init(); }
	~CFriend() {};
	
	CFriend(CClientRef client);
	CFriend( const CMD4Hash& userhash, uint32 tm_dwLastSeen, uint32 tm_dwLastUsedIP, uint32 tm_nLastUsedPort, uint32 tm_dwLastChatted, const wxString& tm_strName);
	CFriend(uint32 ecid) : CECID(ecid)	{ Init(); }
	
	void	SetUserHash(const CMD4Hash& userhash) { m_UserHash = userhash;}
	bool	HasHash() const			{ return !m_UserHash.IsEmpty(); }
	const	CMD4Hash& GetUserHash() const { return m_UserHash; }
	
	void SetName(const wxString& name) { m_strName = name; }
	
	void	LinkClient(CClientRef client);
	const CClientRef& GetLinkedClient() const { return m_LinkedClient; }
	void	UnLinkClient(bool notify = true);
	
	bool	HasFriendSlot();

	const wxString& GetName() const	{ return m_strName; }
	uint16 GetPort() const			{ return m_nLastUsedPort; }
	uint32 GetIP() const			{ return m_dwLastUsedIP; }
	
	void	LoadFromFile(CFileDataIO* file);
	void	WriteToFile(CFileDataIO* file);

private:
	void	Init();

	CClientRef	m_LinkedClient;

	CMD4Hash	m_UserHash;
	uint32		m_dwLastUsedIP;
	uint16		m_nLastUsedPort;
	wxString	m_strName;

	// write-only info, never used (kept in order not to break the save file)
	uint32		m_dwLastSeen;
	uint32		m_dwLastChatted;
};

#endif
// File_checked_for_headers
