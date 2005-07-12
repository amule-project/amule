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
#include <wx/fs_zip.h>		// Needed for wxZipFSHandler

#include "IPFilter.h"		// Interface declarations.
#include "StringFunctions.h"
#include "NetworkFunctions.h"
#include "Preferences.h"	// Needed for CPreferences
#include "amule.h"			// Needed for theApp
#include "Statistics.h"		// Needed for CStatistics
#include "HTTPDownload.h"
#include "GetTickCount.h"
#include "CFile.h"
#include "Logger.h"


CIPFilter::CIPFilter()
{
	LoadFromFile( theApp.ConfigDir + wxT("ipfilter.dat"), false );
	
	if (thePrefs::IPFilterAutoLoad()) {
		Update(thePrefs::IPFilterURL());
	}
}


CIPFilter::~CIPFilter()
{
	RemoveAllIPs();
}


void CIPFilter::Reload()
{
	RemoveAllIPs();
	
	LoadFromFile( theApp.ConfigDir + wxT("ipfilter.dat"), false );
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


bool CIPFilter::m_inet_atoh(const wxString &str, uint32& ipA, uint32& ipB)
{
	wxString first = str.BeforeFirst(wxT('-'));
	wxString second = str.Mid(first.Len() + 1);

	bool result = StringIPtoUint32(first, ipA) && StringIPtoUint32(second, ipB);

	// StringIPtoUint32 saves the ip in anti-host order, but in order
	// to be able to make relational comparisons, we need to convert
	// it back to host-order.
	ipA = wxUINT32_SWAP_ALWAYS(ipA);
	ipB = wxUINT32_SWAP_ALWAYS(ipB);
	
	return result;
}


bool CIPFilter::ProcessPeerGuardianLine( const wxString& sLine )
{
	CSimpleTokenizer tkz(sLine, wxT(','));
	
	wxString first	= tkz.next();
	wxString second	= tkz.next();
	wxString third  = tkz.remaining().Strip(wxString::both);

	// If there were less than two tokens, fail
	if (tkz.tokenCount() != 2) {
		return false;
	}

	// Convert string IP's to host order IP numbers
	uint32 IPStart = 0;
	uint32 IPEnd   = 0;

	// This will also fail if the line is commented out
	if ( !m_inet_atoh( first, IPStart, IPEnd ) ) {
		return false;
	}

	// Second token is Access Level, default is 0.
	unsigned long AccessLevel = 0;
	if (!second.Strip(wxString::both).ToULong(&AccessLevel) || AccessLevel >= 255) {
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


int CIPFilter::LoadFromFile( const wxString& file, bool merge )
{
	int filtercounter = 0;

	if ( !merge ) {
		RemoveAllIPs();
	}

	if ( wxFileExists( file ) ) {
		// Attempt to discover the filetype
		if ( IsZipFile( file ) ) {
			filtercounter = LoadFromZipFile( file );
		} else {
			filtercounter = LoadFromDatFile( file );
		}
	
		AddLogLineM(true, wxString::Format(_("Loaded ipfilter with %d new IP addresses."), filtercounter));
	}

	if (merge) {
		SaveToFile();
	}

	return filtercounter;
}


bool CIPFilter::IsZipFile( const wxString& filename )
{
	CFile file( filename, CFile::read );
	char head[2];

	if ( file.Read( head, 2 ) == 2 ) {
		// Zip-archives have a header of "PK".
		return ( head[0] == 'P' ) && ( head[1] == 'K' );
	}

	return false;
}


int CIPFilter::LoadFromZipFile( const wxString& file )
{
	int filtercounter = 0;
	
	if ( wxFileExists( file ) ) {
		wxMutexLocker lock( m_mutex );
	
		// Try to load every file in the archive
		wxZipFSHandler archive; 
		wxString filename = archive.FindFirst( file + wxT("#file:/*"), wxFILE );

		while ( !filename.IsEmpty() ) {
			// Extract the filename part of the URI
			filename = filename.AfterLast( ':' );
			
			wxZipInputStream inputStream( file, filename );
			wxBufferedInputStream buffer( inputStream );
			wxTextInputStream stream( buffer );

			// Function pointer-type of the parse-functions we can use
			typedef bool (CIPFilter::*ParseFunc)(const wxString&);

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
			
			filename = archive.FindNext();
		}
	}

	return filtercounter;
}


int CIPFilter::LoadFromDatFile( const wxString& file )
{
	int filtercounter = 0;
	
	wxTextFile readFile(file);
	if( readFile.Exists() && readFile.Open() ) {
		wxMutexLocker lock( m_mutex );
	
		// Function pointer-type of the parse-functions we can use
		typedef bool (CIPFilter::*ParseFunc)(const wxString&);

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
	
		IPTest = wxUINT32_SWAP_ALWAYS(IPTest);

		IPMap::iterator it = m_iplist.find_range( IPTest );

		if ( it != m_iplist.end() ) {
			if ( it->AccessLevel < thePrefs::GetIPFilterLevel() ) {
				AddDebugLogLineM( false, logIPFilter, wxT("Filtered IP: ") + Uint32toStringIP( wxUINT32_SWAP_ALWAYS(IPTest) ) + wxT(" (") + it->Description + wxT(")") );
				theApp.statistics->AddFilteredClient();
				
				return true;
			}
		}
	}

	return false;
}


void CIPFilter::Update(const wxString& strURL)
{
	if ( !strURL.IsEmpty() ) {
		wxString filename = theApp.ConfigDir + wxT("ipfilter.download");
		CHTTPDownloadThread *downloader = new CHTTPDownloadThread( strURL, filename, HTTP_IPFilter );
		
		downloader->Create();
		
		downloader->Run();
	}
}


void CIPFilter::DownloadFinished(uint32 result)
{
	if ( result == 1 ) {
		// curl succeeded. proceed with ipfilter loading
		wxString filename = theApp.ConfigDir + wxT("ipfilter.download");
		
		// Load the downloaded file, which may be a zip archive
		if ( !LoadFromFile( filename, true ) ) {
			AddDebugLogLineM( true, logIPFilter, wxT("Failed to load the ipfilter from ") + thePrefs::IPFilterURL());
		}
		
		// Remove the now unused file
		wxRemoveFile( filename );
	} else {
		AddDebugLogLineM(true, logIPFilter, wxT("Failed to download the ipfilter from ") + thePrefs::IPFilterURL());
	}
}
