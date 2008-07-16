//
// This file is part of the aMule Project.
//
// Copyright (c) 2004 aMule Team ( http://www.amule-project.net )
//
// Copyright (c) Angel Vidal Veiga ( kry@users.sourceforge.net )
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

#ifndef MULENOTEBOOK_H
#define MULENOTEBOOK_H


#include <wx/notebook.h>


BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(wxEVT_COMMAND_MULENOTEBOOK_PAGE_CLOSED, 4804)
END_DECLARE_EVENT_TYPES()

#define EVT_MULENOTEBOOK_PAGE_CLOSED(id, fn)		\
	DECLARE_EVENT_TABLE_ENTRY(						\
		wxEVT_COMMAND_MULENOTEBOOK_PAGE_CLOSED,		\
		id,											\
		-1,											\
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

protected:
	// Madcat - closing engine
	void CalculatePositions();   // Fills the widths/begins/ends arrays
	void MouseClick(wxMouseEvent &event);  // Mouse clicks event handler
	void MouseMotion(wxMouseEvent &event); // Mouse moving around
	wxArrayInt widths, begins, ends;       // Positions of tabs

	
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

#endif
