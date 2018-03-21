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

#ifndef MULENOTEBOOK_H
#define MULENOTEBOOK_H

#include <wx/notebook.h>

#define MULE_NEEDS_DELETEPAGE_WORKAROUND	wxCHECK_VERSION(3,0,2)


DECLARE_LOCAL_EVENT_TYPE(wxEVT_COMMAND_MULENOTEBOOK_PAGE_CLOSING, -1)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_COMMAND_MULENOTEBOOK_ALL_PAGES_CLOSED, -1)

#if MULE_NEEDS_DELETEPAGE_WORKAROUND
DECLARE_LOCAL_EVENT_TYPE(wxEVT_COMMAND_MULENOTEBOOK_DELETE_PAGE, -1)

#define EVT_MULENOTEBOOK_DELETE_PAGE(id, fn)						\
	DECLARE_EVENT_TABLE_ENTRY(							\
		wxEVT_COMMAND_MULENOTEBOOK_DELETE_PAGE,					\
		id,									\
		-1,									\
		(wxObjectEventFunction)(wxEventFunction)(wxNotebookEventFunction) &fn,  \
		NULL                                                                    \
	),
#endif // MULE_NEEDS_DELETEPAGE_WORKAROUND

#define EVT_MULENOTEBOOK_PAGE_CLOSING(id, fn)						\
	DECLARE_EVENT_TABLE_ENTRY(							\
		wxEVT_COMMAND_MULENOTEBOOK_PAGE_CLOSING,					\
		id,									\
		-1,									\
		(wxObjectEventFunction)(wxEventFunction)(wxNotebookEventFunction) &fn,  \
		NULL                                                                    \
	),
#define EVT_MULENOTEBOOK_ALL_PAGES_CLOSED(id, fn)					\
	DECLARE_EVENT_TABLE_ENTRY(							\
		wxEVT_COMMAND_MULENOTEBOOK_ALL_PAGES_CLOSED,				\
		id,									\
		-1,									\
		(wxObjectEventFunction)(wxEventFunction)(wxNotebookEventFunction) &fn,  \
		NULL                                                                    \
	),


class wxWindow;


/**
 * This is an NoteBook control which adds additional features above what is
 * provided by the wxNoteBook widget. Currently it includes:
 *  - Use of images on the tabs for closing the pages.
 *  - A popup-menu for closing one or more pages.
 *  - Events triggered when pages are closed.
 */
class CMuleNotebook : public wxNotebook
{
public:
	/**
	 * Constructor.
	 *
	 * @see wxNotebook::wxNotebook
	 */
	CMuleNotebook( wxWindow *parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxT("notebook") );

	/**
	 * Destructor.
	 */
	virtual ~CMuleNotebook();

	/**
	 * Deletes the page and triggers an event.
	 *
	 * @param nPage The page to be removed.
	 */
	virtual bool DeletePage(int nPage);

	/**
	 * Deletes and triggers and event for every page.
	 */
	virtual bool DeleteAllPages();


	/**
	 * Enables or disables the displaying of a popup-menu.
	 *
	 * @param enabled The new setting.
	 */
	void EnablePopup( bool enable );

	/**
	 * Sets an external widget to handle the popup-event.
	 *
	 * @param widget The widget which would recieve the event or NULL to disable.
	 *
	 * Setting the handler to a non-NULL pointer means that upon right-clicks, a
	 * right click event will be sent to that widget, so that it can create a
	 * popup-menu. The coordinates will be fixed to fit onto the specified widget,
	 * so no mapping is needed.
	 */
	void SetPopupHandler( wxWindow* widget );

#if MULE_NEEDS_DELETEPAGE_WORKAROUND
private:
	// Internal handler. Workaround for wxWidgets Tab-Crash bug.
	void OnDeletePage(wxBookCtrlEvent& evt);
#endif // MULE_NEEDS_DELETEPAGE_WORKAROUND

protected:
	/**
	 * Event handler for left or middle mouse button to press or release (for closing pages)
	 */
	void OnMouseButton(wxMouseEvent &event);

	/**
	 * Event handler for mouse motion (for highlighting the 'x')
	 */
	void OnMouseMotion(wxMouseEvent &event);

	/**
	 * Event-handler for right-clicks that takes care of displaying the popup-menu.
	 */
	void OnRMButton(wxMouseEvent& event);

	/**
	 * Event-handler fo the Close item on the popup-menu.
	 */
	void OnPopupClose(wxCommandEvent& evt);

	/**
	 * Event-handler fo the CloseAll item on the popup-menu.
	 */
	void OnPopupCloseAll(wxCommandEvent& evt);

	/**
	 * Event-handler fo the CloseOthers item on the popup-menu.
	 */
	void OnPopupCloseOthers(wxCommandEvent& evt);

	//! Keeps track of the popup-menu being enabled or not.
	bool		m_popup_enable;

	//! The pointer to the widget which would recieve right-click events or NULL.
	wxWindow*	m_popup_widget;

	DECLARE_EVENT_TABLE()
};

#ifdef __WINDOWS__
	#define MULE_NOTEBOOK_TAB_HEIGHT 26
#else
	#define MULE_NOTEBOOK_TAB_HEIGHT 40
#endif

#endif
// File_checked_for_headers
