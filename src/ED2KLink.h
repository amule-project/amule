// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef ED2KLINK_H
#define ED2KLINK_H

#include "types.h"		// Needed for uint16 and uint32
#include "CString.h"		// Needed for CString
#include "CTypedPtrList.h"	// Needed for CTypedPtrList

class CMemFile;

// Imported from 0.30d
struct SUnresolvedHostname{
	CString strHostname;
	uint16 nPort;
};
// EOI

class CED2KLink {
public:
	static CED2KLink* CreateLinkFromUrl(  const TCHAR * url);
	typedef enum { kServerList, kServer , kFile , kInvalid } LinkType;
	virtual LinkType GetKind() const =0;
	virtual void GetLink(wxString& lnk) =0;
	virtual class CED2KServerListLink* GetServerListLink() =0;
	virtual class CED2KServerLink* GetServerLink() =0;
	virtual class CED2KFileLink* GetFileLink() =0;
	virtual	~CED2KLink();
};

class CED2KServerLink : public CED2KLink {
public:
	CED2KServerLink(const TCHAR* ip,const TCHAR* port);
	virtual ~CED2KServerLink();
	// inherited from CED2KLink
	virtual LinkType GetKind() const;
	virtual CED2KServerListLink* GetServerListLink();
	virtual CED2KServerLink* GetServerLink();
	virtual CED2KFileLink* GetFileLink();
	virtual void GetLink(wxString& lnk);

    // Accessors
	uint32 GetIP() const { return m_ip;}
	uint16 GetPort() const { return m_port;}
	void GetDefaultName(wxString& defName) const { defName = m_defaultName; }
private:
	CED2KServerLink(); // Not defined
	CED2KServerLink(const CED2KServerLink&); // Not defined
	CED2KServerLink& operator=(const CED2KServerLink&); // Not defined
	uint32 m_ip;
	uint16 m_port;
	wxString m_defaultName;
};

class CED2KFileLink : public CED2KLink {
public:
  CED2KFileLink::CED2KFileLink(const TCHAR* name,const TCHAR* size, const TCHAR* hash,const TCHAR* sources);
	virtual ~CED2KFileLink();
	virtual LinkType GetKind() const;
	virtual CED2KServerListLink* GetServerListLink();
	virtual CED2KServerLink* GetServerLink();
	virtual CED2KFileLink* GetFileLink();
	virtual void GetLink(wxString& lnk);
	const char* GetName() const { return m_name; }
	uint64 GetSize() const { return atoll(m_size); }
	const unsigned char* GetHashKey() const { return m_hash;}
	bool HasValidSources() const {return (SourcesList!=NULL); }
	CMemFile* SourcesList;
	// Imported from 0.30d
	bool HasHostnameSources() const {return (!m_HostnameSourcesList.IsEmpty()); }
	CTypedPtrList<CPtrList, SUnresolvedHostname*> m_HostnameSourcesList;
	// EOI
	
private:
	CED2KFileLink(); // Not defined
	CED2KFileLink(const CED2KFileLink&); // Not defined
	CED2KFileLink& operator=(const CED2KFileLink&); // Not defined
	wxString m_name;
	wxString m_size;
	unsigned char m_hash[16];
};

class CED2KServerListLink : public CED2KLink {
public:
	CED2KServerListLink(const TCHAR* address);
	virtual ~CED2KServerListLink();
	virtual LinkType GetKind() const;
	virtual CED2KServerListLink* GetServerListLink();
	virtual CED2KServerLink* GetServerLink();
	virtual CED2KFileLink* GetFileLink();
	virtual void GetLink(wxString& lnk);
	const char* GetAddress() const { return m_address; }
private:
	CED2KServerListLink(); // Not defined
	CED2KServerListLink(const CED2KFileLink&); // Not defined
	CED2KServerListLink& operator=(const CED2KFileLink&); // Not defined
	wxString m_address;
};

#endif // ED2KLINK_H
