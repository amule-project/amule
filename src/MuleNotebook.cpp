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


#include "MuleNotebook.h"	// Interface declarations

#include <wx/event.h>
#include <wx/app.h>

#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!

WX_DEFINE_OBJARRAY(SearchDataArray);
DEFINE_EVENT_TYPE(wxEVT_COMMAND_MULENOTEBOOK_PAGE_CLOSED)

BEGIN_EVENT_TABLE(CMuleNotebook, wxNotebook)
	EVT_RIGHT_DOWN(CMuleNotebook::OnRMButton)
	// Madcat - tab closing engine
	EVT_LEFT_DOWN(CMuleNotebook::MouseClick)
	EVT_LEFT_DCLICK(CMuleNotebook::MouseClick)
	EVT_MOTION(CMuleNotebook::MouseMotion)
END_EVENT_TABLE()

CMuleNotebook::~CMuleNotebook() {
	tab_data_array.Clear();
}

bool CMuleNotebook::DeletePage(int nPage) {
	// Remove from array
	tab_data_array.RemoveAt(nPage);

	// Send out close event
	wxNotebookEvent evt(wxEVT_COMMAND_MULENOTEBOOK_PAGE_CLOSED, GetId(), nPage);
	evt.SetEventObject(this);
	GetEventHandler()->ProcessEvent(evt);

	// and finally remove the actual page
	return(wxNotebook::DeletePage((size_t)nPage));
}


bool CMuleNotebook::DeleteAllPages() {
	tab_data_array.Clear();

	return(wxNotebook::DeleteAllPages());
}

// Specific for CMuleNotebook

unsigned long CMuleNotebook::GetUserData(int nPage) const {
	return (tab_data_array.Item(nPage));
}

void CMuleNotebook::SetUserData(int nPage,unsigned long itemData) {
	tab_data_array.RemoveAt(nPage);
	tab_data_array.Insert(itemData, nPage);
}

bool CMuleNotebook::InsertPage(int position, wxNotebookPage* page, const wxString& text, bool select, int imageId, unsigned long itemData) {
	tab_data_array.Insert(itemData, position);
	return (wxNotebook::InsertPage((size_t)position, page, text, select, imageId));
}

bool CMuleNotebook::AddPage(wxNotebookPage* page, const wxString& text, bool select, int imageId, unsigned long itemData) {
	tab_data_array.Add(itemData);
	return (wxNotebook::AddPage(page, text, select, imageId));
}

void CMuleNotebook::OnRMButton(wxMouseEvent& event) {
	wxMouseEvent evt(wxEVT_RIGHT_DOWN);
	if (this->GetMouseListener()) {
		evt.SetEventObject(this);
		evt.m_x=event.m_x;
		evt.m_y=event.m_y;
		wxPostEvent(this->GetMouseListener(),evt);
	}
}

/**
 * Copyright (c) 2004 Alo Sarv <madcat_@users.sourceforge.net>
 * Most important function in this class. Here we do some serious math to figure
 * out where pages are located, where close-buttons are located etc.
 * @widths array contains the width in pixels of each page
 * @begins array contains the tab beginnings locations relative to window
 *         left border
 * @ends array contains the tab ends locations relative to window left border.
 *
 * First we clear all 3 arrays, and then fill with zeroes. The latter is being
 * done because we need to do +='s in loops and we want to start out at zeros.
 * Then we loop through pages list, measure the text label and image label
 * sizes, add the space the underlying platform adds (needs #defining for other
 * platforms), and fill the arrays with the data. Important notice: The FIRST
 * notebook tab is 3 pixels wider than the rest (at least on GTK)!
 */
void CMuleNotebook::CalculatePositions() {
int i;                       // Loop counter
int imagesizex, imagesizey;  // Notebookpage image size
int textsizex, textsizey;    // Notebookpage text size

	if (GetImageList() == NULL) {
		return; // No images
	}

	// Reset the arrays
	widths.Clear();
	begins.Clear();
	ends.Clear();
	widths.Alloc(GetPageCount());
	begins.Alloc(GetPageCount());
	ends.Alloc(GetPageCount());

	// Fill the arrays with zeros
	for (i=0;i<GetPageCount();i++) {
		widths.Add(0);
		begins.Add(0);
		ends.Add(0);
	}

	// Loop through all pages and calculate their widths.
	// Store all page begins, ends and widths in the arrays.
	for (i=0;i<GetPageCount();i++) {
		GetImageList()->GetSize(
			GetPageImage(i), imagesizex, imagesizey
		);
		GetTextExtent(GetPageText(i), &textsizex, &textsizey);
		widths[i] = 17+imagesizex+textsizex;
		if (i==0) {                              // first page
			begins[i]=0;
			ends[i]=widths[i];
		} else {                                 // other pages
			// Pages after first one are 3 pixels shorter
			widths[i]-=3;
			// Start 1 pixel after previous one
			begins[i]=ends[i-1]+1;
			// End is beginning + width
			ends[i]=begins[i]+widths[i];
		}
	}
}

/**
 * Copyright (c) 2004 Alo Sarv <madcat_@users.sourceforge.net>
 * This method handles mouse clicks on tabs. We need to detect here if the
 * click happened to be on our close button, thus we first request positions
 * recalculation, and then compare the event position to our known close
 * buttons locations. If found, close the neccesery tab.
 */
void CMuleNotebook::MouseClick(wxMouseEvent &event) {
long posx, posy;             // Mouse position at the time of the event
int i;                       // Loop counter

	if (GetImageList() == NULL) {
		event.Skip();
		return; // No images
	}

	CalculatePositions();

	event.GetPosition(&posx, &posy);

	// Determine which page was under the mouse
	for (i=0;i<GetPageCount();i++) {
		if (posx >= begins[i] && posx <= ends[i]) {
			// Found it, check if image was hit
			// Notice: (GTK) First tab is 3 pixels wider, thus the
			//         inline ifs.
			// TODO: Use #defines instead of hardcoded constants and
			//       correct values for GTK2, Mac and MSW.
			if (
				// Horizontal positioning
				posx >= begins[i]+(i?6:9) &&
				posx <= begins[i]+(i?6:9)+12 &&
				// Vertical positioning
				posy >= 11 &&
				posy <= 22
			) {
				// Image was hit, close the page
				// and return w/o passing event to wx
				DeletePage(i);
				return;
			}
		}
	}
	event.Skip();
}

/**
 * Copyright (c) 2004 Alo Sarv <madcat_@users.sourceforge.net>
 * This method handles mouse moving events. Since we can't recalculate positions
 * in EVT_MOUSE_ENTER (for some reason, wxNotebook doesn't receive those events)
 * we have to request recalculation here also, which is rather CPU-heavy.
 * Nonetheless, after we have updated positions in arrays, we can compare the
 * event position to our known close button locations, and if found, highlight
 * the neccesery button.
 */
void CMuleNotebook::MouseMotion(wxMouseEvent &event) {
long posx, posy;                        // Event X and Y positions
int i;                                  // Loop counter
	if (GetImageList() == NULL) {
		event.Skip();
		return; // No images
	}

	CalculatePositions();

	posx = event.m_x;
	posy = event.m_y;

	// Determine which page was under the mouse
	for (i=0;i<GetPageCount();i++) {
		SetPageImage(i, 0);
		if (posx >= begins[i] && posx <= ends[i]) {
			// Found it, check if image was hit
			// Notice: First tab is 3 pixels wider, thus the inline ifs
			if (
				// Horizontal positioning
				posx >= begins[i]+(i?6:9) &&
				posx <= begins[i]+(i?6:9)+12 &&
				// Vertical positioning
				posy >= 11 &&
				posy <= 22
			) {
				// Image is under mouse, change to highlight
				SetPageImage(i, 1);
			}
		}
	}
	event.Skip();
}
