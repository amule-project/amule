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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef IPFILTER_H
#define IPFILTER_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "IPFilter.h"
#endif

#include <wx/thread.h>	// Needed for wxMutex;

#include "Types.h"		// Needed for uint8, uint16 and uint32
#include "RangeMap.h"	// Needed for CRangeMap


/**
 * This class represents a list of IPs that should not be accepted
 * as valid connection destinations nor sources. It provides an
 * interface to query whenever or not a specific an specific IP
 * is filtered.
 *
 * Currently this class can handle IPRange files in the Peer-Guardian
 * format and the AntiP2P format, read from either text files or text
 * files compressed with the zip compression format.
 *
 * This class is thread-safe.
 */
class CIPFilter
{
public:
	/**
	 * Constructor.
	 */
	CIPFilter();

	
	/**
	 * Clears the current list of IP-Ranges.
	 */
	void	RemoveAllIPs();


	/**
	 * Checks if a IP is filtered with the current list and AccessLevel.
	 *
	 * @param IP2test The IP-Address to test for.
	 * @return True if it is filtered, false otherwise.
	 *
	 * Note: IP2Test must be in anti-host order (BE on LE platform, LE on BE platform).
	 */
	bool	IsFiltered( uint32 IP2test );

	
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
	// Contains the number of ranges added and the number of lines discarded while loading.
	typedef std::pair<size_t, size_t> AddedAndDiscarded;
	
	/**
	 * Loads a IP-list from the specified file, can be text or zip.
	 */
	void LoadFromFile(const wxString& file);
	
	/**
	 * Helper-function, loads a IP-list from the text file.
	 */
	AddedAndDiscarded LoadFromDatFile( const wxString& file );

	/**
	 * Helper-function for processing the AntiP2P format.
	 *
	 * @return True if the line was valid, false otherwise.
	 * 
	 * This function will correctly parse files that follow the folllowing
	 * format for specifying IP-ranges (whitespace is optional):
	 *  <Description> : <IPStart> - <IPEnd>
	 */
	bool	ProcessAntiP2PLine( const wxString& sLine );
	
	/**
	 * Helper-function for processing the PeerGuardian format.
	 *
	 * @return True if the line was valid, false otherwise.
	 * 
	 * This function will correctly parse files that follow the folllowing
	 * format for specifying IP-ranges (whitespace is optional):
	 *  <IPStart> - <IPEnd> , <AccessLevel> , <Description>
	 */
	bool	ProcessPeerGuardianLine( const wxString& sLine );
	
	/**
	 * Helper function.
	 *
	 * @param IPstart The start of the IP-range.
	 * @param IPend The end of the IP-range, must be less than or equal to IPstart.
	 * @param AccessLevel The AccessLevel of this range.
	 * @param Description The assosiated description of this range.
	 * 
	 * This function inserts the specified range into the IPMap.
	 */
	bool	AddIPRange(uint32 IPstart, uint32 IPend, uint16 AccessLevel, const wxString& Description);


	/**
	 * Helper function.
	 * 
	 * @param str A string representation of an IP-range in the format "<ip>-<ip>".
	 * @param ipA The target of the first IP in the range.
	 * @param ipB The target of the second IP in the range.
	 * @return True if the parsing succeded, false otherwise (results will be invalid).
	 *
	 * The IPs returned by this function are in host order, not network order.
	 */
	bool	m_inet_atoh(const wxString &str, uint32& ipA, uint32& ipB);


	/**
	 * This structure is used to contain the range-data in the rangemap.
	 */
	struct rangeObject
	{
		bool operator==( const rangeObject& other ) const {
			return AccessLevel == other.AccessLevel;
		}
		
		//! The AccessLevel for this filter.
		uint8		AccessLevel;
		//! Contains the user-description of the range.
		wxString	Description;
	};

	
	//! The is the type of map used to store the IPs.
	typedef CRangeMap< rangeObject > IPMap;
	
	//! The map of IP-ranges
	IPMap m_iplist;

	//! Mutex used to ensure thread-safety of this class
	mutable wxMutex	m_mutex;
};

#endif
