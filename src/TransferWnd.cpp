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


// TransferWnd.cpp : implementation file

#include <wx/settings.h>
#include <wx/splitter.h>
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/sizer.h>		// Needed for wxSizer
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>

#include "TransferWnd.h"	// Interface declarations
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "PartFile.h"		// Needed for PR_LOW
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "CatDialog.h"		// Needed for CCatDialog
#include "opcodes.h"		// Needed for MP_CAT_SET0
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "UploadListCtrl.h"	// Needed for CUploadListCtrl
#include "otherfunctions.h"	// Needed for GetCatTitle
#include "amule.h"			// Needed for theApp
#include "QueueListCtrl.h"	// Needed for CQueueListCtrl
#include "muuli_wdr.h"		// Needed for ID_CATEGORIES
#include "SearchDlg.h"		// Needed for CSearchDlg->UpdateCatChoice()
#include "MuleNotebook.h"
#include "Preferences.h"
#include "ClientList.h"

// CTransferWnd dialog

IMPLEMENT_DYNAMIC_CLASS(CTransferWnd,wxPanel)

BEGIN_EVENT_TABLE(CTransferWnd,wxPanel)
	EVT_NOTEBOOK_PAGE_CHANGED(ID_CATEGORIES,CTransferWnd::OnSelchangeDltab)
	EVT_RIGHT_DOWN(CTransferWnd::OnNMRclickDLtab)
	EVT_SPLITTER_SASH_POS_CHANGED(ID_SPLATTER, CTransferWnd::OnSashPositionChanged)
	EVT_BUTTON(ID_BTNCLRCOMPL, CTransferWnd::OnBtnClearDownloads)
	EVT_BUTTON(ID_BTNSWITCHUP, CTransferWnd::SwitchUploadList)
END_EVENT_TABLE()


//IMPLEMENT_DYNAMIC(CTransferWnd, CDialog)
CTransferWnd::CTransferWnd(wxWindow* pParent /*=NULL*/)
: wxPanel(pParent, -1)
{
	wxSizer* content=transferDlg(this,TRUE);
	content->Show(this,TRUE);

	uploadlistctrl=(CUploadListCtrl*)FindWindowByName(wxT("uploadList"));
	downloadlistctrl=(CDownloadListCtrl*)FindWindowByName(wxT("downloadList"));
	queuelistctrl=(CQueueListCtrl*)FindWindowByName(wxT("uploadQueue"));
	// let's hide the queue
	queueSizer->Remove(queuelistctrl);
	Layout();
	queuelistctrl->Show(FALSE);
	// allow notebook to dispatch right mouse clicks to us
	CMuleNotebook* nb=(CMuleNotebook*)FindWindowById(ID_CATEGORIES);
	// We want to use our own popup
	nb->SetPopupHandler( this );
	#if (wxMINOR_VERSION > 4) && (wxRELEASE_NUMBER > 1)
	nb->SetSizeHints(-1,32,-1,32);
	wxBitmapButton* bmap = (wxBitmapButton*)FindWindowById(ID_BTNSWWINDOW);
	transfer_top_boxsizer->Detach(nb);
	transfer_top_boxsizer->Detach(bmap);
	transfer_top_boxsizer->Add( nb, 1, wxALIGN_BOTTOM|wxALIGN_CENTER_HORIZONTAL|wxFIXED_MINSIZE, 5 );
	transfer_top_boxsizer->Add( bmap, 0, wxALIGN_CENTER|wxALL, 5 );
	Layout();
	#endif
	windowtransferstate=false;
	CatMenu=false;
}

CTransferWnd::~CTransferWnd()
{
}

bool CTransferWnd::OnInitDialog()
{
	m_dlTab=(CMuleNotebook*)FindWindowById(ID_CATEGORIES);

	// show & cat-tabs
	theApp.glob_prefs->GetCategory(0)->title = GetCatTitle(theApp.glob_prefs->GetAllcatType());

	theApp.glob_prefs->GetCategory(0)->incomingpath = theApp.glob_prefs->GetIncomingDir();
	
	for (uint32 ix=0;ix<theApp.glob_prefs->GetCatCount();ix++) {
		wxPanel* nullPanel=new wxPanel(m_dlTab);
		m_dlTab->AddPage(nullPanel,theApp.glob_prefs->GetCategory(ix)->title);
	}
	
	return true;
}

void CTransferWnd::ShowQueueCount(uint32 number)
{
	wxString fmtstr= (wxString::Format(wxT("%u (%u ") ,number,theApp.clientlist->GetBannedCount()) + wxT("Banned")) +wxT(")");
	wxStaticCast(FindWindowByName(wxT("clientCount")),wxStaticText)->SetLabel(fmtstr);
}

void CTransferWnd::SwitchUploadList(wxCommandEvent& WXUNUSED(evt))
{
	if( windowtransferstate == false) {
		windowtransferstate=true;
		// hide the upload list
		queueSizer->Remove(uploadlistctrl);
		uploadlistctrl->Show(FALSE);
		queueSizer->Add(queuelistctrl,1,wxGROW|wxALIGN_CENTER_VERTICAL ,5);
		queuelistctrl->Show();
		queueSizer->Layout();
		wxStaticCast(FindWindowByName(wxT("uploadTitle")),wxStaticText)->SetLabel(_("On Queue"));
	} else {
		windowtransferstate=false;
		// hide the queuelist
		queueSizer->Remove(queuelistctrl);
		queuelistctrl->Show(FALSE);
		queueSizer->Add(uploadlistctrl,1,wxGROW|wxALIGN_CENTER_VERTICAL, 5);
		uploadlistctrl->Show();
		queueSizer->Layout();
		wxStaticCast(FindWindowByName(wxT("uploadTitle")),wxStaticText)->SetLabel(_("Uploads"));
	}
}

void CTransferWnd::Localize()
{
}


void CTransferWnd::OnSelchangeDltab(wxNotebookEvent& evt)
{
  downloadlistctrl->ChangeCategory(evt.GetSelection());
  downloadlistctrl->SortList();
}

void CTransferWnd::OnNMRclickDLtab(wxMouseEvent& evt)
{
	CMuleNotebook* notebook = (CMuleNotebook*)FindWindowById(ID_CATEGORIES);
	
	// Only handle events from the category-notebook
	if ( evt.GetEventObject() != (wxObject*)notebook )
		return;
	
	if ( notebook->GetSelection() == -1 ) {
		return;
	}
	
	// Avoid opening another menu when it's already open
	if (CatMenu==false) {  
		
		CatMenu=true;
		wxMenu* menu=new wxMenu(_("Category"));

		if(notebook->GetSelection()==0) {
			wxMenu* m_CatMenu=new wxMenu();

			m_CatMenu->Append(MP_CAT_SET0,_("all"));
			m_CatMenu->Append(MP_CAT_SET0+1,_("all others"));
			m_CatMenu->AppendSeparator();
			m_CatMenu->Append(MP_CAT_SET0+2,_("Incomplete"));
			m_CatMenu->Append(MP_CAT_SET0+3,_("Completed"));
			m_CatMenu->Append(MP_CAT_SET0+4,_("Waiting"));
			m_CatMenu->Append(MP_CAT_SET0+5,_("Downloading"));
			m_CatMenu->Append(MP_CAT_SET0+6,_("Erroneous"));
			m_CatMenu->Append(MP_CAT_SET0+7,_("Paused"));
			m_CatMenu->Append(MP_CAT_SET0+8,_("Stopped"));
			m_CatMenu->AppendSeparator();
			m_CatMenu->Append(MP_CAT_SET0+9,_("Video"));
			m_CatMenu->Append(MP_CAT_SET0+10,_("Audio"));
			m_CatMenu->Append(MP_CAT_SET0+11,_("Archive"));
			m_CatMenu->Append(MP_CAT_SET0+12,_("CD-Images"));
			m_CatMenu->Append(MP_CAT_SET0+13,_("Pictures"));
			m_CatMenu->Append(MP_CAT_SET0+14,_("Text"));
			//m_CatMenu.CheckMenuItem( MP_CAT_SET0+theApp.glob_prefs->GetAllcatType() ,MF_CHECKED | MF_BYCOMMAND);
			menu->Append(47321,_("Select view filter"),m_CatMenu);
		}

		menu->Append(MP_CAT_ADD,_("Add category"));
		if(notebook->GetSelection()!=0) {
			menu->Append(MP_CAT_EDIT,_("Edit category"));
			menu->Append(MP_CAT_REMOVE, _("Remove category"));
		}
		menu->AppendSeparator();
		//menu->Append(472834,_("Priority"),m_PrioMenu);

		menu->Append(MP_CANCEL,_("Cancel"));
		menu->Append(MP_STOP, _("&Stop"));
		menu->Append(MP_PAUSE, _("&Pause"));
		menu->Append(MP_RESUME, _("&Resume"));
		//menu->Append(MP_RESUMENEXT, _("Resume next paused"));
		// the point coming from mulenotebook control isn't in screen coordinates
		// (unlike std mouse event, which always returns screen coordinates)
		// so we must do the conversion here
		
		PopupMenu(menu, evt.GetPosition());
		delete menu;

		CatMenu=false;
	}
}

bool CTransferWnd::ProcessEvent(wxEvent& evt)
{
	if(evt.GetEventType()!=wxEVT_COMMAND_MENU_SELECTED) {
		return wxPanel::ProcessEvent(evt);
	}
	/*
	if(evt.GetEventObject()!=this)
	return wxPanel::ProcessEvent(evt);
	*/
	wxCommandEvent& event=(wxCommandEvent&)evt;
	switch(event.GetId()) {
		case MP_CAT_SET0: 
		case MP_CAT_SET0+1: 
		case MP_CAT_SET0+2:
		case MP_CAT_SET0+3: 
		case MP_CAT_SET0+4: 
		case MP_CAT_SET0+5: 
		case MP_CAT_SET0+6:
		case MP_CAT_SET0+7:
		case MP_CAT_SET0+8: 
		case MP_CAT_SET0+9:
		case MP_CAT_SET0+10: 
		case MP_CAT_SET0+11:
		case MP_CAT_SET0+12: 
		case MP_CAT_SET0+13: 
		case MP_CAT_SET0+14: {
			theApp.glob_prefs->SetAllcatType(event.GetId()-MP_CAT_SET0);
			theApp.glob_prefs->GetCategory(0)->title = GetCatTitle(theApp.glob_prefs->GetAllcatType());
			EditCatTabLabel(0,theApp.glob_prefs->GetCategory(0)->title);
			downloadlistctrl->ChangeCategory(0);
			downloadlistctrl->SortList();
			break;
		}

		case MP_CAT_ADD: {
			int newindex=AddCategorie(wxT("?"),theApp.glob_prefs->GetIncomingDir(),wxEmptyString,false);
			//m_dlTab.InsertItem(newindex,theApp.glob_prefs->GetCatego
			//	       ry(newindex)->title);
			wxPanel* nullPanel=new wxPanel(m_dlTab,-1);
			m_dlTab->AddPage(nullPanel,theApp.glob_prefs->GetCategory(newindex)->title);
			CCatDialog dialog(this,newindex);
			dialog.OnInitDialog();
			dialog.ShowModal();

			EditCatTabLabel(newindex,theApp.glob_prefs->GetCategory(newindex)->title);
			theApp.glob_prefs->SaveCats();
			break;
		}
		case MP_CAT_EDIT: {
			CCatDialog dialog(this,m_dlTab->GetSelection());
			dialog.OnInitDialog();
			dialog.ShowModal();

			EditCatTabLabel(m_dlTab->GetSelection(),theApp.glob_prefs->GetCategory(m_dlTab->GetSelection())->title );

			theApp.glob_prefs->SaveCats();
			break;
		}
		case MP_CAT_REMOVE: {
			if (m_dlTab->GetSelection() != 0) {
				theApp.downloadqueue->ResetCatParts(m_dlTab->GetSelection());
				theApp.glob_prefs->RemoveCat(m_dlTab->GetSelection());
				m_dlTab->RemovePage(m_dlTab->GetSelection());
				m_dlTab->SetSelection(0);
				downloadlistctrl->ChangeCategory(0);
				theApp.glob_prefs->SaveCats();
				if (theApp.glob_prefs->GetCatCount()==1) {
					theApp.glob_prefs->SetAllcatType(0);
				}
				theApp.amuledlg->searchwnd->UpdateCatChoice();
			}
			break;
		}
		case MP_PRIOLOW: {
			CoreNotify_Download_Set_Cat_Prio(m_dlTab->GetSelection(),PR_LOW);
			break;
		}
		case MP_PRIONORMAL: {
			CoreNotify_Download_Set_Cat_Prio(m_dlTab->GetSelection(),PR_NORMAL);
			break;
		}
		case MP_PRIOHIGH: {
			CoreNotify_Download_Set_Cat_Prio(m_dlTab->GetSelection(),PR_HIGH );
			break;
		}
		case MP_PRIOAUTO: {
			CoreNotify_Download_Set_Cat_Prio(m_dlTab->GetSelection(),PR_AUTO );
			break;
		}
		case MP_PAUSE: {
			downloadlistctrl->SetCatStatus(m_dlTab->GetSelection(),MP_PAUSE);
			break;
		}
		case MP_STOP : {
			downloadlistctrl->SetCatStatus(m_dlTab->GetSelection(),MP_STOP);
			break;
		}

		case MP_CANCEL:
			if (wxMessageBox(_("Are you sure you wish to cancel and delete all files in this category?"),_("Confirmation Required"),
			   wxYES_NO|wxCENTRE|wxICON_EXCLAMATION) == wxYES) {
				downloadlistctrl->SetCatStatus(m_dlTab->GetSelection(),MP_CANCEL);
			}
			break;

		case MP_RESUME: {
			downloadlistctrl->SetCatStatus(m_dlTab->GetSelection(),MP_RESUME);
			break;
		}
		default: {
			return wxPanel::ProcessEvent(evt);
		}
	}
	return false;
}

int CTransferWnd::AddCategorie(wxString newtitle,wxString newincoming,wxString newcomment,bool addTab){
        Category_Struct* newcat=new Category_Struct;

        newcat->title = newtitle;
        newcat->prio=0;
        newcat->incomingpath = newincoming;
        newcat->comment = newcomment;
        int index=theApp.glob_prefs->AddCat(newcat);

        if (addTab) {
	  //m_dlTab.InsertItem(index,newtitle);
	  wxPanel* nullPanel=new wxPanel(m_dlTab,-1);
	  m_dlTab->AddPage(nullPanel,newtitle);
	}
	theApp.amuledlg->searchwnd->UpdateCatChoice();
        return index;
}

void CTransferWnd::EditCatTabLabel(int index,wxString newlabel)
{
	if (theApp.glob_prefs->ShowCatTabInfos()) {
		unsigned int count = 0;
		unsigned int dwl = 0;
		for (unsigned int i = 0; i < theApp.downloadqueue->GetFileCount(); ++i) {
			CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
			if ( !cur_file ) {
				continue;
			}
			if ( cur_file->CheckShowItemInGivenCat(index) ) {
				++count;
				if ( cur_file->GetTransferingSrcCount() > 0 ) {
					++dwl;
				}
			}
		}
		newlabel += wxString::Format(wxT(" (%u/%u)"), dwl, count);
	}
	m_dlTab->SetPageText(index,newlabel);
	theApp.amuledlg->searchwnd->UpdateCatChoice();
}

void CTransferWnd::OnSashPositionChanged(wxSplitterEvent& WXUNUSED(evt))
{
	theApp.amuledlg->split_pos = ((wxSplitterWindow*)FindWindow(wxT("splitterWnd")))->GetSashPosition();
}

void CTransferWnd::OnBtnClearDownloads(wxCommandEvent& WXUNUSED(evt)) {

    downloadlistctrl->Freeze();
    downloadlistctrl->ClearCompleted();
    downloadlistctrl->Thaw();
}

/*
void CTransferWnd::OnBtnSwitchUpload(wxCommandEvent& WXUNUSED(evt)) {
	
	if( windowtransferstate == false) {
		windowtransferstate=true;
		// hide the upload list
		queueSizer->Remove(uploadlistctrl);
		uploadlistctrl->Show(FALSE);
		queueSizer->Add(queuelistctrl,1,wxGROW|wxALIGN_CENTER_VERTICAL ,5);
		queuelistctrl->Show();
		queueSizer->Layout();
	} else {
		windowtransferstate=false;
		// hide the queuelist
		queueSizer->Remove(queuelistctrl);
		queuelistctrl->Show(FALSE);
		queueSizer->Add(uploadlistctrl,1,wxGROW|wxALIGN_CENTER_VERTICAL, 5);
		uploadlistctrl->Show();
		queueSizer->Layout();
	}
}
*/

void CTransferWnd::UpdateCatTabTitles() {
	for (uint8 i=0;i<m_dlTab->GetPageCount();i++) {
		EditCatTabLabel(i,(i==0)? GetCatTitle( theApp.glob_prefs->GetAllcatType() ):theApp.glob_prefs->GetCategory(i)->title);
	}
}
