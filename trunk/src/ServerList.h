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


#ifndef SERVERLIST_H
#define SERVERLIST_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ServerList.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/timer.h>		// Needed for wxTimer

#include "types.h"		// Needed for int8, uint16 and uint32
#include "CTypedPtrList.h"	// Needed for CTypedPtrList

class CServer;
class Packet;

class CServerList
{
	friend class CServerListCtrl;
public:
	CServerList();
	~CServerList();
	bool		Init();
	bool		AddServer(CServer* in_server );
	void		RemoveServer(CServer* out_server);
	void		RemoveAllServers();
	void		RemoveDeadServers();	
	bool		AddServermetToList(const wxString& strFile, bool merge = true);
	void		AddServersFromTextFile(wxString strFilename,bool isstaticserver=true, bool writetolog = false);// emanuelw(20030924) added writetolog
	bool		SaveServermetToFile(); //<<--9/22/02
	void		ServerStats();
	void		ResetServerPos()	{serverpos = 0;}
	void		ResetSearchServerPos()	{searchserverpos = 0;}
	CServer*	GetNextServer();
	CServer*	GetNextSearchServer();
	CServer*	GetNextStatServer();
	CServer*	GetServerAt(uint32 pos)	{return list.GetAt(list.FindIndex(pos));}
	uint32		GetServerCount()	{return list.GetCount();}
	CServer*	GetNextServer(CServer* lastserver); // slow
	CServer*	GetServerByAddress(const wxString& address, uint16 port);
	CServer*	GetServerByIP(uint32 nIP);
	CServer*	GetServerByIP(uint32 nIP, uint16 nPort);	
	bool		IsGoodServerIP( CServer* in_server ); //<<--
	void		GetStatus( uint32 &total, uint32 &failed, uint32 &user, uint32 &file, uint32 &tuser, uint32 &tfile, float &occ);
	void		GetUserFileStatus( uint32 &user, uint32 &file);
	bool		BroadCastPacket(Packet* packet); //send Packet to all server in the list
	void		CancelUDPBroadcast();
	void		Sort();
	void		MoveServerDown(CServer* aServer);
	uint32		GetServerPostion()	{return serverpos;}
	void		SetServerPosition(uint32 newPosition) { if (newPosition<(uint32)list.GetCount() ) serverpos=newPosition; else serverpos=0;}
	uint32		GetDeletedServerCount()		{return delservercount;}
	void		Process();
	void		SendNextPacket();
	void 		UpdateServerMetFromURL(wxString strURL);	
	void		DownloadFinished(uint32 result);	
	void		AutoDownloadFinished(uint32 result);	
	
	//void static CALLBACK UDPTimerProc(HWND hwnd, unsigned int uMsg,unsigned int* idEvent,DWORD dwTime);
protected:
	void			AutoUpdate();
private:
	uint8			current_url_index;
	uint32		serverpos;
	uint32		searchserverpos;
	uint32		statserverpos;
	uchar		version;
	uint32		servercount;
	CTypedPtrList<CPtrList, CServer*>	list;
	//uint32		udp_timer;
	 AMULE_TIMER_CLASS        udp_timer;
	POSITION	broadcastpos;
	Packet*		broadcastpacket;
	uint32		delservercount;
	uint32		m_nLastSaved;
	uint32		m_nLastED2KServerLinkCheck;// emanuelw(20030924) added
	wxString		URLUpdate;
	wxString		URLAutoUpdate;
};

#endif // SERVERLIST_H
