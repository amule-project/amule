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

#ifndef SEARCHFILE_H
#define SEARCHFILE_H

#include "KnownFile.h"	// Needed for CAbstractFile


class CMemFile;
class CMD4Hash;
class CSearchFile;


typedef std::vector<CSearchFile*> CSearchResultList;


/**
 * Represents a search-result returned from a server or client.
 * 
 * A file may have either a parent or any number of children.
 * When a child is added to a result, the parent becomes a generic
 * representation of all its children, which will include a copy
 * of the original result. The parent object will contain the sum
 * of sources (total/complete) and will have the most common 
 * filename. Children are owned by their parents, and can be
 * displayed on CSearchListCtrl.
 * 
 * Basic file parameters (hash, name, size, rating) can be read 
 * via the CAbstractFile functions. Tags pertaining to meta-data
 * are stored in the taglist inherited from CAbstractFile.
 *
 * TODO: Server IP/Port are currently not used.
 * TODO: Client ID/Port are currently not used.
 * TODO: Directories are currently not used.
 */
class CSearchFile : public CAbstractFile
{	
public:
	/** Constructor used to create results on the remote GUI. */
	CSearchFile(class CEC_SearchFile_Tag* tag);
	/** Copy constructor, also copies children. */
	CSearchFile(const CSearchFile& other);
	
	/**
	 * Normal constructor, reads a result from a packet.
	 *
	 * @param data Source of results-packet.
	 * @param optUTF8 Specifies if text-strings are to be read as UTF8.
	 * @param searchID searchID The 
	 * @param serverIP The IP of the server that sent this result.
	 * @param serverPort The port of the server that sent this result.
	 * @param directory If from a clients shared files, the directory this file is in.
	 * @param kademlia Specifies if this was from a kad-search.
	 */
	CSearchFile(const CMemFile& data, bool optUTF8, long searchID, uint32 serverIP = 0, uint16 serverPort = 0, const wxString& directory = wxEmptyString, bool kademlia = false);

	
	/** Frees all children owned by this file. */
	virtual ~CSearchFile();

	
	/**
	 * Adds the given sources to the file.
	 *
	 * @param count Total source count.
	 * @param count_complete Number of sources that have the complete file.
	 *
	 * Note that for Kademlia results, only the largest value is used.
	 */
	void	AddSources(uint32 count, uint32 count_complete);

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
	
	/**
	 * Adds the given file as a child of this file.
	 *
	 * Note that a file can either be a parent _or_
	 * a child, but not both. Also note that it is 
	 * only legal to add children whoose filesize and
	 * filehash matches the parent's. AddChild takes
	 * ownership of the file.
	 */
	void				AddChild(CSearchFile* file);

	
	//@{
	//! TODO: Currently not used.
	inline uint32		GetClientID() const;
	inline void			SetClientID(uint32 clientID);
	inline uint16		GetClientPort() const;
	inline void			SetClientPort(uint16 port);
	//@}
	
private:
	//! CSearchFile is not assignable.
	CSearchFile& operator=(const CSearchFile& other);

	/**
	 * Updates a parent file so that it shows various common traits.
	 *
	 * Currently, the most common filename is selected, and an average
	 * of fileratings is set, based on files that have a rating only.
	 */
	void	UpdateParent();

	
	//! The parent of this result.
	CSearchFile*		m_parent;
	//! Any children this result may have.
	CSearchResultList	m_children;
	//! If true, children will be shown on the GUI.
	bool				m_showChildren;
	//! The unique ID of this search owning this result.
	long				m_searchID;
	//! The total number of sources for this file.
	uint32				m_sourceCount;
	//! The number of sources that have the complete file.
	uint32				m_completeSourceCount;
	//! Specifies if the result is from a kademlia search.
	bool				m_kademlia;


	//@{
	//! TODO: Currently not used.
	uint32				m_clientID;
	uint16				m_clientPort;
	wxString			m_directory;
	//@}
	

	friend class CPartFile;
	friend class CSearchListRem;
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
	return m_parent;
}


bool CSearchFile::ShowChildren() const
{
	return m_showChildren;
}


void CSearchFile::SetShowChildren(bool show)
{
	m_showChildren = show;
}


const CSearchResultList& CSearchFile::GetChildren() const
{
	return m_children;
}


bool CSearchFile::HasChildren() const
{
	return not m_children.empty();
}


uint32 CSearchFile::GetClientID() const
{
   return m_clientID;
}


void CSearchFile::SetClientID(uint32 clientID)
{
	m_clientID = clientID;
}


uint16 CSearchFile::GetClientPort() const
{
	return m_clientPort;
}


void CSearchFile::SetClientPort(uint16 port)
{
	m_clientPort = port;
}

#endif // SEARCHLIST_H
// File_checked_for_headers
