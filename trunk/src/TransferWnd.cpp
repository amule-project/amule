//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "TransferWnd.h"	// Interface declarations

#include <common/MenuIDs.h>

#include <wx/config.h>


// This include must be before amuleDlg.h
#ifdef __WXMSW__
    #include <wx/msw/winundef.h>	// Needed for windows compilation
#endif


#include "amuleDlg.h"			// Needed for CamuleDlg
#include "PartFile.h"			// Needed for PR_LOW
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "CatDialog.h"			// Needed for CCatDialog
#include "DownloadListCtrl.h"		// Needed for CDownloadListCtrl
#include "SourceListCtrl.h"		// Needed for CSourceListCtrl
#include "amule.h"			// Needed for theApp
#include "muuli_wdr.h"			// Needed for ID_CATEGORIES
#include "SearchDlg.h"			// Needed for CSearchDlg->UpdateCatChoice()
#include "MuleNotebook.h"
#include "Preferences.h"
#include "Statistics.h"			// Needed for theStats
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "GuiEvents.h"			// Needed for CoreNotify_*


BEGIN_EVENT_TABLE(CTransferWnd, wxPanel)
	EVT_RIGHT_DOWN(CTransferWnd::OnNMRclickDLtab)
	EVT_NOTEBOOK_PAGE_CHANGED(ID_CATEGORIES,	CTransferWnd::OnCategoryChanged)
	
 	EVT_SPLITTER_SASH_POS_CHANGING(ID_DOWNLOADSSPLATTER, CTransferWnd::OnSashPositionChanging)

	EVT_BUTTON(ID_BTNCLRCOMPL,		CTransferWnd::OnBtnClearDownloads)
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
	clientlistctrl   = CastChild( ID_CLIENTLIST, CSourceListCtrl );
	m_dlTab          = CastChild( ID_CATEGORIES, CMuleNotebook );

	// Set disabled image for clear complete button
	CastChild(ID_BTNCLRCOMPL, wxBitmapButton)->SetBitmapDisabled(amuleDlgImages(34));
	
	// We want to use our own popup
	m_dlTab->SetPopupHandler( this );

	// Set default category
	theApp->glob_prefs->GetCategory(0)->title = GetCatTitle(thePrefs::GetAllcatFilter());
	theApp->glob_prefs->GetCategory(0)->path = thePrefs::GetIncomingDir();
	
	// Show default + userdefined categories
	for ( uint32 i = 0; i < theApp->glob_prefs->GetCatCount(); i++ ) {
		m_dlTab->AddPage( new wxPanel(m_dlTab), theApp->glob_prefs->GetCategory(i)->title );
	}

	m_menu = NULL;
	m_splitter = 0;
	
	wxConfigBase *config = wxConfigBase::Get();
	
	// Check if the clientlist is hidden
	bool show = true;
	config->Read( wxT("/GUI/TransferWnd/ShowClientList"), &show, true );
	clientlistctrl->SetShowing(show);
	// Load the last used splitter position
	m_splitter = config->Read( wxT("/GUI/TransferWnd/Splitter"), 463l );
}


CTransferWnd::~CTransferWnd()
{
	wxConfigBase *config = wxConfigBase::Get();

	if ( !clientlistctrl->GetShowing() ) {
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

	theApp->amuledlg->m_searchwnd->UpdateCatChoice();
}

void CTransferWnd::UpdateCategory(int index)
{
	uint32 nrCats = theApp->glob_prefs->GetCatCount();
	std::vector<uint16> files, downloads;
	bool showCatTabInfos = thePrefs::ShowCatTabInfos();
	if (showCatTabInfos) {
		files.insert(files.begin(), nrCats, 0);
		downloads.insert(downloads.begin(), nrCats, 0);
		
#ifdef CLIENT_GUI
		for (CDownQueueRem::const_iterator it = theApp->downloadqueue->begin(); it != theApp->downloadqueue->end(); ++it) {
			CPartFile *cur_file = it->second;
#else
		std::vector<CPartFile*> fileList;
		theApp->downloadqueue->CopyFileList(fileList, true);
		int size = fileList.size();
		for (int i = 0; i < size; ++i ) {
			CPartFile *cur_file = fileList[i];
#endif
			bool downloading = cur_file->GetTransferingSrcCount() > 0;
			int fileCat = cur_file->GetCategory();
			if ((index == -1 || fileCat == index) && cur_file->CheckShowItemInGivenCat(fileCat)) {
				files[fileCat]++;
				if (downloading) {
					downloads[fileCat]++;
				}
			}
			if (index == -1 && fileCat > 0 && cur_file->CheckShowItemInGivenCat(0)) {
				files[0]++;
				if (downloading) {
					downloads[0]++;
				}
			}
		}
		
	}
	int start, end;
	if (index == -1) {
		start = 0;
		end = nrCats - 1;
	} else {
		start = index;
		end = index;
	}
	for (int i = start; i <= end; i++) {
		wxString label = theApp->glob_prefs->GetCategory(i)->title;
		if (showCatTabInfos) {
			label += CFormat(wxT(" (%u/%u)")) % downloads[i] % files[i];
		}
		m_dlTab->SetPageText(i, label);
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


void CTransferWnd::OnAddCategory(wxCommandEvent& WXUNUSED(event))
{
	if (theApp->glob_prefs->GetCatCount() >= 99) {
		wxMessageBox(_("Only 99 categories are supported."), _("Too many categories!"), wxOK | wxICON_EXCLAMATION);
		return;
	}
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
		Category_Struct* newcat =
			theApp->glob_prefs->GetCategory(
				theApp->glob_prefs->GetCatCount()-1);
		theApp->sharedfiles->AddFilesFromDirectory(newcat->path);
		theApp->sharedfiles->Reload();		
	}
}


void CTransferWnd::OnDelCategory(wxCommandEvent& WXUNUSED(event))
{
	RemoveCategory(m_dlTab->GetSelection());
}


void CTransferWnd::RemoveCategory(int index)
{
	if ( index > 0 ) {
		theApp->downloadqueue->ResetCatParts(index);
		theApp->glob_prefs->RemoveCat(index);
		RemoveCategoryPage(index);
		if ( theApp->glob_prefs->GetCatCount() == 1 ) {
			thePrefs::SetAllcatFilter( acfAll );
		}
		theApp->glob_prefs->SaveCats();
		theApp->amuledlg->m_searchwnd->UpdateCatChoice();
	}
}


void CTransferWnd::RemoveCategoryPage(int index)
{
	m_dlTab->RemovePage(index);
	m_dlTab->SetSelection(0);
	downloadlistctrl->ChangeCategory(0);
}


void CTransferWnd::OnEditCategory( wxCommandEvent& WXUNUSED(event) )
{
	Category_Struct* cat = theApp->glob_prefs->GetCategory(m_dlTab->GetSelection());
	CPath oldpath = cat->path;
	CCatDialog dialog( this, 
	// Allow browse?
#ifdef CLIENT_GUI	
	false
#else
	true
#endif
		, m_dlTab->GetSelection());
	
	if (dialog.ShowModal() == wxOK) {
		if (!oldpath.IsSameDir(cat->path)) {
			theApp->sharedfiles->AddFilesFromDirectory(cat->path);
			theApp->sharedfiles->Reload();			
		}
	}
}


void CTransferWnd::OnSetDefaultCat( wxCommandEvent& event )
{
	thePrefs::SetAllcatFilter( static_cast<AllCategoryFilter>(event.GetId() - MP_CAT_SET0) );
	theApp->glob_prefs->GetCategory(0)->title = GetCatTitle( thePrefs::GetAllcatFilter() );
	
	UpdateCategory( 0 );
	
	downloadlistctrl->ChangeCategory( 0 );
	
	theApp->glob_prefs->SaveCats();
	
	downloadlistctrl->SortList();
}


void CTransferWnd::OnCategoryChanged(wxNotebookEvent& evt)
{
	// First remove currently showing sources (switching cat will deselect all)
	CKnownFileVector filesVector;
	clientlistctrl->ShowSources(filesVector);
	// Then change cat
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


void CTransferWnd::Prepare()
{
	wxSplitterWindow* splitter = CastChild( wxT("splitterWnd"), wxSplitterWindow );
	int height = splitter->GetSize().GetHeight();
	int header_height = s_clientlistHeader->GetSize().GetHeight();	
	
	if ( m_splitter ) {
		// Some sanity checking
		if ( m_splitter < s_splitterMin ) {
			m_splitter = s_splitterMin;
		} else if ( m_splitter > height - header_height * 2 ) {
			m_splitter = height - header_height * 2;
		}
		splitter->SetSashPosition( m_splitter );
		m_splitter = 0;
	}

	if ( !clientlistctrl->GetShowing() ) {
		// use a toggle event to close it (calculate size, change button)
		clientlistctrl->SetShowing( true );	// so it will be toggled to false
		wxCommandEvent evt1;
		OnToggleClientList( evt1 );
	}
}

void CTransferWnd::OnToggleClientList(wxCommandEvent& WXUNUSED(evt))
{
	wxSplitterWindow* splitter = CastChild( wxT("splitterWnd"), wxSplitterWindow );
	wxBitmapButton*   button = CastChild( ID_CLIENTTOGGLE, wxBitmapButton );

	if ( !clientlistctrl->GetShowing() ) {
		splitter->SetSashPosition( m_splitter );		
		m_splitter = 0;
		
		clientlistctrl->SetShowing( true );
		
		button->SetBitmapLabel( amuleDlgImages( 10 ) );
		button->SetBitmapFocus( amuleDlgImages( 10 ) );
		button->SetBitmapSelected( amuleDlgImages( 10 ) );
		button->SetBitmapHover( amuleDlgImages( 10 ) );
	} else {
		clientlistctrl->SetShowing( false );
	
		m_splitter = splitter->GetSashPosition();
	
		// Add the height of the listctrl to the top-window
		int height = clientlistctrl->GetSize().GetHeight()
					 + splitter->GetWindow1()->GetSize().GetHeight();
	
		splitter->SetSashPosition( height );
		
		button->SetBitmapLabel( amuleDlgImages( 11 ) );
		button->SetBitmapFocus( amuleDlgImages( 11 ) );
		button->SetBitmapSelected( amuleDlgImages( 11 ) );
		button->SetBitmapHover( amuleDlgImages( 11 ) );
	}
}


void CTransferWnd::OnSashPositionChanging(wxSplitterEvent& evt)
{
	if ( evt.GetSashPosition() < s_splitterMin ) {
		evt.SetSashPosition( s_splitterMin );
	} else {
		wxSplitterWindow* splitter = wxStaticCast( evt.GetEventObject(), wxSplitterWindow);
		wxCHECK_RET(splitter, wxT("ERROR: NULL splitter in CTransferWnd::OnSashPositionChanging"));
			
		int height = splitter->GetSize().GetHeight();
		int header_height = s_clientlistHeader->GetSize().GetHeight();	
		int mousey = wxGetMousePosition().y - splitter->GetScreenRect().GetTop();

		if ( !clientlistctrl->GetShowing() ) {
			// lower window hidden
			if ( height - mousey < header_height * 2 ) {
				// no moving down if already hidden
				evt.Veto();
			} else {
				// show it
				m_splitter = mousey;	// prevent jumping if it was minimized and is then shown by dragging the sash
				wxCommandEvent event;
				OnToggleClientList( event );
			}
		} else {
			// lower window showing
			if ( height - mousey < header_height * 2 ) {
				// hide it
				wxCommandEvent event;
				OnToggleClientList( event );
			} else {
				// normal resize
				// If several events queue up, setting the sash to the current mouse position
				// will speed up things and make sash moving more smoothly.
				evt.SetSashPosition( mousey );
			}
		}
	}
}
// File_checked_for_headers
