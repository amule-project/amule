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

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/datetime.h>	// Needed for wxDateTime

#include "pixmaps/chat.ico.xpm"
#include "ChatSelector.h"	// Interface declarations
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "packets.h"		// Needed for Packet
#include "opcodes.h"		// Needed for OP_MESSAGE
#include "ListenSocket.h"	// Needed for CClientReqSocket
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
#define COLOR_BLACK wxTextAttr( wxColor(   0,   0,   0 ) )
#define COLOR_BLUE  wxTextAttr( wxColor(   0,   0, 255 ) )
#define COLOR_GREEN wxTextAttr( wxColor(   0, 102,   0 ) )
#define COLOR_RED   wxTextAttr( wxColor( 255,   0,   0 ) )



CChatSession::CChatSession(wxWindow* parent, wxWindowID id, const wxString& value, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name)
: CMuleTextCtrl( parent, id, value, pos, size, style | wxTE_READONLY | wxTE_RICH | wxTE_MULTILINE, validator, name )
{
	m_client = NULL;
	m_active = false;
}


CChatSession::~CChatSession()
{
	m_client->SetChatState(MS_NONE);
}


void CChatSession::AddText(const wxString& text, const wxTextAttr& style)
{
	wxString line;

	// Check if we should add a time-stamp
	if ( GetNumberOfLines() > 1 ) {
		// Check if the last line ended with a newline
		wxString line = GetLineText( GetNumberOfLines() - 1 );
		if ( line.IsEmpty() ) {
			SetDefaultStyle( COLOR_BLACK );

			AppendText( wxT(" [") + wxDateTime::Now().FormatTime() + wxT("] ") );
		}
	}
		

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
	chatsession->m_client = client;

	wxString text;
	text += wxString(_(" *** Chat-Session Started: ")) + client->GetUserName() + wxT(" - ");
	text += wxDateTime::Now().FormatDate() + wxT(" ") + wxDateTime::Now().FormatTime() + wxT("\n");
	
	chatsession->AddText( text, COLOR_RED );
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
		
		if( page->m_client == client ) {
			return page;
		}
	}
	
	return NULL;
}


int CChatSelector::GetTabByClient(CUpDownClient* client)
{
	for ( unsigned int i = 0; i < (unsigned int) GetPageCount(); i++ ) {
		CChatSession* page = (CChatSession*)GetPage( i );
		
		if( page->m_client == client ) {
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

	// Other client connected after disconnection or a new session
	if ( !session->m_active ) {
		session->m_active = true;
		
		session->AddText( _("*** Connected to Client ***\n"), COLOR_RED );
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
	if ( ci->m_client->GetChatState() == MS_CONNECTING )
		return false;

	ci->m_active = true;
	
	if (ci->m_client->IsConnected()) {
		CSafeMemFile data;
		data.WriteString(message);
		Packet* packet = new Packet(&data);
		packet->SetOpCode(OP_MESSAGE);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
		if ( ci->m_client->SendPacket(packet, true, true) ) {
			ci->AddText( thePrefs::GetUserNick(), COLOR_GREEN );
			ci->AddText( wxT(": ") + message + wxT("\n"), COLOR_BLACK );
		}
	} else {
		ci->AddText( _("*** Connecting to Client ***\n") , COLOR_RED );
		ci->m_pending = message;
		ci->m_client->SetChatState(MS_CONNECTING);
		ci->m_client->TryToConnect();
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
	
	ci->m_client->SetChatState( MS_CHATTING );
	if ( !success ) {
		if ( !ci->m_pending.IsEmpty() ) {
			ci->AddText( _("*** Failed to Connect ***\n"), COLOR_RED );
		} else if ( ci->m_active ) {
			ci->AddText( _("*** Disconnected from Client ***\n"), COLOR_RED );
		}
		
		ci->m_active = false;
		ci->m_pending.Clear();
	} else {
		// Kry - Woops, fix for the everlasting void message sending.
		if ( !ci->m_pending.IsEmpty() ) {
			ci->AddText( _("*** Connected to Client ***\n"), COLOR_RED );
			
			CSafeMemFile data;
			data.WriteString(ci->m_pending);
			Packet* packet = new Packet(&data);
			packet->SetOpCode(OP_MESSAGE);
			theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
			if ( ci->m_client->SendPacket(packet, true, true) ) {
				ci->AddText( thePrefs::GetUserNick(), COLOR_GREEN );
				ci->AddText( wxT(": ") + ci->m_pending + wxT("\n"), COLOR_BLACK );
			
				ci->m_pending.Clear();
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

		if ( page->m_client == toupdate->m_LinkedClient ) {
			SetPageText( i, toupdate->m_strName );
			break;
		};
	}	
}
