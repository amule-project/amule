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

#ifdef __WXMAC__
	#include <wx/wx.h>
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
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "SearchList.h"		// Needed for CSearchList
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "otherfunctions.h"	// Needed for URLEncode
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

#define ID_SEARCHTIMER 55219
#define ID_SEARCHLISTCTRL wxID_HIGHEST+666

// just to keep compiler happy
static wxCommandEvent nullEvent;


BEGIN_EVENT_TABLE(CSearchDlg, wxPanel)
	EVT_BUTTON(IDC_STARTS, CSearchDlg::OnBnClickedStarts)
	
	EVT_TEXT_ENTER(IDC_SEARCHNAME, CSearchDlg::OnBnClickedStarts)
	EVT_TEXT_ENTER(IDC_EDITSEARCHEXTENSION, CSearchDlg::OnBnClickedStarts)
	
	EVT_TEXT(IDC_SEARCHNAME, CSearchDlg::OnEditFieldsChange)
	EVT_TEXT(IDC_EDITSEARCHEXTENSION, CSearchDlg::OnEditFieldsChange)
	
	EVT_SPINCTRL(IDC_SPINSEARCHMIN, CSearchDlg::OnSpinFieldsChange)
	EVT_SPINCTRL(IDC_SPINSEARCHMAX, CSearchDlg::OnSpinFieldsChange)
	EVT_SPINCTRL(IDC_SPINSEARCHAVAIBILITY, CSearchDlg::OnSpinFieldsChange)
	
	EVT_BUTTON(IDC_CANCELS, CSearchDlg::OnBnClickedCancels)
	EVT_BUTTON(IDC_CLEARALL, CSearchDlg::OnBnClickedClearall)
	
	EVT_LIST_ITEM_SELECTED(ID_SEARCHLISTCTRL, CSearchDlg::OnListItemSelected)
	EVT_BUTTON(IDC_SDOWNLOAD, CSearchDlg::OnBnClickedSdownload)
	EVT_BUTTON(IDC_SEARCH_RESET, CSearchDlg::OnBnClickedSearchReset)
	EVT_BUTTON(ID_BTN_DDLOAD, CSearchDlg::DirectDownload)
	EVT_RIGHT_DOWN(CSearchDlg::OnRMButton)

	EVT_MENU(MP_CLOSE_TAB, CSearchDlg::OnPopupClose)
	EVT_MENU(MP_CLOSE_ALL_TABS, CSearchDlg::OnPopupCloseAll)
	EVT_MENU(MP_CLOSE_OTHER_TABS, CSearchDlg::OnPopupCloseOthers)
	
	EVT_CHECKBOX(ID_EXTENDEDSEARCHCHECK,CSearchDlg::OnExtendedSearchChange)
	
	EVT_MULENOTEBOOK_PAGE_CLOSED(ID_NOTEBOOK, CSearchDlg::OnSearchClosed)
	EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK, CSearchDlg::OnSearchPageChanged)
END_EVENT_TABLE()



CSearchDlg::CSearchDlg(wxWindow* pParent)
: wxPanel(pParent, -1)
{
	last_search_time = 0;
	
	globalsearch = false;

	searchpacket = NULL;

	wxSizer* content=searchDlg(this, true);
	content->Show(this, true);

	progressbar = (wxGauge*)FindWindow(ID_SEARCHPROGRESS);
	wxASSERT( progressbar );
	progressbar->SetRange(100);
	
	notebook = (CMuleNotebook*)FindWindow(ID_NOTEBOOK);
	wxASSERT( notebook );

	// Initialise the image list
	wxImageList* m_ImageList = new wxImageList(16,16);
	m_ImageList->Add(amuleSpecial(3));
	m_ImageList->Add(amuleSpecial(4));
	notebook->AssignImageList(m_ImageList);

	// allow notebook to dispatch right mouse clicks to us
	notebook->SetMouseListener(GetEventHandler());
	
	ToggleLinksHandler();
	
	((wxChoice*)FindWindow(ID_SEARCHTYPE))->SetSelection(0);
	((wxChoice*)FindWindow(IDC_TypeSearch))->SetSelection(0);
	
	// Not there initially.
	s_searchsizer->Show(s_extendedsizer, false);
	Layout();
}


// Enable the download button when there are items selected
void CSearchDlg::OnListItemSelected(wxListEvent& WXUNUSED(event))
{
	FindWindow(IDC_SDOWNLOAD)->Enable(true);
}

// Enable the extended options 
void CSearchDlg::OnExtendedSearchChange(wxCommandEvent& event)
{
	s_searchsizer->Show(s_extendedsizer, event.IsChecked());
	Layout();
}


void CSearchDlg::OnSearchClosed(wxNotebookEvent& evt) 
{
	// Abort global search if it was last tab that was closed.
	if ( evt.GetSelection() == ((int)notebook->GetPageCount() - 1 ) ) {
		OnBnClickedCancels(nullEvent);
	}
}

void CSearchDlg::OnSearchPageChanged(wxNotebookEvent& evt)
{
	// Only enable the Download button for pages where files have been selected
	if ( evt.GetSelection() != -1 ) {
		bool enable = ((CSearchListCtrl*)notebook->GetPage(evt.GetSelection()))->GetSelectedItemCount();

		FindWindow(IDC_SDOWNLOAD)->Enable( enable );
	}

}

void CSearchDlg::OnBnClickedStarts(wxCommandEvent& WXUNUSED(evt))
{
	wxString searchString = ((wxTextCtrl*)FindWindow(IDC_SEARCHNAME))->GetValue();
	searchString.Trim(true);
	searchString.Trim(false);	
	
	if ( searchString.IsEmpty() ) {
		return;
	}


	wxChoice* choice = (wxChoice*)FindWindow(ID_SEARCHTYPE);

	// Web seaches
	switch ( choice->GetSelection() ) {
		// Local Search
		case 0: 
		// Global Search
		case 1:
			// lugdunum will kill us if we don't fix this ;)
			if ((GetTickCount() - last_search_time)  > 2000) {
				last_search_time = GetTickCount();
		
				OnBnClickedCancels(nullEvent);
		
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


void CSearchDlg::FieldsChanged()
{
	bool enable = false;
	
	// These are the IDs of the search-fields 
	int textfields[] = { IDC_SEARCHNAME, IDC_EDITSEARCHEXTENSION };

	for ( uint16 i = 0; i < itemsof(textfields); i++ ) {
		enable |= ((wxTextCtrl*)FindWindow( textfields[i] ))->GetLineLength(0);
	}

	// These are the IDs of the search-fields
	int spinfields[] = { IDC_SPINSEARCHMIN, IDC_SPINSEARCHMAX, IDC_SPINSEARCHAVAIBILITY };

	for ( uint16 i = 0; i < itemsof(spinfields); i++ ) {
		enable |= ((wxSpinCtrl*)FindWindow( spinfields[i] ))->GetValue();
	}

	// Enable the Clear and Clear-All button if any fields contain text
	FindWindow(IDC_SEARCH_RESET)->Enable( enable );
	FindWindow(IDC_CLEARALL)->Enable( enable || notebook->GetPageCount() );

	
	// Enable the Server Search button if the Name field contains text
	enable = ((wxTextCtrl*)FindWindow(IDC_SEARCHNAME))->GetLineLength(0);
	FindWindow(IDC_STARTS)->Enable( enable );
}

// Enables or disables the Reset and Start button depending on the conents of the text fields
void CSearchDlg::OnEditFieldsChange(wxCommandEvent& WXUNUSED(evt))
{
	FieldsChanged();
}


// Enables or disables the Reset and Start button depending on the conents of the text fields
void CSearchDlg::OnSpinFieldsChange(wxSpinEvent& WXUNUSED(evt))
{
	FieldsChanged();
}

bool CSearchDlg::CheckTabNameExists(wxString searchString) 
{
	int nPages = notebook->GetPageCount();
	for ( int i = 0; i < nPages ; i++ ) {
		if ( notebook->GetPageText(i).BeforeLast(wxT(' ')) == searchString ) {
			return true;
		}
	}
	
	return false;
}


void CSearchDlg::CreateNewTab(wxString searchString, long nSearchID)
{
    CSearchListCtrl* list = new CSearchListCtrl( (wxWindow*)notebook, ID_SEARCHLISTCTRL, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxNO_BORDER );
	
	list->Init(theApp.searchlist);
	list->ShowResults(nSearchID);
	
	notebook->AddPage(list, searchString, true, 0, nSearchID);
	
	Layout();

	FindWindow(IDC_CLEARALL)->Enable(true);
}


void CSearchDlg::OnBnClickedCancels(wxCommandEvent& WXUNUSED(evt))
{
	canceld = true;

 	if (globalsearch) {
		theApp.searchlist->searchthread->Delete();
 	}

	progressbar->SetValue(0);

	FindWindow(IDC_CANCELS)->Disable();
	FindWindow(IDC_STARTS)->Enable();
}

void CSearchDlg::LocalSearchEnd(uint16 WXUNUSED(count))
{
	if (!canceld) {
		if (!globalsearch) {
			FindWindow(IDC_CANCELS)->Disable();
			FindWindow(IDC_STARTS)->Enable();
		}
	}
}


void CSearchDlg::OnBnClickedSdownload(wxCommandEvent& WXUNUSED(evt))
{
	if( notebook->GetSelection() == -1 ) {
		return;
	}

	wxMutexLocker slock(notebook->m_LockTabs);

	CSearchListCtrl* searchlistctrl = (CSearchListCtrl*)notebook->GetPage(notebook->GetSelection());
	if ( searchlistctrl == NULL ) {
		printf("searchlistctrl == NULL in CSearchDlg::OnBnClickedSdownload -- please, report this on amule forums: www.amule.org");
		return;
	}

	int index = searchlistctrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while ( (index > -1) ) {
		FindWindowById(IDC_SDOWNLOAD)->Enable(FALSE);
		CoreNotify_Search_Add_Download((CSearchFile*)searchlistctrl->GetItemData(index), GetCatChoice());

		index = searchlistctrl->GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
}


void CSearchDlg::OnBnClickedClearall(wxCommandEvent& WXUNUSED(ev))
{
	OnBnClickedCancels(nullEvent);
	DeleteAllSearchs();
	OnBnClickedSearchReset(nullEvent);
}


void CSearchDlg::StartNewSearch()
{
	static uint16 m_nSearchID = 0;
	m_nSearchID++;

	// No searching if not connected
	if (!theApp.serverconnect->IsConnected()) {
		wxMessageDialog* dlg = new wxMessageDialog(this, wxString(_("You are not connected to a server!")), wxString(_("Not Connected")), wxOK|wxCENTRE|wxICON_INFORMATION);
		dlg->ShowModal();
		delete dlg;
		return;
	}

	FindWindow(IDC_STARTS)->Disable();
	FindWindow(IDC_CANCELS)->Enable();

	canceld = false;
	
	wxString searchString = ((wxTextCtrl*)FindWindow(IDC_SEARCHNAME))->GetValue();
	searchString.Trim(true);
	searchString.Trim(false);	
	if ( searchString.IsEmpty() ) {
		return;
	}
	
	wxString extension = ((wxTextCtrl*)FindWindow(IDC_EDITSEARCHEXTENSION))->GetValue();
	if ( !extension.IsEmpty() && !extension.StartsWith(wxT(".")) ) {
		extension = wxT(".") + extension;
	}		
		
	wxString typeText = ((wxChoice*)FindWindow(IDC_TypeSearch))->GetStringSelection();
	theApp.searchlist->NewSearch(typeText, m_nSearchID);

	// Parameter Minimum Size
	uint32 min = ((wxSpinCtrl*)FindWindow(IDC_SPINSEARCHMIN))->GetValue() * 1048576;
	
	// Parameter Maximum Size
	uint32 max = ((wxSpinCtrl*)FindWindow(IDC_SPINSEARCHMAX))->GetValue() * 1048576;
	
	if ( max < min ) max = 0;
	
	// Parameter Availability
	uint32 avaibility = ((wxSpinCtrl*)FindWindow(IDC_SPINSEARCHAVAIBILITY))->GetValue();
	
	
	switch ( ((wxChoice*)FindWindow(IDC_TypeSearch))->GetSelection() ) {
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

	Packet *packet = CreateSearchPacket(searchString, typeText, extension, min, max, avaibility);
	
	globalsearch = ((wxChoice*)FindWindow(ID_SEARCHTYPE))->GetSelection() == 1;

	CoreNotify_Search_Req(packet, globalsearch);
	
	CreateNewTab(searchString + wxT(" (0)"), m_nSearchID);
}


// Only needed for CSearchListCtrl to be able to close us by passing only ID
void CSearchDlg::DeleteSearch(uint16 nSearchID)
{
	theApp.searchlist->RemoveResults(nSearchID);

	wxMutexLocker sLock(notebook->m_LockTabs);
	for ( unsigned i = 0; i < notebook->GetPageCount(); i++ ) {
		wxWindow * slctrl = (wxWindow *)notebook->GetPage( i );
		
		// Make sure we have a valid pointer
		if( slctrl ) {
			if( nSearchID == ((CSearchListCtrl*)slctrl)->GetSearchId() ) {
				// Force further actions on the searchlistctrl to fail
				((CSearchListCtrl *)slctrl)->InvalidateSearchId();
				
				notebook->DeletePage(i);
				break;
			}
		}
	}
}


void CSearchDlg::DeleteAllSearchs()
{
	theApp.searchlist->Clear();
	
	notebook->DeleteAllPages();

	FindWindow(IDC_CLEARALL)->Enable(FALSE);
}


void CSearchDlg::UpdateHitCount(CSearchListCtrl* page)
{
	for ( unsigned i = 0; i < notebook->GetPageCount(); ++i ) {
		if ( notebook->GetPage(i) == page ) {
			wxString searchtxt = notebook->GetPageText(i).BeforeLast(wxT(' '));
		
			if ( !searchtxt.IsEmpty() ) {
				notebook->SetPageText( i, searchtxt + wxString::Format(wxT(" (%i)"), page->GetItemCount()));
			}
		
			break;
		}
	}
}


void CSearchDlg::OnBnClickedSearchReset(wxCommandEvent& WXUNUSED(evt))
{
	((wxTextCtrl*)FindWindow(IDC_SEARCHNAME))->Clear();
	((wxTextCtrl*)FindWindow(IDC_EDITSEARCHEXTENSION))->Clear();
	
	((wxSpinCtrl*)FindWindow(IDC_SPINSEARCHMIN))->SetValue(0);
	((wxSpinCtrl*)FindWindow(IDC_SPINSEARCHMAX))->SetValue(0);
	((wxSpinCtrl*)FindWindow(IDC_SPINSEARCHAVAIBILITY))->SetValue(0);

	FindWindow(IDC_CLEARALL)->Enable( notebook->GetPageCount() );

	((wxChoice*)FindWindow(IDC_TypeSearch))->SetSelection(0);
	
	FindWindow(IDC_SEARCH_RESET)->Enable(FALSE);
}


/**
 * Sends the contents of the directdownload textctrl box to fast links handler
 **/
void CSearchDlg::DirectDownload(wxCommandEvent& WXUNUSED(event))
{
	theApp.amuledlg->StartFast( (wxTextCtrl*)FindWindowById(ID_ED2KLINKHANDLER) );
}


/**
 * Update category autoassign choice box according to existing categories.
 **/
void CSearchDlg::UpdateCatChoice()
{
	CMuleNotebook* catbook = (CMuleNotebook*)FindWindowById(ID_CATEGORIES);
	wxASSERT(catbook);

	wxChoice *c_cat = (wxChoice*)FindWindow(ID_AUTOCATASSIGN);
	c_cat->Clear();

	for ( unsigned i = 0; i < catbook->GetPageCount(); i++ ) {
		c_cat->Append( catbook->GetPageText(i) );
	}
	
	c_cat->SetSelection(0); 
}


/**
 * Returns current selected category-assignment choice.
 **/
uint8 CSearchDlg::GetCatChoice()
{
	wxChoice *c_cat = (wxChoice*)FindWindow(ID_AUTOCATASSIGN);
	return (uint8)c_cat->GetSelection();
}


/**
 * Toggles Links Handler on this page on/off depending wether
 * global FastED2KLinksHandler is turned on or off.
 **/
void CSearchDlg::ToggleLinksHandler()
{
	s_searchdlgsizer->Show(s_ed2ksizer, !theApp.glob_prefs->GetFED2KLH());
	
	// Doesn't get entirely hidden for some reason =/
	((wxStaticBoxSizer*)s_ed2ksizer)->GetStaticBox()->Show( !theApp.glob_prefs->GetFED2KLH() );
	
	Layout();
}


void CSearchDlg::OnRMButton(wxMouseEvent& evt)
{
	if( !notebook->GetPageCount() ) {
		return;
	}
	
	// Translate the global position to a position relative to the notebook
	wxPoint pt=evt.GetPosition();
	wxPoint newpt=((wxWindow*)evt.GetEventObject())->ClientToScreen(pt);
	newpt = ScreenToClient(newpt);

	// Only show the popup-menu if we are inside the notebook widget
	if ( notebook->GetRect().Inside( newpt ) ) {
		wxMenu* menu = new wxMenu(wxString(_("Close")));
		menu->Append(MP_CLOSE_TAB, wxString(_("Close tab")));
		menu->Append(MP_CLOSE_ALL_TABS, wxString(_("Close all tabs")));
		menu->Append(MP_CLOSE_OTHER_TABS, wxString(_("Close other tabs")));
	
		PopupMenu(menu, newpt);
	
		delete menu;
	} else {
		evt.Skip();
	}
}


void CSearchDlg::OnPopupClose(wxCommandEvent& WXUNUSED(evt))
{
	if ( notebook->GetPageCount() == 1 ) {
		// Ensure that the dialog is tidied up
		DeleteAllSearchs();
	} else {
		notebook->DeletePage( notebook->GetSelection() );
	}
}


void CSearchDlg::OnPopupCloseAll(wxCommandEvent& WXUNUSED(evt))
{
	DeleteAllSearchs();
}


void CSearchDlg::OnPopupCloseOthers(wxCommandEvent& WXUNUSED(evt))
{
	wxMutexLocker slock(notebook->m_LockTabs);
	wxNotebookPage* current = notebook->GetPage( notebook->GetSelection() );
	
	unsigned i = 0;
	while ( i < notebook->GetPageCount() ) {
		if ( current == notebook->GetPage( i ) ) {
			i++;
			continue;
		}
		
		notebook->DeletePage( i );
	}
}
