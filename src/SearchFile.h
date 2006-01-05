//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef SEARCHFILE_H
#define SEARCHFILE_H

#include "Types.h"		// Needed for uint8, uint16 and uint32
#include "KnownFile.h"		// Needed for CAbstractFile

#include <vector>

class CMemFile;
class CMD4Hash;
class CSearchFile;


typedef std::vector<CSearchFile*> CSearchResultList;


class CSearchFile : public CAbstractFile
{
	friend class CPartFile;
public:
	CSearchFile(const CSearchFile& other);
	CSearchFile(const CMemFile& data, bool bOptUTF8, long nSearchID, uint32 nServerIP = 0, uint16 nServerPort = 0, const wxString& pszDirectory = wxEmptyString, bool nKademlia = false);
	
	virtual ~CSearchFile();

#ifdef CLIENT_GUI
	friend class CSearchListRem;
	CSearchFile(class CEC_SearchFile_Tag* tag);
#else
	void	AddSources(uint32 count, uint32 count_complete);
#endif
	
	uint32	GetClientID() const				{ return m_clientID; }
	void	SetClientID(uint32 clientID)	{ m_clientID = clientID; }
	uint16	GetClientPort() const			{ return m_clientPort; }
	void	SetClientPort(uint16 port)		{ m_clientPort = port; }

	/** Returns the total number of sources. */
	inline uint32		GetSourceCount() const;
	/** Returns the number of sources that have the entire file. */
	inline uint32		GetCompleteSourceCount() const;
	/** Returns the ID of the search, used to select the right list when displaying. */
	inline long			GetSearchID() const;
	/** Returns true if the result is from a Kademlia search. */
	inline bool			IsKademlia() const;
	/** Returns the parent of this file. */
	inline CSearchFile*	GetParent() const;
	/** Returns the list of children belonging to this file. */
	inline const CSearchResultList&	GetChildren() const;
	/** Returns true if this item has children. */
	inline bool			HasChildren() const;
	/** Returns true if children should be displayed. */
	inline bool			ShowChildren() const;
	/** Enable/Disable displaying of children (set in CSearchListCtrl). */
	inline void			SetShowChildren(bool show);
	
private:

	//! The unique ID of this search owning this result.
	long				m_searchID;
	//! The total number of sources for this file.
	uint32				m_sourceCount;
	//! The number of sources that have the complete file.
	uint32				m_completeSourceCount;
	//! Specifies if the result is from a kademlia search.
	bool				m_kademlia;	

	uint32		m_clientID;
	uint16		m_clientPort;
	wxString	m_directory;
};




////////////////////////////////////////////////////////////
// Implementations


uint32 CSearchFile::GetSourceCount() const
{
	return m_sourceCount;
}


uint32 CSearchFile::GetCompleteSourceCount() const
{
	return m_completeSourceCount; 
}


long CSearchFile::GetSearchID() const
{
	return m_searchID;
}


bool CSearchFile::IsKademlia() const
{
   return m_kademlia;
}


CSearchFile* CSearchFile::GetParent() const
{
	return NULL;
}


bool CSearchFile::ShowChildren() const
{
	return false;
}


void CSearchFile::SetShowChildren(bool)
{
}


const CSearchResultList& CSearchFile::GetChildren() const
{
	static CSearchResultList tmpList;
	
	return tmpList;
}


bool CSearchFile::HasChildren() const
{
	return false;
}

#endif // SEARCHLIST_H
