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

#ifndef IPFILTER_H
#define IPFILTER_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "IPFilter.h"
#endif

#include <wx/thread.h>	// Needed for wxMutex;

#include "types.h"		// Needed for uint8, uint16 and uint32
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
	 * Destructor.
	 */
	~CIPFilter();
	
	
	/**
	 * Clears the current list of IP-Ranges.
	 */
	void	RemoveAllIPs();

	/**
	 * Loads a IP-list from the specified text file.
	 */
	int		LoadFromDatFile(wxString file, bool merge);

	/**
	 * Loads a IP-list from the specified zip file.
	 */
	int		LoadFromZipFile(wxString file, bool merge);

	/**
	 * This function saves the list in memory to the file 'ipfilter.dat'.
	 */
	void	SaveToFile();

	
	/**
	 * Checks if a IP is filtered with the current list and AccessLevel.
	 *
	 * @param IP2test The IP-Address to test for.
	 * @return True if it is filtered, false otherwise.
	 *
	 * Note: IP2Test must be in network order.
	 */
	bool	IsFiltered( uint32 IP2test );

	
	/**
	 * Returns the description of the last range to be hit.
	 */
	const wxString& GetLastHit() const;
	
	/**
	 * Returns the number of banned ranges.
	 */
	uint32	BanCount() const;

	/**
	 * Reloads the current ipfilter.dat file, discarding the current list of ranges.
	 */
	void 	Reload();


	/**
	 * Starts a download of the ipfilter-list at the specified URL.
	 *
	 * @param A valid URL or an empty string to use the value set in preferences.
	 */
	void	Update( const wxString& strURL = wxEmptyString );

	/**
	 * This function is called when a download is completed.
	 */
	void	DownloadFinished(uint32 result);
	
private:
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
	void	AddIPRange(uint32 IPstart, uint32 IPend, uint16 AccessLevel, const wxString& Description);


	/**
	 * Helper function.
	 * 
	 * @param str A string representation of an IP-range in the format "<ip>-<ip>".
	 * @param ipA The target of the first IP in the range.
	 * @param ipB The target of the second IP in the range.
	 * @return True if the parsing succeded, false otherwise (results will be invalid).
	 *
	 * This function parses a string containing a IP-range and saves the two IP-
	 * addresses into the specified uint32s. It will ignore whitespace around the
	 * addresses, but otherwise perform strict error-checking.
	 *
	 * The original purpose of this function was to provide support for IP-strings
	 * with leading zeroes, which would confuce inet_aton, since it would interpret
	 * it as an octal rather than a deciman. The current version has also been 
	 * optimized for speed and is a fast mutter-farker.
	 *
	 * The IPs returned by this function are in host order, not network order.
	 */
	bool	m_inet_atoh(const wxString &str, uint32& ipA, uint32& ipB);

	/**
	 * Helper function.
	 *
	 * @param str The string to be tokenized.
	 * @param token The token used to split the string.
	 * @param pos Position which to start after.
	 * @return The last found token, or the entire string if there are no tokens.
	 *
	 * This function will return the string after pos to the next instance of
	 * token and set pos to the position of that token. If no tokens are found
	 * the rest of the string is returned and pos set to -1.
	 */
	wxString GetNextToken( const wxString& str, wxChar token, int& pos );

	/**
	 * Helper function.
	 *
	 * @param str A string representing an unsigned long, possibly with whitespace.
	 * @param i The target of the value represented in the string.
	 * @return true if the converstion succeded, false otherwise.
	 *
	 * This function will transform a string representation of a unsigned long
	 * into an actual unsigned long, while ignoring any whitespace found in the
	 * string. This is faster than having to call wxString::Strip first.
	 */
	bool StrToU( const wxString& str, unsigned& i );


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
	
	//! The map of IP-anges
	IPMap m_iplist;

	//! The description belonging to the last range that was hit in IsFiltered
	wxString m_lasthit;

	//! Mutex used to ensure thread-safety of this class
	mutable wxMutex	m_mutex;
};

#endif
