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
class CSearchFile : public CAbstractFile, public CECID
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
	CSearchFile(
		const CMemFile& data,
		bool optUTF8,
		wxUIntPtr searchID,
		uint32_t serverIP = 0,
		uint16_t serverPort = 0,
		const wxString& directory = wxEmptyString,
		bool kademlia = false);

	
	/** Frees all children owned by this file. */
	virtual ~CSearchFile();

	
	/**
	 * Merges the two results into one.
	 *
	 * Merges the other result into this one, updating
	 * various informations.
	 *
	 * @param other The file to be merged into this.
	 */
	void MergeResults(const CSearchFile& other);

	/** Returns the total number of sources. */
	uint32 GetSourceCount() const			{ return m_sourceCount; }
	/** Returns the number of sources that have the entire file. */
	uint32 GetCompleteSourceCount() const	{ return m_completeSourceCount; }
	/** Returns the ID of the search, used to select the right list when displaying. */
	wxUIntPtr GetSearchID() const			{ return m_searchID; }
	/** Returns true if the result is from a Kademlia search. */
	bool IsKademlia() const					{ return m_kademlia; }

	// Possible download status of a file
	enum DownloadStatus {
		NEW,				// not known
		DOWNLOADED,			// successfully downloaded or shared
		QUEUED,				// downloading (Partfile)
		CANCELED,			// canceled
		QUEUEDCANCELED		// canceled once, but now downloading again
	};

	/** Returns the download status. */
	enum DownloadStatus GetDownloadStatus() const	{ return m_downloadStatus; }
	/** Set download status according to the global lists of knownfile, partfiles, canceledfiles. */
	void SetDownloadStatus();
	/** Set download status directly. */
	void SetDownloadStatus(enum DownloadStatus s)	{ m_downloadStatus = s; }
		
	/** Returns the parent of this file. */
	CSearchFile *GetParent() const			{ return m_parent; }
	/** Returns the list of children belonging to this file. */
	const CSearchResultList &GetChildren() const	{ return m_children; }
	/** Returns true if this item has children. */
	bool HasChildren() const				{ return !m_children.empty(); }
	/** Returns true if children should be displayed. */
	bool ShowChildren() const				{ return m_showChildren; }
	/** Enable/Disable displaying of children (set in CSearchListCtrl). */
	void SetShowChildren(bool show)			{ m_showChildren = show; }
	
	/**
	 * Adds the given file as a child of this file.
	 *
	 * Note that a file can either be a parent _or_
	 * a child, but not both. Also note that it is 
	 * only legal to add children whose filesize and
	 * filehash matches the parent's. AddChild takes
	 * ownership of the file.
	 */
	void		AddChild(CSearchFile* file);

	struct ClientStruct {
		ClientStruct()
			: m_ip(0), m_port(0), m_serverIP(0), m_serverPort(0)
		{}

		ClientStruct(uint32_t ip, uint16_t port, uint32_t serverIP, uint16_t serverPort)
			: m_ip(ip), m_port(port), m_serverIP(serverIP), m_serverPort(serverPort)
		{}

		uint32_t m_ip;
		uint16_t m_port;
		uint32_t m_serverIP;
		uint32_t m_serverPort;
	};

	void	 AddClient(const ClientStruct& client);
	const std::list<ClientStruct>& GetClients() const	{ return m_clients; }

	uint32_t GetClientID() const throw()			{ return m_clientID; }
	void	 SetClientID(uint32_t clientID) throw()		{ m_clientID = clientID; }
	uint16_t GetClientPort() const throw()			{ return m_clientPort; }
	void	 SetClientPort(uint16_t port) throw()		{ m_clientPort = port; }
	uint32_t GetClientServerIP() const throw()		{ return m_clientServerIP; }
	void	 SetClientServerIP(uint32_t serverIP) throw()	{ m_clientServerIP = serverIP; }
	uint16_t GetClientServerPort() const throw()		{ return m_clientServerPort; }
	void	 SetClientServerPort(uint16_t port) throw()	{ m_clientServerPort = port; }
	int	 GetClientsCount() const			{ return ((GetClientID() && GetClientPort()) ? 1 : 0) + m_clients.size(); }

	void	 SetKadPublishInfo(uint32_t val) throw()	{ m_kadPublishInfo = val; }
	uint32_t GetKadPublishInfo() const throw()		{ return m_kadPublishInfo; }

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
	bool			m_showChildren;
	//! The unique ID of this search owning this result.
	wxUIntPtr		m_searchID;
	//! The total number of sources for this file.
	uint32			m_sourceCount;
	//! The number of sources that have the complete file.
	uint32			m_completeSourceCount;
	//! Specifies if the result is from a kademlia search.
	bool			m_kademlia;
	//! The download status.
	enum DownloadStatus m_downloadStatus;

	//@{
	//! TODO: Currently not used.
	wxString		m_directory;
	//@}

	std::list<ClientStruct>	m_clients;
	uint32_t		m_clientID;
	uint16_t		m_clientPort;
	uint32_t		m_clientServerIP;
	uint16_t		m_clientServerPort;

	//! Kademlia publish information.
	uint32_t		m_kadPublishInfo;

	friend class CPartFile;
	friend class CSearchListRem;
};


#endif // SEARCHLIST_H
// File_checked_for_headers
