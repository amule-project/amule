//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef ED2KLINK_H
#define ED2KLINK_H


#include "MD4Hash.h"		// Needed for CMD4Hash
#include "SHAHashSet.h"		// Needed for CAICHHash



class CMemFile;


class CED2KLink
{
public:
	typedef enum { kServerList, kServer , kFile , kInvalid } LinkType;
	
	static CED2KLink* CreateLinkFromUrl(const wxString& link);

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
	friend class CED2KLink;
	CED2KFileLink(const wxString& link);
	
public:
	virtual ~CED2KFileLink();

	virtual wxString GetLink() const;

	wxString GetName() const;
	uint64 GetSize() const;
	const CMD4Hash& GetHashKey() const;

	// AICH data
	bool	HasValidAICHHash() const;
	const CAICHHash&	GetAICHHash() const;

	CMemFile* m_hashset;

	/**
	 * Structure used to store sources found in file links.
	 */
	struct SED2KLinkSource
	{
		//! Hostname or dot-address.
		wxString addr;
		//! The source's TCP-port.
		uint16 port;
		//! Client hash for encryption
		wxString hash;
		//! Client cryptoptions
		uint8 cryptoptions;		
	};

	typedef std::deque<SED2KLinkSource> CED2KLinkSourceList;	
	CED2KLinkSourceList m_sources;
	
private:
	CED2KFileLink(); // Not defined
	CED2KFileLink(const CED2KFileLink&); // Not defined
	CED2KFileLink& operator=(const CED2KFileLink&); // Not defined

	wxString	m_name;
	uint64		m_size;
	CMD4Hash	m_hash;
	bool		m_bAICHHashValid;
	CAICHHash	m_AICHHash;
};


class CED2KServerLink : public CED2KLink
{
	friend class CED2KLink;
	CED2KServerLink(const wxString& link);

public:
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
	friend class CED2KLink;	
	CED2KServerListLink(const wxString& link);

public:
	virtual wxString GetLink() const;

	const wxString& GetAddress() const;

private:
	CED2KServerListLink(); // Not defined
	CED2KServerListLink(const CED2KFileLink&); // Not defined
	CED2KServerListLink& operator=(const CED2KFileLink&); // Not defined

	wxString m_address;
};


#endif
// File_checked_for_headers
