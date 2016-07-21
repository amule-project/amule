//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/menu.h>
#include <wx/intl.h>

#include "MuleNotebook.h"	// Interface declarations

#include <common/MenuIDs.h>

DEFINE_LOCAL_EVENT_TYPE(wxEVT_COMMAND_MULENOTEBOOK_PAGE_CLOSING)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_COMMAND_MULENOTEBOOK_ALL_PAGES_CLOSED)

BEGIN_EVENT_TABLE(CMuleNotebook, wxNotebook)
	EVT_RIGHT_DOWN(CMuleNotebook::OnRMButton)

	EVT_MENU(MP_CLOSE_TAB,			CMuleNotebook::OnPopupClose)
	EVT_MENU(MP_CLOSE_ALL_TABS,		CMuleNotebook::OnPopupCloseAll)
	EVT_MENU(MP_CLOSE_OTHER_TABS,	CMuleNotebook::OnPopupCloseOthers)

	// Madcat - tab closing engine
	EVT_LEFT_UP(CMuleNotebook::OnMouseLeftRelease)
	EVT_MOTION(CMuleNotebook::OnMouseMotion)
END_EVENT_TABLE()

CMuleNotebook::CMuleNotebook( wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name )
	: wxNotebook(parent, id, pos, size, style, name)
{
	m_popup_enable = true;
	m_popup_widget = NULL;
}


CMuleNotebook::~CMuleNotebook()
{
	// Ensure that all notifications gets sent
	DeleteAllPages();
}


bool CMuleNotebook::DeletePage(int nPage)
{
	wxCHECK_MSG((nPage >= 0) && (nPage < (int)GetPageCount()), false,
		wxT("Trying to delete invalid page-index in CMuleNotebook::DeletePage"));

	// Send out close event
	wxNotebookEvent evt( wxEVT_COMMAND_MULENOTEBOOK_PAGE_CLOSING, GetId(), nPage );
	evt.SetEventObject(this);
	ProcessEvent( evt );

	// and finally remove the actual page
	bool result = wxNotebook::DeletePage( nPage );

	// Ensure a valid selection
	if ( GetPageCount() && (int)GetSelection() >= (int)GetPageCount() ) {
		SetSelection( GetPageCount() - 1 );
	}

	// Send a page change event to work around wx problem when newly selected page
	// is identical with deleted page (wx sends a page change event during deletion,
	// but the control is still the one to be deleted at that moment).
	if (GetPageCount()) {
		// Select the tab that took the place of the one we just deleted.
		size_t page = nPage;
		// Except if we deleted the last one - then select the one that is last now.
		if (page == GetPageCount()) {
			page--;
		}
		wxNotebookEvent event( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, GetId(), page );
		event.SetEventObject(this);
		ProcessEvent( event );
	} else {
	// Send an event when no pages are left open
		wxNotebookEvent event( wxEVT_COMMAND_MULENOTEBOOK_ALL_PAGES_CLOSED, GetId() );
		event.SetEventObject(this);
		ProcessEvent( event );
	}

	return result;
}


bool CMuleNotebook::DeleteAllPages()
{
	Freeze();

	bool result = wxNotebook::DeleteAllPages();

	// Send an event when no pages are left open
	wxNotebookEvent event( wxEVT_COMMAND_MULENOTEBOOK_ALL_PAGES_CLOSED, GetId() );
	event.SetEventObject(this);
	ProcessEvent( event );

	Thaw();

	return result;
}


void CMuleNotebook::EnablePopup( bool enable )
{
	m_popup_enable = enable;
}


void CMuleNotebook::SetPopupHandler( wxWindow* widget )
{
	m_popup_widget = widget;
}


//#warning wxMac does not support selection by right-clicking on tabs!
void CMuleNotebook::OnRMButton(wxMouseEvent& event)
{
	// Cases where we shouldn't be showing a popup-menu.
	if ( !GetPageCount() || !m_popup_enable ) {
		event.Skip();
		return;
	}


// For some reason, gtk1 does a rather poor job when using the HitTest
	wxPoint eventPoint = event.GetPosition();

	int tab = HitTest(eventPoint);
	if (tab != wxNOT_FOUND) {
		SetSelection(tab);
	} else {
		event.Skip();
		return;
	}

	// Should we send the event to a specific widget?
	if ( m_popup_widget ) {
		wxMouseEvent evt = event;

		// Map the coordinates onto the parent
		wxPoint point = evt.GetPosition();
		point = ClientToScreen( point );
		point = m_popup_widget->ScreenToClient( point );

		evt.m_x = point.x;
		evt.m_y = point.y;

		m_popup_widget->GetEventHandler()->AddPendingEvent( evt );
	} else {
		wxMenu menu(_("Close"));
		menu.Append(MP_CLOSE_TAB, wxString(_("Close tab")));
		menu.Append(MP_CLOSE_ALL_TABS, wxString(_("Close all tabs")));
		menu.Append(MP_CLOSE_OTHER_TABS, wxString(_("Close other tabs")));

		PopupMenu( &menu, event.GetPosition() );
	}
}


void CMuleNotebook::OnPopupClose(wxCommandEvent& WXUNUSED(evt))
{
	DeletePage( GetSelection() );
}


void CMuleNotebook::OnPopupCloseAll(wxCommandEvent& WXUNUSED(evt))
{
	DeleteAllPages();
}


void CMuleNotebook::OnPopupCloseOthers(wxCommandEvent& WXUNUSED(evt))
{
	wxNotebookPage* current = GetPage( GetSelection() );

	for ( int i = GetPageCount() - 1; i >= 0; i-- ) {
		if ( current != GetPage( i ) )
			DeletePage( i );
	}
}


void CMuleNotebook::OnMouseLeftRelease(wxMouseEvent &event)
{

	if (GetImageList() == NULL) {
		// This Mulenotebook has no images on tabs, so nothing to do.
		event.Skip();
		return;
	}

	long xpos, ypos;
	event.GetPosition(&xpos, &ypos);

	long flags = 0;
	int tab = HitTest(wxPoint(xpos,ypos),&flags);

	if ((tab != -1) &&  (flags == wxNB_HITTEST_ONICON)) {
		// User did click on a 'x'
		DeletePage(tab);
	} else {
		// Is not a 'x'. Send this event up.
		event.Skip();
	}

}

void CMuleNotebook::OnMouseMotion(wxMouseEvent &event)
{

	if (GetImageList() == NULL) {
		// This Mulenotebook has no images on tabs, so nothing to do.
		event.Skip();
		return;
	}

	long flags = 0;
	int tab = HitTest(wxPoint(event.m_x,event.m_y),&flags);

	// Clear the highlight for all tabs.
	for (int i=0;i<(int)GetPageCount();++i) {
		SetPageImage(i, 0);
	}

	if ((tab != -1) &&  (flags == wxNB_HITTEST_ONICON)) {
		// Mouse is over a 'x'
		SetPageImage(tab, 1);
	} else {
		// Is not a 'x'. Send this event up.
		event.Skip();
	}

}

// File_checked_for_headers
