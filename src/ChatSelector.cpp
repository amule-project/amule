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


CChatSession::CChatSession(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
: wxHtmlWindow( parent, id, pos, size, style, name )
{
	client = NULL;
	SetBorders(5);
	int sizes[]={7,8,10,12,16,22,30};
	SetFonts("","",sizes);
}


void CChatSession::AddText(const wxString& text)
{
	log += text;
	SetText( log );
}

void CChatSession::SetText(const wxString& text)
{
	log = text;

	wxString result = "<html><body>" + text + "</body></html>";
	result.Replace("\n", "<br>\n");
	
	Freeze();
	SetPage(result);
	
	int x, y;
	GetVirtualSize(&x, &y);
	Scroll(-1, y);
	Thaw();
}



CChatSelector::CChatSelector(wxWindow* parent, wxWindowID id, const wxPoint& pos, wxSize siz, long style)
: wxNotebook(parent, id, pos, siz, style)
{
	imagelist.Add(wxBitmap(chat_ico_xpm));
	SetImageList(&imagelist);
}


CChatSession* CChatSelector::StartSession(CUpDownClient* client, bool show) 
{
	// Check to see if we've already opened a session for this user
	if ( GetPageByClient( client ) ) {
		if ( show ) {
		  SetSelection( GetTabByClient( client ) );
		}

		return NULL;
	}

	CChatSession* chatsession = new CChatSession(this);
	chatsession->client = client;

	wxString text = wxString(_("*** Chatsession Start : "))+(client->GetUserName()) + " - "+" (aMule client)\n";
	chatsession->SetText( ColorText( text, RGB(255, 0, 0) ) );
	AddPage(chatsession, client->GetUserName(), show, 0);
	
	client->SetChatState(MS_CHATTING);

	(wxButton*)(GetParent()->FindWindow(IDC_CSEND))->Enable(true);
	(wxButton*)(GetParent()->FindWindow(IDC_CCLOSE))->Enable(true);
	
	return chatsession;
}


CChatSession* CChatSelector::GetPageByClient(CUpDownClient* client)
{
	for ( int i = 0; i < GetPageCount(); i++ ) {
		CChatSession* page = (CChatSession*)GetPage( i );
		
		if( page->client == client ) {
			return page;
		}
	}
	
	return NULL;
}


int CChatSelector::GetTabByClient(CUpDownClient* client)
{
	for ( int i = 0; i < GetPageCount(); i++ ) {
		CChatSession* page = (CChatSession*)GetPage( i );
		
		if( page->client == client ) {
			return i;
		}
	}
	
	return -1;
}


void CChatSelector::ProcessMessage(CUpDownClient* sender, char* message)
{
	CChatSession* session = GetPageByClient(sender);

	if ( !session ) {
		session = StartSession( sender, true );
	}
	
	session->AddText( ColorText( wxString(sender->GetUserName()), RGB(0, 0, 255) ) );
	session->AddText( wxString(": ") + wxString(message) + wxString("\n") );
}


bool CChatSelector::SendMessage( const wxString& message )
{
	int usedtab = GetSelection();

	if (usedtab == -1) {
		return false;
	}

	// Workaround for a problem with wxNotebook, where an invalid selection is returned
	if (usedtab >= GetPageCount()) {
		usedtab = GetPageCount() - 1;
	}
		
	
	CChatSession* ci = (CChatSession*)GetPage( usedtab );
	if ( ci->client->GetChatState() == MS_CONNECTING ) {
		return false;
	}
	
	if (ci->client->socket && ci->client->socket->IsConnected()) {
		CMemFile data;
		data.Write(message);
		Packet* packet = new Packet(&data);
		packet->opcode = OP_MESSAGE;
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		ci->client->socket->SendPacket(packet, true, true);
		ci->AddText( ColorText( wxString(theApp.glob_prefs->GetUserNick()), RGB(0, 102, 0) ) );
		ci->AddText( ": " + message + "\n" );
	} else {
		printf("Not connected to Chat. Trying to connect...\n");
		ci->AddText( ColorText( wxString("*** ") + wxString(_("Connecting")) , RGB(255, 0, 0) ));
		ci->messagepending = message;
		ci->client->SetChatState(MS_CONNECTING);
		ci->client->TryToConnect();
		printf("Chat Connected\n");
	}
	
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
	CChatSession* ci = GetPageByClient(sender);
	if ( !ci ) {
		return;
	}
	
	ci->client->SetChatState( MS_CHATTING );
	if ( !success ) {
		if ( !ci->messagepending.IsEmpty() ) {
			ci->AddText( ColorText( wxString(" ") + wxString(_("failed")) + wxString("\n"), RGB(255, 0, 0) ) );
		} else {
			ci->AddText( ColorText( wxString(_("*** Disconnected")) + wxString("\n"), RGB(255, 0, 0) ) );
		}
		
		ci->messagepending.Clear();
	} else {
		ci->AddText( ColorText( wxString(" ok\n"), RGB(255, 0, 0) ) );
		
		CMemFile data;
		data.Write(wxString(ci->messagepending));
		Packet* packet = new Packet(&data);
		packet->opcode = OP_MESSAGE;
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		ci->client->socket->SendPacket(packet, true, true);
		
		ci->AddText( ColorText( wxString(theApp.glob_prefs->GetUserNick()), RGB(0, 102, 0) ) );
		ci->AddText( ": " + ci->messagepending + "\n" );
		
		ci->messagepending.Clear();
	}
	
}


void CChatSelector::EndSession(CUpDownClient* client)
{
	sint16 usedtab;
	if (client) {
		usedtab = GetTabByClient(client);
	} else {
		usedtab = GetSelection();
	}

	if (usedtab == -1)
		return;

	// Workaround for a problem with wxNotebook, where an invalid selection is returned
	if (usedtab >= GetPageCount()) {
		usedtab = GetPageCount() - 1;
	}
	
	
	CChatSession* session = (CChatSession*)GetPage(usedtab);
	session->client->SetChatState(MS_NONE);
	DeletePage(usedtab);


	(wxButton*)(GetParent()->FindWindow(IDC_CSEND))->Enable(GetPageCount());
	(wxButton*)(GetParent()->FindWindow(IDC_CCLOSE))->Enable(GetPageCount());
}


wxString CChatSelector::ColorText( const wxString& text, COLORREF iColor )
{
	return wxString::Format("<font color=\"#%06x\">%s</font>", iColor, text.c_str());
}
