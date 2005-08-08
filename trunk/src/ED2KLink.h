//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef ED2KLINK_H
#define ED2KLINK_H


#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ED2KLink.h"
#endif


#include "Types.h"			// Needed for uint16 and uint32
#include "CMD4Hash.h"		// Needed for CMD4Hash
#include "SHAHashSet.h"		// Needed for CAICHHash

#include <deque>

class CMemFile;


struct SUnresolvedHostname
{
	wxString strHostname;
	uint16 nPort;
};


class CED2KLink
{
public:
	static CED2KLink* CreateLinkFromUrl( const wxString& url );
	typedef enum { kServerList, kServer , kFile , kInvalid } LinkType;

	LinkType GetKind() const;
	virtual wxString GetLink() const = 0;

	virtual	~CED2KLink();

protected:
	CED2KLink( LinkType type );

private:
	LinkType	m_type;
};


class CED2KFileLink : public CED2KLink
{
public:
	CED2KFileLink( const wxString& name, const wxString& size, const wxString& hash, const wxString& hashset, const wxString& masterhash, const wxString& sources );

	virtual ~CED2KFileLink();

	virtual wxString GetLink() const;

	wxString GetName() const;
	uint32 GetSize() const;
	const CMD4Hash& GetHashKey() const;
	bool HasValidSources() const;

	bool HasHostnameSources() const;

	// AICH data
	bool	HasValidAICHHash() const;
	const CAICHHash&	GetAICHHash() const;

	CMemFile* m_sources;
	CMemFile* m_hashset;
	std::deque<SUnresolvedHostname*> m_hostSources;

private:
	CED2KFileLink(); // Not defined
	CED2KFileLink(const CED2KFileLink&); // Not defined
	CED2KFileLink& operator=(const CED2KFileLink&); // Not defined

	wxString	m_name;
	wxString	m_size;
	CMD4Hash	m_hash;
	bool		m_bAICHHashValid;
	CAICHHash	m_AICHHash;
};


class CED2KServerLink : public CED2KLink
{
public:
	CED2KServerLink( const wxString& ip, const wxString& port);
	virtual wxString GetLink() const;

	uint32 GetIP() const;
	uint16 GetPort() const;

private:
	CED2KServerLink(); // Not defined
	CED2KServerLink(const CED2KServerLink&); // Not defined
	CED2KServerLink& operator=(const CED2KServerLink&); // Not defined

	uint32 m_ip;
	uint16 m_port;
};


class CED2KServerListLink : public CED2KLink
{
public:
	CED2KServerListLink(const wxString& address);
	virtual wxString GetLink() const;

	const wxString& GetAddress() const;

private:
	CED2KServerListLink(); // Not defined
	CED2KServerListLink(const CED2KFileLink&); // Not defined
	CED2KServerListLink& operator=(const CED2KFileLink&); // Not defined

	wxString m_address;
};


#endif
