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
#pragma implementation "Server.h"
#endif

#include "Server.h"		// Interface declarations.
#include "SafeFile.h"		// Needed for CSafeFile
#include "OtherFunctions.h"	// Needed for nstrdup
#include "NetworkFunctions.h" // Needed for StringIPtoUint32
#include "OtherStructs.h"	// Needed for ServerMet_Struct
#include "Packet.h"		// Needed for CTag
#include "StringFunctions.h" // Needed for unicode2char 

#include <wx/intl.h>	// Needed for _

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(TagList);

CServer::CServer(ServerMet_Struct* in_data)
{
	port = in_data->port;
	ip = in_data->ip;

	Init();
}

CServer::CServer(uint16 in_port, const wxString i_addr)
{

	port = in_port;
	ip = StringIPtoUint32(i_addr);

	Init();

	// GonoszTopi - Init() would clear dynip !
	// Check that the ip isn't in fact 0.0.0.0
	if (!ip && !StringIPtoUint32( i_addr, ip ) ) {
		// If ip == 0, the address is a hostname
		dynip = i_addr;
	}
}

#ifdef CLIENT_GUI
CServer::CServer(CEC_Server_Tag *tag)
{
	CECTag *tmpTag;
	if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_NAME)) != NULL) {
		listname = tmpTag->GetStringData();
	}
	if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_DESC)) != NULL) {
	    description = tmpTag->GetStringData();
	}
	ip = tag->GetIPv4Data().IP();
	ipfull = Uint32toStringIP(ip);
	port = tag->GetIPv4Data().port;
    if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_USERS_MAX)) != NULL) {
        maxusers = tmpTag->GetInt32Data();
    } else {
        maxusers = 0;
    }
    if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_FILES)) != NULL) {
        hardfiles = tmpTag->GetInt32Data();
    } else {
	    hardfiles = 0;
    }
   
	if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_PRIO)) != NULL) {
    	preferences = tmpTag->GetInt8Data();
    } else {
	    preferences = SRV_PR_NORMAL;
    }
    
	if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_STATIC)) != NULL) {
		staticservermember = tmpTag->GetInt8Data();
	} else {
	    staticservermember = 0;
	}
	
	if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_USERS)) != NULL) {
		users = tmpTag->GetInt32Data();
	} else {
		users = 0;
	}
	if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_PING)) != NULL) {
		ping = tmpTag->GetInt32Data();
	} else {
		ping = 0;
	}
	if ((tmpTag = tag->GetTagByName(EC_TAG_SERVER_FAILED)) != NULL) {
		failedcount = tmpTag->GetInt8Data();
	} else {
		failedcount = 0;
	}
	
}
#endif

// copy constructor
CServer::CServer(CServer* pOld)
{
	wxASSERT(pOld != NULL);
	taglist = new TagList;
	// Got a bt here with pOld->taglist == NULL. Check that.
	if (pOld && pOld->taglist) {
		for(	TagList::Node* pos = pOld->taglist->GetFirst();
			pos != NULL;
			pos = pos->GetNext()) {
			CTag* pOldTag = pos->GetData(); //pOld->taglist->GetAt(pos);
			taglist->Append(pOldTag->CloneTag()); //AddTail(pOldTag->CloneTag());
		}
	}
	port = pOld->port;
	ip = pOld->ip; 
	realport = pOld->realport;
	staticservermember=pOld->IsStaticMember();
	tagcount = pOld->tagcount;
	ipfull = pOld->ipfull;
	files = pOld->files;
	users = pOld->users;
	preferences = pOld->preferences;
	ping = pOld->ping;
	failedcount = pOld->failedcount; 
	lastpinged = pOld->lastpinged;
	description = pOld->description;
	listname = pOld->listname;
	dynip = pOld->dynip;
	maxusers = pOld->maxusers;
	m_strVersion = pOld->m_strVersion;
	m_uTCPFlags = pOld->m_uTCPFlags;
	m_uUDPFlags = pOld->m_uUDPFlags;
	challenge = pOld->challenge;
	softfiles = pOld->softfiles;
	hardfiles = pOld->hardfiles;
	m_uDescReqChallenge = pOld->m_uDescReqChallenge;
	m_uLowIDUsers = pOld->m_uLowIDUsers;
	lastdescpingedcout = pOld->lastdescpingedcout;
	m_auxPorts = pOld->m_auxPorts;
}

CServer::~CServer()
{

	if(taglist) {
		for(TagList::Node* pos = taglist->GetFirst(); pos != NULL; pos=pos->GetNext()) {
			delete pos->GetData(); //taglist->GetAt(pos);
		}
		taglist->Clear(); //RemoveAll();
		delete taglist;
		taglist=NULL;
	}
}

void CServer::Init() {
	
	ipfull = Uint32toStringIP(ip);
	
	taglist = new TagList;
	realport = 0;
	tagcount = 0;
	files = 0;
	users = 0;
	preferences = 0;
	ping = 0;
	description = wxEmptyString;
	listname = wxEmptyString;
	dynip = wxEmptyString;
	failedcount = 0; 
	lastpinged = 0;
	staticservermember=0;
	maxusers=0;
	m_uTCPFlags = 0;
	m_uUDPFlags = 0;
	challenge = 0;
	softfiles = 0;
	hardfiles = 0;	
	m_strVersion = _("Unknown");
	m_uLowIDUsers = 0;
	m_uDescReqChallenge = 0;
	lastdescpingedcout = 0;
	m_auxPorts = wxEmptyString;
}	

bool CServer::AddTagFromFile(CFileDataIO* servermet){
	
	if (servermet == NULL) {
		return false;
	}
	
	try {
		CTag tag(*servermet, true);

		switch(tag.GetNameID()){		
		case ST_SERVERNAME:
			#if wxUSE_UNICODE
			if (listname.IsEmpty())
			#endif
				listname = tag.GetStr();
			break;
		case ST_DESCRIPTION:
			#if wxUSE_UNICODE
			if (description.IsEmpty())
			#endif
				description = tag.GetStr();		
			break;
		case ST_PREFERENCE:
			preferences =tag.GetInt();
			break;
		case ST_PING:
			ping = tag.GetInt();
			break;
		case ST_DYNIP:
			#if wxUSE_UNICODE
			if (dynip.IsEmpty())
			#endif	
				dynip = tag.GetStr();
			break;
		case ST_FAIL:
			failedcount = tag.GetInt();
			break;
		case ST_LASTPING:
			lastpinged = tag.GetInt();
			break;
		case ST_MAXUSERS:
			maxusers = tag.GetInt();
			break;
		case ST_SOFTFILES:
			softfiles = tag.GetInt();
			break;
		case ST_HARDFILES:
			hardfiles = tag.GetInt();
			break;
		case ST_VERSION:
			if (tag.IsStr()) {
				#ifdef wxUSE_UNICODE
				if (m_strVersion.IsEmpty())
				#endif
					m_strVersion = tag.GetStr();
			} else if (tag.IsInt()) {
				m_strVersion = wxString::Format(wxT("%u.%u"), tag.GetInt() >> 16, tag.GetInt() & 0xFFFF);
			} else {
				wxASSERT(0);
			}
			break;
		case ST_UDPFLAGS:
			if (tag.IsInt()) {
				m_uUDPFlags = tag.GetInt();
			}
			break;
		case ST_AUXPORTSLIST:
			if (tag.IsStr()) {
				m_auxPorts = tag.GetStr();
				realport = port;
				port = StrToULong(m_auxPorts.BeforeFirst(','));
			}
			break;
		case ST_LOWIDUSERS:
			if (tag.IsInt())			
				m_uLowIDUsers = tag.GetInt();
			break;
		default:
			if (tag.GetNameID()){
				wxASSERT(0);
			} else if (!CmpED2KTagName(tag.GetName(), "files")) {
				wxASSERT( tag.IsInt() );
				if (tag.IsInt()) {
					files = tag.GetInt();
				}
			} else if (!CmpED2KTagName(tag.GetName(), "users")) {
				wxASSERT( tag.IsInt() );
				if (tag.IsInt()) {
					users = tag.GetInt();
				}
			}
		}
	} catch (const CInvalidPacket& e) {
		printf("Caught CInvalidPacket exception in CServer::AddTagFromFile! server.met is corrupted.\n");
		throw;
	} catch (const wxString& error) {
		printf("Caught exception in CServer::AddTagFromFile! server.met is corrupted.\n");
		printf("Error: %s\n", (const char *)unicode2char(error)); 
		throw CInvalidPacket("Error reading server.met");		
	} catch (...) {
		printf("Caught unknown exception in CServer::AddTagFromFile! server.met is corrupted.\n");
		throw CInvalidPacket("Error reading server.met");				
	}
	return true;
}

void CServer::SetListName(const wxString& newname)
{
	listname = newname;
}

void CServer::SetDescription(const wxString& newname)
{
	description = newname;
}

void CServer::SetID(uint32 newip)
{
	wxASSERT(newip);
	ip = newip;
	ipfull = Uint32toStringIP(ip);
}

void CServer::SetDynIP(const wxString& newdynip)
{
	dynip = newdynip;
}


void CServer::SetLastDescPingedCount(bool bReset)
{
	if (bReset) {
		lastdescpingedcout = 0;
	} else {
		lastdescpingedcout++;
	}
}
