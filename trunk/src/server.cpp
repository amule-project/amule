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

#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
#else
#ifdef __OPENBSD__
       #include <sys/types.h>
#endif /* __OPENBSD__ */
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

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

CServer::CServer(uint16 in_port, const char* i_addr)
{

	port = in_port;
	if (inet_addr(i_addr) == INADDR_NONE) {
		dynip = nstrdup(i_addr);
		ip = 0;
	} else {
		ip = inet_addr(i_addr);
		dynip = 0;
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
	strcpy(ipfull,pOld->ipfull);
	files = pOld->files;
	users = pOld->users;
	preferences = pOld->preferences;
	ping = pOld->ping;
	failedcount = pOld->failedcount; 
	lastpinged = pOld->lastpinged;
	if (pOld->description) {
		description = nstrdup(pOld->description);
	} else {
		description = NULL;
	}
	if (pOld->listname) {
		listname = nstrdup(pOld->listname);
	} else {
		listname = NULL;
	}
	if (pOld->dynip) {
		dynip = nstrdup(pOld->dynip);
	} else {
		dynip = NULL;
	}
	maxusers = pOld->maxusers;
	m_strVersion = pOld->m_strVersion;
	m_uTCPFlags = pOld->m_uTCPFlags;
	m_uUDPFlags = pOld->m_uUDPFlags;
	challenge = pOld->challenge;
	softfiles = pOld->softfiles;
	hardfiles = pOld->hardfiles;
	
}

CServer::~CServer()
{
	if (description) {
		delete[] description;
	}
	if (listname) {
		delete[] listname;
	}
	if (dynip) {
		delete[] dynip;
	}
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
	
	in_addr host;
	host.s_addr = ip;
	strcpy(ipfull,inet_ntoa(host));
	
	taglist = new TagList;
	tagcount = 0;
	files = 0;
	users = 0;
	preferences = 0;
	ping = 0;
	description = 0;
	listname = 0;
	dynip = 0;
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
	
}	

bool CServer::AddTagFromFile(CFile* servermet){
	if (servermet == 0)
		return false;
	CTag* tag = new CTag(servermet);
	switch(tag->tag.specialtag){		
	case ST_SERVERNAME:
		if(tag->tag.stringvalue)
			listname = nstrdup(tag->tag.stringvalue);
		else
			listname = NULL;
		delete tag;
		break;
	case ST_DESCRIPTION:
		if( tag->tag.stringvalue )
			description = nstrdup(tag->tag.stringvalue);
		else
			description = NULL;
		delete tag;
		break;
	case ST_PREFERENCE:
		preferences =tag->tag.intvalue;
		delete tag;
		break;
	case ST_PING:
		ping = tag->tag.intvalue;
		delete tag;
		break;
	case ST_DYNIP:
		if ( tag->tag.stringvalue )
			dynip = nstrdup(tag->tag.stringvalue);
		else
			dynip = NULL;
		delete tag;
		break;
	case ST_FAIL:
		failedcount = tag->tag.intvalue;
		delete tag;
		break;
	case ST_LASTPING:
		lastpinged = tag->tag.intvalue;
		delete tag;
		break;
	case ST_MAXUSERS:
		maxusers = tag->tag.intvalue;
		delete tag;
		break;
	case ST_SOFTFILES:
		softfiles = tag->tag.intvalue;
		delete tag;
		break;
	case ST_HARDFILES:
		hardfiles = tag->tag.intvalue;
		delete tag;
		break;
	case ST_VERSION:
		if (tag->tag.type == 2)
			m_strVersion = char2unicode(tag->tag.stringvalue);
		delete tag;
		break;
	case ST_UDPFLAGS:
		if (tag->tag.type == 3)
			m_uUDPFlags = tag->tag.intvalue;
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

#if 0
bool CServer::AddTagFromFile(CSafeFile* servermet)
{
	if (servermet == 0) {
		return false;
	}
	CTag* tag = new CTag(servermet);
	switch(tag->tag->specialtag) {
		case ST_SERVERNAME:
			listname = nstrdup(tag->tag->stringvalue);
			delete tag;
			break;
		case ST_DESCRIPTION:
			description = nstrdup(tag->tag->stringvalue);
			delete tag;
			break;
		case ST_PREFERENCE:
			preferences = tag->tag->intvalue;
			delete tag;
			break;
		case ST_PING:
			ping = tag->tag->intvalue;
			delete tag;
			break;
		case ST_DYNIP:
			dynip = nstrdup(tag->tag->stringvalue);
			delete tag;
			break;
		case ST_FAIL:
			failedcount = tag->tag->intvalue;
			delete tag;
			break;
		case ST_LASTPING:
			lastpinged = tag->tag->intvalue;
			delete tag;
			break;
		case ST_MAXUSERS:
			maxusers = tag->tag->intvalue;
			delete tag;
			break;
		case ST_SOFTFILES:
			softfiles = tag->tag->intvalue;
			delete tag;
			break;
		case ST_HARDFILES:
			hardfiles = tag->tag->intvalue;
			delete tag;
			break;
		case ST_VERSION:
			if (tag->tag->type == 2) {
				m_strVersion = char2unicode(tag->tag->stringvalue);
			}
			delete tag;
			break;
		case ST_UDPFLAGS:
			if (tag->tag->type == 3) {
				m_uUDPFlags = tag->tag->intvalue;
			}
			delete tag;
			break;
		default:
			if (tag->tag->specialtag) {
				tag->tag->tagname = nstrdup("Unknown");
				AddTag(tag);
			} else if (!strcmp(tag->tag->tagname,"files")) {
				files = tag->tag->intvalue;
				delete tag;
			} else if (!strcmp(tag->tag->tagname,"users")) {
				users = tag->tag->intvalue;
				delete tag;
			} else {
				AddTag(tag);
			}
	}
	return true;
}
#endif

void CServer::SetListName(char* newname)
{
	if (listname) {
		delete[] listname;
	}
	listname = nstrdup(newname);
}

void CServer::SetDescription(char* newname)
{
	if (description) {
		delete[] description;
	}
	description = nstrdup(newname);
}

void CServer::FillWindowTags(wxTreeCtrl* wnd,wxTreeItemId rootitem)
{
#if 0
	POSITION pos;
	char buffer[255];
	if (description) {
		sprintf(buffer,_("Description: %s"),description);
		wnd->InsertItem(buffer,-1,-1,rootitem);
	}
	sprintf(buffer,wxString(_("IP"))+": %s",ipfull);
	wnd->InsertItem(buffer,-1,-1,rootitem);
	sprintf(buffer,wxString(_("Port"))+": %i",port);
	wnd->InsertItem(buffer,-1,-1,rootitem);
	if (ping) {
		sprintf(buffer,wxString(_("Ping"))+": %i",ping);
		wnd->InsertItem(buffer,-1,-1,rootitem);
	}
	if (users) {
		sprintf(buffer,_("User: %i"),users);
		wnd->InsertItem(buffer,-1,-1,rootitem);
	}
	if (files) {
		sprintf(buffer,wxString(_("Files"))+": %i",files);
		wnd->InsertItem(buffer,-1,-1,rootitem);
	}
	CTag* cur_tag;
	for(int i = 0; i != taglist->GetCount();i++) {
		pos = taglist->FindIndex(i);
		cur_tag = taglist->GetAt(pos);
		if (cur_tag->tag->specialtag)
			continue;
		if (cur_tag->tag->type == 2)
			sprintf(buffer,"%s: %s",cur_tag->tag->tagname,cur_tag->tag->stringvalue);
		else if (cur_tag->tag->type == 3)
			sprintf(buffer,"%s: %i",cur_tag->tag->tagname,cur_tag->tag->intvalue);
		else
			continue;
		wnd->InsertItem(buffer,-1,-1,rootitem);
   }
#endif
	printf("todo. fill window tags\n");
}

char* CServer::GetAddress()
{
	if (dynip) {
		return dynip;
	} else {
		return ipfull;
	}
}

void CServer::SetID(uint32 newip)
{
	ip = newip;
	in_addr host;
	host.s_addr = ip;
	strcpy(ipfull,inet_ntoa(host));
}

void CServer::SetDynIP(const char* newdynip)
{
	if (dynip) {
		delete[] dynip;
	}
	dynip = nstrdup(newdynip);
}
