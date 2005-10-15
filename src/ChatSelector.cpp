//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/datetime.h>	// Needed for wxDateTime
#include <wx/tokenzr.h>

#include "pixmaps/chat.ico.xpm"
#include "ChatSelector.h"	// Interface declarations
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "Packet.h"		// Needed for CPacket
#include "OPCodes.h"		// Needed for OP_MESSAGE
#include "Preferences.h"	// Needed for CPreferences
#include "ChatWnd.h"		// Needed for CChatWnd
#ifdef __WXMSW__
	#include <wx/msw/winundef.h> // Needed to be able to include wx headers
#endif
#include "amule.h"		// Needed for theApp
#include "updownclient.h"	// Needed for CUpDownClient
#include "Color.h"		// Needed for RGB
#include "FriendListCtrl.h"	// Needed for CDlgFriend
#include "OtherFunctions.h"
#include "muuli_wdr.h"		// Needed for amuleSpecial

#warning Needed while not ported
#include "Friend.h"
#include "FriendList.h"
#include "ClientList.h"
#include "Format.h"		// Needed for CFormat


// Default colors, 
#define COLOR_BLACK wxTextAttr( wxColor(   0,   0,   0 ) )
#define COLOR_BLUE  wxTextAttr( wxColor(   0,   0, 255 ) )
#define COLOR_GREEN wxTextAttr( wxColor(   0, 102,   0 ) )
#define COLOR_RED   wxTextAttr( wxColor( 255,   0,   0 ) )

CChatSession::CChatSession(wxWindow* parent, wxWindowID id, const wxString& value, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name)
: CMuleTextCtrl( parent, id, value, pos, size, style | wxTE_READONLY | wxTE_RICH | wxTE_MULTILINE, validator, name )
{
	m_client_id = 0;
	m_active = false;
	SetBackgroundColour(*wxWHITE);
}


CChatSession::~CChatSession()
{
	#warning EC NEEDED
	#ifndef CLIENT_GUI
	theApp.clientlist->SetChatState(m_client_id,MS_NONE);
	#endif
}


void CChatSession::AddText(const wxString& text, const wxTextAttr& style, bool newline)
{
	// Split multi-line messages into individual lines
	wxStringTokenizer tokens( text, wxT("\n") );

	while ( tokens.HasMoreTokens() ) {
		// Check if we should add a time-stamp
		if ( GetNumberOfLines() > 1 ) {
			// Check if the last line ended with a newline
			wxString line = GetLineText( GetNumberOfLines() - 1 );
			if ( line.IsEmpty() ) {
				SetDefaultStyle( COLOR_BLACK );

				AppendText( wxT(" [") + wxDateTime::Now().FormatISOTime() + wxT("] ") );
			}
		}
		
		SetDefaultStyle(style);
	
		AppendText( tokens.GetNextToken() );

		// Only add newlines after the last line if it is desired
		if ( tokens.HasMoreTokens() || newline ) {
			AppendText( wxT("\n") );
		}
	}
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

CChatSession* CChatSelector::StartSession(uint64 client_id, const wxString& client_name, bool show) 
{

	// Check to see if we've already opened a session for this user
	if ( GetPageByClientID( client_id ) ) {
		if ( show ) {
		  SetSelection( GetTabByClientID( client_id ) );
		}

		return NULL;
	}

	CChatSession* chatsession = new CChatSession(this);
	
	chatsession->m_client_id = client_id;

	wxString text;
	text = wxT(" *** ") + (CFormat(_("Chat-Session Started: %s - %s %s")) % client_name % wxDateTime::Now().FormatISODate() % wxDateTime::Now().FormatISOTime());
	
	chatsession->AddText( text, COLOR_RED );
	AddPage(chatsession, client_name, show, 0);
	
	return chatsession;
}


CChatSession* CChatSelector::GetPageByClientID(uint64 client_id)
{
	for ( unsigned int i = 0; i < (unsigned int ) GetPageCount(); i++ ) {
		CChatSession* page = (CChatSession*)GetPage( i );
		
		if( page->m_client_id == client_id ) {
			return page;
		}
	}
	
	return NULL;
}


int CChatSelector::GetTabByClientID(uint64 client_id)
{
	for ( unsigned int i = 0; i < (unsigned int) GetPageCount(); i++ ) {
		CChatSession* page = (CChatSession*)GetPage( i );
		
		if( page->m_client_id == client_id ) {
			return i;
		}
	}
	
	return -1;
}


bool CChatSelector::ProcessMessage(uint64 sender_id, const wxString& message)
{
	CChatSession* session = GetPageByClientID(sender_id);

	// Try to get the name (core sent it?)
	int separator = message.Find(wxT("|"));
	wxString client_name;
	wxString client_message;
	if (separator != -1) {
		client_name = message.Left(separator);
		client_message = message.Mid(separator+1);
	} else {
		// No need to define client_name. If needed, will be build on tab creation.
		client_message = message;
	}
	
	bool newtab = !session;
	
	if ( !session ) {
		// This must be a mesage from a client that is not already chatting 
		if (client_name.IsEmpty()) {
			// Core did not send us the name.
			// This must NOT happen.
			// Build a client name based on the ID
			uint32 ip = IP_FROM_GUI_ID(sender_id);
			client_name =  wxString::Format(wxT("IP: %u.%u.%u.%u Port: %u"),(uint8)ip,(uint8)(ip>>8),(uint8)(ip>>16),(uint8)(ip>>24),(unsigned)PORT_FROM_GUI_ID(sender_id));
		}
		
		session = StartSession( sender_id, client_name, true );
	}

	// Other client connected after disconnection or a new session
	if ( !session->m_active ) {
		session->m_active = true;
		
		session->AddText( _("*** Connected to Client ***"), COLOR_RED );
	}
	
	// Page text is client name
	session->AddText( GetPageText(GetTabByClientID(sender_id)), COLOR_BLUE, false );
	session->AddText( wxT(": ") + client_message, COLOR_BLACK );
	
	return newtab;
}

bool CChatSelector::SendMessage( const wxString& message, const wxString& client_name, uint64 to_id )
{
	// Dont let the user send empty messages
	// This is also a user-fix for people who mash the enter-key ...
	if ( message.IsEmpty() ) {
		return false;
	}
	
	if (to_id) {
		// Checks if there's a page with this client, and selects it or creates it
		StartSession(to_id, client_name, true);
	}
	
	int usedtab = GetSelection();	
	// Workaround for a problem with wxNotebook, where an invalid selection is returned
	if (usedtab >= (int)GetPageCount()) {
		usedtab = GetPageCount() - 1;
	}
	if (usedtab == -1) {
		return false;
	}
	
	CChatSession* ci = (CChatSession*)GetPage( usedtab );

	ci->m_active = true;
	
	#warning EC needed here.
	
	#ifndef CLIENT_GUI
	if (theApp.clientlist->SendMessage(ci->m_client_id, message)) {
		ci->AddText( thePrefs::GetUserNick(), COLOR_GREEN, false );
		ci->AddText( wxT(": ") + message, COLOR_BLACK );
	} else {
		ci->AddText( _("*** Connecting to Client ***"), COLOR_RED );
	}
	#endif

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


void CChatSelector::ConnectionResult(bool success, const wxString& message, uint64 id)
{
	CChatSession* ci = GetPageByClientID(id);
	if ( !ci ) {
		return;
	}
	
	if ( !success ) {
		ci->AddText( _("*** Failed to Connect to client / Connection lost ***"), COLOR_RED );
	
		ci->m_active = false;
	} else {
		// Kry - Woops, fix for the everlasting void message sending.
		if ( !message.IsEmpty() ) {
			ci->AddText( _("*** Connected to Client ***"), COLOR_RED );
			ci->AddText( thePrefs::GetUserNick(), COLOR_GREEN, false );
			ci->AddText( wxT(": ") + message, COLOR_BLACK );
		}
	}
}


void CChatSelector::EndSession(uint64 client_id)
{
	int usedtab;
	if (client_id) {
		usedtab = GetTabByClientID(client_id);
	} else {
		usedtab = GetSelection();
	}

	if (usedtab == -1) {
		return;
	}

	DeletePage(usedtab);
}


// Refresh the tab assosiated with a client
void CChatSelector::RefreshFriend(uint64 toupdate_id, const wxString& new_name)
{
	wxASSERT( toupdate_id );

	int tab = GetTabByClientID(toupdate_id); 
	
	if (tab != -1) {
		// This client has a tab.
		SetPageText(tab,new_name);
	} else {
		// This client has no tab (friend disconnecting, etc)
		// Nothing to be done here.
	}
}
