//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef SEARCHLIST_H
#define SEARCHLIST_H

#include "types.h"		// Needed for uint8, uint16 and uint32
#include "KnownFile.h"		// Needed for CAbstractFile
#include <wx/thread.h>

#include <map>
#include <vector>

class CSafeMemFile;
class CMD4Hash;
class CServer;
class CSearchList;


Packet *CreateSearchPacket(wxString &searchString, wxString& typeText,
				wxString &extension, uint32 min, uint32 max, uint32 avaibility);


class CGlobalSearchThread : public wxThread 
{	
public:
	CGlobalSearchThread( Packet *packet );
	~CGlobalSearchThread();

private:
	Packet* 	m_packet;

	virtual void* Entry();
};


class CSearchFile : public CAbstractFile
{
	friend class CPartFile;
public:
	CSearchFile(const CSafeMemFile& in_data, bool bOptUTF8, long nSearchID, uint32 nServerIP=0, uint16 nServerPort=0, wxString pszDirectory = wxEmptyString);
	
	virtual ~CSearchFile();

	uint32	GetIntTagValue(uint8 tagname);
	wxString	GetStrTagValue(uint8 tagname);
	void	AddSources(uint32 count, uint32 count_complete);
	
	uint32	GetSourceCount();
	uint32	GetCompleteSourceCount();
	long	GetSearchID() 					{ return m_nSearchID; }
	uint32	GetClientID() const				{ return m_nClientID; }
	void	SetClientID(uint32 nClientID)	{ m_nClientID = nClientID; }
	uint16	GetClientPort() const			{ return m_nClientPort; }
	void	SetClientPort(uint16 nPort)		{ m_nClientPort = nPort; }
	
private:
	long		m_nSearchID;

	typedef		std::vector<CTag*> TagList;
	TagList		m_taglist;

	uint32		m_nClientID;
	uint16		m_nClientPort;
	wxString	m_Directory;
};


class CSearchList
{
friend class CSearchListCtrl;
public:
	CSearchList();
	~CSearchList();
	void	Clear();

	void	NewSearch(const wxString& resTypes, long nSearchID);
	void	ProcessSearchanswer(const char* in_packet, uint32 size, CUpDownClient* Sender, bool* pbMoreResultsAvailable, LPCTSTR pszDirectory);
	void	ProcessSearchanswer(const char* packet, uint32 size, bool bOptUTF8, uint32 nServerIP, uint16 nServerPort);
	void	ProcessUDPSearchanswer(const CSafeMemFile& packet, bool bOptUTF8, uint32 nServerIP, uint16 nServerPort);	

	void	RemoveResults(long nSearchID);
	void	ShowResults(long nSearchID);
	const std::vector<CSearchFile*> GetSearchResults(long nSearchID);
	
	wxString GetWebList(const wxString& linePattern,int sortby,bool asc) const;
	void	AddFileToDownloadByHash(const CMD4Hash& hash, uint8 cat = 0);
	void LocalSearchEnd();

	Packet* 	m_searchpacket;
	CGlobalSearchThread* m_searchthread;
	
private:
	bool AddToList(CSearchFile* toadd, bool bClientResponse = false);

	typedef std::vector<CSearchFile*> SearchList;
	typedef std::map<long, SearchList> ResultMap;
	
	ResultMap	m_Results;
	
	wxString	m_resultType;
	
	long		m_CurrentSearch;
};

#endif // SEARCHLIST_H
