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

#include "Timer.h"		// Needed for CTimer
#include "ObservableQueue.h"	// Needed for CQueueObserver
#include "SearchFile.h"		// Needed for CSearchFile
#include <common/SmartPtr.h>	// Needed for CSmartPtr
#include <memory>		// Needed for std::unique_ptr
#include <set>		// Needed for std::set
#include <map>		// Needed for std::map

// Forward declarations
class PerSearchState;
class SearchTimeoutManager;

// Forward declare SearchTimeoutManager::SearchType enum for use in method signatures
// We need to use a separate enum to avoid circular dependency issues
enum SearchTimeoutType {
	TimeoutLocalSearch = 0,
	TimeoutGlobalSearch,
	TimeoutKadSearch
};

namespace search {
	class SearchAutoRetry;
	class SearchPackageValidator;
	class ED2KSearchPacketBuilder;
	class KadSearchPacketBuilder;
	class SearchResultHandler;
}


class CMemFile;
class CMD4Hash;
class CPacket;
class CServer;
class CSearchFile;

namespace Kademlia {
	class CUInt128;
}


enum SearchType {
	LocalSearch = 0,
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
		/** Prevents accidental use of uninitialized variables. */
		CSearchParams() { minSize = maxSize = availability = 0; searchType = LocalSearch; }

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
		//! The minimum available (source-count), zero for any.
		uint32_t availability;
		//! The type of search (Local, Global, or Kad)
		SearchType searchType;
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
	
	/** Stops a specific search by ID. */
	void StopSearch(long searchID, bool globalOnly = false);

	/** Returns the completion percentage of a specific search. */
	uint32 GetSearchProgress(long searchId) const;

	/**
	 * Requests more results for a specific search ID.
	 * This is a thread-safe method that can be called from any thread.
	 * 
	 * @param searchId The search ID to request more results for
	 * @return Empty string on success, error message on failure
	 */
	wxString RequestMoreResultsForSearch(long searchId);

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
	bool	AddToList(CSearchFile* toadd, bool clientResponse = false);


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
	void	ProcessSharedFileList(const uint8_t* packet, uint32 size, CUpDownClient* sender, bool* moreResultsAvailable, const wxString& directory);

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
	void SetKadSearchFinished();

	/** Get the next unique search ID */
	uint32 GetNextSearchID();

	/** Get the search parameters for a given search ID */
	CSearchParams GetSearchParams(long searchID);

	/** Request more results for a given search ID */
	wxString RequestMoreResults(long searchID);

	/** Request more results from a specific server */
	wxString RequestMoreResultsFromServer(const CServer* server, long searchID);

	//! This smart pointer is used to safely prevent leaks.
	typedef CSmartPtr<CMemFile> CMemFilePtr;

	/** Create a basic search-packet for the given search-type. */
	CMemFilePtr CreateSearchData(CSearchParams& params, SearchType type, bool supports64bit, bool& packetUsing64bit, const wxString& kadKeyword = wxEmptyString);

	/** Per-search state management methods */
	
	/**
	 * Create or get per-search state for a search ID
	 * 
	 * @param searchId The search ID
	 * @param searchType The search type
	 * @param searchString The search string
	 * @return Pointer to the PerSearchState, or nullptr on error
	 */
	::PerSearchState* getOrCreateSearchState(long searchId, SearchType searchType, const wxString& searchString);
	
	/**
	 * Get per-search state for a search ID
	 * 
	 * @param searchId The search ID
	 * @return Pointer to the PerSearchState, or nullptr if not found
	 */
	::PerSearchState* getSearchState(long searchId);
	
	/**
	 * Get per-search state for a search ID (const version)
	 * 
	 * @param searchId The search ID
	 * @return Pointer to the PerSearchState, or nullptr if not found
	 */
	const ::PerSearchState* getSearchState(long searchId) const;
	
	/**
	 * Remove per-search state for a search ID
	 *
	 * @param searchId The search ID to remove
	 * @param releaseId Whether to release the search ID for reuse (default: true)
	 */
	void removeSearchState(long searchId, bool releaseId = true);

	/**
	 * Check if a search state exists
	 *
	 * @param searchId The search ID to check
	 * @return true if the search state exists, false otherwise
	 */
	bool hasSearchState(long searchId) const;

	/**
	 * Get all active search IDs
	 *
	 * @return Vector of active search IDs
	 */
	std::vector<long> getActiveSearchIds() const;

	/**
	 * Map a Kad search ID to an original search ID
	 *
	 * @param kadSearchId The Kad search ID (0xffffff?? format)
	 * @param originalSearchId The original search ID
	 */
	void mapKadSearchId(uint32_t kadSearchId, long originalSearchId);

	/**
	 * Get the original search ID for a Kad search ID
	 *
	 * @param kadSearchId The Kad search ID (0xffffff?? format)
	 * @return The original search ID, or 0 if not found
	 */
	long getOriginalSearchId(uint32_t kadSearchId) const;

	/**
	 * Remove a Kad search ID mapping
	 *
	 * @param kadSearchId The Kad search ID to remove
	 */
	void removeKadSearchIdMapping(uint32_t kadSearchId);

private:
	/** Event-handler for search timers (both local timeout and global search). */
	void OnSearchTimer(CTimerEvent& evt);

	/** Handle search timeout from SearchTimeoutManager */
	void OnSearchTimeout(uint32_t searchId, SearchTimeoutType type, const wxString& reason);

	/** Validate and recover stuck searches */
	void ValidateAndRecoverSearches();


	//! Map of active searches and their per-search state
	//! This is the single source of truth for active searches
	std::map<long, std::unique_ptr<::PerSearchState>>	m_searchStates;

	//! Mutex for thread-safe access to search states
	mutable wxMutex m_searchMutex;

	//! Shorthand for the map of results (key is a SearchID).
	typedef std::map<long, CSearchResultList> ResultMap;

	//! Map of all search-results added.
	ResultMap	m_results;

	//! Map of search parameters for each search ID.
	typedef std::map<long, CSearchParams> ParamMap;
	ParamMap	m_searchParams;

	//! Map of Kad search IDs to original search IDs
	//! Kad uses special IDs in format 0xffffff??, but we need to route results
	//! to the original search ID used by SearchResultRouter
	typedef std::map<uint32_t, long> KadSearchIdMap;
	KadSearchIdMap	m_kadSearchIdMap;

	//! Search timeout manager for detecting and recovering stuck searches
	std::unique_ptr<SearchTimeoutManager> m_timeoutManager;

// Result handlers now managed by SearchResultRouter
// Package validators now used by controllers directly

	//! Handle search completion with auto-retry
	void OnSearchComplete(long searchId, SearchType type, bool hasResults);

	//! Handle retry callback from auto-retry manager
	void OnSearchRetry(long searchId, SearchType type, int retryNum);

	DECLARE_EVENT_TABLE()
};


#endif // SEARCHLIST_H
// File_checked_for_headers
