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

#if MULE_NEEDS_DELETEPAGE_WORKAROUND
DEFINE_LOCAL_EVENT_TYPE(wxEVT_COMMAND_MULENOTEBOOK_DELETE_PAGE)
#endif

BEGIN_EVENT_TABLE(CMuleNotebook, wxNotebook)
	EVT_RIGHT_DOWN(CMuleNotebook::OnRMButton)

	EVT_MENU(MP_CLOSE_TAB,		CMuleNotebook::OnPopupClose)
	EVT_MENU(MP_CLOSE_ALL_TABS,	CMuleNotebook::OnPopupCloseAll)
	EVT_MENU(MP_CLOSE_OTHER_TABS,	CMuleNotebook::OnPopupCloseOthers)

	// Madcat - tab closing engine
	EVT_LEFT_DOWN(CMuleNotebook::OnMouseButtonRelease)
	EVT_LEFT_UP(CMuleNotebook::OnMouseButtonRelease)
	EVT_MIDDLE_DOWN(CMuleNotebook::OnMouseButtonRelease)
	EVT_MIDDLE_UP(CMuleNotebook::OnMouseButtonRelease)
	EVT_MOTION(CMuleNotebook::OnMouseMotion)
#if MULE_NEEDS_DELETEPAGE_WORKAROUND
	EVT_MULENOTEBOOK_DELETE_PAGE(wxID_ANY, CMuleNotebook::OnDeletePage)
#endif
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


#if MULE_NEEDS_DELETEPAGE_WORKAROUND
void CMuleNotebook::OnDeletePage(wxBookCtrlEvent& evt)
{
	int page = evt.GetSelection();
	DeletePage(page);
}
#endif // MULE_NEEDS_DELETEPAGE_WORKAROUND


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

	bool result = true;
	while ( GetPageCount() ) {
		result &= DeletePage( 0 );
	}

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


void CMuleNotebook::OnMouseButtonRelease(wxMouseEvent &event)
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
	static int tab_down_icon = -1;
	static int tab_down_label = -1;

	if (event.LeftDown() &&  (flags == wxNB_HITTEST_ONICON)) {
		tab_down_icon = tab;
		return;
	}
	else if (event.MiddleDown() && (flags == wxNB_HITTEST_ONLABEL)) {
		tab_down_label = tab;
		return;
	}
	else if (event.LeftDown() || event.MiddleDown()) {
		tab_down_icon = -1;
		tab_down_label = -1;
		return;
	}
	
	if (((tab != -1) &&  (((flags == wxNB_HITTEST_ONICON) && event.LeftUp() && (tab == tab_down_icon)) ||
			((flags == wxNB_HITTEST_ONLABEL) && event.MiddleUp() && (tab == tab_down_label))))) {
		// User did click on a 'x' or middle click on the label
		tab_down_icon = -1;
		tab_down_label = -1;
#if MULE_NEEDS_DELETEPAGE_WORKAROUND
		/*	WORKAROUND: Instead of calling DeletePage, we need to wait for the
		 *	mouse release signal to reach Gtk. Inconsistent with normal wxEvent
		 *	behaviour the button release handler in wxWidgets don't evaluate
		 *	the result of the signal handling. */
		wxNotebookEvent evt( wxEVT_COMMAND_MULENOTEBOOK_DELETE_PAGE, GetId(), tab );
		evt.SetEventObject(this);
		AddPendingEvent( evt );
#else
		DeletePage(tab);
#endif // MULE_NEEDS_DELETEPAGE_WORKAROUND
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
