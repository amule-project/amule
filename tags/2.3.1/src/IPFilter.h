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

#ifndef IPFILTER_H
#define IPFILTER_H

#include <wx/event.h>	// Needed for wxEvent

#include "Types.h"	// Needed for uint8, uint16 and uint32

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
	
	/**
	 * True once initial startup has finished (stays true while reloading later).
	 */
	bool	IsReady() const { return m_ready; }
	
	/**
	 * These functions are called to tell the filter to start networks once it 
	 * has finished loading.
	 */
	void	StartKADWhenReady() { m_startKADWhenReady = true; }
	void	ConnectToAnyServerWhenReady() { m_connectToAnyServerWhenReady = true; }
	
private:
	/** Handles the result of loading the dat-files. */
	void	OnIPFilterEvent(CIPFilterEvent&);
	
	//! The URL from which the IP filter was downloaded
	wxString m_URL;
	
	// The IP ranges
	typedef std::vector<uint32> RangeIPs;
	RangeIPs m_rangeIPs;
	typedef std::vector<uint16> RangeLengths;
	RangeLengths m_rangeLengths;
	// Name for each range. This usually stays empty for memory reasons,
	// except if IP-Filter debugging is active.
	typedef std::vector<std::string> RangeNames;
	RangeNames m_rangeNames;

	//! Mutex used to ensure thread-safety of this class
	mutable wxMutex	m_mutex;

	// false if loading (on startup only)
	bool m_ready;
	// flags to start networks after loading
	bool m_startKADWhenReady;
	bool m_connectToAnyServerWhenReady;
	// should update be performed after filter is loaded ?
	bool m_updateAfterLoading;

	friend class CIPFilterEvent;
	friend class CIPFilterTask;

	DECLARE_EVENT_TABLE()
};

#endif
// File_checked_for_headers
