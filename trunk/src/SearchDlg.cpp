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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "SearchDlg.h"
#endif

#include <wx/sizer.h>
#include <wx/gauge.h>
#include <wx/textctrl.h>
#include <wx/msgdlg.h>
#include <wx/menu.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/choice.h>

#include "SearchDlg.h"		// Interface declarations.
#include "StringFunctions.h"		// Needed for unicode2char
#include "SearchListCtrl.h"	// Needed for CSearchListCtrl
#include "muuli_wdr.h"		// Needed for IDC_STARTS
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "MuleNotebook.h"
#include "GetTickCount.h"
#include "Preferences.h"
#include "OtherFunctions.h"	// Needed for URLEncode, GetTypeSize
#include "amule.h"			// Needed for theApp
#include "SearchList.h"		// Needed for CSearchList
#include "Format.h"
#include "Logger.h"

#define ID_SEARCHLISTCTRL wxID_HIGHEST+667

// just to keep compiler happy
static wxCommandEvent nullEvent;


BEGIN_EVENT_TABLE(CSearchDlg, wxPanel)
	EVT_BUTTON(		IDC_STARTS,		CSearchDlg::OnBnClickedStart)
	EVT_TEXT_ENTER(	IDC_SEARCHNAME,	CSearchDlg::OnBnClickedStart)
	
	EVT_BUTTON(IDC_CANCELS, CSearchDlg::OnBnClickedStop)
	
	EVT_LIST_ITEM_SELECTED(ID_SEARCHLISTCTRL, CSearchDlg::OnListItemSelected)
	
	EVT_BUTTON(IDC_SDOWNLOAD, CSearchDlg::OnBnClickedDownload)
	EVT_BUTTON(IDC_SEARCH_RESET, CSearchDlg::OnBnClickedReset)
	EVT_BUTTON(IDC_CLEAR_RESULTS, CSearchDlg::OnBnClickedClear)

	EVT_CHECKBOX(IDC_EXTENDEDSEARCHCHECK,CSearchDlg::OnExtendedSearchChange)
	EVT_CHECKBOX(IDC_FILTERCHECK,CSearchDlg::OnFilterCheckChange)
	
	EVT_MULENOTEBOOK_PAGE_CLOSED(ID_NOTEBOOK, CSearchDlg::OnSearchClosed)
	EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK, CSearchDlg::OnSearchPageChanged)

	// Event handlers for the parameter fields getting changed
	EVT_CUSTOM( wxEVT_COMMAND_TEXT_UPDATED,     IDC_SEARCHNAME, CSearchDlg::OnFieldChanged) 
	EVT_CUSTOM( wxEVT_COMMAND_TEXT_UPDATED,     IDC_EDITSEARCHEXTENSION, CSearchDlg::OnFieldChanged) 
	EVT_CUSTOM( wxEVT_COMMAND_SPINCTRL_UPDATED, wxID_ANY, CSearchDlg::OnFieldChanged)

	// Event handlers for the filter fields getting changed.
	EVT_TEXT_ENTER(ID_FILTER_TEXT,	CSearchDlg::OnFilteringChange)
	EVT_CHECKBOX(ID_FILTER_INVERT,	CSearchDlg::OnFilteringChange)
	EVT_CHECKBOX(ID_FILTER_KNOWN,	CSearchDlg::OnFilteringChange)
	EVT_BUTTON(ID_FILTER,			CSearchDlg::OnFilteringChange)
END_EVENT_TABLE()



CSearchDlg::CSearchDlg(wxWindow* pParent)
: wxPanel(pParent, -1)
{
	m_last_search_time = 0;

	wxSizer* content = searchDlg(this, true);
	content->Show(this, true);

	m_progressbar = CastChild( ID_SEARCHPROGRESS, wxGauge );
	m_progressbar->SetRange(100);
	
	m_notebook = CastChild( ID_NOTEBOOK, CMuleNotebook );

	// Initialise the image list
	wxImageList* m_ImageList = new wxImageList(16,16);
	m_ImageList->Add(amuleSpecial(3));
	m_ImageList->Add(amuleSpecial(4));
	m_notebook->AssignImageList(m_ImageList);

	CastChild( ID_SEARCHTYPE, wxChoice )->SetSelection(0);
	CastChild( IDC_TypeSearch, wxChoice )->SetSelection(0);
	CastChild( IDC_SEARCHMINSIZE, wxChoice )->SetSelection(2);
	CastChild( IDC_SEARCHMAXSIZE, wxChoice )->SetSelection(2);
	
	// Not there initially.
	s_searchsizer->Show(s_extendedsizer, false);
	s_searchsizer->Show(s_filtersizer, false);
	
	Layout();
}


CSearchDlg::~CSearchDlg()
{
}


CSearchListCtrl* CSearchDlg::GetSearchList( long id )
{
	int nPages = m_notebook->GetPageCount();
	for ( int i = 0; i < nPages; i++ ) {
		CSearchListCtrl* page = (CSearchListCtrl*)m_notebook->GetPage( i );

		if ( page->GetSearchId() == id )
			return page;	
	}

	return NULL;
}


void CSearchDlg::AddResult(CSearchFile* toadd)
{
	CSearchListCtrl* outputwnd = GetSearchList( toadd->GetSearchID() );
	
	if ( outputwnd ) {
		outputwnd->AddResult( toadd );
		
		// Update the result count
		UpdateHitCount( outputwnd );
	}
}


void CSearchDlg::UpdateResult(CSearchFile* toupdate)
{
	CSearchListCtrl* outputwnd = GetSearchList( toupdate->GetSearchID() );
	
	if ( outputwnd ) {
		outputwnd->UpdateResult( toupdate );
	
		// Update the result count
		UpdateHitCount( outputwnd );
	}
}


void CSearchDlg::OnListItemSelected(wxListEvent& WXUNUSED(event))
{
	FindWindow(IDC_SDOWNLOAD)->Enable(true);
}


void CSearchDlg::OnExtendedSearchChange(wxCommandEvent& event)
{
	s_searchsizer->Show(s_extendedsizer, event.IsChecked());
	
	Layout();
}

void CSearchDlg::OnFilterCheckChange(wxCommandEvent& event)
{
	s_searchsizer->Show(s_filtersizer, event.IsChecked());
	
	Layout();
	
	// Remove any current filtering
	if (!event.IsChecked()) {
		CastChild(ID_FILTER_TEXT, wxTextCtrl)->SetValue(wxT(""));
		wxCommandEvent evt;
		OnFilteringChange(evt);
	}
}

void CSearchDlg::OnSearchClosed(wxNotebookEvent& evt) 
{
	// Abort global search if it was last tab that was closed.
	if ( evt.GetSelection() == ((int)m_notebook->GetPageCount() - 1 ) ) {
		OnBnClickedStop(nullEvent);
	}
	CSearchListCtrl *ctrl = (CSearchListCtrl*)m_notebook->GetPage(evt.GetSelection());
	wxASSERT(ctrl);
	theApp.searchlist->RemoveResults(ctrl->GetSearchId());
	
	// Do cleanups if this was the last tab
	if ( m_notebook->GetPageCount() == 1 ) {
		FindWindow(IDC_SDOWNLOAD)->Enable(FALSE);
		FindWindow(IDC_CLEAR_RESULTS)->Enable(FALSE);
	}
}


void CSearchDlg::OnSearchPageChanged(wxNotebookEvent& WXUNUSED(evt))
{
	int selection = m_notebook->GetSelection();

	// Only enable the Download button for pages where files have been selected
	if ( selection != -1 ) {
		bool enable = ((CSearchListCtrl*)m_notebook->GetPage( selection ))->GetSelectedItemCount();

		FindWindow(IDC_SDOWNLOAD)->Enable( enable );
	}

}


void CSearchDlg::OnBnClickedStart(wxCommandEvent& WXUNUSED(evt))
{
	wxString searchString = CastChild( IDC_SEARCHNAME, wxTextCtrl )->GetValue();
	searchString.Trim(true);
	searchString.Trim(false);	
	
	if ( searchString.IsEmpty() ) {
		return;
	}

	wxChoice* choice = CastChild( ID_SEARCHTYPE, wxChoice );

	// Web seaches
	switch ( choice->GetSelection() ) {
		// Local Search
		case 0: 
		// Global Search
		case 1:
		// Kad Search
		case 2:
			// We musn't search more often than once every 2 secs
			if ((GetTickCount() - m_last_search_time) > 2000) {
				m_last_search_time = GetTickCount();
		
				OnBnClickedStop(nullEvent);
		
				StartNewSearch();
			}

			break;

		// Web Search (FileHash.com)
		case 3:
			theApp.amuledlg->LaunchUrl(theApp.amuledlg->GenWebSearchUrl(searchString, CamuleDlg::wsFileHash));
			break;

		// Web Search (Jugle.net)
		case 4:
    		theApp.amuledlg->LaunchUrl(theApp.amuledlg->GenWebSearchUrl(searchString, CamuleDlg::wsJugle));
			break;

		// Error
		default:
			wxASSERT(0);
	}
}


void CSearchDlg::OnFieldChanged( wxEvent& WXUNUSED(evt) )
{
	bool enable = false;
	
	// These are the IDs of the search-fields 
	int textfields[] = { IDC_SEARCHNAME, IDC_EDITSEARCHEXTENSION };

	for ( uint16 i = 0; i < itemsof(textfields); i++ ) {
		enable |= !CastChild( textfields[i], wxTextCtrl )->GetValue().IsEmpty();
	}


	// These are the IDs of the search-fields
	int spinfields[] = { IDC_SPINSEARCHMIN, IDC_SPINSEARCHMAX, IDC_SPINSEARCHAVAIBILITY };

	for ( uint16 i = 0; i < itemsof(spinfields); i++ ) {
		enable |= CastChild( spinfields[i], wxSpinCtrl )->GetValue();
	}

	// Enable the Clear and Clear-All button if any fields contain text
	FindWindow(IDC_SEARCH_RESET)->Enable( enable );
	FindWindow(IDC_CLEAR_RESULTS)->Enable( enable || m_notebook->GetPageCount() );

	
	// Enable the Server Search button if the Name field contains text
	enable = !CastChild( IDC_SEARCHNAME, wxTextCtrl )->GetValue().IsEmpty();
	FindWindow(IDC_STARTS)->Enable( enable );
}


void CSearchDlg::OnFilteringChange(wxCommandEvent& WXUNUSED(evt))
{
	wxString filter = CastChild(ID_FILTER_TEXT, wxTextCtrl)->GetValue();
	bool     invert = CastChild(ID_FILTER_INVERT, wxCheckBox)->GetValue();
	bool     known = CastChild(ID_FILTER_KNOWN, wxCheckBox)->GetValue();

	// Check that the expression compiles before we try to assign it
	// Otherwise we will get an error-dialog for each result-list.
	if (wxRegEx(filter, wxRE_DEFAULT | wxRE_ICASE).IsValid()) {
		int nPages = m_notebook->GetPageCount();
		for ( int i = 0; i < nPages; i++ ) {
			CSearchListCtrl* page = (CSearchListCtrl*)m_notebook->GetPage( i );
			
			page->SetFilter(filter, invert, known);
		
			UpdateHitCount(page);
		}
	}
}


bool CSearchDlg::CheckTabNameExists(const wxString& searchString) 
{
	int nPages = m_notebook->GetPageCount();
	for ( int i = 0; i < nPages; i++ ) {
		// The BeforeLast(' ') is to strip the hit-count from the name
		if ( m_notebook->GetPageText(i).BeforeLast(wxT(' ')) == searchString ) {
			return true;
		}
	}
	
	return false;
}


void CSearchDlg::CreateNewTab(const wxString& searchString, long nSearchID)
{
    CSearchListCtrl* list = new CSearchListCtrl( (wxWindow*)m_notebook, ID_SEARCHLISTCTRL, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxNO_BORDER );
	m_notebook->AddPage(list, searchString, true, 0);

	// Ensure that new results are filtered
	wxString filter = CastChild(ID_FILTER_TEXT, wxTextCtrl)->GetValue();
	bool     invert = CastChild(ID_FILTER_INVERT, wxCheckBox)->GetValue();
	bool     known = CastChild(ID_FILTER_KNOWN, wxCheckBox)->GetValue();	
	
	list->SetFilter(filter, invert, known);
	list->ShowResults(nSearchID);
	
	Layout();
	FindWindow(IDC_CLEAR_RESULTS)->Enable(true);
}


void CSearchDlg::OnBnClickedStop(wxCommandEvent& WXUNUSED(evt))
{
	theApp.searchlist->StopGlobalSearch();
	ResetControls();
}


void CSearchDlg::ResetControls()
{
	m_progressbar->SetValue(0);
	
	FindWindow(IDC_CANCELS)->Disable();
	FindWindow(IDC_STARTS)->Enable(!CastChild( IDC_SEARCHNAME, wxTextCtrl )->GetValue().IsEmpty());
}


void CSearchDlg::LocalSearchEnd()
{
	ResetControls();
}


void CSearchDlg::OnBnClickedDownload(wxCommandEvent& WXUNUSED(evt))
{
	int selection = m_notebook->GetSelection();

	if ( selection == -1 )
		return;
	
	
	CSearchListCtrl* searchlistctrl = (CSearchListCtrl*)m_notebook->GetPage( selection );
	
	if ( !searchlistctrl->GetSelectedItemCount() )
		return;
	
	FindWindow(IDC_SDOWNLOAD)->Enable(FALSE);

	uint8 category = 0;
	if (CastChild(IDC_EXTENDEDSEARCHCHECK, wxCheckBox)->GetValue()) {
		category = CastChild( ID_AUTOCATASSIGN, wxChoice )->GetSelection();
	}
	
	int index = searchlistctrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while ( index > -1 ) {
		CoreNotify_Search_Add_Download( (CSearchFile*)searchlistctrl->GetItemData(index), category );
		
		searchlistctrl->UpdateColor( index );

		index = searchlistctrl->GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
}


void CSearchDlg::OnBnClickedClear(wxCommandEvent& WXUNUSED(ev))
{
	OnBnClickedStop(nullEvent);
	
	m_notebook->DeleteAllPages();

	FindWindow(IDC_CLEAR_RESULTS)->Enable(FALSE);
	FindWindow(IDC_SDOWNLOAD)->Enable(FALSE);

	CastChild( IDC_SEARCHNAME, wxTextCtrl )->Clear();
}


void CSearchDlg::StartNewSearch()
{
	static uint32 m_nSearchID = 0;
	m_nSearchID++;
	
	FindWindow(IDC_STARTS)->Disable();
	FindWindow(IDC_SDOWNLOAD)->Disable();
	FindWindow(IDC_CANCELS)->Enable();
	
	wxString searchString = CastChild( IDC_SEARCHNAME, wxTextCtrl )->GetValue();
	searchString.Trim(true);
	searchString.Trim(false);	
	if ( searchString.IsEmpty() ) {
		return;
	}

	wxString typeText(wxT("Any")), extension;
	uint32 min = 0, max = 0, availability = 0;
	
	if (CastChild(IDC_EXTENDEDSEARCHCHECK, wxCheckBox)->GetValue()) {

		extension = CastChild( IDC_EDITSEARCHEXTENSION, wxTextCtrl )->GetValue();
		if ( !extension.IsEmpty() && !extension.StartsWith(wxT(".")) ) {
			extension = wxT(".") + extension;
		}		

		uint32 sizemin = GetTypeSize( (uint8) CastChild( IDC_SEARCHMINSIZE, wxChoice )->GetSelection() ); 
		uint32 sizemax = GetTypeSize( (uint8) CastChild( IDC_SEARCHMAXSIZE, wxChoice )->GetSelection() );

		// Parameter Minimum Size
		min = CastChild( IDC_SPINSEARCHMIN, wxSpinCtrl )->GetValue() * sizemin;

		// Parameter Maximum Size
		max = CastChild( IDC_SPINSEARCHMAX, wxSpinCtrl )->GetValue() * sizemax;

		if ( max < min ) max = 0;

		// Parameter Availability
		availability = CastChild( IDC_SPINSEARCHAVAIBILITY, wxSpinCtrl )->GetValue();

		switch ( CastChild( IDC_TypeSearch, wxChoice )->GetSelection() ) {
			case 0: 
				typeText = wxT("Any"); 
				break;
			case 1: 
				typeText = wxT("Archives"); 
				break;
			case 2: 
				typeText = wxT("Audio"); 
				break;
			case 3: 
				typeText = wxT("CD-Images"); 
				break;
			case 4: 
				typeText = wxT("Pictures"); 
				break;
			case 5: 
				typeText = wxT("Programs"); 
				break;
			case 6: 
				typeText = wxT("Texts"); 
				break;
			case 7: 
				typeText = wxT("Videos"); 
				break;
			default:
				AddDebugLogLineM( true, logGeneral,
					CFormat( wxT("Warning! Unknown search-category (%s) selected!") )
						% typeText
				);
				
				break;
		}
		// This will break if we change the order (good to know!)
		wxASSERT(CastChild( IDC_TypeSearch, wxChoice )->GetStringSelection() == wxGetTranslation(typeText));
	}

	SearchType search_type = KadSearch;
	
	uint32 real_id = m_nSearchID;
	
	switch (CastChild( ID_SEARCHTYPE, wxChoice )->GetSelection()) {
		case 0: // Local Search	
			search_type = LocalSearch;
		case 1: // Global Search
			if (search_type != LocalSearch) {
				search_type = GlobalSearch;
			}
		case 2: // Kad search
			if (!theApp.searchlist->StartNewSearch(&real_id, search_type, searchString, typeText, extension, min, max, availability)) {
				// Search failed (not connected?)
				wxString error;
				if (search_type == KadSearch) {
					#ifdef __COMPILE_KAD__
					error = _("Impossible to make Kad search (invalid chars? keywords too short? not connected?)");
					#else
					error = _("Kad search is not available yet");
					#endif
				} else {
					error = _("You are not connected to a server!");
				}
				wxMessageDialog* dlg = new wxMessageDialog(this, error, wxString(_("Search not possible")), wxOK|wxCENTRE|wxICON_INFORMATION);
				dlg->ShowModal();
				delete dlg;
				FindWindow(IDC_STARTS)->Enable();
				FindWindow(IDC_SDOWNLOAD)->Enable();
				FindWindow(IDC_CANCELS)->Disable();
				return;
			}
			break;
		default:
			// Should never happen
			wxASSERT(0);
			break;
	}
	
	CreateNewTab(searchString + wxT(" (0)"), real_id);
}


void CSearchDlg::UpdateHitCount(CSearchListCtrl* page)
{
	for ( uint32 i = 0; i < (uint32)m_notebook->GetPageCount(); ++i ) {
		if ( m_notebook->GetPage(i) == page ) {
			wxString searchtxt = m_notebook->GetPageText(i).BeforeLast(wxT(' '));
		
			if ( !searchtxt.IsEmpty() ) {
				size_t shown = page->GetItemCount();
				size_t hidden = page->GetHiddenItemCount();
				
				if (hidden) {
					searchtxt += wxString::Format(wxT(" (%u/%u)"), shown, shown + hidden);
				} else {
					searchtxt += wxString::Format(wxT(" (%u)"), shown);
				}
				
				m_notebook->SetPageText(i, searchtxt);
			}
		
			break;
		}
	}
}


void CSearchDlg::OnBnClickedReset(wxCommandEvent& WXUNUSED(evt))
{
	CastChild( IDC_SEARCHNAME, wxTextCtrl )->Clear();
	CastChild( IDC_EDITSEARCHEXTENSION, wxTextCtrl )->Clear();

	CastChild( IDC_SPINSEARCHMIN, wxSpinCtrl )->SetValue(0);
	CastChild( IDC_SPINSEARCHMAX, wxSpinCtrl )->SetValue(0);
	CastChild( IDC_SPINSEARCHAVAIBILITY, wxSpinCtrl )->SetValue(0);

	FindWindow(IDC_CLEAR_RESULTS)->Enable( m_notebook->GetPageCount() );

	CastChild( IDC_TypeSearch, wxChoice )->SetSelection(0);
	
	FindWindow(IDC_SEARCH_RESET)->Enable(FALSE);
}


void CSearchDlg::UpdateCatChoice()
{
	wxChoice* c_cat = CastChild( ID_AUTOCATASSIGN, wxChoice );
	c_cat->Clear();
	
	c_cat->Append(_("Main"));

	for ( unsigned i = 1; i < theApp.glob_prefs->GetCatCount(); i++ ) {
		c_cat->Append( theApp.glob_prefs->GetCategory( i )->title );
	}
	
	c_cat->SetSelection( 0 );
}

void	CSearchDlg::UpdateProgress(uint32 new_value) {
	m_progressbar->SetValue(new_value);
}
