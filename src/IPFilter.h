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

#ifndef IPFILTER_H
#define IPFILTER_H

#include <wx/event.h>	// Needed for wxEvent

#include "Types.h"	// Needed for uint8, uint16 and uint32
#include "RangeMap.h"	// Needed for CRangeMap


class CIPFilterEvent;


/**
 * This class represents a list of IPs that should not be accepted
 * as valid connection destinations nor sources. It provides an
 * interface to query whether or not a specific IP is filtered.
 *
 * Currently this class can handle IPRange files in the Peer-Guardian
 * format and the AntiP2P format, read from either text files or text
 * files compressed with the zip compression format.
 *
 * This class is thread-safe.
 */
class CIPFilter : public wxEvtHandler
{
public:
	/**
	 * Constructor.
	 */
	CIPFilter();

	/**
	 * Checks if a IP is filtered with the current list and AccessLevel.
	 *
	 * @param IP2test The IP-Address to test for.
	 * @param isServer Whether this IP belongs to a server or a client. Needed for statistical purposes only.
	 * @return True if it is filtered, false otherwise.
	 *
	 * Note: IP2Test must be in anti-host order (BE on LE platform, LE on BE platform).
	 */
	bool	IsFiltered( uint32 IP2test, bool isServer = false );

	
	/**
	 * Returns the number of banned ranges.
	 */
	uint32	BanCount() const;

	/**
	 * Reloads the ipfilter files, discarding the current list of ranges.
	 */
	void 	Reload();

	/**
	 * Starts a download of the ipfilter-list at the specified URL.
	 *
	 * @param A valid URL.
	 *
	 * Once the file has been downloaded, the ipfilter.dat file 
	 * will be replaced with the new file and Reload will be called.
	 */
	void	Update(const wxString& strURL);

	/**
	 * This function is called when a download is completed.
	 */
	void	DownloadFinished(uint32 result);
	
private:
	/** Handles the result of loading the dat-files. */
	void	OnIPFilterEvent(CIPFilterEvent&);
	
	/**
	 * This structure is used to contain the range-data in the rangemap.
	 */
	struct rangeObject
	{
		bool operator==( const rangeObject& other ) const {
			return AccessLevel == other.AccessLevel;
		}

// Since descriptions are only used for debugging messages, there 
// is no need to keep them in memory when running a non-debug build.
#ifdef __DEBUG__
		//! Contains the user-description of the range.
		wxString	Description;
#endif
		
		//! The AccessLevel for this filter.
		uint8		AccessLevel;
	};

	
	//! The is the type of map used to store the IPs.
	typedef CRangeMap<rangeObject, uint32> IPMap;
	
	//! The map of IP-ranges
	IPMap m_iplist;

	//! Mutex used to ensure thread-safety of this class
	mutable wxMutex	m_mutex;

	friend class CIPFilterEvent;
	friend class CIPFilterTask;

	DECLARE_EVENT_TABLE();
};

#endif
// File_checked_for_headers
