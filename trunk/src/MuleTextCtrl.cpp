//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "MuleTextCtrl.h"
#include <wx/menu.h>
#include <wx/intl.h>
#include <wx/clipbrd.h>

/**
 * These are the IDs used to identify the different menu-items.
 *
 * Please note that I make use of predefined wxIDs for the first two, but not 
 * for Paste. This is because wxMenu poses some restrictions on what can be 
 * done with items using those IDs, and by default, Paste is enabled even if 
 * there's nothing to paste!
 */ 
enum CMTC_Events
{
	//! Cut text, uses provided ID
	CMTCE_Cut	= wxID_CUT,
	//! Copy text, uses privided ID
	CMTCE_Copy 	= wxID_COPY,
	//! Paste text, uses custom ID
	CMTCE_Paste = wxID_HIGHEST + 666,	// Random satanic ID
	//! Clear text, uses custom ID
	CMTCE_Clear,
	//! Select All text, uses custom ID
	CMTCE_SelAll
};


BEGIN_EVENT_TABLE(CMuleTextCtrl, wxTextCtrl)
#ifndef __WXGTK20__
	EVT_RIGHT_DOWN	(CMuleTextCtrl::OnRightDown)

	EVT_MENU    	(CMTCE_Paste,	CMuleTextCtrl::OnPaste)
	EVT_MENU    	(CMTCE_Clear,	CMuleTextCtrl::OnClear)
	EVT_MENU    	(CMTCE_SelAll,	CMuleTextCtrl::OnSelAll)
#endif
END_EVENT_TABLE()


CMuleTextCtrl::CMuleTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name)
 :  wxTextCtrl( parent, id, value, pos, size, style, validator, name)
{
}


void CMuleTextCtrl::OnRightDown( wxMouseEvent& evt )
{
	// If this control doesn't have focus, then set it
	if ( FindFocus() != this )
		SetFocus();

	wxMenu popup_menu;
	
	popup_menu.Append( CMTCE_Cut, _("Cut") );
	popup_menu.Append( CMTCE_Copy, _("Copy") );
	popup_menu.Append( CMTCE_Paste, _("Paste") );
	popup_menu.Append( CMTCE_Clear, _("Clear") );
	
	popup_menu.AppendSeparator();
	
	popup_menu.Append( CMTCE_SelAll, _("Select All") );


	// wxMenu will automatically enable/disable the Cut and Copy items,
	// however, were are a little more pricky about the Paste item than they
	// are, so we enable/disable it on our own, depending on whenever or not
	// there's actually something to paste
	bool canpaste = false;
	if ( CanPaste() ) {
		if ( wxTheClipboard->Open() ) {
			if ( wxTheClipboard->IsSupported( wxDF_TEXT ) ) {
				wxTextDataObject data;
	 			wxTheClipboard->GetData( data );

				canpaste = data.GetTextLength();
			}
			wxTheClipboard->Close();
		}
	}


	popup_menu.Enable( CMTCE_Paste,		canpaste );
	popup_menu.Enable( CMTCE_Clear,		IsEditable() && !GetValue().IsEmpty() );

	PopupMenu( &popup_menu, evt.GetX(), evt.GetY() );
}


void CMuleTextCtrl::OnPaste( wxCommandEvent& WXUNUSED(evt) )
{
	Paste();
}


void CMuleTextCtrl::OnSelAll( wxCommandEvent& WXUNUSED(evt) )
{
	// Move the pointer to the front
	SetInsertionPoint( 0 );

	// Selects everything
	SetSelection( -1, -1 );
}


void CMuleTextCtrl::OnClear( wxCommandEvent& WXUNUSED(evt) )
{
	Clear();
}


#ifdef __WXMAC__
#warning Remove this when wxMAC has been fixed.
// https://sourceforge.net/tracker/?func=detail&atid=109863&aid=1189859&group_id=9863
void CMuleTextCtrl::Clear()
{
	if (IsMultiLine()) {
		wxFont font = GetFont();
		wxTextCtrl::Clear();
		SetFont(font);
	} else {
		wxTextCtrl::Clear();
	}
}
#endif

