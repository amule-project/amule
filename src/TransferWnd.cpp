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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <wx/settings.h>
#include <wx/splitter.h>
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/sizer.h>		// Needed for wxSizer
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>
#include <wx/config.h>

#include "TransferWnd.h"	// Interface declarations
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "PartFile.h"		// Needed for PR_LOW
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "CatDialog.h"		// Needed for CCatDialog
#include "OPCodes.h"		// Needed for MP_CAT_SET0
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "ClientListCtrl.h"	// Needed for CClientListCtrl
#include "OtherFunctions.h"	// Needed for GetCatTitle
#include "amule.h"		// Needed for theApp
#include "muuli_wdr.h"		// Needed for ID_CATEGORIES
#include "SearchDlg.h"		// Needed for CSearchDlg->UpdateCatChoice()
#include "MuleNotebook.h"
#include "Preferences.h"
#include "ClientList.h"
#include "Statistics.h"		// Needed for theStats
#include "SharedFileList.h"		// Needed for CSharedFileList

BEGIN_EVENT_TABLE(CTransferWnd, wxPanel)
	EVT_RIGHT_DOWN(CTransferWnd::OnNMRclickDLtab)
	EVT_NOTEBOOK_PAGE_CHANGED(ID_CATEGORIES,	CTransferWnd::OnCategoryChanged)
	
 	EVT_SPLITTER_SASH_POS_CHANGING(ID_SPLATTER, CTransferWnd::OnSashPositionChanging)

	EVT_BUTTON(ID_BTNCLRCOMPL,		CTransferWnd::OnBtnClearDownloads)
 	EVT_BUTTON(ID_BTNSWITCHUP, 		CTransferWnd::SwitchUploadList)
 	EVT_BUTTON(ID_CLIENTTOGGLE,		CTransferWnd::OnToggleClientList)

	EVT_MENU_RANGE(MP_CAT_SET0, MP_CAT_SET0 + 15, CTransferWnd::OnSetDefaultCat)
	EVT_MENU(MP_CAT_ADD, 			CTransferWnd::OnAddCategory)
	EVT_MENU(MP_CAT_EDIT, 			CTransferWnd::OnEditCategory)
	EVT_MENU(MP_CAT_REMOVE, 		CTransferWnd::OnDelCategory)
	EVT_MENU(MP_PRIOLOW,			CTransferWnd::OnSetCatPriority)
	EVT_MENU(MP_PRIONORMAL,			CTransferWnd::OnSetCatPriority)
	EVT_MENU(MP_PRIOHIGH, 			CTransferWnd::OnSetCatPriority)
	EVT_MENU(MP_PRIOAUTO,			CTransferWnd::OnSetCatPriority)
	EVT_MENU(MP_PAUSE,			CTransferWnd::OnSetCatStatus)
	EVT_MENU(MP_STOP,			CTransferWnd::OnSetCatStatus)
	EVT_MENU(MP_CANCEL,			CTransferWnd::OnSetCatStatus)
	EVT_MENU(MP_RESUME,			CTransferWnd::OnSetCatStatus)
END_EVENT_TABLE()



CTransferWnd::CTransferWnd( wxWindow* pParent )
	: wxPanel( pParent, -1 )
{
	wxSizer* content = transferDlg(this, true);
	content->Show(this, true);

	downloadlistctrl = CastChild( wxT("downloadList"), CDownloadListCtrl );
	clientlistctrl   = CastChild( ID_CLIENTLIST, CClientListCtrl );
	m_dlTab          = CastChild( ID_CATEGORIES, CMuleNotebook );
	
	CMuleNotebook* nb = CastChild( ID_CATEGORIES, CMuleNotebook );
	// We want to use our own popup
	nb->SetPopupHandler( this );
	
	// Set default category
	theApp.glob_prefs->GetCategory(0)->title = GetCatTitle(thePrefs::GetAllcatType());
	theApp.glob_prefs->GetCategory(0)->incomingpath = thePrefs::GetIncomingDir();
	
	// Show default + userdefined categories
	for ( uint32 i = 0; i < theApp.glob_prefs->GetCatCount(); i++ ) {
		m_dlTab->AddPage( new wxPanel(m_dlTab), theApp.glob_prefs->GetCategory(i)->title );
	}

	m_menu = NULL;
	m_splitter = 0;

	
	wxConfigBase *config = wxConfigBase::Get();
	
	// Check if the clientlist is hidden
	bool show = true;
	config->Read( wxT("/GUI/TransferWnd/ShowClientList"), &show, true );

	if ( !show ) {
		// Disable the client-list
		wxCommandEvent event;
		OnToggleClientList( event );	
	}

	// Load the last used splitter position
	m_splitter = config->Read( wxT("/GUI/TransferWnd/Splitter"), 463l );
}


CTransferWnd::~CTransferWnd()
{
	wxConfigBase *config = wxConfigBase::Get();

	if ( clientlistctrl->GetListView() == vtNone ) {
		// Save the splitter position
		config->Write( wxT("/GUI/TransferWnd/Splitter"), m_splitter );
	
		// Save the visible status of the list
		config->Write( wxT("/GUI/TransferWnd/ShowClientList"), false );
	} else {
		wxSplitterWindow* splitter = CastChild( wxT("splitterWnd"), wxSplitterWindow );
		
		// Save the splitter position
		config->Write( wxT("/GUI/TransferWnd/Splitter"), splitter->GetSashPosition() );		
		
		// Save the visible status of the list
		config->Write( wxT("/GUI/TransferWnd/ShowClientList"), true );
	}
}


void CTransferWnd::AddCategory( Category_Struct* category )
{
	// Add the initial page
	m_dlTab->AddPage( new wxPanel(m_dlTab), category->title );

	// Update the title
	UpdateCategory( m_dlTab->GetPageCount() - 1 );

	theApp.amuledlg->searchwnd->UpdateCatChoice();
}

void CTransferWnd::RemoveCategory(int index)
{
	m_dlTab->RemovePage(index);
				
	m_dlTab->SetSelection(0);
	downloadlistctrl->ChangeCategory(0);
}

void CTransferWnd::UpdateCategory( int index, bool titleChanged )
{
	wxString label = theApp.glob_prefs->GetCategory( index )->title;

	if ( thePrefs::ShowCatTabInfos() ) {
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
		if ( wxMessageBox(_("Are you sure you wish to cancel and delete all files in this category?"),_("Confirmation Required"), wxYES_NO|wxCENTRE|wxICON_EXCLAMATION, this) == wxNO) {
			return;
		}
	}

	CoreNotify_Download_Set_Cat_Status( m_dlTab->GetSelection(), event.GetId() );
}


void CTransferWnd::OnSetCatPriority( wxCommandEvent& event )
{
	int priority = 0;

	switch ( event.GetId() ) {
		case MP_PRIOLOW:	priority = PR_LOW;	break;
		case MP_PRIONORMAL:	priority = PR_NORMAL;	break;
		case MP_PRIOHIGH:	priority = PR_HIGH;	break;
		case MP_PRIOAUTO:	priority = PR_AUTO;	break;
		default:
			return;
	}
		
	CoreNotify_Download_Set_Cat_Prio( m_dlTab->GetSelection(), priority );
}


void CTransferWnd::OnAddCategory( wxCommandEvent& WXUNUSED(event) )
{
	CCatDialog dialog( this,
	// Allow browse?
#ifdef CLIENT_GUI	
	false
#else
	true
#endif	
	);
	if (dialog.ShowModal() == wxOK) {
		// Add the files on this folder.
		Category_Struct* newcat = theApp.glob_prefs->GetCategory(theApp.glob_prefs->GetCatCount()-1);
		theApp.sharedfiles->AddFilesFromDirectory(newcat->incomingpath);
		theApp.sharedfiles->Reload();		
	}
}


void CTransferWnd::OnDelCategory( wxCommandEvent& WXUNUSED(event) )
{
	if ( m_dlTab->GetSelection() > 0 ) {
		theApp.downloadqueue->ResetCatParts( m_dlTab->GetSelection() );
		theApp.glob_prefs->RemoveCat( m_dlTab->GetSelection() );
		
		RemoveCategory(m_dlTab->GetSelection());
		
		if ( theApp.glob_prefs->GetCatCount() == 1 ) {
			thePrefs::SetAllcatType(0);
		}
		
		theApp.glob_prefs->SaveCats();

		theApp.amuledlg->searchwnd->UpdateCatChoice();
		
	}
}


void CTransferWnd::OnEditCategory( wxCommandEvent& WXUNUSED(event) )
{
	Category_Struct* cat = theApp.glob_prefs->GetCategory(m_dlTab->GetSelection());
	wxString oldpath = cat->incomingpath;
	CCatDialog dialog( this, 
	// Allow browse?
#ifdef CLIENT_GUI	
	false
#else
	true
#endif
		, m_dlTab->GetSelection());
	
	if (dialog.ShowModal() == wxOK) {
		if (oldpath != cat->incomingpath) {
			theApp.sharedfiles->AddFilesFromDirectory(cat->incomingpath);
			theApp.sharedfiles->Reload();			
		}
	}
}


void CTransferWnd::OnSetDefaultCat( wxCommandEvent& event )
{
	thePrefs::SetAllcatType( event.GetId() - MP_CAT_SET0 );
	theApp.glob_prefs->GetCategory(0)->title = GetCatTitle( thePrefs::GetAllcatType() );
	
	UpdateCategory( 0 );
	
	downloadlistctrl->ChangeCategory( 0 );
	
	theApp.glob_prefs->SaveCats();
	
	downloadlistctrl->SortList();
}


void CTransferWnd::ShowQueueCount(uint32 number)
{
	wxString str = wxString::Format( wxT("%u (%u %s)"), number, theStats::GetBannedCount(), _("Banned") );
	wxStaticText* label = CastChild( ID_CLIENTCOUNT, wxStaticText );
	
	label->SetLabel( str );
	label->GetParent()->Layout();
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
		m_menu = new wxMenu( _("Category") );

		if ( m_dlTab->GetSelection() == 0 ) {
			wxMenu* catmenu = new wxMenu();

			catmenu->Append( MP_CAT_SET0,      _("All") );
			catmenu->Append( MP_CAT_SET0 + 1,  _("All others") );
			
			catmenu->AppendSeparator();
			
			catmenu->Append( MP_CAT_SET0 + 2,  _("Incomplete") );
			catmenu->Append( MP_CAT_SET0 + 3,  _("Completed") );
			catmenu->Append( MP_CAT_SET0 + 4,  _("Waiting") );
			catmenu->Append( MP_CAT_SET0 + 5,  _("Downloading") );
			catmenu->Append( MP_CAT_SET0 + 6,  _("Erroneous") );
			catmenu->Append( MP_CAT_SET0 + 7,  _("Paused") );
			catmenu->Append( MP_CAT_SET0 + 8,  _("Stopped") );
			catmenu->Append( MP_CAT_SET0 + 15, _("Active") );
			
			catmenu->AppendSeparator();
			
			catmenu->Append( MP_CAT_SET0 + 9,  _("Video") );
			catmenu->Append( MP_CAT_SET0 + 10, _("Audio") );
			catmenu->Append( MP_CAT_SET0 + 11, _("Archive") );
			catmenu->Append( MP_CAT_SET0 + 12, _("CD-Images") );
			catmenu->Append( MP_CAT_SET0 + 13, _("Pictures") );
			catmenu->Append( MP_CAT_SET0 + 14, _("Text") );
			
			m_menu->Append(0, _("Select view filter"), catmenu);
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


void CTransferWnd::Prepare()
{
	int header_height = s_clientlistHeader->GetSize().GetHeight();
	wxSplitterWindow* splitter = CastChild( wxT("splitterWnd"), wxSplitterWindow );
	
	if ( clientlistctrl->GetListView() == vtNone ) {
		int height = splitter->GetWindow1()->GetSize().GetHeight();
	    	height += splitter->GetWindow2()->GetSize().GetHeight();
	
		splitter->SetSashPosition( height - header_height );
	} else if ( m_splitter ) {
		// Some sainity checking
		if ( m_splitter < 90 ) {
			m_splitter = 90;
		} else {
			int height = splitter->GetWindow1()->GetSize().GetHeight();
		    	height += splitter->GetWindow2()->GetSize().GetHeight();
		
			if ( height - header_height * 2 < m_splitter ) {
				m_splitter = height - header_height * 2;
			}
		}
		
		splitter->SetSashPosition( m_splitter );

		m_splitter = 0;
	}
}


void CTransferWnd::SwitchUploadList(wxCommandEvent& WXUNUSED(evt))
{
 	clientlistctrl->ToggleView();
	wxStaticText* label = CastChild( wxT("uploadTitle"), wxStaticText );
 	
 	switch ( clientlistctrl->GetListView() ) {
 		case vtNone:
 			return;
 		
 		case vtUploading:
 			label->SetLabel( _("Uploads") );
 			break;
 			
 		case vtQueued:
 			label->SetLabel( _("On Queue") );
 			break;
 
 		case vtClients:
 			label->SetLabel( _("Clients") );
 			break;
 	}

	label->GetParent()->Layout();
}


void CTransferWnd::OnToggleClientList(wxCommandEvent& WXUNUSED(evt))
{
	wxSplitterWindow* splitter = CastChild( wxT("splitterWnd"), wxSplitterWindow );
	wxBitmapButton*   button = CastChild( ID_CLIENTTOGGLE, wxBitmapButton );

	// Keeps track of what was last selected.
	static ViewType lastView = vtNone;
		
		
	if ( clientlistctrl->GetListView() == vtNone ) {
		splitter->SetSashPosition( m_splitter );		
		
		clientlistctrl->SetListView( lastView );
		
		button->SetBitmapLabel( amuleDlgImages( 10 ) );
		button->SetBitmapFocus( amuleDlgImages( 10 ) );
		button->SetBitmapSelected( amuleDlgImages( 10 ) );

		m_splitter = 0;
	} else {
		lastView = clientlistctrl->GetListView();
		clientlistctrl->SetListView( vtNone );
	
		int pos = splitter->GetSashPosition();
	
		// Add the height of the listctrl to the top-window
		int height  = clientlistctrl->GetSize().GetHeight();
		height += splitter->GetWindow1()->GetSize().GetHeight();
	
		splitter->SetSashPosition( height );
		
		m_splitter = pos;

		button->SetBitmapLabel( amuleDlgImages( 11 ) );
		button->SetBitmapFocus( amuleDlgImages( 11 ) );
		button->SetBitmapSelected( amuleDlgImages( 11 ) );
	}

	FindWindow(ID_BTNSWITCHUP)->Enable( clientlistctrl->GetListView() != vtNone );
}


void CTransferWnd::OnSashPositionChanging(wxSplitterEvent& evt)
{
	int header_height = s_clientlistHeader->GetSize().GetHeight();
	
	if ( evt.GetSashPosition() < 90 ) {
		evt.SetSashPosition( 90 );
	} else {
		wxSplitterWindow* splitter = wxStaticCast( evt.GetEventObject(), wxSplitterWindow);
			
		int height = splitter->GetWindow1()->GetSize().GetHeight();
		    height += splitter->GetWindow2()->GetSize().GetHeight();

		if ( clientlistctrl->GetListView() == vtNone ) {
			if ( height - evt.GetSashPosition() < header_height * 2 ) {
				evt.Veto();
			} else {
				wxCommandEvent event;
				OnToggleClientList( event );
			}
		} else {
			if ( height - evt.GetSashPosition() < header_height * 2 ) {
				evt.SetSashPosition( height - header_height );

				wxCommandEvent event;
				OnToggleClientList( event );
			}
		}
	}
}
