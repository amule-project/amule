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

#include "types.h" 
#include "server.h"		// Interface declarations.
#include "SafeFile.h"		// Needed for CSafeFile
#include "otherfunctions.h"	// Needed for nstrdup
#include "otherstructs.h"	// Needed for ServerMet_Struct
#include "packets.h"		// Needed for CTag

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
	if (!ip) {
		// If ip == 0, the address is a hostname
		dynip = i_addr;
	} else {
		dynip.Clear();
	}
	
	Init();

}

// copy constructor
CServer::CServer(CServer* pOld)
{
	taglist=new TagList;
	for(TagList::Node* pos = pOld->taglist->GetFirst(); pos != NULL;pos=pos->GetNext()) {
		CTag* pOldTag = pos->GetData(); //pOld->taglist->GetAt(pos);
		taglist->Append(pOldTag->CloneTag()); //AddTail(pOldTag->CloneTag());
	}
	port = pOld->port;
	ip = pOld->ip; 
	staticservermember=pOld->IsStaticMember();
	tagcount = pOld->tagcount;
	ipfull = pOld->ipfull;
	files = pOld->files;
	users = pOld->users;
	preferences = pOld->preferences;
	ping = pOld->ping;
	failedcount = pOld->failedcount; 
	lastpinged = pOld->lastpinged;
	if (!pOld->description.IsEmpty()) {
		description = pOld->description;
	} else {
		description = wxEmptyString;
	}
	if (!pOld->listname.IsEmpty()) {
		listname = pOld->listname;
	} else {
		listname = wxEmptyString;
	}
	if (!pOld->dynip.IsEmpty()) {
		dynip = pOld->dynip;
	} else {
		dynip = wxEmptyString;
	}
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
	m_strVersion = wxT("Unknown");
	m_uLowIDUsers = 0;
	m_uDescReqChallenge = 0;
	lastdescpingedcout = 0;
}	

bool CServer::AddTagFromFile(CFile* servermet){
	if (servermet == 0)
		return false;
	
	CTag* tag = NULL;
	
	try {
		tag = new CTag(*servermet);
	} catch (CInvalidPacket e) {
		if (tag) {
			delete tag;
		}
		printf("Caught CInvalidPacket exception in CServer::AddTagFromFile! server.met is corrupted.\n");
		throw e;
	} catch (wxString error) {
		if (tag) {
			delete tag;
		}
		printf("Caught exception in CServer::AddTagFromFile! server.met is corrupted.\n");
		printf("Error: %s\n",unicode2char(error)); 
		throw CInvalidPacket("Error reading server.met");		
	}
	
	switch(tag->tag.specialtag){		
	case ST_SERVERNAME:
		if(tag->tag.stringvalue)
			listname = char2unicode(tag->tag.stringvalue);
		else
			listname = wxEmptyString;
		delete tag;
		break;
	case ST_DESCRIPTION:
		if( tag->tag.stringvalue )
			description = char2unicode(tag->tag.stringvalue);
		else
			description = wxEmptyString;
		delete tag;
		break;
	case ST_PREFERENCE:
		wxASSERT( tag->tag.type == 3 );
		preferences =tag->tag.intvalue;
		delete tag;
		break;
	case ST_PING:
		wxASSERT( tag->tag.type == 3 );
		ping = tag->tag.intvalue;
		delete tag;
		break;
	case ST_DYNIP:
		if ( tag->tag.stringvalue )
			dynip = char2unicode(tag->tag.stringvalue);
		else
			dynip = wxEmptyString;
		delete tag;
		break;
	case ST_FAIL:
		wxASSERT( tag->tag.type == 3 );
		failedcount = tag->tag.intvalue;
		delete tag;
		break;
	case ST_LASTPING:
		wxASSERT( tag->tag.type == 3 );
		lastpinged = tag->tag.intvalue;
		delete tag;
		break;
	case ST_MAXUSERS:
		wxASSERT( tag->tag.type == 3 );
		maxusers = tag->tag.intvalue;
		delete tag;
		break;
	case ST_SOFTFILES:
		wxASSERT( tag->tag.type == 3 );
		softfiles = tag->tag.intvalue;
		delete tag;
		break;
	case ST_HARDFILES:
		wxASSERT( tag->tag.type == 3 );
		hardfiles = tag->tag.intvalue;
		delete tag;
		break;
	case ST_VERSION:
		if (tag->tag.type == 2)
			m_strVersion = char2unicode(tag->tag.stringvalue);
		delete tag;
		break;
	case ST_UDPFLAGS:
		wxASSERT( tag->tag.type == 3 );		
		if (tag->tag.type == 3)
			m_uUDPFlags = tag->tag.intvalue;
		delete tag;
		break;
	case ST_LOWIDUSERS:
		wxASSERT( tag->tag.type == 3 );
		if (tag->tag.type == 3)			
			m_uLowIDUsers = tag->tag.intvalue;
		delete tag;
		break;	
	default:
		if (tag->tag.specialtag){
			tag->tag.tagname = nstrdup("Unknown");
			AddTag(tag);
		}
		else if (!strcmp(tag->tag.tagname,"files")){
			files = tag->tag.intvalue;
			delete tag;
		}
		else if (!strcmp(tag->tag.tagname,"users")){
			users = tag->tag.intvalue;
			delete tag;
		}
		else
			AddTag(tag);
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
