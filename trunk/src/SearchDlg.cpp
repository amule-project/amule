// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


// SearchDlg.cpp : implementation file
//

#include <wx/sizer.h>
#include <wx/gauge.h>
#include <wx/textctrl.h>
#include <wx/msgdlg.h>
#include <wx/menu.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/choice.h>

#include "SearchDlg.h"		// Interface declarations.
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "SearchList.h"		// Needed for CSearchList
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "StringFunctions.h"		// Needed for unicode2char
#include "otherfunctions.h"	// Needed for URLEncode, GetTypeSize
#include "packets.h"		// Needed for Packet
#include "server.h"			// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "sockets.h"		// Needed for CServerConnect
#include "amule.h"			// Needed for theApp
#include "SearchListCtrl.h"	// Needed for CSearchListCtrl
#include "muuli_wdr.h"		// Needed for IDC_STARTS
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "MuleNotebook.h"
#include "GetTickCount.h"
#include "Preferences.h"

#define ID_SEARCHLISTCTRL wxID_HIGHEST+667

// just to keep compiler happy
static wxCommandEvent nullEvent;


BEGIN_EVENT_TABLE(CSearchDlg, wxPanel)
	EVT_BUTTON(		IDC_STARTS,		CSearchDlg::OnBnClickedStart)
	EVT_TEXT_ENTER(	IDC_SEARCHNAME,	CSearchDlg::OnBnClickedStart)
	
	EVT_BUTTON(IDC_CANCELS, CSearchDlg::OnBnClickedCancel)
	
	EVT_LIST_ITEM_SELECTED(ID_SEARCHLISTCTRL, CSearchDlg::OnListItemSelected)
	
	EVT_BUTTON(IDC_SDOWNLOAD, CSearchDlg::OnBnClickedDownload)
	EVT_BUTTON(IDC_SEARCH_RESET, CSearchDlg::OnBnClickedReset)
	EVT_BUTTON(IDC_CLEAR_RESULTS, CSearchDlg::OnBnClickedClear)

	EVT_CHECKBOX(ID_EXTENDEDSEARCHCHECK,CSearchDlg::OnExtendedSearchChange)
	
	EVT_MULENOTEBOOK_PAGE_CLOSED(ID_NOTEBOOK, CSearchDlg::OnSearchClosed)
	EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK, CSearchDlg::OnSearchPageChanged)

	// Event handlers for the parameter fields getting changed
	EVT_CUSTOM( wxEVT_COMMAND_TEXT_UPDATED,     -1, CSearchDlg::OnFieldChanged) 
	EVT_CUSTOM( wxEVT_COMMAND_SPINCTRL_UPDATED, -1, CSearchDlg::OnFieldChanged)
END_EVENT_TABLE()



CSearchDlg::CSearchDlg(wxWindow* pParent)
: wxPanel(pParent, -1)
{
	m_last_search_time = 0;
	m_globalsearch = false;

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


void CSearchDlg::OnSearchClosed(wxNotebookEvent& evt) 
{
	// Abort global search if it was last tab that was closed.
	if ( evt.GetSelection() == ((int)m_notebook->GetPageCount() - 1 ) ) {
		OnBnClickedCancel(nullEvent);
	}

	// Do cleanups if this was the last tab
	if ( m_notebook->GetPageCount() == 1 ) {
		theApp.searchlist->Clear();

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
			// We musn't search more often than once every 2 secs
			if ((GetTickCount() - m_last_search_time) > 2000) {
				m_last_search_time = GetTickCount();
		
				OnBnClickedCancel(nullEvent);
		
				StartNewSearch();
			}

			break;

		// Web Search (FileHash.com)
		case 2:
    		theApp.amuledlg->LaunchUrl(theApp.amuledlg->GenWebSearchUrl(searchString, CamuleDlg::wsFileHash));
			break;

		// Web Search (Jugle.net)
		case 3:
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
	
	m_notebook->AddPage( list, searchString, true, 0 );
	
	list->ShowResults(nSearchID);
	
	Layout();

	FindWindow(IDC_CLEAR_RESULTS)->Enable(true);
}


void CSearchDlg::OnBnClickedCancel(wxCommandEvent& WXUNUSED(evt))
{
	m_canceld = true;

 	if ( m_globalsearch && theApp.searchlist->m_searchthread) {
		theApp.searchlist->m_searchthread->Delete();
 	}
	ResetControls();
}


void CSearchDlg::ResetControls()
{
	m_progressbar->SetValue(0);
	
	FindWindow(IDC_CANCELS)->Disable();
	FindWindow(IDC_STARTS)->Enable();
}


void CSearchDlg::LocalSearchEnd()
{
	if ( !m_canceld ) {
		if ( !m_globalsearch ) {
			ResetControls();
		}
	}
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
	
	int index = searchlistctrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while ( index > -1 ) {
		uint8 cat = CastChild( ID_AUTOCATASSIGN, wxChoice )->GetSelection();
		
		CoreNotify_Search_Add_Download( (CSearchFile*)searchlistctrl->GetItemData(index), cat );
		
		searchlistctrl->UpdateColor( index );

		index = searchlistctrl->GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
}


void CSearchDlg::OnBnClickedClear(wxCommandEvent& WXUNUSED(ev))
{
	OnBnClickedCancel(nullEvent);
	
	m_notebook->DeleteAllPages();

	FindWindow(IDC_CLEAR_RESULTS)->Enable(FALSE);
	FindWindow(IDC_SDOWNLOAD)->Enable(FALSE);
}


void CSearchDlg::StartNewSearch()
{
	static uint16 m_nSearchID = 0;
	m_nSearchID++;
	
	// 0xffff is reserved for websearch
	m_nSearchID %= 0xfffe; 
	
	// No searching if not connected
	if (!theApp.serverconnect->IsConnected()) {
		wxMessageDialog* dlg = new wxMessageDialog(this, wxString(_("You are not connected to a server!")), wxString(_("Not Connected")), wxOK|wxCENTRE|wxICON_INFORMATION);
		dlg->ShowModal();
		delete dlg;
		return;
	}

	FindWindow(IDC_STARTS)->Disable();
	FindWindow(IDC_SDOWNLOAD)->Disable();
	FindWindow(IDC_CANCELS)->Enable();

	m_canceld = false;
	
	wxString searchString = CastChild( IDC_SEARCHNAME, wxTextCtrl )->GetValue();
	searchString.Trim(true);
	searchString.Trim(false);	
	if ( searchString.IsEmpty() ) {
		return;
	}

	wxString typeText(wxT("Any")), extension(wxT(""));
	uint32 min = 0, max = 0, availability = 0;
	
	if (CastChild(ID_EXTENDEDSEARCHCHECK, wxCheckBox)->GetValue()) {

		extension = CastChild( IDC_EDITSEARCHEXTENSION, wxTextCtrl )->GetValue();
		if ( !extension.IsEmpty() && !extension.StartsWith(wxT(".")) ) {
			extension = wxT(".") + extension;
		}		

		typeText = CastChild( IDC_TypeSearch, wxChoice )->GetStringSelection();

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
			case 0: typeText = wxT("Any"); break;
			case 1: typeText = wxT("Archives"); break;
			case 2: typeText = wxT("Audio"); break;
			case 3: typeText = wxT("CD-Images"); break;
			case 4: typeText = wxT("Pictures"); break;
			case 5: typeText = wxT("Programs"); break;
			case 6: typeText = wxT("Texts"); break;
			case 7: typeText = wxT("Videos"); break;
			default:
				printf("Warning! Unknown search-category ( %s ) selected!\n", unicode2char(typeText));
				break;
		}
	}

	theApp.searchlist->NewSearch(typeText, m_nSearchID);
	Packet *packet = CreateSearchPacket(searchString, typeText, extension, min, max, availability);
	
	m_searchtype = CastChild( ID_SEARCHTYPE, wxChoice )->GetSelection();
	m_globalsearch = m_searchtype == 1;

	CoreNotify_Search_Req(packet, m_globalsearch);
	
	CreateNewTab(searchString + wxT(" (0)"), m_nSearchID);
}


void CSearchDlg::UpdateHitCount(CSearchListCtrl* page)
{
	for ( uint32 i = 0; i < (uint32)m_notebook->GetPageCount(); ++i ) {
		if ( m_notebook->GetPage(i) == page ) {
			wxString searchtxt = m_notebook->GetPageText(i).BeforeLast(wxT(' '));
		
			if ( !searchtxt.IsEmpty() ) {
				m_notebook->SetPageText( i, searchtxt + wxString::Format(wxT(" (%i)"), page->GetItemCount()));
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

	for ( unsigned i = 0; i < theApp.glob_prefs->GetCatCount(); i++ ) {
		c_cat->Append( theApp.glob_prefs->GetCategory( i )->title );
	}
	
	c_cat->SetSelection( 0 );
}
