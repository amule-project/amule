// This file is part of the aMule project.
//
// Copyright (c) 2004, aMule team
//
// Copyright (c) Angel Vidal Veiga (kry@users.sourceforge.net)
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

#include <wx/dynarray.h>		// Needed for WX_DECLARE_LIST
#include <wx/notebook.h>

typedef unsigned long item_data;

WX_DECLARE_OBJARRAY(item_data, SearchDataArray);

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(wxEVT_COMMAND_MULENOTEBOOK_PAGE_CLOSED,4804)
END_DECLARE_EVENT_TYPES()

#define EVT_MULENOTEBOOK_PAGE_CLOSED(id, fn)               \
	DECLARE_EVENT_TABLE_ENTRY(                         \
		wxEVT_COMMAND_MULENOTEBOOK_PAGE_CLOSED,    \
		id,                                        \
		-1,                                        \
		(wxObjectEventFunction)(wxEventFunction)(wxNotebookEventFunction) &fn,  \
		NULL                                                                    \
	),


class CMuleNotebook :public wxNotebook
{
public:
      // default for dynamic class
	CMuleNotebook() : wxNotebook() {
		m_listener=NULL;
	};
      // the same arguments as for wxControl

	CMuleNotebook(wxWindow *parent,
             wxWindowID id,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = 0,
             const wxString& name = wxT("notebook")) :
	wxNotebook(parent, id, pos, size, style, name) {
		
		m_listener=NULL;
	};

	~CMuleNotebook();

	// wxNotebook

	bool DeletePage(int nPage);
	
	bool DeleteAllPages();
		
	// Specific for CMuleNotebook

	bool AddPage(wxNotebookPage* page, const wxString& text, bool select = false, int imageId = -1, unsigned long itemData = 0);
	unsigned long GetUserData(int nPage) const; 
	void SetUserData(int nPage,unsigned long itemData); 

	// sets the size of the tabs (assumes all tabs are the same size)
	// Seems never used
	//void SetTabSize(const wxSize& sz); // NEW?
	
	// adds a new page to the notebook (it will be deleted ny the notebook,
	// don't delete it yourself). If bSelect, this page becomes active.
	// the same as AddPage(), but adds it at the specified position
	bool InsertPage( int position,
                     wxNotebookPage *page,
                     const wxString& text,
                     bool select = FALSE,
                     int imageId = -1,unsigned long itemData=0 ); // Item data is the new one

	void SetMouseListener(wxEvtHandler* _listener) {
		m_listener=_listener;
	}
	
	wxEvtHandler* GetMouseListener() {
		return m_listener;
	}
	
	wxMutex m_LockTabs;

private:
	SearchDataArray tab_data_array;
	wxEvtHandler* m_listener;

	// Madcat - closing engine
	void CalculatePositions();   // Fills the widths/begins/ends arrays
	void MouseClick(wxMouseEvent &event);  // Mouse clicks event handler
	void MouseMotion(wxMouseEvent &event); // Mouse moving around
	wxArrayInt widths, begins, ends;       // Positions of tabs

protected:
	void OnRMButton(wxMouseEvent& event);	
	DECLARE_EVENT_TABLE()
};

#endif // MULENOTEBOOK_H
