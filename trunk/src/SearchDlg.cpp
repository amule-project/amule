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
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/tokenzr.h>

#include "SearchDlg.h"		// Interface declarations.
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "CMemFile.h"		// Needed for CMemFile
#include "SearchList.h"		// Needed for CSearchList
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "otherfunctions.h"	// Needed for URLEncode
#include "packets.h"		// Needed for Packet
#include "server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "sockets.h"		// Needed for CServerConnect
#include "amule.h"			// Needed for theApp
#include "SearchListCtrl.h"	// Needed for CSearchListCtrl
#include "muuli_wdr.h"		// Needed for IDC_STARTS
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "MuleNotebook.h"

#define ID_SEARCHTIMER 55219

// just to keep compiler happy
static wxCommandEvent nullEvent;

typedef void (CSearchListCtrl::*slistFunc)(wxListEvent&);

IMPLEMENT_DYNAMIC_CLASS(CSearchDlg,wxPanel)

BEGIN_EVENT_TABLE(CSearchDlg, wxPanel)
	EVT_BUTTON(IDC_STARTS, CSearchDlg::OnBnClickedStarts)
	EVT_TEXT_ENTER(IDC_SEARCHNAME,CSearchDlg::OnBnClickedStarts)
	EVT_TEXT_ENTER(IDC_EDITSEARCHMIN,CSearchDlg::OnBnClickedStarts)
	EVT_TEXT_ENTER(IDC_EDITSEARCHMAX,CSearchDlg::OnBnClickedStarts)
	EVT_TEXT_ENTER(IDC_EDITSEARCHEXTENSION,CSearchDlg::OnBnClickedStarts)
	EVT_TEXT_ENTER(IDC_EDITSEARCHAVAIBILITY,CSearchDlg::OnBnClickedStarts)
	EVT_TEXT(IDC_SEARCHNAME,CSearchDlg::OnFieldsChange)
	EVT_TEXT(IDC_EDITSEARCHMIN,CSearchDlg::OnFieldsChange)
	EVT_TEXT(IDC_EDITSEARCHMAX,CSearchDlg::OnFieldsChange)
	EVT_TEXT(IDC_EDITSEARCHEXTENSION,CSearchDlg::OnFieldsChange)
	EVT_TEXT(IDC_EDITSEARCHAVAIBILITY,CSearchDlg::OnFieldsChange)
	EVT_BUTTON(IDC_CANCELS,CSearchDlg::OnBnClickedCancels)
	EVT_BUTTON(IDC_CLEARALL,CSearchDlg::OnBnClickedClearall)
	EVT_TIMER(ID_SEARCHTIMER, CSearchDlg::OnTimer)
	EVT_LIST_ITEM_SELECTED(ID_SEARCHLISTCTRL,CSearchDlg::OnListItemSelected)
	EVT_BUTTON(IDC_SDOWNLOAD,CSearchDlg::OnBnClickedSdownload)
	EVT_BUTTON(IDC_SEARCH_RESET,CSearchDlg::OnBnClickedSearchReset)
	EVT_MULENOTEBOOK_PAGE_CLOSED(ID_NOTEBOOK,CSearchDlg::OnSearchClosed)
	EVT_BUTTON(ID_BTN_DDLOAD, CSearchDlg::DirectDownload)
	EVT_RIGHT_DOWN(CSearchDlg::OnRMButton)
END_EVENT_TABLE()

CSearchDlg::CSearchDlg(wxWindow* pParent /*=NULL*/)
: wxPanel(pParent,CSearchDlg::IDD), m_timer(this,ID_SEARCHTIMER)
{
	m_nSearchID = 0;
	global_search_timer = 0;
	globsearch = false;

	searchpacket=NULL;

	wxSizer* content=searchDlg(this,TRUE);
	content->Show(this,TRUE);

	searchprogress=(wxGauge*)FindWindowById(ID_SEARCHPROGRESS);

	CMuleNotebook* nb=(CMuleNotebook*)FindWindowById(ID_NOTEBOOK);

	// initialise the image list
	m_ImageList.Add(wxBitmap(amuleSpecial(3)));
	m_ImageList.Add(wxBitmap(amuleSpecial(4)));
	nb->AssignImageList(&m_ImageList);

	// allow notebook to dispatch right mouse clicks to us
	nb->SetMouseListener(GetEventHandler());

	searchlistctrl=NULL;

	ToggleLinksHandler();

}

CSearchDlg::~CSearchDlg() {
}

// we must implement Rclick here as wxWindows does not redirect events correctly :(
void CSearchDlg::OnListItemSelected(wxListEvent& event)
{
	// change selection
	FindWindowById(IDC_SDOWNLOAD)->Enable(TRUE);
}

void CSearchDlg::OnSearchClosed(wxNotebookEvent& evt) {
	// Query the search id
	CMuleNotebook* nb=(CMuleNotebook*)FindWindowById(ID_NOTEBOOK);
	wxWindow* page=(wxWindow*)nb->GetPage(evt.GetSelection());
	CSearchListCtrl* sl=(CSearchListCtrl*)page->FindWindowById(ID_SEARCHLISTCTRL,page);
	if(!sl) {
		// hmm? can this happen?
		FindWindowById(IDC_CLEARALL)->Enable(FALSE);
		return; // do not close anything then
	}
	// Abort global search if it was last tab that was closed.
	wxPuts(wxString::Format(wxT("OnSearchClosed. Selection: %d PageCount: %d"), evt.GetSelection(), nb->GetPageCount()));
	if ((size_t)evt.GetSelection() ==(size_t) (nb->GetPageCount()-1)) {
		OnBnClickedCancels(nullEvent);
	}
}

void CSearchDlg::OnBnClickedStarts(wxCommandEvent& evt)
{
	if (((wxTextCtrl*)FindWindowById(IDC_SEARCHNAME))->GetLineLength(0) !=0) {
		OnBnClickedCancels(nullEvent);
		// start a search
		StartNewSearch();
	}
}

void CSearchDlg::OnFieldsChange(wxCommandEvent& evt)
{
	if (((wxTextCtrl*)FindWindowById(IDC_SEARCHNAME))->GetLineLength(0) !=0) {
		FindWindowById(IDC_SEARCH_RESET)->Enable(TRUE);
		FindWindowById(IDC_STARTS)->Enable(TRUE);
	} else {
		FindWindowById(IDC_SEARCH_RESET)->Enable(FALSE);
		FindWindowById(IDC_STARTS)->Enable(FALSE);
	}
}

void CSearchDlg::OnTimer(wxTimerEvent& evt) {
	if (theApp.serverconnect->IsConnected()) {
		CServer* toask = theApp.serverlist->GetNextSearchServer();
		if (toask == theApp.serverlist->GetServerByAddress(theApp.serverconnect->GetCurrentServer()->GetAddress(),theApp.serverconnect->GetCurrentServer()->GetPort())) {
			toask = theApp.serverlist->GetNextSearchServer();
		}
		if (toask && theApp.serverlist->GetServerCount()-1 != servercount) {
			servercount++;
			theApp.serverconnect->SendUDPPacket(searchpacket,toask,false);
			// Only update progress if we have any tabs around...
			// Kry - And if we're on search dialog actually.
			if ((((CMuleNotebook*)FindWindowById(ID_NOTEBOOK))->GetPageCount() > 0) && (theApp.amuledlg->GetActiveDialog() == IDD_SEARCH) && (searchprogress->GetValue() < searchprogress->GetRange())) {
				searchprogress->SetValue(searchprogress->GetValue()+1);}
		} else {
			OnBnClickedCancels(nullEvent);
		}
	} else {
		OnBnClickedCancels(nullEvent);
	}
}

bool CSearchDlg::CheckTabNameExists(wxString searchString) {

	CMuleNotebook* nb=(CMuleNotebook*)FindWindowById(ID_NOTEBOOK);
	int nPages = nb->GetPageCount();

	for (int n=0;n<nPages;n++) {
		if (nb->GetPageText(n)==searchString) {
			return true;
		}

	}
	return false;
}

void CSearchDlg::CreateNewTab(wxString searchString,uint32 nSearchID)
{
	// open new search window
	CMuleNotebook* nb=(CMuleNotebook*)FindWindowById(ID_NOTEBOOK);

	wxPanel* sizCont=new wxPanel(nb,-1);
	searchPage(sizCont,TRUE);	// Create searchPage.
	nb->AddPage(sizCont,searchString,TRUE,0,nSearchID);

	// set the range here!
	searchprogress->SetRange(theApp.serverlist->GetServerCount()-1);

	GetParent()->Layout();
	searchlistctrl=(CSearchListCtrl*)sizCont->FindWindowById(ID_SEARCHLISTCTRL,sizCont);
	searchlistctrl->Init(theApp.searchlist);
	searchlistctrl->ShowResults(nSearchID);
	FindWindowById(IDC_CLEARALL)->Enable(TRUE);
}

void CSearchDlg::OnBnClickedCancels(wxCommandEvent &evt)
{
	canceld = true;

	if (globsearch) {
		delete searchpacket;
		globsearch = false;
	}

	m_timer.Stop();
	global_search_timer = 0;
	searchprogress->SetValue(0);

	FindWindowById(IDC_CANCELS)->Disable();
	FindWindowById(IDC_STARTS)->Enable();
}

void CSearchDlg::LocalSearchEnd(uint16 count)
{
	if (!canceld) {
		if (!globsearch) {
			FindWindowById(IDC_CANCELS)->Disable();
			FindWindowById(IDC_STARTS)->Enable();
		} else {
			//global_search_timer = SetTimer(1, 750, 0);
			wxPuts(wxT("Starting m_timer"));
			m_timer.Start(750);
			global_search_timer=(UINT*)1;
		}
	}
}

// Madcat - Huh? Whats this?
void CSearchDlg::AddUDPResult(uint16 count)
{
	#if 0
	if (!canceld && count > MAX_RESULTS) {
		OnBnClickedCancels(nullEvent);
	}
	#endif
}

void CSearchDlg::OnBnClickedSdownload(wxCommandEvent &evt)
{
	//start download(s)
	DownloadSelected();
}

// Madcat - Whats this? Used? Unimplemented? *duh*
wxString CSearchDlg::CreateWebQuery()
{
	wxString query;
	query = "http://www.filedonkey.com/fdsearch/index.php?media=";
	//switch (typebox.GetCurSel()){
	int x=1;
	switch(x) {
		case 1:
			query += "Audio";
			break;
		case 2:
			query += "Video";
			break;
		case 3:
			query += "Pro";
			break;
		default:
			;
	}
	CString tosearch;
	//GetDlgItem(IDC_SWEB)->GetWindowText(tosearch);
	tosearch = CString(URLEncode(tosearch).GetData());
	tosearch.Replace("%20","+");
	query += "&pattern=";
	query += tosearch;
	query += "&action=search&name=FD-Search&op=modload&file=index&requestby=amule";
	return query;
}

void CSearchDlg::DownloadSelected()
{
	int index=-1;
	// download from the selected page.
	CMuleNotebook* nb=(CMuleNotebook*)FindWindowById(ID_NOTEBOOK);
	wxMutexLocker slock(nb->m_LockTabs);
	if(nb->GetSelection()==-1) {
		return;
	}

	CSearchListCtrl* searchlistctrl=(CSearchListCtrl*)nb->FindWindowById(ID_SEARCHLISTCTRL,nb->GetPage(nb->GetSelection()));

	for(;;) {
		index=searchlistctrl->GetNextItem(index,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
		if(index==-1) {
			break;
		}
		theApp.downloadqueue->AddSearchToDownload((CSearchFile*)searchlistctrl->GetItemData(index));
		//searchlistctrl->SetSelection(index,FALSE);
		FindWindowById(IDC_SDOWNLOAD)->Enable(FALSE);
	}
}

void CSearchDlg::OnBnClickedClearall(wxCommandEvent& ev)
{
	OnBnClickedCancels(nullEvent);
	DeleteAllSearchs();
	OnBnClickedSearchReset(nullEvent);
}

void CSearchDlg::StartNewSearch() {
	// No searching if not connected
	if (!theApp.serverconnect->IsConnected()) {
		wxMessageDialog* dlg=new wxMessageDialog(this,CString(_("You are not connected to a server!")),CString(_("Not Connected")),wxOK|wxCENTRE|wxICON_INFORMATION);
		dlg->ShowModal();
		return;
	}

	m_nSearchID++;
	wxString typeText;
	wxString searchString;
	searchString=((wxTextCtrl*)FindWindowById(IDC_SEARCHNAME))->GetValue();
	// printf("searchString ->%s<-\n", searchString.c_str());
	typeText=((wxChoice*)FindWindowById(IDC_TypeSearch))->GetStringSelection();
	theApp.searchlist->NewSearch(CString(typeText),m_nSearchID);
	FindWindowById(IDC_STARTS)->Disable();
	FindWindowById(IDC_CANCELS)->Enable();
	canceld = false;
	uint32 typeNemonic = 0x00030001;
	uint32 extensionNemonic = 0x00040001;
	uint32 avaibilityNemonic = 0x15000101;
	uint32 minNemonic = 0x02000101;
	uint32 maxNemonic = 0x02000102;
	byte stringParameter = 1;
	byte typeParameter = 2;
	byte numericParameter = 3;
	uint16 andParameter = 0x0000;
	// uint16 orParameter = 0x0100;
	CMemFile* data = new CMemFile(100);


	uint32 avaibility;
	wxString sizeMin,sizeMax,extension,avaibilitystr,type;
	sizeMin=((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHMIN))->GetValue();
	// printf("sizeMin ->%s<-\n", sizeMin.c_str());
	sizeMax=((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHMAX))->GetValue();
	// printf("sizeMax ->%s<-\n", sizeMax.c_str());
	uint32 max=atol(sizeMax.GetData())*1048576;
	uint32 min=atol(sizeMin.GetData())*1048576;
	if (max==0 || max<min) {
		max=2147483646; // max file size
	}
	extension=((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHEXTENSION))->GetValue();
	// printf("extension ->%s<-\n", extension.c_str());
	if (extension.Length()>0 && extension.GetChar(0)!='.') {
		extension="."+extension;
	}
	type="";
	if (CString(_("Audio"))==typeText) {
		type.Printf("Audio");
	}
	if (CString(_("Video"))==typeText) {
		type.Printf("Video");
	}
	if (CString(_("Programs"))==typeText) {
		type.Printf("Pro");
	}
	if (CString(_("Pictures"))==typeText) {
		type.Printf("Image");
	}
	avaibilitystr=((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHAVAIBILITY))->GetValue();
	// printf("avaibilitystr ->%s<-\n", avaibilitystr.c_str());
	avaibility=atol(avaibilitystr.GetData());
	if ( searchString.Length()==0 && extension.Length()==0) {
		delete data;
		return;
	}
	int parametercount=0; // must be written parametercount-1 parameter headers
	if ( searchString.Length()>0) {
		parametercount++;
		if (parametercount>1) {
			data->Write(andParameter);
		}
	}
	if (type.Length()>0) {
		parametercount++;
		if (parametercount>1) {
			data->Write(andParameter);
		}
	}
	if (min>0) {
		parametercount++;
		if (parametercount>1) {
			data->Write(andParameter);
		}
	}
	if (max>0) {
		parametercount++;
		if (parametercount>1) {
			data->Write(andParameter);
		}
	}
	if (avaibility>0) {
		parametercount++;
		if (parametercount>1) {
			data->Write(andParameter);
		}
	}
	if (extension.Length()>0) {
		parametercount++;
		if (parametercount>1) {
			data->Write(andParameter);
		}
	}
	// body
	// search a string
	if (searchString.Length()>0) {
		data->Write(stringParameter); // write the parameter type
		data->Write(((wxTextCtrl*)FindWindowById(IDC_SEARCHNAME))->GetValue().GetData());
	}
	if (type.Length()>0) {
		data->Write(typeParameter); // write the parameter type
		data->Write(type); // write parameter
		data->WriteRaw(&typeNemonic,3); // nemonic for this kind of parameter (only 3 bytes!!)
	}
	if (min>0) {
		data->Write(numericParameter); // write the parameter type
		data->Write(min); // write the parameter
		data->Write(minNemonic); // nemonic for this kind of parameter
	}
	if (max>0) {
		data->Write(numericParameter); // write the parameter type
		data->Write(max); // write the parameter
		data->Write(maxNemonic); // nemonic for this kind of parameter
	}
	if (avaibility>0) {
		data->Write(numericParameter); // write the parameter type
		data->Write(avaibility); // write the parameter
		data->Write(avaibilityNemonic); // nemonic for this kind of parameter
	}
	if (extension.Length()>0) {
		data->Write(stringParameter); // write the parameter type
		data->Write(extension);
		data->WriteRaw(&extensionNemonic,3); // nemonic for this kind of parameter (only 3 bytes!!)
	}
	Packet* packet = new Packet(data);
	packet->opcode = OP_SEARCHREQUEST;
	delete data;
	theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
	theApp.serverconnect->SendPacket(packet,false);
	if(((wxCheckBox*)FindWindowById(IDC_SGLOBAL))->IsChecked()) {
		if( theApp.glob_prefs->Score() ) {
			theApp.serverlist->ResetSearchServerPos();
		}
		searchpacket = packet;
		searchpacket->opcode = OP_GLOBSEARCHREQ;
		servercount = 0;
		globsearch = true;
	} else {
		globsearch = false;
		delete packet;
	}
	CreateNewTab(searchString,m_nSearchID);
}

// Only needed for CSearchListCtrl to be able to close us by passing only ID
void CSearchDlg::DeleteSearch(uint16 nSearchID) {
	theApp.searchlist->RemoveResults(nSearchID);

	/* get the searchlist notebook widget */
	CMuleNotebook* nb=(CMuleNotebook*)FindWindowById(ID_NOTEBOOK);
	if(nb == NULL) {
		// well.. if this actually happens..
		return;
	}

	/* find the searchlist & its parent in the notebooks page list */
	wxMutexLocker sLock(nb->m_LockTabs);
	if(nb->GetPageCount() > 0) {
		/* Remove the notebook page */
		for(unsigned int i=0; i < (unsigned int) nb->GetPageCount(); i++) {
			/* get searchlist->GetId() from the current notebook entry (?) */
			wxWindow * page = (wxWindow *)nb->GetPage(i);
			wxWindow * slctrl = page->FindWindowById(ID_SEARCHLISTCTRL, page);
			if(!slctrl) {
				continue;
			}
			uint16 sID = ((CSearchListCtrl *)slctrl)->GetSearchId();
			/* is this the searchlist tab we want to remove ? */
			if(sID == nSearchID) {
				/* Force further actions on the searchlistctrl to fail */
				((CSearchListCtrl *)slctrl)->InvalidateSearchId();
				nb->DeletePage(i);
				break;
			}
		}
	}
}

void CSearchDlg::DeleteAllSearchs()
{
	theApp.searchlist->Clear();
	/* get the searchlist notebook widget */
	CMuleNotebook* nb=(CMuleNotebook*)FindWindowById(ID_NOTEBOOK);
	if(nb == NULL) {
		printf("error: nb=NULL in deletesearch\n");
		return;
	}
	
	nb->DeleteAllPages();

	FindWindowById(IDC_CLEARALL)->Enable(FALSE);
}

void CSearchDlg::OnBnClickedSearchReset(wxCommandEvent& evt) {
	((wxTextCtrl*)FindWindowById(IDC_SEARCHNAME))->Clear();
	((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHMIN))->Clear();
	((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHMAX))->Clear();
	((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHEXTENSION))->Clear();
	((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHAVAIBILITY))->Clear();
	((wxTextCtrl*)FindWindowById(ID_ED2KLINKHANDLER))->Clear();
	wxChoice* Stypebox=(wxChoice*)FindWindowById(IDC_TypeSearch);
	Stypebox->SetSelection(Stypebox->FindString(CString(_("Any"))));
	FindWindowById(IDC_SEARCH_RESET)->Enable(FALSE);
}


/**
 * Sends the contents of the directdownload textctrl box to fast links handler
 */
void CSearchDlg::DirectDownload(wxCommandEvent &event) {
	theApp.amuledlg->StartFast((wxTextCtrl*)FindWindowById(ID_ED2KLINKHANDLER));
	((wxTextCtrl*)FindWindowById(ID_ED2KLINKHANDLER))->Clear();
}

/**
 * Update category autoassign choice box according to existing categories.
 */
void CSearchDlg::UpdateCatChoice() {
	CMuleNotebook* catbook = (CMuleNotebook*)FindWindowById(ID_CATEGORIES);
	wxASSERT(catbook);

	wxChoice *c_cat = (wxChoice*)FindWindowById(ID_AUTOCATASSIGN);
	c_cat->Clear();

	for (unsigned int i=0;i<(unsigned int)catbook->GetPageCount();i++) {
		c_cat->Append(catbook->GetPageText(i));
	}
	c_cat->SetSelection(0);
}

/**
 * Returns current selected category-assignment choice.
 */
uint8 CSearchDlg::GetCatChoice() {
	wxChoice *c_cat = (wxChoice*)FindWindowById(ID_AUTOCATASSIGN);
	return (uint8)c_cat->GetSelection();
}

/**
 * Toggles Links Handler on this page on/off depending wether
 * global FastED2KLinksHandler is turned on or off.
 */
void CSearchDlg::ToggleLinksHandler() {
	s_srcopts->Show(s_srced2klh, !theApp.glob_prefs->GetFED2KLH());
	Layout();
}

void CSearchDlg::OnRMButton(wxMouseEvent& evt) {
	CMuleNotebook* nb=(CMuleNotebook*)FindWindowById(ID_NOTEBOOK);
	if(nb->GetSelection() == -1) {
		return;
	}

	wxMenu* menu=new wxMenu(CString(_("Close")));
	menu->Append(MP_CLOSE_TAB,CString(_("Close tab")));
	menu->Append(MP_CLOSE_ALL_TABS,CString(_("Close all tabs")));
	menu->Append(MP_CLOSE_OTHER_TABS, CString(_("Close other tabs")));
	// the point coming from mulenotebook control isn't in screen coordinates
	// (unlike std mouse event, which always returns screen coordinates)
	// so we must do the conversion here
	wxPoint pt=evt.GetPosition();
	wxPoint newpt=nb->ClientToScreen(pt);
	newpt=ScreenToClient(newpt);
	//evt.Skip();
	PopupMenu(menu,newpt);
	delete menu;
}


bool CSearchDlg::ProcessEvent(wxEvent& evt)
{
	if(evt.GetEventType()!=wxEVT_COMMAND_MENU_SELECTED) {
		return wxPanel::ProcessEvent(evt);
	}	
	return true;
	CMuleNotebook* nb=(CMuleNotebook*)FindWindowById(ID_NOTEBOOK);	

	wxCommandEvent& event=(wxCommandEvent&)evt;
	switch(event.GetId()) {
		case MP_CLOSE_TAB: {
			int sel = nb->GetSelection();
			nb->DeletePage(sel);		
			break;
		}
		case MP_CLOSE_ALL_TABS:
				nb->DeleteAllPages();
			break;
		case MP_CLOSE_OTHER_TABS: {
			int sel = nb->GetSelection();
			int count = nb->GetPageCount();
			int where = 0;
			int i =  0;
			while (i < count) {
				if (i!= sel) {
					nb->DeletePage(where);
				} else {
					where = 1;
				}
				i++;
			}
			break;	
		}		
	}
	return true;
}
