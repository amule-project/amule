//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include <wx/app.h>

#include <wx/gauge.h>		// Do_not_auto_remove (win32)

#include <tags/FileTags.h>

#include "SearchDlg.h"		// Interface declarations.
#include "SearchListCtrl.h"	// Needed for CSearchListCtrl
#include "muuli_wdr.h"		// Needed for IDC_STARTS
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "MuleNotebook.h"
#include "GetTickCount.h"
#include "Preferences.h"
#include "amule.h"			// Needed for theApp
#include "SearchList.h"		// Needed for CSearchList
#include <common/Format.h>
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

	EVT_MULENOTEBOOK_PAGE_CLOSING(ID_NOTEBOOK, CSearchDlg::OnSearchClosing)
	EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK, CSearchDlg::OnSearchPageChanged)

	// Event handlers for the parameter fields getting changed
	EVT_CUSTOM( wxEVT_COMMAND_TEXT_UPDATED,     IDC_SEARCHNAME, CSearchDlg::OnFieldChanged)
	EVT_CUSTOM( wxEVT_COMMAND_TEXT_UPDATED,     IDC_EDITSEARCHEXTENSION, CSearchDlg::OnFieldChanged)
	EVT_CUSTOM( wxEVT_COMMAND_SPINCTRL_UPDATED, wxID_ANY, CSearchDlg::OnFieldChanged)
	EVT_CUSTOM( wxEVT_COMMAND_CHOICE_SELECTED, wxID_ANY, CSearchDlg::OnFieldChanged)

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

#ifdef __WXMAC__
	//#warning TODO: restore the image list if/when wxMac supports locating the image
#else
	// Initialise the image list
	wxImageList* m_ImageList = new wxImageList(16,16);
	m_ImageList->Add(amuleSpecial(3));
	m_ImageList->Add(amuleSpecial(4));
	m_notebook->AssignImageList(m_ImageList);
#endif

	// Sanity sanity
	wxChoice* searchchoice = CastChild( ID_SEARCHTYPE, wxChoice );
	wxASSERT(searchchoice);
	wxASSERT(searchchoice->GetString(0) == _("Local"));
	wxASSERT(searchchoice->GetString(2) == _("Kad"));
	wxASSERT(searchchoice->GetCount() == 4);

	m_searchchoices = searchchoice->GetStrings();

	// Let's break it now.

	FixSearchTypes();

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

void CSearchDlg::FixSearchTypes()
{
	wxChoice* searchchoice = CastChild( ID_SEARCHTYPE, wxChoice );

	searchchoice->Clear();

	// We should have only filedonkey now. Let's insert stuff.

	int pos = 0;

	if (thePrefs::GetNetworkED2K()){
		searchchoice->Insert(m_searchchoices[0], pos++);
		searchchoice->Insert(m_searchchoices[1], pos++);
	}

	if (thePrefs::GetNetworkKademlia()) {
		searchchoice->Insert(m_searchchoices[2], pos++);
	}

	searchchoice->Insert(m_searchchoices[3], pos++);

	searchchoice->SetSelection(0);
}

CSearchListCtrl* CSearchDlg::GetSearchList( wxUIntPtr id )
{
	int nPages = m_notebook->GetPageCount();
	for ( int i = 0; i < nPages; i++ ) {
		CSearchListCtrl* page = dynamic_cast<CSearchListCtrl*>(m_notebook->GetPage(i));

		if (page->GetSearchId() == id) {
			return page;
		}
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


void CSearchDlg::OnListItemSelected(wxListEvent& event)
{
	FindWindow(IDC_SDOWNLOAD)->Enable(true);

	event.Skip();
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

	int nPages = m_notebook->GetPageCount();
	for ( int i = 0; i < nPages; i++ ) {
		CSearchListCtrl* page = dynamic_cast<CSearchListCtrl*>(m_notebook->GetPage(i));

		page->EnableFiltering(event.IsChecked());

		UpdateHitCount(page);
	}
}


void CSearchDlg::OnSearchClosing(wxBookCtrlEvent& evt)
{
	// Abort global search if it was last tab that was closed.
	if ( evt.GetSelection() == ((int)m_notebook->GetPageCount() - 1 ) ) {
		OnBnClickedStop(nullEvent);
	}

	CSearchListCtrl *ctrl = dynamic_cast<CSearchListCtrl*>(m_notebook->GetPage(evt.GetSelection()));
	wxASSERT(ctrl);
	// Zero to avoid results added while destructing.
	ctrl->ShowResults(0);
	theApp->searchlist->RemoveResults(ctrl->GetSearchId());

	// Do cleanups if this was the last tab
	if ( m_notebook->GetPageCount() == 1 ) {
		FindWindow(IDC_SDOWNLOAD)->Enable(FALSE);
		FindWindow(IDC_CLEAR_RESULTS)->Enable(FALSE);
	}
}


void CSearchDlg::OnSearchPageChanged(wxBookCtrlEvent& WXUNUSED(evt))
{
	int selection = m_notebook->GetSelection();

	// Workaround for a bug in wxWidgets, where deletions of pages
	// can result in an invalid selection. This has been reported as
	// http://sourceforge.net/tracker/index.php?func=detail&aid=1865141&group_id=9863&atid=109863
	if (selection >= (int)m_notebook->GetPageCount()) {
		selection = m_notebook->GetPageCount() - 1;
	}

	// Only enable the Download button for pages where files have been selected
	if ( selection != -1 ) {
		CSearchListCtrl *ctrl = dynamic_cast<CSearchListCtrl*>(m_notebook->GetPage(selection));

		bool enable = (ctrl->GetSelectedItemCount() > 0);
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

	// Magic.

	int searchtype = choice->GetSelection();

	if (!thePrefs::GetNetworkED2K()) {
		searchtype += 2;
	}

	if (!thePrefs::GetNetworkKademlia()) {
		searchtype += 1;
	}

	switch ( searchtype ) {
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
			theApp->amuledlg->LaunchUrl(theApp->amuledlg->GenWebSearchUrl(searchString, CamuleDlg::WS_FILEHASH));
			break;

		// Error
		default:
			wxFAIL;
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

	// Check if either of the dropdowns have been changed
	enable |= (CastChild(IDC_SEARCHMINSIZE, wxChoice)->GetSelection() != 2);
	enable |= (CastChild(IDC_SEARCHMAXSIZE, wxChoice)->GetSelection() != 2);
	enable |= (CastChild(IDC_TypeSearch, wxChoice)->GetSelection() > 0);
	enable |= (CastChild(ID_AUTOCATASSIGN, wxChoice)->GetSelection() > 0);

	// These are the IDs of the search-fields
	int spinfields[] = { IDC_SPINSEARCHMIN, IDC_SPINSEARCHMAX, IDC_SPINSEARCHAVAIBILITY };
	for ( uint16 i = 0; i < itemsof(spinfields); i++ ) {
		enable |= (CastChild( spinfields[i], wxSpinCtrl )->GetValue() > 0);
	}

	// Enable the "Reset" button if any fields contain text
	FindWindow(IDC_SEARCH_RESET)->Enable( enable );

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
			CSearchListCtrl* page = dynamic_cast<CSearchListCtrl*>(m_notebook->GetPage(i));

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


void CSearchDlg::CreateNewTab(const wxString& searchString, wxUIntPtr nSearchID)
{
	CSearchListCtrl* list = new CSearchListCtrl(m_notebook, ID_SEARCHLISTCTRL, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxNO_BORDER);
	m_notebook->AddPage(list, searchString, true, 0);

	// Ensure that new results are filtered
	bool     enable = CastChild(IDC_FILTERCHECK, wxCheckBox)->GetValue();
	wxString filter = CastChild(ID_FILTER_TEXT, wxTextCtrl)->GetValue();
	bool     invert = CastChild(ID_FILTER_INVERT, wxCheckBox)->GetValue();
	bool     known = CastChild(ID_FILTER_KNOWN, wxCheckBox)->GetValue();

	list->SetFilter(filter, invert, known);
	list->EnableFiltering(enable);
	list->ShowResults(nSearchID);

	Layout();
	FindWindow(IDC_CLEAR_RESULTS)->Enable(true);
}


void CSearchDlg::OnBnClickedStop(wxCommandEvent& WXUNUSED(evt))
{
	theApp->searchlist->StopSearch();
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

void CSearchDlg::KadSearchEnd(uint32 id)
{
	int nPages = m_notebook->GetPageCount();
	for (int i = 0; i < nPages; ++i) {
		CSearchListCtrl* page =
			dynamic_cast<CSearchListCtrl*>(m_notebook->GetPage(i));
		if (page->GetSearchId() == id || id == 0) {	// 0: just update all pages (there is only one KAD search running at a time anyway)
			wxString rest;
			if (m_notebook->GetPageText(i).StartsWith(wxT("!"),&rest)) {
				m_notebook->SetPageText(i,rest);
			}
		}
	}
}

void CSearchDlg::OnBnClickedDownload(wxCommandEvent& WXUNUSED(evt))
{
	int sel = m_notebook->GetSelection();
	if (sel != -1) {
		CSearchListCtrl* list = dynamic_cast<CSearchListCtrl*>(m_notebook->GetPage(sel));

		// Download with items added to category specified in the drop-down menu
		list->DownloadSelected();
	}
}


void CSearchDlg::OnBnClickedClear(wxCommandEvent& WXUNUSED(ev))
{
	OnBnClickedStop(nullEvent);

	m_notebook->DeleteAllPages();

	FindWindow(IDC_CLEAR_RESULTS)->Enable(FALSE);
	FindWindow(IDC_SDOWNLOAD)->Enable(FALSE);
}


void CSearchDlg::StartNewSearch()
{
	static uint32 m_nSearchID = 0;
	m_nSearchID++;

	FindWindow(IDC_STARTS)->Disable();
	FindWindow(IDC_SDOWNLOAD)->Disable();
	FindWindow(IDC_CANCELS)->Enable();

	CSearchList::CSearchParams params;

	params.searchString = CastChild( IDC_SEARCHNAME, wxTextCtrl )->GetValue();
	params.searchString.Trim(true);
	params.searchString.Trim(false);

	if (params.searchString.IsEmpty()) {
		return;
	}

	if (CastChild(IDC_EXTENDEDSEARCHCHECK, wxCheckBox)->GetValue()) {
		params.extension = CastChild( IDC_EDITSEARCHEXTENSION, wxTextCtrl )->GetValue();

		uint32 sizemin = GetTypeSize( (uint8) CastChild( IDC_SEARCHMINSIZE, wxChoice )->GetSelection() );
		uint32 sizemax = GetTypeSize( (uint8) CastChild( IDC_SEARCHMAXSIZE, wxChoice )->GetSelection() );

		// Parameter Minimum Size
		params.minSize = (uint64_t)(CastChild( IDC_SPINSEARCHMIN, wxSpinCtrl )->GetValue()) * (uint64_t)sizemin;

		// Parameter Maximum Size
		params.maxSize = (uint64_t)(CastChild( IDC_SPINSEARCHMAX, wxSpinCtrl )->GetValue()) * (uint64_t)sizemax;

		if ((params.maxSize < params.minSize) && (params.maxSize)) {
			wxMessageDialog dlg(this,
				_("Min size must be smaller than max size. Max size ignored."),
				_("Search warning"), wxOK|wxCENTRE|wxICON_INFORMATION);
			dlg.ShowModal();

			params.maxSize = 0;
		}

		// Parameter Availability
		params.availability = CastChild( IDC_SPINSEARCHAVAIBILITY, wxSpinCtrl )->GetValue();

		switch ( CastChild( IDC_TypeSearch, wxChoice )->GetSelection() ) {
		case 0:	params.typeText.Clear();	break;
		case 1:	params.typeText = ED2KFTSTR_ARCHIVE;	break;
		case 2: params.typeText = ED2KFTSTR_AUDIO;	break;
		case 3:	params.typeText = ED2KFTSTR_CDIMAGE;	break;
		case 4: params.typeText = ED2KFTSTR_IMAGE;	break;
		case 5: params.typeText = ED2KFTSTR_PROGRAM;	break;
		case 6:	params.typeText = ED2KFTSTR_DOCUMENT;	break;
		case 7:	params.typeText = ED2KFTSTR_VIDEO;	break;
		default:
			AddDebugLogLineC( logGeneral,
				CFormat( wxT("Warning! Unknown search-category (%s) selected!") )
					% params.typeText
			);
			break;
		}
	}

	SearchType search_type = KadSearch;

	int selection = CastChild( ID_SEARCHTYPE, wxChoice )->GetSelection();

	if (!thePrefs::GetNetworkED2K()) {
		selection += 2;
	}

	if (!thePrefs::GetNetworkKademlia()) {
		selection += 1;
	}

	switch (selection) {
		case 0: // Local Search
			search_type = LocalSearch;
			break;
		case 1: // Global Search
			search_type = GlobalSearch;
			break;
		case 2: // Kad search
			search_type = KadSearch;
			break;
		default:
			// Should never happen
			wxFAIL;
			break;
	}

	uint32 real_id = m_nSearchID;
	wxString error = theApp->searchlist->StartNewSearch(&real_id, search_type, params);
	if (!error.IsEmpty()) {
		// Search failed / Remote in progress
		wxMessageBox(error, _("Search warning"),
			wxOK | wxCENTRE | wxICON_INFORMATION, this);
		FindWindow(IDC_STARTS)->Enable();
		FindWindow(IDC_SDOWNLOAD)->Disable();
		FindWindow(IDC_CANCELS)->Disable();
	} else {
		CreateNewTab(
			((search_type == KadSearch) ? wxT("!") : wxEmptyString) +
				params.searchString + wxT(" (0)"),
			real_id);
	}
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
					searchtxt += CFormat(wxT(" (%u/%u)")) % shown % (shown + hidden);
				} else {
					searchtxt += CFormat(wxT(" (%u)")) % shown;
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
	CastChild( IDC_SEARCHMINSIZE, wxChoice )->SetSelection(2);
	CastChild( IDC_SPINSEARCHMAX, wxSpinCtrl )->SetValue(0);
	CastChild( IDC_SEARCHMAXSIZE, wxChoice )->SetSelection(2);
	CastChild( IDC_SPINSEARCHAVAIBILITY, wxSpinCtrl )->SetValue(0);
	CastChild( IDC_TypeSearch, wxChoice )->SetSelection(0);
	CastChild( ID_AUTOCATASSIGN, wxChoice )->SetSelection(0);

	FindWindow(IDC_SEARCH_RESET)->Enable(FALSE);
}


void CSearchDlg::UpdateCatChoice()
{
	wxChoice* c_cat = CastChild( ID_AUTOCATASSIGN, wxChoice );
	c_cat->Clear();

	c_cat->Append(_("Main"));

	for ( unsigned i = 1; i < theApp->glob_prefs->GetCatCount(); i++ ) {
		c_cat->Append( theApp->glob_prefs->GetCategory( i )->title );
	}

	c_cat->SetSelection( 0 );
}

void	CSearchDlg::UpdateProgress(uint32 new_value) {
	m_progressbar->SetValue(new_value);
}
// File_checked_for_headers
