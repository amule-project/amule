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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef SEARCHLIST_H
#define SEARCHLIST_H

#include "Types.h"		// Needed for uint8, uint16 and uint32
#include "KnownFile.h"		// Needed for CAbstractFile
#include "Tag.h"		// Needed for TagPtrList

#include <wx/thread.h>

#include <map>
#include <vector>
#include <list>

class CMemFile;
class CMD4Hash;
class CServer;
class CSearchList;
class CPacket;
	
namespace Kademlia {
	class CUInt128;
}

enum SearchType {
	LocalSearch,
	GlobalSearch,
	KadSearch
};

class CGlobalSearchThread : public wxThread 
{	
	int m_progress;
public:
	CGlobalSearchThread( CPacket *packet );
	~CGlobalSearchThread();

	int Progress() { return m_progress; }

private:
	CPacket* 	m_packet;

	virtual void* Entry();
};


class CSearchFile : public CAbstractFile
{
	friend class CPartFile;
public:
	CSearchFile(const CMemFile& in_data, bool bOptUTF8, long nSearchID, uint32 nServerIP=0, uint16 nServerPort=0, const wxString& pszDirectory = wxEmptyString, bool nKademlia = false);
	
	virtual ~CSearchFile();

#ifdef CLIENT_GUI
	friend class CSearchListRem;
	CSearchFile(class CEC_SearchFile_Tag *);
	
	uint32	GetSourceCount() { return m_SourceCount; }
	uint32	GetCompleteSourceCount() { return m_CompleteSourceCount; }
	uint32  GetFileSize() { return m_nFileSize; }
#else

	uint32	GetIntTagValue(uint8 tagname) const;
	wxString	GetStrTagValue(uint8 tagname) const;
	void	AddSources(uint32 count, uint32 count_complete);
	
	uint32	GetSourceCount() const;
	uint32	GetCompleteSourceCount() const;
	uint32  GetFileSize() const;
#endif
	long	GetSearchID() const				{ return m_nSearchID; }
	uint32	GetClientID() const				{ return m_nClientID; }
	void	SetClientID(uint32 nClientID)	{ m_nClientID = nClientID; }
	uint16	GetClientPort() const			{ return m_nClientPort; }
	void	SetClientPort(uint16 nPort)		{ m_nClientPort = nPort; }
	
	// This is used for some Kad checks on the child search results also, but 
	// as we don't implement any child search results, it's not used there :)
	bool	IsKademlia() const { return m_nKademlia; }
	// This can be used to raise a warning on downloading a file with 
	// i.e. 50 sources and none complete.
	int IsComplete() const;
	int IsComplete(uint32 uSources, uint32 uCompleteSources) const;
	
private:
	long		m_nSearchID;
	
#ifdef CLIENT_GUI
	uint32 m_SourceCount, m_CompleteSourceCount;
#else
	typedef		std::vector<CTag*> TagList;
	TagList		m_taglist;
#endif
	uint32		m_nClientID;
	uint16		m_nClientPort;
	wxString	m_Directory;
	bool m_nKademlia;
};


class CSearchList
{
friend class CSearchListCtrl;
typedef std::vector<CSearchFile*> SearchList;

public:
	CSearchList();
	~CSearchList();
	void	Clear();

	wxString	StartNewSearch(uint32* nSearchID, 
								SearchType search_type, 
								const wxString& searchString, 
								const wxString& typeText,
								const wxString& extension, 
								uint32 min, 
								uint32 max, 
								uint32 availability);

	void	ProcessSearchanswer(const char* in_packet, uint32 size, CUpDownClient* Sender, bool* pbMoreResultsAvailable, const wxString& pszDirectory);
	void	ProcessSearchanswer(const char* packet, uint32 size, bool bOptUTF8, uint32 nServerIP, uint16 nServerPort);
	void	ProcessUDPSearchanswer(const CMemFile& packet, bool bOptUTF8, uint32 nServerIP, uint16 nServerPort);	

	void	RemoveResults(long nSearchID);

	const SearchList GetSearchResults(long nSearchID);
	
	void	AddFileToDownloadByHash(const CMD4Hash& hash, uint8 cat = 0);
	void	LocalSearchEnd();

	void StopGlobalSearch();
	
	void ClearThreadData(CGlobalSearchThread* thread = NULL) 
	{ 
		// The thread going down might not be the actual one.
		if (!thread || (thread == m_searchthread)) {
			m_searchthread = NULL; 
		}
		
		m_SearchInProgress = false;
		
	};
	
	bool IsGlobalSearch() { return m_searchthread != NULL; };

	bool SearchInProgress() { return m_SearchInProgress; }
	
	int Progress()
	{
		if ( m_searchthread ) {
			return m_searchthread->Progress();
		} else {
			return m_SearchInProgress ? 0 : 0xffff;
		}
	}
	
	void KademliaSearchKeyword(uint32 searchID, const Kademlia::CUInt128* pfileID, const wxString& name, uint32 size, const wxString& type, const TagPtrList& taglist);
	
private:

	CMemFile *CreateSearchData(const wxString &searchString, const wxString& typeText,
				const wxString &extension, uint32 min, uint32 max, uint32 availability, bool kad);

	CPacket* m_searchpacket;

	bool AddToList(CSearchFile* toadd, bool bClientResponse = false);

	typedef std::map<long, SearchList> ResultMap;
	
	ResultMap	m_Results;
	
	wxString	m_resultType;
	
	long		m_CurrentSearch;

	CGlobalSearchThread* m_searchthread;

	bool m_SearchInProgress;
};

#endif // SEARCHLIST_H
