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

// ChatSelector.cpp : implementation file
//

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _

#include "pixmaps/chat.ico.xpm"
#include "ChatSelector.h"	// Interface declarations
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "packets.h"		// Needed for Packet
#include "opcodes.h"		// Needed for OP_MESSAGE
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "SysTray.h"		// Needed for TBN_CHAT
#include "Preferences.h"	// Needed for CPreferences
#include "ChatWnd.h"		// Needed for CChatWnd
#ifdef __WXMSW__
	#include <wx/msw/winundef.h> // Needed to be able to include wx headers
#endif
#include "amule.h"			// Needed for theApp
#include "updownclient.h"	// Needed for CUpDownClient
#include "color.h"			// Needed for RGB
#include "SafeFile.h"		// Needed for CSafeMemFile



CChatSelector::CChatSelector(wxWindow* parent, wxWindowID id, const wxPoint& pos, wxSize siz, long style)
: wxNotebook(parent, id, pos, siz, style)
{
	imagelist.Add(wxBitmap(chat_ico_xpm));
	SetImageList(&imagelist);
}


CChatItem* CChatSelector::StartSession(CUpDownClient* client, bool show) 
{
	// Check to see if we've already opened a session for this user
	if ( GetTabByClient( client ) != -1 ) {
		if ( show ){
		  SetSelection( GetTabByClient(client) );
		  SetChatText( GetItemByClient(client)->log );
		}
		return NULL;
	}

	CChatItem* chatitem = new CChatItem();
	chatitem->client = client;

	m_items.AddTail(chatitem);

	wxString sessions = wxString(_("*** Chatsession Start : "))+(client->GetUserName()) + " - "+" (aMule client)\n";
	chatitem->log = ColorText( sessions, RGB(255, 0, 0) );
	client->SetChatState(MS_CHATTING);

	wxPanel* nullPanel = new wxPanel( this, -1);
	AddPage(nullPanel, client->GetUserName(), false, 0);

	if ( show ) {
	  SetSelection( GetPageCount() - 1 );
	  SetChatText( chatitem->log );
	}

	(wxButton*)(GetParent()->FindWindow(IDC_CSEND))->Enable(true);
	(wxButton*)(GetParent()->FindWindow(IDC_CCLOSE))->Enable(true);
	
	return chatitem;
}


void CChatSelector::SetChatText(const wxString& text)
{
	wxHtmlWindow* wnd = (wxHtmlWindow*)(GetParent()->FindWindow(ID_HTMLWIN));
	
	wxString result = "<html><body>" + text + "</body></html>";
	result.Replace("\n", "<br>\n");
	
	wnd->Freeze();
	wnd->SetPage(result);
	
	int x, y;
	wnd->GetVirtualSize(&x, &y);
	wnd->Scroll(-1, y);
	wnd->Thaw();
}


int CChatSelector::GetTabByClient(CUpDownClient* client)
{
	for ( int i = 0; i < GetPageCount(); i++ ) {
		CChatItem *item = m_items.GetAt(m_items.FindIndex(i));
		if(item->client == client) {
			return i;
		}
	}
	
	return -1;
}


CChatItem* CChatSelector::GetItemByClient(CUpDownClient* client)
{
	for (int i = 0; i < GetPageCount(); i++) {
		CChatItem* item=m_items.GetAt(m_items.FindIndex(i));
		if( item->client == client ) {
			return item;
		}
	}
	
	return NULL;
}


void CChatSelector::ProcessMessage(CUpDownClient* sender, char* message)
{
	CChatItem* ci = GetItemByClient(sender);

	if ( !ci ) {
		ci = StartSession(sender, true);
	}
	
	ci->log += ColorText( wxString(sender->GetUserName()), RGB(50, 200, 250) );
	ci->log += wxString(": ") + wxString(message) + wxString("\n");
	
	if ( GetSelection() == GetTabByClient(sender) ) {
		SetChatText(ci->log);
	}
}


bool CChatSelector::SendMessage(const wxString& message)
{
	if (GetSelection() == -1) {
		return false;
	}
	
	CChatItem* ci=m_items.GetAt(m_items.FindIndex( GetSelection() ));
	if (ci->client->GetChatState() == MS_CONNECTING) {
		return false;
	}
	
	if (ci->client->socket && ci->client->socket->IsConnected()) {
		CMemFile data;
		data.Write(message);
		Packet* packet = new Packet(&data);
		packet->opcode = OP_MESSAGE;
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		ci->client->socket->SendPacket(packet, true, true);
		ci->log +=  ColorText( wxString(theApp.glob_prefs->GetUserNick()), RGB(1, 180, 20) );
		ci->log += ": " + message + "\n";
	} else {
		printf("Not connected to Chat. Trying to connect...\n");
		ci->log += ColorText( wxString("*** ") + wxString(_("Connecting")) , RGB(255, 0, 0) );
		ci->messagepending = message;
		ci->client->SetChatState(MS_CONNECTING);
		ci->client->TryToConnect();
		printf("Chat Connected\n");
	}
	
	SetChatText(ci->log);

	return true;
}


//#warning Creteil?  I know you are here Creteil... follow the white rabbit.
/* Madcat - knock knock ...
	        ,-.,-.
            \ \\ \
             \ \\_\
             /     \
          __|    a a|
        /`   `'. = y)=
       /        `"`}
     _|    \       }
    { \     ),   //
     '-',  /__\ ( (
   jgs (______)\_)_)
*/


void CChatSelector::ConnectingResult(CUpDownClient* sender, bool success)
{
	CChatItem* ci = GetItemByClient(sender);
	if (!ci) {
		return;
	}
	
	ci->client->SetChatState(MS_CHATTING);
	if (!success) {
		if ( !ci->messagepending.IsEmpty() ) {
			ci->log += ColorText( wxString(" ") + wxString(_("failed")) + wxString("\n"), RGB(255, 0, 0) );
		} else {
			ci->log += ColorText( wxString(_("*** Disconnected")) + wxString("\n"), RGB(255, 0, 0) );
		}
		
		ci->messagepending.Clear();
	} else {
		ci->log += ColorText( wxString(" ok\n"), RGB(255, 0, 0) );
		
		CMemFile data;
		data.Write(wxString(ci->messagepending));
		Packet* packet = new Packet(&data);
		packet->opcode = OP_MESSAGE;
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		ci->client->socket->SendPacket(packet, true, true);
		
		ci->log += ColorText( wxString(theApp.glob_prefs->GetUserNick()), RGB(1, 180, 20) );
		ci->log += ": " + ci->messagepending + "\n";
		
		ci->messagepending.Clear();
	}
	
	if ( GetSelection() == GetTabByClient(sender) )	
		SetChatText(ci->log);
}


void CChatSelector::EndSession(CUpDownClient* client)
{
	sint16 usedtab;
	if (client) {
		usedtab = GetTabByClient(client);
	} else {
		usedtab = GetSelection();
	}
	
	if (usedtab == -1) {
		return;
	}
	
	if ( usedtab == GetSelection() )
		SetChatText("");
	
	CChatItem* ci = m_items.GetAt(m_items.FindIndex(usedtab));
	ci->client->SetChatState(MS_NONE);
	DeletePage(usedtab);
	m_items.RemoveAt(m_items.FindIndex(usedtab));
	
	delete ci;
	
	(wxButton*)(GetParent()->FindWindow(IDC_CSEND))->Enable(GetPageCount());
	(wxButton*)(GetParent()->FindWindow(IDC_CCLOSE))->Enable(GetPageCount());
}


wxString CChatSelector::ColorText( const wxString& text, COLORREF iColor )
{
	return wxString::Format("<font color=\"#%06x\">%s</font>", iColor, text.c_str());
}

