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
#include "otherfunctions.h"
#include "Friend.h"
#include "muuli_wdr.h"            // Needed for amuleSpecial



// Default colors, 
#define COLOR_BLACK wxTextAttr( wxColor( 255,   0,   0 ) )
#define COLOR_BLUE  wxTextAttr( wxColor(   0,   0, 255 ) )
#define COLOR_GREEN wxTextAttr( wxColor(   0, 102,   0 ) )
#define COLOR_RED   wxTextAttr( wxColor(   0,   0, 255 ) )



CChatSession::CChatSession(wxWindow* parent, wxWindowID id, const wxString& value, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name)
: wxTextCtrl( parent, id, value, pos, size, style | wxTE_READONLY | wxTE_RICH | wxTE_MULTILINE, validator, name )
{
	client = NULL;
}


CChatSession::~CChatSession()
{
	client->SetChatState(MS_NONE);
}


void CChatSession::AddText(const wxString& text, const wxTextAttr& style)
{
	SetDefaultStyle(style);
	
	AppendText(text);
}




CChatSelector::CChatSelector(wxWindow* parent, wxWindowID id, const wxPoint& pos, wxSize siz, long style)
: CMuleNotebook(parent, id, pos, siz, style)
{
	wxImageList* imagelist = new wxImageList(16,16);
	
	// Chat icon -- default state
	imagelist->Add(wxBitmap(chat_ico_xpm));
	// Close icon -- on mouseover
	imagelist->Add(amuleSpecial(4));
	
	AssignImageList(imagelist);
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

	wxString text = wxString(wxT("*** Chatsession Start : ")) + client->GetUserName() + wxT("\n");
	chatsession->AddText( text, COLOR_BLACK );
	AddPage(chatsession, client->GetUserName(), show, 0);
	
	client->SetChatState(MS_CHATTING);

	GetParent()->FindWindow(IDC_CSEND)->Enable(true);
	GetParent()->FindWindow(IDC_CCLOSE)->Enable(true);
	
	return chatsession;
}


CChatSession* CChatSelector::GetPageByClient(CUpDownClient* client)
{
	for ( unsigned int i = 0; i < (unsigned int ) GetPageCount(); i++ ) {
		CChatSession* page = (CChatSession*)GetPage( i );
		
		if( page->client == client ) {
			return page;
		}
	}
	
	return NULL;
}


int CChatSelector::GetTabByClient(CUpDownClient* client)
{
	for ( unsigned int i = 0; i < (unsigned int) GetPageCount(); i++ ) {
		CChatSession* page = (CChatSession*)GetPage( i );
		
		if( page->client == client ) {
			return i;
		}
	}
	
	return -1;
}


void CChatSelector::ProcessMessage(CUpDownClient* sender, const wxString& message)
{
	CChatSession* session = GetPageByClient(sender);

	if ( !session ) {
		session = StartSession( sender, true );
	}
	
	session->AddText( sender->GetUserName(), COLOR_BLUE );
	session->AddText( wxString(wxT(": ")) + message + wxString(wxT("\n")), COLOR_BLACK );
}


bool CChatSelector::SendMessage( const wxString& message )
{
	// Dont let the user send empty messages
	// This is also a user-fix for people who mash the enter-key ...
	if ( message.IsEmpty() )
		return false;

	int usedtab = GetSelection();

	if (usedtab == -1) {
		return false;
	}

	// Workaround for a problem with wxNotebook, where an invalid selection is returned
	if (usedtab >= (int)GetPageCount()) {
		usedtab = GetPageCount() - 1;
	}
		
	
	CChatSession* ci = (CChatSession*)GetPage( usedtab );
	if ( ci->client->GetChatState() == MS_CONNECTING ) {
		return false;
	}
	
	if (ci->client->IsConnected()) {
		CSafeMemFile data;
		data.WriteString(message);
		Packet* packet = new Packet(&data);
		packet->SetOpCode(OP_MESSAGE);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
		if ( ci->client->SendPacket(packet, true, true) ) {
			ci->AddText( theApp.glob_prefs->GetUserNick(), COLOR_GREEN );
			ci->AddText( wxT(": ") + message + wxT("\n"), COLOR_BLACK );
		}
	} else {
		printf("Not connected to Chat. Trying to connect...\n");
		ci->AddText( wxString(wxT("*** ")) + wxString(wxT("Connecting")) , COLOR_RED );
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


void CChatSelector::ConnectionResult(CUpDownClient* sender, bool success)
{
	CChatSession* ci = GetPageByClient(sender);
	if ( !ci ) {
		return;
	}
	
	ci->client->SetChatState( MS_CHATTING );
	if ( !success ) {
		if ( !ci->messagepending.IsEmpty() ) {
			ci->AddText( wxString(wxT(" failed\n")) , COLOR_RED );
		} else {
			ci->AddText( wxString(wxT("*** Disconnected\n")), COLOR_RED );
		}
		
		ci->messagepending.Clear();
	} else {
		ci->AddText( wxString(wxT(" ok\n")), COLOR_RED );
		// Kry - Woops, fix for the everlasting void message sending.
		if (!ci->messagepending.IsEmpty()) {
			CSafeMemFile data;
			data.WriteString(ci->messagepending);
			Packet* packet = new Packet(&data);
			packet->SetOpCode(OP_MESSAGE);
			theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
			if ( ci->client->SendPacket(packet, true, true) ) {
				ci->AddText( theApp.glob_prefs->GetUserNick(), COLOR_GREEN );
				ci->AddText( wxT(": ") + ci->messagepending + wxT("\n"), COLOR_BLACK );
			
				ci->messagepending.Clear();
			}
		}
	}
	
}


void CChatSelector::EndSession(CUpDownClient* client)
{
	int usedtab;
	if (client) {
		usedtab = GetTabByClient(client);
	} else {
		usedtab = GetSelection();
	}

	if (usedtab == -1)
		return;

	DeletePage(usedtab);

	GetParent()->FindWindow(IDC_CSEND)->Enable(GetPageCount());
	GetParent()->FindWindow(IDC_CCLOSE)->Enable(GetPageCount());
}




// Refresh the tab assosiated with a friend
void CChatSelector::RefreshFriend(CFriend* toupdate)
{
	wxASSERT( toupdate );

	for ( unsigned int i = 0; i < (unsigned int)GetPageCount(); i++ ) {
		CChatSession* page = (CChatSession*)GetPage( i );

		if ( page->client == toupdate->m_LinkedClient ) {
			SetPageText( i, toupdate->m_strName );
			break;
		};
	}	
}
