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

#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/file.h>

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _()
#include <wx/textfile.h>
#include <wx/arrimpl.cpp>	// this is a magic incantation which must be done!
#include <wx/string.h>		// for wxString
#include <wx/tokenzr.h>		// for wxTokenizer


#if defined(__WXMSW__)
  #include <winsock2.h>
#else
  #include <sys/socket.h>	// for inet_aton() and struct in_addr
  #include <netinet/in.h>
  #include <arpa/inet.h>
#endif

#include "IPFilter.h"		// Interface declarations.
#include "NetworkFunctions.h"
#include "Preferences.h"	// Needed for CPreferences
#include "amule.h"		// Needed for theApp
#include "HTTPDownload.h"
#include "GetTickCount.h"

CIPFilter::CIPFilter(){
	lasthit = wxEmptyString;
	LoadFromFile(theApp.ConfigDir + wxT("ipfilter.dat"), false); // No merge on construction
	if (thePrefs::IPFilterAutoLoad()) {
		Update();	
	}
}

CIPFilter::~CIPFilter(){
	RemoveAllIPs();
}

void CIPFilter::Reload(){
	RemoveAllIPs();
	lasthit = wxEmptyString;
	LoadFromFile(theApp.ConfigDir + wxT("ipfilter.dat"), false); // No merge on reload.
}


/* *
 * IPFilter is an map of IPRanges, ordered by IPstart, of banned ranges of IP addresses.
 */
void CIPFilter::AddBannedIPRange(uint32 IPStart, uint32 IPEnd, uint16 AccessLevel, const wxString& Description)
{
	if ( !iplist.empty() ) {
		// Find the first element larger than or equal to IPTest
		IPListMap::iterator it = iplist.lower_bound( IPStart );
	
		if ( it != iplist.begin() ) {
			// To a step back to the first element smaller than IPStart
			it--;
		}
		
		while ( it != iplist.end() ) {
			// Begins before the current span
			if ( IPStart < it->second->IPStart ) {
				// Never touches the current span
				if ( IPEnd < it->second->IPStart - 1 ) {
					break;
				}

				// Stops just before the current span
				else if ( IPEnd == it->second->IPStart - 1 ) {
					// If same AccessLevel: Merge
					if ( AccessLevel == it->second->AccessLevel ) {
						IPEnd = it->second->IPEnd;
						iplist.erase( it );
					}

					break;
				}

				// Covers part of the current span (maybe entire span)
				else {
					// If it only covers part of the span
					if ( IPEnd < it->second->IPEnd ) {
						// Same AccessLevel?
						if ( AccessLevel == it->second->AccessLevel ) {
							IPEnd = it->second->IPEnd;
							iplist.erase( it );
						} else {
							// Re-insert the partially covered span
							IPRange_Struct* item = it->second;
							item->IPStart = IPEnd + 1;

							iplist.erase( it );
							iplist[ item->IPStart ] = item;
						}

						break;
					} else {
						// It covers the entire span
						IPListMap::iterator tmp = it++;
						iplist.erase( tmp );
						continue;
					}
				}
			}
			// It starts at the current span
			else if ( IPStart == it->second->IPStart ) {
				// Covers only part of the current span
				if ( IPEnd < it->second->IPEnd ) {
					// Same AccessLevel, nothing to do
					if ( AccessLevel == it->second->AccessLevel ) {
						return;
					} else {
						// Re-insert the partially covered span
						IPRange_Struct* item = it->second;
						item->IPStart = IPEnd + 1;

						iplist.erase( it );
						iplist[ item->IPStart ] = item;
					}

					break;

				} else {
					// Covers the entire span
					IPListMap::iterator tmp = it++;
					iplist.erase( tmp );					
					continue;
				}
			}

			// Starts inside the current span or after the current span
			else if ( IPStart > it->second->IPStart ) {
				// Starts inside the current span
				if ( IPStart < it->second->IPEnd ) {
					// Ends inside the current span
					if ( IPEnd < it->second->IPEnd ) {
						// Adding a span with same AccessLevel inside a existing span is fruitless
						if ( AccessLevel == it->second->AccessLevel ) {
							return;
						}
					
						// Split the currens span and stop
						uint32 oldend = it->second->IPEnd;
						// Resize the current span to fit before the new span
						it->second->IPEnd = IPStart - 1;

						// Create a new span to cover the second block
						IPRange_Struct* item = new IPRange_Struct();
						*item = *it->second;
						item->IPStart     = IPEnd + 1;
						item->IPEnd       = oldend;
						
						// Insert the new span	
						iplist[ item->IPStart ] = item;
						
						break;
					} else {
						// If access-level is the same, then we remove the current and
						// resize the new span
						if ( AccessLevel == it->second->AccessLevel ) {
							IPStart = it->second->IPStart;
							
							IPListMap::iterator tmp = it++;
							iplist.erase( tmp );
							continue;
						} else {
							// Continues past the end of the current span, resize current span
							it->second->IPEnd = IPStart - 1;
						}
					}
				} else if ( IPStart == it->second->IPEnd ) {
					// If access-level is the same, then we remove the current and
					// resize the new span
					if ( AccessLevel == it->second->AccessLevel ) {
						IPStart = it->second->IPStart;
						
						IPListMap::iterator tmp = it++;
						iplist.erase( tmp );
						continue;
					} else {
						// Continues past the end of the current span, resize current span
						it->second->IPEnd = IPStart - 1;
					}
				} else {
					// Starts after the current span, nothing to do
				}
			}

			it++;
		}
	}


	IPRange_Struct *newFilter = new IPRange_Struct();
	newFilter->IPStart	= IPStart;
	newFilter->IPEnd	= IPEnd;
	wxASSERT(AccessLevel < 256);
	newFilter->AccessLevel	= AccessLevel;
	newFilter->Description	= Description;


	// It should now be safe to insert the span, without risking multiple spans covering the same range
	iplist[IPStart]		= newFilter;
}


/* *
 * This was done because the ipfilter.dat format uses sometimes leading zeroes,
 * and inet_aton interprets a leading zero as an octal number, not a decimal.
 * This function returns an ip in host order, not network.
 */
bool CIPFilter::m_inet_atoh(wxString &s, uint32 *ip)
{
	int i;
	unsigned long n;
	int ret = true;
	register uint32 hostip;
	wxStringTokenizer t( s, wxT("."), wxTOKEN_RET_EMPTY_ALL );
	wxString s1;
	hostip = 0;
	for( i = 0; i < 4; i++ ) {
		s1 = t.GetNextToken();
		n = 0;
		if( !s1.IsEmpty() ) {
			s1.ToULong(&n);
		}
		hostip = ( hostip << 8 ) | n;
	}
	*ip = hostip;
	
	return ret;
}


/**
 * Helper-function for removing bad chars from str-IPs
 */
wxString CleanUp( const wxString& str ) 
{
	wxString result;

	for ( unsigned int i = 0; i < str.Length(); i++ ) {
		if ( ( str[i] >= wxT('0') && str[i] <= wxT('9') ) || str[i] == wxT('.') ) {
			result += str[i];		
		}
	}

	return result;
}


bool CIPFilter::ProcessLineOk(const wxString& sLine, unsigned long linecounter)
{
	// remove spaces from the left and right.
	wxString line = sLine.Strip(wxString::both);
	
	// ignore comments & too short lines
	if ( line.GetChar(0) == wxT('#') ||
		 line.GetChar(0) == wxT('/') || 
		 line.Length() < 5 )
		return false;
	
	// Create the tokenizer. Fields are separated with commas. 
	// Returns the empty last token if that is the case
	wxStringTokenizer tokens( line, wxT(","), wxTOKEN_RET_EMPTY_ALL );
	
	// First token is IP Range
	wxString IPRange = tokens.GetNextToken();
	if( IPRange.IsEmpty() )
		return false;
	// Separate the two IP's
	wxStringTokenizer IPToken( IPRange, wxT("-"), wxTOKEN_RET_EMPTY_ALL );
	wxString sIPStart = IPToken.GetNextToken().Strip(wxString::both);
	wxString sIPEnd   = IPToken.GetNextToken().Strip(wxString::both);
	
	// Ensure that we only have valid chars in the ips
	sIPStart = CleanUp( sIPStart );
	sIPEnd   = CleanUp( sIPEnd );
	
	if( sIPStart.IsEmpty() || sIPEnd.IsEmpty() ) {
		AddLogLineM(true, wxString::Format(_("Invalid line in file ipfilter.dat(%d)"), linecounter));
		return false;
	}
	
	// Convert string IP's to host order IP numbers
	uint32 IPStart, IPEnd;
	bool ok = 
		m_inet_atoh( sIPStart, &IPStart ) &&
		m_inet_atoh( sIPEnd  , &IPEnd   );
	if ( !ok ) {
		AddLogLineM(true, wxString::Format(_("Invalid line in file ipfilter.dat(%d)"), linecounter));
		return false;
	}
	// Second token is Access Level, default is 0.
	long AccessLevel = 0;
	wxString sAccessLevel = tokens.GetNextToken();
	if ( !sAccessLevel.IsEmpty() ) {
		sAccessLevel.ToLong(&AccessLevel);
	}
	// Third Token is description
	wxString Description = tokens.GetNextToken().Strip(wxString::both);
	// add a filter
	AddBannedIPRange( IPStart, IPEnd, AccessLevel, Description );

	return true;
}

int CIPFilter::LoadFromFile(wxString file, bool merge)
{
	int filtercounter = 0;
	if (!merge) {
		RemoveAllIPs();
	}
	wxTextFile readFile(file);
	if( readFile.Exists() && readFile.Open() ) {
		// Ok, the file exists and was opened, lets read it
		
		for ( size_t i = 0; i < readFile.GetLineCount(); i++ ) {
			if( ProcessLineOk( readFile.GetLine( i ), i ) ) {
				filtercounter++;
			}
		}
		
		// Close it for completeness ;)
		readFile.Close();
	}
	AddLogLineM(true, wxString::Format(_("Loaded ipfilter with %d new IP addresses."), filtercounter));

	if (merge) {
		SaveToFile();
	}
	
	return filtercounter;
}

void CIPFilter::SaveToFile() {
	wxString IPFilterName = theApp.ConfigDir + wxT("ipfilter.dat");
	wxTextFile IPFilterFile(IPFilterName + wxT(".new"));
	if (IPFilterFile.Exists()) {
		// We're gonna do a new one, baby
		::wxRemoveFile(IPFilterName + wxT(".new"));
	}
	
	// Create the new ipfilter.
	IPFilterFile.Create();
	
	IPListMap::iterator it = iplist.begin();
	while(it != iplist.end()) {
		wxString line;
		// Range Start
		line += Uint32toStringIP(wxUINT32_SWAP_ALWAYS(it->second->IPStart));
		// Make it nice
		for (uint32 i = line.Len(); i < 15; i++) { // 15 -> "xxx.xxx.xxx.xxx"
			line += wxT(" ");
		}
		// Range Separator
		line += wxT(" - ");
		// Range End
		line += Uint32toStringIP(wxUINT32_SWAP_ALWAYS(it->second->IPEnd));
		// Make it nice
		for (uint32 i = line.Len(); i < 33; i++) { // 33 -> "xxx.xxx.xxx.xxx - yyy.yyy.yyy.yyy"
			line += wxT(" ");
		}
		// Token separator
		line += wxT(" , ");		
		// Access level
		line += wxString::Format(wxT("%i"),it->second->AccessLevel);
		// Token separator
		line += wxT(" , ");		
		// Description
		line += it->second->Description;		
		// Add it to file...
		IPFilterFile.AddLine(line);
		// And to next :)
		it++;
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

  /* prevents access violation errors */
  IPListMap temp = iplist;

  iplist.clear();

	// For wxObjArray classes, this destroys all of the array elements 
	// and additionally frees the memory allocated to the array.
	for( IPListMap::iterator it = temp.begin(); it != temp.end(); it++ ) {
		delete it->second;
	}
}

/*
 * IP2Test should to be in network order
 */
bool CIPFilter::IsFiltered(uint32 IPTest)
{
	// Return false if not using ip filter or ip filter is disabled
	if ( iplist.empty() || ( !thePrefs::GetIPFilterOn() ) )
		return false;
	
	IPTest = ntohl(IPTest);
	
	// Find the first element larger than IPTest
	IPListMap::iterator it = iplist.upper_bound( IPTest );
	
	if ( it != iplist.begin() ) {
		// Go back to the first element smaller than or equal to IPTest
		it--;

		// Check if this range covers the IP
		if ( IPTest <= it->second->IPEnd ) {
			// Is this filter active with the current access-level?
			if ( it->second->AccessLevel < thePrefs::GetIPFilterLevel() ) {
				lasthit = it->second->Description;
				return true;
			}
		}
	}

	return false;
}

void CIPFilter::Update(wxString strURL) {
	if (strURL.IsEmpty()) {
		strURL = thePrefs::IPFilterURL();
	}
	if (!strURL.IsEmpty()) {
		wxString strTempFilename(theApp.ConfigDir + wxT("ipfilter.dat.download"));
		CHTTPDownloadThread *downloader = new CHTTPDownloadThread(strURL,strTempFilename, HTTP_IPFilter);
		downloader->Create();
		downloader->Run();
	}	
}

void CIPFilter::DownloadFinished(uint32 result) {
	if(result==1) {
		wxString strTempFilename(theApp.ConfigDir + wxT("ipfilter.dat.download"));
		// curl succeeded. proceed with ipfilter loading
		LoadFromFile(strTempFilename, true); // merge it
		// So, file is loaded and merged, and also saved to ipfilter.dat
		wxRemoveFile(strTempFilename);
	} else {
		AddLogLineM(true, _("Failed to download the ipfilter from ") + thePrefs::IPFilterURL());
	}
}
