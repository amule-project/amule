//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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

#include <wx/defs.h>		// Needed before any other wx/*.h

#include <wx/datetime.h>	// Needed for wxDateTime

#include "ED2KLink.h"		// Interface declarations.
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "NetworkFunctions.h" // Needed for Uint32toStringIP

namespace {
	struct autoFree {
		autoFree(TCHAR* p) : m_p(p) {}
		~autoFree() { free(m_p); }
	private:
		TCHAR * m_p;
	};
	inline unsigned int FromHexDigit(TCHAR digit) {
		switch (digit) {
		case ('0'): return 0;
		case ('1'): return 1;
		case ('2'): return 2;
		case ('3'): return 3;
		case ('4'): return 4;
		case ('5'): return 5;
		case ('6'): return 6;
		case ('7'): return 7;
		case ('8'): return 8;
		case ('9'): return 9;
		case ('A'): return 10;
		case ('B'): return 11;
		case ('C'): return 12;
		case ('D'): return 13;
		case ('E'): return 14;
		case ('F'): return 15;
		case ('a'): return 10;
		case ('b'): return 11;
		case ('c'): return 12;
		case ('d'): return 13;
		case ('e'): return 14;
		case ('f'): return 15;
		default: throw wxString(wxT("Malformed hash"));
		}
	}
}

CED2KLink::~CED2KLink()
{
}

///////////////////////////////////////////// 
// CED2KServerListLink implementation 
///////////////////////////////////////////// 
CED2KServerListLink::CED2KServerListLink(const TCHAR* address)
{
	m_address = char2unicode(address);
}

CED2KServerListLink::~CED2KServerListLink()
{
} 

void
CED2KServerListLink::GetLink(wxString& lnk)
{
	lnk = (wxT("ed2k://|serverlist|"));
	lnk += m_address;
	lnk += (wxT("|/"));
}

CED2KServerListLink*
CED2KServerListLink::GetServerListLink()
{
	return this;
}

CED2KServerLink*
CED2KServerListLink::GetServerLink()
{
	return 0;
}

CED2KFileLink*
CED2KServerListLink::GetFileLink()
{
	return 0;
}

CED2KLink::LinkType
CED2KServerListLink::GetKind() const
{
	return kServerList;
}

/////////////////////////////////////////////
// CED2KServerLink implementation
/////////////////////////////////////////////
CED2KServerLink::CED2KServerLink(const TCHAR* ip,const TCHAR* port)
{
	unsigned long ul = atoi(port); 
	if ( ul > 0xFFFF )
		throw wxString(wxT("bad port number"));
	m_port = static_cast<uint16>(ul);
	m_defaultName = wxT("Server ");
	m_defaultName += Uint32toStringIP(m_ip);
	m_defaultName += wxT(":");
	m_defaultName += char2unicode(port);
}

CED2KServerLink::~CED2KServerLink()
{
}


void 
CED2KServerLink::GetLink(wxString& lnk)
{
	char buffer[32];
	lnk = (wxT("ed2k://|server|"));
	lnk += Uint32toStringIP(m_ip);
	lnk += (wxT("|"));
	sprintf(buffer,"%d",static_cast<int>(m_port));
	lnk += char2unicode(buffer);
	lnk += (wxT("|/"));
}

CED2KServerListLink*
CED2KServerLink::GetServerListLink() 
{ 
	return 0; 
}

CED2KServerLink* 
CED2KServerLink::GetServerLink() 
{ 
	return this; 
}

CED2KFileLink* 
CED2KServerLink::GetFileLink() 
{ 
	return 0; 
}

CED2KLink::LinkType 
CED2KServerLink::GetKind() const
{
	return kServer;
}


/////////////////////////////////////////////
// CED2KFileLink implementation
/////////////////////////////////////////////
CED2KFileLink::CED2KFileLink(const TCHAR* name,const TCHAR* size, const TCHAR* hash, const TCHAR* hashset, const TCHAR* masterhash, const TCHAR* sources)
: m_name(char2unicode(name))
, m_size(char2unicode(size))
{
	SourcesList=NULL;
	m_hashset = NULL;
	m_bAICHHashValid = false;	

	if ( strlen(hash) != 32 )
		throw wxString(wxT("Ill-formed hash"));
	for ( int idx = 0 ; idx < 16 ; ++idx) {
		m_hash[idx] = FromHexDigit(*hash++)*16;
		m_hash[idx] += FromHexDigit(*hash++);
	}

	if (sources){

		TCHAR* pNewString = strdup(sources);
		autoFree liberator(pNewString);
		TCHAR* pCh = pNewString;
		TCHAR* pEnd;
		TCHAR* pIP;
		TCHAR* pPort;

		bool bAllowSources;
		TCHAR date[3];
		//COleDateTime expirationDate;
		wxDateTime expirationDate;
		int nYear,nMonth,nDay;

		uint16 nCount = 0;
		uint32 dwID;
		uint16 nPort;
		uint32 dwServerIP = 0; 
		uint16 nServerPort = 0;
		unsigned long ul;

		int nInvalid = 0;

		pCh = strstr( pCh, unicode2char(_T("sources")) );
		if( pCh != NULL ) {
			pCh = pCh + 7; // point to char after "sources"
			pEnd = pCh;
			while( *pEnd ) pEnd++; // make pEnd point to the terminating NULL
			bAllowSources=true;
			// if there's an expiration date...
			if( *pCh == _T('@') && (pEnd-pCh) > 7 )
			{
				pCh++; // after '@'
				date[2] = 0; // terminate the two character string
				date[0] = *(pCh++); date[1] = *(pCh++);
				nYear = atol( date ) + 2000;
				date[0] = *(pCh++); date[1] = *(pCh++);
				nMonth = atol( date );
				date[0] = *(pCh++); date[1] = *(pCh++);
				nDay = atol( date );
				expirationDate.Set(nYear,(wxDateTime::Month)nMonth,nDay,0,0,0,0);
				bAllowSources = expirationDate.IsValid();
				if (bAllowSources) bAllowSources=(wxDateTime::UNow() < expirationDate);
			}

			// increment pCh to point to the first "ip:port" and check for sources
			if ( bAllowSources && ++pCh < pEnd ) {
				SourcesList=new CSafeMemFile();
				SourcesList->WriteUInt16(nCount); // init to 0, we'll fix this at the end.
				// for each "ip:port" source string until the end
				// limit to prevent overflow (uint16 due to CPartFile::AddClientSources)
#ifndef __WXMSW__			
	#define MAXSHORT 65535
#endif
				while( *pCh != 0 && nCount < MAXSHORT ) {
					pIP = pCh;
					// find the end of this ip:port string & start of next ip:port string.
					if((pCh = strchr(pCh, _T(',')))) {
						*pCh = 0; // terminate current "ip:port"
						pCh++; // point to next "ip:port"
					}
					else
						pCh = pEnd;

					// if port is not present for this ip, go to the next ip.
					if( (pPort = strchr(pIP, _T(':'))) == NULL )
					{	nInvalid++;	continue;	}

					*pPort = 0;	// terminate ip string
					pPort++;	// point pPort to port string.

					dwID = CStringIPtoUint32(pIP);
					ul = atoi(pPort); //tcstoul( pPort, 0, 10 );
					nPort = static_cast<uint16>(ul);
					
					// skip bad ips / ports
					// Import from 030d
					
					if (ul > 0xFFFF || ul == 0 )	// port
					{	nInvalid++;	continue;	}
					
					if(!dwID) {	// hostname?
						if (strlen(pIP) > 512) {
							nInvalid++;	
							continue;	
						}
						SUnresolvedHostname* hostname = new SUnresolvedHostname;
						hostname->nPort = nPort;
						hostname->strHostname = char2unicode(pIP);
						m_HostnameSourcesList.AddTail(hostname);
						continue;
					}
					
					if( dwID < 16777216 )	// ip
					{	nInvalid++;	continue;	}

					SourcesList->WriteUInt32(dwID);
					SourcesList->WriteUInt16(nPort);
					SourcesList->WriteUInt32(dwServerIP);
					SourcesList->WriteUInt16(nServerPort);
					nCount++;
				}
				SourcesList->Seek(0); //ToBegin();
				SourcesList->WriteUInt16(nCount);
				SourcesList->Seek(0);//ToBegin();
				if (nCount==0) {
					delete SourcesList;
					SourcesList=NULL;
				}
			}
		}
	}
	
	if (hashset) {
		if (m_hashset != NULL){
			// W00t? 2 Hashsets?
			wxASSERT(0);
			return;
		}		
		m_hashset = new CSafeMemFile(256);
		m_hashset->WriteHash16(m_hash);
		m_hashset->WriteUInt16(0);

		int iPartHashs = 0;
		
		TCHAR* pCh = (TCHAR*)hashset;
		TCHAR* pStart = pCh;
		while( (pCh = strchr(pCh,_T(':'))) !=0 ) {
			*pCh = 0;
			if (strlen(pStart)!=16){
				delete m_hashset;
				m_hashset = NULL;
				return;
			}
			m_hashset->WriteHash16((uchar*)pStart);
			iPartHashs++;			
			++ pCh;
			pStart = pCh;
		}		

		m_hashset->Seek(16, wxFromStart);
		m_hashset->WriteUInt16(iPartHashs);
		m_hashset->Seek(0, wxFromStart);
	}
	
	if (masterhash) {
		wxString strHash = char2unicode(masterhash);
		if (!strHash.IsEmpty()) {
			if (otherfunctions::DecodeBase32(masterhash, CAICHHash::GetHashSize(), m_AICHHash.GetRawHash()) == CAICHHash::GetHashSize()){
				m_bAICHHashValid = true;
				wxASSERT( m_AICHHash.GetString().CmpNoCase(strHash) == 0 );
			} else {
				wxASSERT( false );
			}
		} else {
			wxASSERT( false );		
		}
	}

}

CED2KFileLink::~CED2KFileLink()
{
	
	if (SourcesList){
		delete SourcesList;
		SourcesList=NULL;
	}
	
	while (!m_HostnameSourcesList.IsEmpty()) {
		delete m_HostnameSourcesList.RemoveHead();
	}
	
	if (m_hashset) {
		delete m_hashset;
		m_hashset =  NULL;
	}

}

void 
CED2KFileLink::GetLink(wxString& lnk)
{
	lnk = wxT("ed2k://|file|");
	lnk += m_name;
	lnk += wxT("|");
	lnk += m_size;
	lnk += wxT("|");
	for (int idx=0; idx != 16 ; ++idx ) {
		unsigned int ui1 = m_hash[idx] / 16;
		unsigned int ui2 = m_hash[idx] % 16;
		lnk+= static_cast<TCHAR>( ui1 > 9 ? (('0')+ui1) : (('A')+(ui1-10)) );
		lnk+= static_cast<TCHAR>( ui2 > 9 ? (('0')+ui2) : (('A')+(ui2-10)) );
	}
	lnk += wxT("|/");
}

CED2KServerListLink*
CED2KFileLink::GetServerListLink() 
{ 
	return 0; 
}

CED2KServerLink* 
CED2KFileLink::GetServerLink() 
{ 
	return 0; 
}
CED2KFileLink* 
CED2KFileLink::GetFileLink() 
{ 
	m_name.Replace(wxT("%20"),wxT(" "));
	return this; 
}

CED2KLink::LinkType 
CED2KFileLink::GetKind() const
{
	return kFile;
}

//static 
CED2KLink* 
CED2KLink::CreateLinkFromUrl( const TCHAR * uri)
{
	// Parse pseudo-URI
	const TCHAR* pChArray[10];
	if (uri==0) 
		throw wxString(wxT("null ed2k link"));
	TCHAR* pNewString = strdup(uri);
	autoFree liberator(pNewString);
	TCHAR* pCh = pNewString;
	const TCHAR* pStart = pCh;
	int idx = 0;
	uint32 std_ed2k_link_end = 0;
	for (idx=0;idx<10;idx++) pChArray[idx]=NULL;
	idx = 0;
	while( idx <10 && ((pCh = strchr(pCh,_T('|'))) !=0) ) {
		pChArray[idx++] = pStart;
		*pCh = 0;
		if (strcmp("/",pStart) == 0) {
			std_ed2k_link_end = idx;
		}		
		++ pCh;
		pStart = pCh;
	}
	if ( *pStart != ('/') ) {
		throw wxString((wxT("Not a well-formed ed2k link")));
	}
	if (   idx < 3
		|| pChArray[0] == 0 
		|| pChArray[1] == 0 
		|| pChArray[2] == 0 
//		|| pChArray[3] == 0 // This was preventing ed2k serverlist links from working.. 
		|| strcmp( ("ed2k://") , pChArray[0]  ) != 0 
	   ) {
			throw wxString(wxT("Not a well-formed ed2k link"));
	}
	if ( strcmp( "file" , pChArray[1]  ) == 0 && idx >=  5 && pChArray[4] != 0 ) {
		// If AICH hashset is present, must have 'h=' as start
		// 'p=' marks a masterhash.
		// std_ed2k_link_end marks the end of the link, after that, there might be 
		// dragons, or a sources list.
		const TCHAR* hashset = NULL;
		const TCHAR* masterhash = NULL;
		for (uint32 i = 4; i <= std_ed2k_link_end ; i++) {
			if (strncmp(pChArray[i],"p=",2) == 0) {
				hashset = (pChArray[i]) + 2;
			} else if (strncmp(pChArray[i],"h=",2) == 0) {
				masterhash = (pChArray[i]) + 2;
			}
		}
		
		return new CED2KFileLink(pChArray[2],pChArray[3],pChArray[4], hashset, masterhash,pChArray[std_ed2k_link_end+1]);
	}
	else if ( strcmp( unicode2char(_T("serverlist")) , pChArray[1] ) == 0 && idx == 3 ) {
		return new CED2KServerListLink(pChArray[2]);
	}
	else if ( strcmp( unicode2char(_T("server")) , pChArray[1]  ) == 0 && idx == 4 ) {
		return new CED2KServerLink(pChArray[2],pChArray[3]);
	}
	else {
		throw wxString(wxT("Not an ED2K server or file link"));
	}
	return 0;
}
