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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "IPFilter.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _()
#include <wx/filefn.h>		// Needed for wxFileExists
#include <wx/textfile.h>	// Needed for wxTextFile
#include <wx/string.h>		// Needed for wxString
#include <wx/txtstrm.h>		// Needed for wxTextInputStream
#include <wx/zipstrm.h>		// Needed for wxZipInputStream

#include "IPFilter.h"		// Interface declarations.
#include "NetworkFunctions.h"
#include "Preferences.h"	// Needed for CPreferences
#include "amule.h"			// Needed for theApp
#include "Statistics.h"		// Needed for CStatistics
#include "HTTPDownload.h"
#include "GetTickCount.h"

CIPFilter::CIPFilter()
{
	LoadFromDatFile(theApp.ConfigDir + wxT("ipfilter.dat"), false); // No merge on construction
	
	if (thePrefs::IPFilterAutoLoad()) {
		Update();
	}
}


CIPFilter::~CIPFilter()
{
	RemoveAllIPs();
}


void CIPFilter::Reload()
{
	RemoveAllIPs();
	
	LoadFromDatFile(theApp.ConfigDir + wxT("ipfilter.dat"), false); // No merge on reload.
}


uint32 CIPFilter::BanCount() const
{
	wxMutexLocker lock( m_mutex );

	return m_iplist.size();
}


void CIPFilter::AddIPRange(uint32 IPStart, uint32 IPEnd, uint16 AccessLevel, const wxString& Description)
{
	wxASSERT(AccessLevel < 256);
	wxASSERT(IPStart <= IPEnd);

	rangeObject item;
	item.AccessLevel = AccessLevel;
	item.Description = Description;

	m_iplist.insert( IPStart, IPEnd, item );
}


bool CIPFilter::m_inet_atoh( const wxString &str, uint32& ipA, uint32& ipB )
{
	// Empty strings would cause problems due to the way I use pointers
	if ( str.IsEmpty() ) {
		return false;
	}
	
	// Ensure IPs are zero'd
	ipA = ipB = 0;
	
	// Details if the last char was a digit
	bool lastIsDigit = false;
	
	// The current position in the current field, used to detect malformed fields (x.y..z).
	int digit = 0;

	// The current field, used to ensure only IPs that looks like a.b.c.d are supported
	int field = 0;

	// The value of the current field
	int value = 0;

	// Some ptr magic. Direct pointer access is faster in this case.
	const wxChar* ptr = str.c_str();
	wxChar c;

	while ( ( c = *ptr++ ) ) {
		if ( c >= wxT('0') && c <= wxT('9') ) {
			// Avoid whitespace inside numbers, ie a.b.XY Z.d
			if ( lastIsDigit || !digit ) {
				value = ( value * 10 ) + ( c - wxT('0') );
				++digit;
				lastIsDigit = true;
			} else {
				// There was a whitespace inside a number, bail
				return false;
			}
		} else if ( c == wxT('.') || c == wxT('-') ) {
			// Ensure that the split happens at the right place
			if ( field == 3 && c != wxT('-') ) {
				return false;
				// There must have been at least one digit and value must be valid
			} else if ( digit && value <= 255 ) {
				// Add the current field to either the first or the second IP
				if ( field < 4 ) {
					ipA = ( ipA << 8 ) | value;
				} else {
					ipB = ( ipB << 8 ) | value;
				}

				// Rest the current field values
				value = digit = 0;
				++field;
			} else {
				return false;
			}
		} else if ( c != wxT(' ') && c != wxT('\t') ) {
			// Something in the string, which isn't whitespace
			return false;
		} else {
			lastIsDigit = false;
		}
	}

	// Add the last field to the second IP
	ipB = ( ipB << 8 ) | value;

	// The only valid possibility is 8 fields.
	return ( field == 7 );
}


wxString CIPFilter::GetNextToken( const wxString& str, wxChar token, int& cur )
{
	// Empty strings would cause problems due to the way I use pointers
	if ( str.IsEmpty() ) {
		cur = -1;
		return wxEmptyString;
	}

	int current = ( cur < 0 ? 0 : cur );

	const wxChar* ptr = str.c_str() + cur;
	const wxChar* end = str.c_str() + str.Len() + 1;

	while ( ++ptr < end ) {
		if ( *ptr == token ) {
			cur = ptr - str.c_str() + 1;

			return str.Mid( current, ptr - ( str.c_str() + current ) );
		}
	}

	// Move back to beginning, thus starting the circle over
	cur = -1;

	// Return the last token
	return str.Mid(  current );
}


bool CIPFilter::StrToU( const wxString& str, unsigned& i )
{
	// Empty strings would cause problems due to the way I use pointers
	if ( str.IsEmpty() ) {
		return false;
	}

	// The current digit-number and reset i
	int digit = i = 0;
	// If the last char was a digit. Used to avoid stuff like "12 3".
	bool lastIsDigit = false;

	// Direct ptr-access is used for the sake of speed.
	const wxChar* ptr = str.c_str();
	wxChar c;

	while ( ( c = *ptr++ ) ) {
		if ( c >= wxT('0') && c <= wxT('9') ) {
			if ( lastIsDigit || !digit ) {
				i = i * 10 + ( c - wxT('0') );
				++digit;
				lastIsDigit = true;
			} else {
				return false;
			}
		} else if ( c != wxT(' ') && c != wxT('\t') ) {
			return false;
		} else {
			lastIsDigit = false;
		}
	}

	return digit;
}


bool CIPFilter::ProcessPeerGuardianLine( const wxString& sLine )
{
	if ( sLine.Len() < 18 ) {
		return false;
	}

	int current = -1;
	wxString first	= GetNextToken( sLine, wxT(','), current );
	wxString second	= GetNextToken( sLine, wxT(','), current );

	// If there was only one or two tokens, then fail
	if ( current == -1 ) {
		return false;
	}

	wxString third;
	if ( current < (int)sLine.Len() ) {
		third = sLine.Mid( current + 1 ).Strip( wxString::both );
	}

	// Convert string IP's to host order IP numbers
	uint32 IPStart = 0;
	uint32 IPEnd   = 0;

	// This will also fail if the line is commented out
	if ( !m_inet_atoh( first, IPStart, IPEnd ) ) {
		return false;
	}

	// Second token is Access Level, default is 0.
	unsigned AccessLevel = 0;
	if ( !StrToU( second, AccessLevel ) || AccessLevel >= 255 ) {
		return false;
	}

	// Add the filter
	AddIPRange( IPStart, IPEnd, AccessLevel, third );

	return true;
}


bool CIPFilter::ProcessAntiP2PLine( const wxString& sLine )
{
	// remove spaces from the left and right.
	const wxString line = sLine.Strip(wxString::leading);

	// Ignore comments, too short lines and other odd stuff
	// The theoretical smallest line would be ":0.0.0.0-0.0.0.0"
	if ( line.Len() < 16 ) {
		return false;
	}

	// Extract description (first) and IP-range (second) form the line
	int pos = line.Find( wxT(':'), true );
	if ( pos == -1 ) {
		return false;
	}

	wxString Description = line.Left( pos ).Strip( wxString::trailing );
	wxString IPRange     = line.Right( line.Len() - pos - 1 );

	// Convert string IP's to host order IP numbers
	uint32 IPStart = 0;
	uint32 IPEnd   = 0;

	if ( !m_inet_atoh( IPRange ,IPStart, IPEnd ) ) {
		return false;
	}

	// Add the filter
	AddIPRange( IPStart, IPEnd, 0, Description );

	return true;
}


int CIPFilter::LoadFromZipFile( wxString file, bool merge )
{
	int filtercounter = 0;
	if ( !merge ) {
		RemoveAllIPs();
	}
	
	if ( wxFileExists( file ) ) {
		wxMutexLocker lock( m_mutex );
		
		wxZipInputStream inputStream( file, wxT("ipfilter.dat") );
		wxBufferedInputStream buffer( inputStream );
		wxTextInputStream stream( buffer );

		// Function pointer-type of the parse-functions we can use
		typedef bool (CIPFilter::*ParseFunc)(const wxString&);

		// Default parser is IPFilter.dat format
		ParseFunc func = NULL;

		while ( !inputStream.Eof() ) {
			wxString line = stream.ReadLine();
				
			if ( func ) {
				if ( (*this.*func)( line ) ) {
						filtercounter++;
				}
			} else {
				// Select the parser that can handle this file
				if ( ProcessPeerGuardianLine( line ) ) {
					func = &CIPFilter::ProcessPeerGuardianLine;
					filtercounter++;
				} else if ( ProcessAntiP2PLine( line ) ) {
					func = &CIPFilter::ProcessAntiP2PLine;
					filtercounter++;
				}
			}
		}
	}

	AddLogLineM(true, wxString::Format(_("Loaded ipfilter with %d new IP addresses."), filtercounter));


	if (merge) {
		SaveToFile();
	}

	return filtercounter;
}


int CIPFilter::LoadFromDatFile(wxString file, bool merge)
{
	int filtercounter = 0;
	if ( !merge ) {
		RemoveAllIPs();
	}

	wxTextFile readFile(file);
	if( readFile.Exists() && readFile.Open() ) {
		wxMutexLocker lock( m_mutex );
	
		// Function pointer-type of the parse-functions we can use
		typedef bool (CIPFilter::*ParseFunc)(const wxString&);

		// Default parser is IPFilter.dat format
		ParseFunc func = NULL;

		for ( size_t i = 0; i < readFile.GetLineCount(); i++ ) {
			wxString line = readFile.GetLine( i );

			if ( func ) {
				if ( (*this.*func)( line ) ) {
					filtercounter++;
				}
			} else {
				// Select the parser that can handle this file
				if ( ProcessPeerGuardianLine( line ) ) {
					func = &CIPFilter::ProcessPeerGuardianLine;
					filtercounter++;
				} else if ( ProcessAntiP2PLine( line ) ) {
					func = &CIPFilter::ProcessAntiP2PLine;
					filtercounter++;
				}
			}
		}
	}
	
	AddLogLineM(true, wxString::Format(_("Loaded ipfilter with %d new IP addresses."), filtercounter));

	if (merge) {
		SaveToFile();
	}

	return filtercounter;
}


void CIPFilter::SaveToFile()
{
	wxMutexLocker lock( m_mutex );

	wxString IPFilterName = theApp.ConfigDir + wxT("ipfilter.dat");
	wxTextFile IPFilterFile(IPFilterName + wxT(".new"));
	if (IPFilterFile.Exists()) {
		// We're gonna do a new one, baby
		::wxRemoveFile(IPFilterName + wxT(".new"));
	}

	// Create the new ipfilter.
	IPFilterFile.Create();

	IPMap::iterator it = m_iplist.begin();
	for ( ; it != m_iplist.end(); ++it ) {
		wxString line;
		wxString ipA = Uint32toStringIP( wxUINT32_SWAP_ALWAYS( it.keyStart() ) );
		wxString ipB = Uint32toStringIP( wxUINT32_SWAP_ALWAYS( it.keyEnd() ) );

		// Range Start
		line << ipA.Pad( 15 - ipA.Len() )
		// Range Seperator
		<< wxT(" - ")
		// Range End
		<< ipB.Pad( 15 - ipB.Len() )
		// Token separator
		<< wxT(" , ")
		// Access level
		<< (int)it->AccessLevel
		// Token separator
		<< wxT(" , ")
		// Description
		<< it->Description;

		// Add it to file...
		IPFilterFile.AddLine(line);
	}

	// Close & write the file
	IPFilterFile.Write();
	IPFilterFile.Close();

	// Remove old ipfilter
	::wxRemoveFile(IPFilterName);

	// Make new ipfilter the default one.
	wxRenameFile(IPFilterName + wxT(".new"),IPFilterName);
}


void CIPFilter::RemoveAllIPs()
{
	wxMutexLocker lock( m_mutex );

	m_iplist.clear();
}


bool CIPFilter::IsFiltered(uint32 IPTest)
{
	if ( thePrefs::GetIPFilterOn() ) {
		wxMutexLocker lock( m_mutex );
	
		IPTest = ntohl(IPTest);

		IPMap::iterator it = m_iplist.find_range( IPTest );

		if ( it != m_iplist.end() ) {
			if ( it->AccessLevel < thePrefs::GetIPFilterLevel() ) {
				AddDebugLogLineM( true, wxT("Filtered IP: ") + Uint32toStringIP( IPTest ) + wxT("(") + it->Description + wxT(")") );
				theApp.statistics->AddFilteredClient();
				
				return true;
			}
		}
	}

	return false;
}


void CIPFilter::Update( const wxString& strURL )
{
	wxString URL = strURL.IsEmpty() ? thePrefs::IPFilterURL() : strURL;
	
	if ( !URL.IsEmpty() ) {
		wxString filename = theApp.ConfigDir + wxT("ipfilter.download");
		CHTTPDownloadThread *downloader = new CHTTPDownloadThread( URL, filename, HTTP_IPFilter );
		
		downloader->Create();
		
		downloader->Run();
	}
}


void CIPFilter::DownloadFinished(uint32 result)
{
	if ( result == 1 ) {
		// curl succeeded. proceed with ipfilter loading
		wxString filename = theApp.ConfigDir + wxT("ipfilter.download");
		
#warning Needs improvement. Kry?
		// Simply try to load as a Zip file first, then as a Dat file, I check Zip
		// first since that is much better at detecting whenever or not it is a zip-file.
		if ( !LoadFromZipFile( filename, true ) ) {
			if ( !LoadFromDatFile( filename, true ) ) {
				AddLogLineM(true, _("Failed to download the ipfilter from ") + thePrefs::IPFilterURL());
			}
		}
		
		// Remove the now unused file
		wxRemoveFile( filename );
	} else {
		AddLogLineM(true, _("Failed to download the ipfilter from ") + thePrefs::IPFilterURL());
	}
}

