//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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



BEGIN_EVENT_TABLE(CTransferWnd, wxPanel)
	EVT_RIGHT_DOWN(CTransferWnd::OnNMRclickDLtab)
	EVT_NOTEBOOK_PAGE_CHANGED(ID_CATEGORIES,	CTransferWnd::OnCategoryChanged)
	EVT_SPLITTER_SASH_POS_CHANGED(ID_SPLATTER, 	CTransferWnd::OnSashPositionChanged)
	EVT_BUTTON(ID_BTNCLRCOMPL, 					CTransferWnd::OnBtnClearDownloads)
	EVT_BUTTON(ID_BTNSWITCHUP, 					CTransferWnd::SwitchUploadList)

	EVT_MENU_RANGE(MP_CAT_SET0, MP_CAT_SET0 + 14, CTransferWnd::OnSetDefaultCat)
	EVT_MENU(MP_CAT_ADD, 			CTransferWnd::OnAddCategory)
	EVT_MENU(MP_CAT_EDIT, 			CTransferWnd::OnEditCategory)
	EVT_MENU(MP_CAT_REMOVE, 		CTransferWnd::OnDelCategory)
	EVT_MENU(MP_PRIOLOW,			CTransferWnd::OnSetCatPriority)
	EVT_MENU(MP_PRIONORMAL,			CTransferWnd::OnSetCatPriority)
	EVT_MENU(MP_PRIOHIGH, 			CTransferWnd::OnSetCatPriority)
	EVT_MENU(MP_PRIOAUTO,			CTransferWnd::OnSetCatPriority)
	EVT_MENU(MP_PAUSE,				CTransferWnd::OnSetCatStatus)
	EVT_MENU(MP_STOP,				CTransferWnd::OnSetCatStatus)
	EVT_MENU(MP_CANCEL,				CTransferWnd::OnSetCatStatus)
	EVT_MENU(MP_RESUME,				CTransferWnd::OnSetCatStatus)
END_EVENT_TABLE()



CTransferWnd::CTransferWnd( wxWindow* pParent )
	: wxPanel( pParent, -1 )
{
	wxSizer* content = transferDlg(this, true);
	content->Show(this, true);

	uploadlistctrl   = CastChild( wxT("uploadList"), CUploadListCtrl );
	downloadlistctrl = CastChild( wxT("downloadList"), CDownloadListCtrl );
	queuelistctrl    = CastChild( wxT("uploadQueue"), CQueueListCtrl );
	m_dlTab          = CastChild( ID_CATEGORIES, CMuleNotebook );
	
	// Let's hide the queue
	queueSizer->Remove(queuelistctrl);
	queuelistctrl->Show(FALSE);
	
	CMuleNotebook* nb = CastChild( ID_CATEGORIES, CMuleNotebook );
	// We want to use our own popup
	nb->SetPopupHandler( this );
	
	Layout();
	
	windowtransferstate = false;

	// Set default category
	theApp.glob_prefs->GetCategory(0)->title = GetCatTitle(theApp.glob_prefs->GetAllcatType());
	theApp.glob_prefs->GetCategory(0)->incomingpath = theApp.glob_prefs->GetIncomingDir();
	
	// Show default + userdefined categories
	for ( uint32 i = 0; i < theApp.glob_prefs->GetCatCount(); i++ ) {
		m_dlTab->AddPage( new wxPanel(m_dlTab), theApp.glob_prefs->GetCategory(i)->title );
	}

	m_menu = NULL;
}


CTransferWnd::~CTransferWnd()
{
}


void CTransferWnd::AddCategory( Category_Struct* category )
{
	// Add the initial page
	m_dlTab->AddPage( new wxPanel(m_dlTab), wxT("") );

	// Update the title
	UpdateCategory( m_dlTab->GetPageCount() - 1 );

	theApp.amuledlg->searchwnd->UpdateCatChoice();
}


void CTransferWnd::UpdateCategory( int index, bool titleChanged )
{
	wxString label = theApp.glob_prefs->GetCategory( index )->title;

	if ( theApp.glob_prefs->ShowCatTabInfos() ) {
		uint16 files = 0;
		uint16 download = 0;
		
		for ( unsigned int i = 0; i < theApp.downloadqueue->GetFileCount(); ++i ) {
			CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
			
			if ( cur_file && cur_file->CheckShowItemInGivenCat(index) ) {
				files++;
				
				if ( cur_file->GetTransferingSrcCount() ) {
					download++;
				}
			}
		}
		
		label += wxString::Format(wxT(" (%u/%u)"), download, files );
	}
	
	m_dlTab->SetPageText( index, label );


	if ( titleChanged ) {
		theApp.amuledlg->searchwnd->UpdateCatChoice();
	}
}


void CTransferWnd::OnSetCatStatus( wxCommandEvent& event )
{
	if ( event.GetId() == MP_CANCEL ) {
		if ( wxMessageBox(_("Are you sure you wish to cancel and delete all files in this category?"),_("Confirmation Required"), wxYES_NO|wxCENTRE|wxICON_EXCLAMATION) == wxNO ) {
			return;
		}
	}

	downloadlistctrl->SetCatStatus( m_dlTab->GetSelection(), event.GetId() );
}


void CTransferWnd::OnSetCatPriority( wxCommandEvent& event )
{
	int priority = 0;

	switch ( event.GetId() ) {
		case MP_PRIOLOW:	priority = PR_LOW;		break;
		case MP_PRIONORMAL:	priority = PR_NORMAL;	break;
		case MP_PRIOHIGH:	priority = PR_HIGH;		break;
		case MP_PRIOAUTO:	priority = PR_AUTO;		break;
		default:
			return;
	}
		
	CoreNotify_Download_Set_Cat_Prio( m_dlTab->GetSelection(), priority );
}


void CTransferWnd::OnAddCategory( wxCommandEvent& event )
{
	CCatDialog dialog( this );
	dialog.ShowModal();
}


void CTransferWnd::OnDelCategory( wxCommandEvent& WXUNUSED(event) )
{
	if ( m_dlTab->GetSelection() > 0 ) {
		theApp.downloadqueue->ResetCatParts( m_dlTab->GetSelection() );
		theApp.glob_prefs->RemoveCat( m_dlTab->GetSelection() );
		
		m_dlTab->RemovePage(m_dlTab->GetSelection());
		m_dlTab->SetSelection(0);
		
		downloadlistctrl->ChangeCategory(0);
		
		if ( theApp.glob_prefs->GetCatCount() == 1 ) {
			theApp.glob_prefs->SetAllcatType(0);
		}
		
		theApp.glob_prefs->SaveCats();

		theApp.amuledlg->searchwnd->UpdateCatChoice();
	}
}


void CTransferWnd::OnEditCategory( wxCommandEvent& WXUNUSED(event) )
{
	CCatDialog dialog( this, m_dlTab->GetSelection() );
	
	dialog.ShowModal();
}


void CTransferWnd::OnSetDefaultCat( wxCommandEvent& event )
{
	theApp.glob_prefs->SetAllcatType( event.GetId() - MP_CAT_SET0 );
	theApp.glob_prefs->GetCategory(0)->title = GetCatTitle( theApp.glob_prefs->GetAllcatType() );

	UpdateCategory( 0 );
	
	downloadlistctrl->ChangeCategory( 0 );
	downloadlistctrl->SortList();
}


void CTransferWnd::ShowQueueCount(uint32 number)
{
	wxString str = wxString::Format( wxT("%u (%u %s)"), number, theApp.clientlist->GetBannedCount(), _("Banned") );
	CastChild( wxT("clientCount"), wxStaticText )->SetLabel( str );
}


void CTransferWnd::SwitchUploadList(wxCommandEvent& WXUNUSED(evt))
{
	if ( windowtransferstate ) {
		windowtransferstate=false;
		// hide the queuelist
		queueSizer->Remove(queuelistctrl);
		queuelistctrl->Show(FALSE);
		queueSizer->Add(uploadlistctrl,1,wxGROW|wxALIGN_CENTER_VERTICAL, 5);
		uploadlistctrl->Show();
		CastChild( wxT("uploadTitle"), wxStaticText )->SetLabel(_("Uploads"));
	} else {
		windowtransferstate=true;
		// hide the upload list
		queueSizer->Remove(uploadlistctrl);
		uploadlistctrl->Show(FALSE);
		queueSizer->Add(queuelistctrl,1,wxGROW|wxALIGN_CENTER_VERTICAL ,5);
		queuelistctrl->Show();
		CastChild( wxT("uploadTitle"), wxStaticText )->SetLabel(_("On Queue"));

	}
	
	queueSizer->Layout();
}


void CTransferWnd::OnCategoryChanged(wxNotebookEvent& evt)
{
  downloadlistctrl->ChangeCategory(evt.GetSelection());
  downloadlistctrl->SortList();
}


void CTransferWnd::OnNMRclickDLtab(wxMouseEvent& evt)
{
	// Only handle events from the category-notebook
	if ( evt.GetEventObject() != (wxObject*)m_dlTab )
		return;
	
	if ( m_dlTab->GetSelection() == -1 ) {
		return;
	}
	
	// Avoid opening another menu when it's already open
	if ( m_menu == NULL ) {  
		wxMenu* m_menu = new wxMenu( _("Category") );

		if ( m_dlTab->GetSelection() == 0 ) {
			wxMenu* catmenu = new wxMenu();

			catmenu->Append( MP_CAT_SET0,      _("all") );
			catmenu->Append( MP_CAT_SET0 + 1,  _("all others") );
			
			catmenu->AppendSeparator();
			
			catmenu->Append( MP_CAT_SET0 + 2,  _("Incomplete") );
			catmenu->Append( MP_CAT_SET0 + 3,  _("Completed") );
			catmenu->Append( MP_CAT_SET0 + 4,  _("Waiting") );
			catmenu->Append( MP_CAT_SET0 + 5,  _("Downloading") );
			catmenu->Append( MP_CAT_SET0 + 6,  _("Erroneous") );
			catmenu->Append( MP_CAT_SET0 + 7,  _("Paused") );
			catmenu->Append( MP_CAT_SET0 + 8,  _("Stopped") );
			
			catmenu->AppendSeparator();
			
			catmenu->Append( MP_CAT_SET0 + 9,  _("Video") );
			catmenu->Append( MP_CAT_SET0 + 10, _("Audio") );
			catmenu->Append( MP_CAT_SET0 + 11, _("Archive") );
			catmenu->Append( MP_CAT_SET0 + 12, _("CD-Images") );
			catmenu->Append( MP_CAT_SET0 + 13, _("Pictures") );
			catmenu->Append( MP_CAT_SET0 + 14, _("Text") );
			
			m_menu->Append(-1, _("Select view filter"), catmenu);
		}

		m_menu->Append( MP_CAT_ADD, _("Add category") );
		
		if ( m_dlTab->GetSelection() ) {
			m_menu->Append(MP_CAT_EDIT,_("Edit category"));
			m_menu->Append(MP_CAT_REMOVE, _("Remove category"));
		}
		
		m_menu->AppendSeparator();
		
		m_menu->Append( MP_CANCEL, _("Cancel"));
		m_menu->Append( MP_STOP,   _("&Stop"));
		m_menu->Append( MP_PAUSE,  _("&Pause"));
		m_menu->Append( MP_RESUME, _("&Resume"));
		
		PopupMenu(m_menu, evt.GetPosition());
		
		delete m_menu;
		m_menu = NULL;
	}
}


void CTransferWnd::OnSashPositionChanged( wxSplitterEvent& WXUNUSED(evt) )
{
	theApp.amuledlg->split_pos = CastChild( wxT("splitterWnd"), wxSplitterWindow )->GetSashPosition();
}


void CTransferWnd::OnBtnClearDownloads( wxCommandEvent& WXUNUSED(evt) )
{
    downloadlistctrl->Freeze();
    downloadlistctrl->ClearCompleted();
    downloadlistctrl->Thaw();
}


void CTransferWnd::UpdateCatTabTitles()
{
	for ( uint8 i = 0; i < m_dlTab->GetPageCount(); i++ ) {
		UpdateCategory( i, false );
	}
}

