// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/textfile.h>
#include <wx/arrimpl.cpp>	// this is a magic incantation which must be done!
#include <wx/string.h>		// for wxString
#include <wx/tokenzr.h>		// for wxTokenizer

#include <sys/socket.h>		// for inet_aton() and struct in_addr
#include <netinet/in.h>
#include <arpa/inet.h>

#include "IPFilter.h"		// Interface declarations.
#include "otherfunctions.h"
#include "Preferences.h"	// Needed for CPreferences
#include "amule.h"		// Needed for theApp

CIPFilter::CIPFilter(){
	lasthit = wxEmptyString;
	LoadFromFile();
}

CIPFilter::~CIPFilter(){
	RemoveAllIPs();
}

void CIPFilter::Reload(){
	RemoveAllIPs();
	lasthit = wxEmptyString;
	LoadFromFile();
}

/* *
 * IPFilter is an map of IPRanges, ordered by IPstart, of banned ranges of IP addresses.
 */
void CIPFilter::AddBannedIPRange(uint32 IPStart, uint32 IPEnd, uint8 AccessLevel, const wxString& Description)
{
	IPRange_Struct *newFilter = new IPRange_Struct();
	newFilter->IPStart	= IPStart;
	newFilter->IPEnd	= IPEnd;
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

int CIPFilter::LoadFromFile()
{
	int filtercounter = 0;
	int linecounter = 0;
	RemoveAllIPs();
	wxTextFile readFile(theApp.ConfigDir + wxT("ipfilter.dat"));
	if( readFile.Exists() && readFile.Open() ) {
		// Ok, the file exists and was opened, lets read it
		wxString sbuffer = readFile.GetFirstLine();
		for( ; !readFile.Eof(); sbuffer = readFile.GetNextLine() ) {
			// increment line counter
			linecounter++;
			// remove spaces from the left and right.
			sbuffer.Strip(wxString::both);
			// ignore comments & too short lines
			if( 	sbuffer.GetChar(0) == '#' ||
				sbuffer.GetChar(0) == '/' || 
				sbuffer.Length() < 5 )
				continue;
			// Create the tokenizer. Fields are separated with commas. 
			// Returns the empty last token if that is the case
			wxStringTokenizer tokens( sbuffer, wxT(","), wxTOKEN_RET_EMPTY_ALL );
			// First token is IP Range
			wxString IPRange = tokens.GetNextToken();
			if( IPRange.IsEmpty() ) continue;
			// Separate the two IP's
			wxStringTokenizer IPToken( IPRange, wxT("-"), wxTOKEN_RET_EMPTY_ALL );
			wxString sIPStart = IPToken.GetNextToken().Strip(wxString::both);
			wxString sIPEnd   = IPToken.GetNextToken().Strip(wxString::both);
			if( sIPStart.IsEmpty() || sIPEnd.IsEmpty() ) {
				AddLogLineM(true, wxString::Format(_("Invalid line in file ipfilter.dat(%d)"), linecounter));
				continue;
			}
			// Convert string IP's to host order IP numbers
			uint32 IPStart, IPEnd;
			bool ok = 
				m_inet_atoh( sIPStart, &IPStart ) &&
				m_inet_atoh( sIPEnd  , &IPEnd   );
			if ( !ok ) {
				AddLogLineM(true, wxString::Format(_("Invalid line in file ipfilter.dat(%d)"), linecounter));
				continue;
			}
			// Second token is Access Level, default is 0.
			long AccessLevel = 0;
			wxString sAccessLevel = tokens.GetNextToken();
			if ( !sAccessLevel.IsEmpty() ) {
				sAccessLevel.ToLong(&AccessLevel);
			}
			// Third Token is description
			wxString Description = tokens.GetNextToken();
			// add a filter
			AddBannedIPRange( IPStart, IPEnd, AccessLevel, Description );
			filtercounter++;
		}
		// Close it for completeness ;)
		readFile.Close();
	}
	AddLogLineM(true, wxString::Format(_("Loaded ipfilter with %d IP addresses."), filtercounter));

	return filtercounter;
}

/*
 * Not implemented
 */
void CIPFilter::SaveToFile()
{
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

