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
#include "otherfunctions.h"
#include "Preferences.h"	// Needed for CPreferences
#include "amule.h"		// Needed for theApp
#include "HTTPDownloadDlg.h"
#include "GetTickCount.h"

CIPFilter::CIPFilter(){
	lasthit = wxEmptyString;
	LoadFromFile(theApp.ConfigDir + wxT("ipfilter.dat"), false); // No merge on construction
	if (theApp.glob_prefs->IPFilterAutoLoad()) {
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
	IPRange_Struct *newFilter = new IPRange_Struct();
	newFilter->IPStart	= IPStart;
	newFilter->IPEnd	= IPEnd;
	wxASSERT(AccessLevel < 256);
	newFilter->AccessLevel	= AccessLevel;
	newFilter->Description	= Description;
	// iplist is a std::map, key is IPstart.
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
	
	struct in_addr ip_data;
	
	IPListMap::iterator it = iplist.begin();
	while(it != iplist.end()) {
		wxString line;
		// Range Start
		ip_data.s_addr = htonl(it->second->IPStart);
		line += char2unicode(inet_ntoa(ip_data));
		// Make it nice
		for (uint32 i = line.Len(); i < 15; i++) { // 15 -> "xxx.xxx.xxx.xxx"
			line += wxT(" ");
		}
		// Range Separator
		line += wxT(" - ");
		// Range End
		ip_data.s_addr = htonl(it->second->IPEnd);
		line += char2unicode(inet_ntoa(ip_data));
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
	// For wxObjArray classes, this destroys all of the array elements 
	// and additionally frees the memory allocated to the array.
	for( IPListMap::iterator it = iplist.begin(); it != iplist.end(); it++ ) {
		delete it->second;
	}
	iplist.clear();
}

/*
 * IP2Test should to be in network order
 */
bool CIPFilter::IsFiltered(uint32 IPTest)
{
	IPTest = ntohl(IPTest);
	// return false if not using ip filter or ip filter is disabled
	if( ( iplist.size() == 0 ) || ( !theApp.glob_prefs->GetIPFilterOn() ) )
		return false;
	bool found = false;
	IPListMap::iterator it = iplist.begin();
	while( !found && it != iplist.end() && it->second->IPStart <= IPTest ) {
		found = IPTest <= it->second->IPEnd && 
			it->second->AccessLevel < theApp.glob_prefs->GetIPFilterLevel();
		if( found ) {
			lasthit = it->second->Description;
		}
		it++;
	}

	return found;
}

void CIPFilter::Update() {
	wxString strURL = theApp.glob_prefs->IPFilterURL();
	if (!strURL.IsEmpty()) {
		
		wxString strTempFilename(theApp.ConfigDir + wxString::Format(wxT("temp-%d-ipfilter.dat"), ::GetTickCount()));
#ifndef AMULE_DAEMON
		CHTTPDownloadDlg *dlg=new CHTTPDownloadDlg(theApp.GetTopWindow(),strURL,strTempFilename);
		int retval=dlg->ShowModal();
		delete dlg;
#else
		#warning Xaignar, please fix auto-update on daemon.
		int retval=0;
#endif
		if(retval==0) {
			// curl succeeded. proceed with ipfilter loading
			LoadFromFile(strTempFilename, true); // merge it
			// So, file is loaded and merged, and also saved to ipfilter.dat
			wxRemoveFile(strTempFilename);
		} else {
			AddLogLineM(true, _("Failed to download the ipfilter from ")+ strURL);
		}
	}	
}
