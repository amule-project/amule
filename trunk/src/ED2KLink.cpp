//this file is part of aMule
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

#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/msw/winundef.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#include <wx/datetime.h>	// Needed for wxDateTime

#include "ED2KLink.h"		// Interface declarations.
#include "CMemFile.h"		// Needed for CMemFile

#if 1
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
#endif

CED2KLink::~CED2KLink()
{
}

///////////////////////////////////////////// 
// CED2KServerListLink implementation 
///////////////////////////////////////////// 
CED2KServerListLink::CED2KServerListLink(const TCHAR* address)
{
	m_address = address;
}

CED2KServerListLink::~CED2KServerListLink()
{
} 

void
CED2KServerListLink::GetLink(wxString& lnk)
{
	lnk = ("ed2k://|serverlist|");
	lnk += m_address;
	lnk += ("|/");
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
	m_ip = inet_addr(ip);
	unsigned long ul = atoi(port); //_tcstoul(port,0,10);
	if ( ul > 0xFFFF )
		throw wxString(wxT("bad port number"));
	m_port = static_cast<uint16>(ul);
	m_defaultName = "Server ";
	m_defaultName += ip;
	m_defaultName += ":";
	m_defaultName += port;
}

CED2KServerLink::~CED2KServerLink()
{
}


void 
CED2KServerLink::GetLink(wxString& lnk)
{
	in_addr adr;
	char buffer[32];
	lnk = ("ed2k://|server|");
	adr.s_addr = m_ip;
	lnk += inet_ntoa(adr);
	lnk += ("|");
	sprintf(buffer,"%d",static_cast<int>(m_port));
	lnk += buffer;
	lnk += ("|/");
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
CED2KFileLink::CED2KFileLink(const TCHAR* name,const TCHAR* size, const TCHAR* hash,const TCHAR* sources)
: m_name(name)
, m_size(size)
{
  SourcesList=NULL;

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

		pCh = strstr( pCh, _T("sources") );
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
				SourcesList=new CMemFile();
				SourcesList->Write(nCount); // init to 0, we'll fix this at the end.
				// for each "ip:port" source string until the end
				// limit to prevent overflow (uint16 due to CPartFile::AddClientSources)
#define MAXSHORT 65535
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

					dwID = inet_addr( pIP );
					ul = atoi(pPort); //tcstoul( pPort, 0, 10 );
					nPort = static_cast<uint16>(ul);
					
					// skip bad ips / ports
					// Import from 030d
					
					if (ul > 0xFFFF || ul == 0 )	// port
					{	nInvalid++;	continue;	}
					
					if( dwID == INADDR_NONE) {	// hostname?
						if (strlen(pIP) > 512)
						{	nInvalid++;	continue;	}
						SUnresolvedHostname* hostname = new SUnresolvedHostname;
						hostname->nPort = nPort;
						hostname->strHostname = pIP;
						m_HostnameSourcesList.AddTail(hostname);
						continue;
					}
					
					if( dwID < 16777216 )	// ip
					{	nInvalid++;	continue;	}

					SourcesList->Write(dwID);
					SourcesList->Write(nPort);
					SourcesList->Write(dwServerIP);
					SourcesList->Write(nServerPort);
					nCount++;
				}
				SourcesList->Seek(0); //ToBegin();
				SourcesList->Write(nCount);
				SourcesList->Seek(0);//ToBegin();
				if (nCount==0) {
					delete SourcesList;
					SourcesList=NULL;
				}
			}
		}
	}

}

CED2KFileLink::~CED2KFileLink()
{
	// Imported from 0.30d
	
	if (SourcesList){
		delete SourcesList;
		SourcesList=NULL;
	}
	
	while (!m_HostnameSourcesList.IsEmpty())
		delete m_HostnameSourcesList.RemoveHead();
	
	// EOI
}

void 
CED2KFileLink::GetLink(wxString& lnk)
{
	lnk = ("ed2k://|file|");
	lnk += m_name;
	lnk += ("|");
	lnk += m_size;
	lnk += ("|");
	for (int idx=0; idx != 16 ; ++idx ) {
		unsigned int ui1 = m_hash[idx] / 16;
		unsigned int ui2 = m_hash[idx] % 16;
		lnk+= static_cast<TCHAR>( ui1 > 9 ? (('0')+ui1) : (('A')+(ui1-10)) );
		lnk+= static_cast<TCHAR>( ui2 > 9 ? (('0')+ui2) : (('A')+(ui2-10)) );
	}
	lnk += ("|/");
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
	m_name.Replace("%20"," ");
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
	const TCHAR* pChArray[7];
	if (uri==0) 
		throw wxString(wxT("null ed2k link"));
	TCHAR* pNewString = strdup(uri);
	autoFree liberator(pNewString);
	TCHAR* pCh = pNewString;
	const TCHAR* pStart = pCh;
	int idx = 0;
	for (idx=0;idx<7;idx++) pChArray[idx]=NULL;
	idx = 0;
	while( idx <7 && ((pCh = strchr(pCh,_T('|'))) !=0) ) {
		pChArray[idx++] = pStart;
		*pCh = 0;
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
	if ( strcmp( ("file") , pChArray[1]  ) == 0 && idx >=  5 && pChArray[4] != 0 ) {
		return new CED2KFileLink(pChArray[2],pChArray[3],pChArray[4],pChArray[6]);
	}
	else if ( strcmp( _T("serverlist") , pChArray[1] ) == 0 && idx == 3 ) {
		return new CED2KServerListLink(pChArray[2]);
	}
	else if ( strcmp( _T("server") , pChArray[1]  ) == 0 && idx == 4 ) {
		return new CED2KServerLink(pChArray[2],pChArray[3]);
	}
	else {
		throw wxString(wxT("Not an ED2K server or file link"));
	}
	return 0;
}
#if 0
//static 
CED2KLink* 
CED2KLink::CreateLinkFromUrl( const TCHAR * uri)
{
	// Parse pseudo-URI
	const TCHAR* pChArray[7];
	if (uri==0) 
		throw wxString(("null ed2k link"));
	TCHAR* pNewString = strdup(uri);
	autoFree liberator(pNewString);
	TCHAR* pCh = pNewString;
	const TCHAR* pStart = pCh;
	int idx = 0;
	for (idx=0;idx<7;idx++) pChArray[idx]=NULL;
	idx = 0;
	while( idx <7 && ((pCh = strchr(pCh,('|'))) !=0) ) {
		pChArray[idx++] = pStart;
		*pCh = 0;
		++ pCh;
		pStart = pCh;
	}
	if ( *pStart != ('/') ) {
		throw wxString((_("not a well-formed ed2k link")));
	}
	if (   idx < 3
		|| pChArray[0] == 0 
		|| pChArray[1] == 0 
		|| pChArray[2] == 0 
		|| pChArray[3] == 0 
		|| strcmp( ("ed2k://") , pChArray[0]  ) != 0 
	   ) {
			throw wxString(_("not a well-formed ed2k link"));
	}
	if ( strcmp( ("file") , pChArray[1]  ) == 0 && idx == 5 && pChArray[4] != 0 ) {
		return new CED2KFileLink(pChArray[2],pChArray[3],pChArray[4]);
	}
	else if ( strcmp( ("serverlist") , pChArray[1] ) == 0 && idx == 3 ) {
		return new CED2KServerListLink(pChArray[2]);
	}
	else if ( strcmp( ("server") , pChArray[1]  ) == 0 && idx == 4 ) {
		return new CED2KServerLink(pChArray[2],pChArray[3]);
	}
	else {
		throw wxString(_("Not an ED2K server or file link"));
	}
	return 0;
}
#endif
