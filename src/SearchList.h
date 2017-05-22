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

#ifndef SEARCHLIST_H
#define SEARCHLIST_H

#include "Timer.h"				// Needed for CTimer
#include "ObservableQueue.h"	// Needed for CQueueObserver
#include "SearchFile.h"			// Needed for CSearchFile
#include <memory>		// Do_not_auto_remove (lionel's Mac, 10.3)


class CMemFile;
class CMD4Hash;
class CPacket;
class CServer;
class CSearchFile;

namespace Kademlia {
	class CUInt128;
}


enum SearchType {
	LocalSearch,
	GlobalSearch,
	KadSearch
};


typedef std::vector<CSearchFile*> CSearchResultList;


class CSearchList : public wxEvtHandler
{
public:
	//! Structure used to pass search-parameters.
	struct CSearchParams
	{
		/** Prevents accidential use of uninitialized variables. */
		CSearchParams() { minSize = maxSize = availability = 0; }

		//! The actual string to search for.
		wxString searchString;
		//! The keyword selected for Kad search
		wxString strKeyword;
		//! The type of files to search for (may be empty), one of ED2KFTSTR_*
		wxString typeText;
		//! The filename extension. May be empty.
		wxString extension;
		//! The smallest filesize in bytes to accept, zero for any.
		uint64_t minSize;
		//! The largest filesize in bytes to accept, zero for any.
		uint64_t maxSize;
		//! The minumum available (source-count), zero for any.
		uint32_t availability;
	};

	/** Constructor. */
	CSearchList();

	/** Frees any remaining search-results. */
	~CSearchList();

	/**
	 * Starts a new search.
	 *
	 * @param searchID The ID of the search, which may be modified.
	 * @param type The type of search, see SearchType.
	 * @param params The search parameters, see CSearchParams.
	 * @return An empty string on success, otherwise an error-message.
	 */
	wxString StartNewSearch(uint32* searchID, SearchType type, CSearchParams& params);

	/** Stops the current search (global or Kad), if any is in progress. */
	void StopSearch(bool globalOnly = false);

	/** Returns the completion percentage of the current search. */
	uint32 GetSearchProgress() const;

	/** This function is called once the local (ed2k) search has ended. */
	void	LocalSearchEnd();


	/**
	 * Returns the list of results for the specified search.
	 *
	 * If the search is not valid, an empty list is returned.
	 */
	const	CSearchResultList& GetSearchResults(long searchID) const;

	/** Removes all results for the specified search. */
	void	RemoveResults(long searchID);


	/** Finds the search-result (by hash) and downloads it in the given category. */
	void	AddFileToDownloadByHash(const CMD4Hash& hash, uint8 category = 0);


	/**
	 * Processes a list of shared files from a client.
	 *
	 * @param packet The raw packet received from the client.
	 * @param size the length of the packet.
	 * @param sender The sender of the packet.
	 * @param moreResultsAvailable Set to a value specifying if more results are available.
	 * @param directory The directory containing the shared files.
	 */
	void	ProcessSharedFileList(const byte* packet, uint32 size, CUpDownClient* sender, bool* moreResultsAvailable, const wxString& directory);

	/**
	 * Processes a search-result sent via TCP from the local server. All results are added.
	 *
	 * @param packet The packet containing one or more search-results.
	 * @param size the length of the packet.
	 * @param optUTF8 Specifies if the server supports UTF8.
	 * @param serverIP The IP of the server sending the results.
	 * @param serverPort The Port of the server sending the results.
	 */
	void	ProcessSearchAnswer(const uint8_t* packet, uint32_t size, bool optUTF8, uint32_t serverIP, uint16_t serverPort);

	/**
	 * Processes a search-result sent via UDP. Only one result is read from the packet.
	 *
	 * @param packet The packet containing one or more search-results.
	 * @param optUTF8 Specifies if the server supports UTF8.
	 * @param serverIP The IP of the server sending the results.
	 * @param serverPort The Port of the server sending the results.
	 */
	void	ProcessUDPSearchAnswer(const CMemFile& packet, bool optUTF8, uint32 serverIP, uint16 serverPort);


	/**
	 * Adds a result in the form of a kad search-keyword to the specified result-list.
	 *
	 * @param searchID The search to which this result belongs.
	 * @param fileID The hash of the result-file.
	 * @param name The filename of the result.
	 * @param size The filesize of the result.
	 * @param type The filetype of the result (TODO: Not used?)
	 * @param kadPublishInfo The kademlia publish information of the result.
	 * @param taglist List of additional tags associated with the search-result.
	 */
	void	KademliaSearchKeyword(uint32_t searchID, const Kademlia::CUInt128 *fileID, const wxString& name, uint64_t size, const wxString& type, uint32_t kadPublishInfo, const TagPtrList& taglist);

	/** Update a certain search result in all lists */
	void UpdateSearchFileByHash(const CMD4Hash& hash);

	/** Mark current KAD search as finished */
	void SetKadSearchFinished() { m_KadSearchFinished = true; }

private:
	/** Event-handler for global searches. */
	void OnGlobalSearchTimer(CTimerEvent& evt);

	/**
	 * Adds the specified file to the current search's results.
	 *
	 * @param toadd The result to add.
	 * @param clientResponse Is the result sent by a client (shared-files list).
	 * @return True if the results were added, false otherwise.
	 *
	 * Note that this function takes ownership of the CSearchFile object,
	 * regardless of whenever or not it was actually added to the results list.
	 */
	bool AddToList(CSearchFile* toadd, bool clientResponse = false);

	//! This auto-pointer is used to safely prevent leaks.
	typedef std::auto_ptr<CMemFile> CMemFilePtr;

	/** Create a basic search-packet for the given search-type. */
	CMemFilePtr CreateSearchData(CSearchParams& params, SearchType type, bool supports64bit, bool& packetUsing64bit);


	//! Timer used for global search intervals.
	CTimer	m_searchTimer;

	//! The current search-type, regarding the last/current search.
	SearchType	m_searchType;

	//! Specifies if a search is being performed.
	bool		m_searchInProgress;

	//! The ID of the current search.
	long		m_currentSearch;

	//! The current packet used for searches.
	CPacket*	m_searchPacket;

	//! Does the current search packet contain 64bit values?
	bool		m_64bitSearchPacket;

	//! If the current search is a KAD search this signals if it is finished.
	bool		m_KadSearchFinished;

	//! Queue of servers to ask when doing global searches.
	//! TODO: Replace with 'cookie' system.
	CQueueObserver<CServer*> m_serverQueue;

	//! Shorthand for the map of results (key is a SearchID).
	typedef std::map<long, CSearchResultList> ResultMap;

	//! Map of all search-results added.
	ResultMap	m_results;

	//! Contains the results type desired in the current search.
	//! If not empty, results of different types are filtered.
	wxString	m_resultType;


	DECLARE_EVENT_TABLE()
};


#endif // SEARCHLIST_H
// File_checked_for_headers
