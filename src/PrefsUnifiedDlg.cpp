//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Original author: Emilio Sandoz
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


#include "PrefsUnifiedDlg.h"


#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/colordlg.h>
#include <wx/config.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>			// Needed in Mac compilation
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/tokenzr.h>
#include <wx/valgen.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/msgdlg.h>
#include <wx/stattext.h>
#include <wx/dirdlg.h>
#include <wx/checklst.h>
#ifdef __WXGTK__
	#include <wx/gtk/tooltip.h>
#endif

#include "amule.h"				// Needed for theApp
#include "amuleDlg.h"
#include "Color.h"
#include "OtherFunctions.h"		// Needed for IsEmptyFile
#include <common/StringFunctions.h>	// Needed for unicode2char
#include "EditServerListDlg.h"
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "StatisticsDlg.h"		// Needed for graph parameters, colors
#include "IPFilter.h"			// Needed for CIPFilter
#include "SearchList.h"
#include "DownloadQueue.h"
#include "ClientList.h"
#include "DirectoryTreeCtrl.h"	// Needed for CDirectoryTreeCtrl
#include "Preferences.h"
#include "muuli_wdr.h"
#include "Logger.h"
#include <common/Format.h>				// Needed for CFormat
#include "TransferWnd.h"		// Needed for CTransferWnd::UpdateCatTabTitles()
#include "KadDlg.h"				// Needed for CKadDlg


BEGIN_EVENT_TABLE(PrefsUnifiedDlg,wxDialog)
	// Proxy
	EVT_CHECKBOX(ID_PROXY_ENABLE_PROXY,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(ID_PROXY_ENABLE_PASSWORD,	PrefsUnifiedDlg::OnCheckBoxChange)
//	EVT_CHECKBOX(ID_PROXY_AUTO_SERVER_CONNECT_WITHOUT_PROXY,	PrefsUnifiedDlg::OnCheckBoxChange)
	
	// Connection
	EVT_SPINCTRL(IDC_PORT,			PrefsUnifiedDlg::OnTCPClientPortChange)
	
	// The rest. Organize it!
	EVT_CHECKBOX(IDC_UDPDISABLE,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_CHECKDISKSPACE,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_USESKIN,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_ONLINESIG,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_REMOVEDEAD,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_ENABLE_AUTO_HQRS,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_AUTOSERVER,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_AUTOIPFILTER,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_MSGFILTER,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_MSGFILTER_ALL,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_MSGFILTER_WORD,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_STARTNEXTFILE,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_ENABLETRAYICON,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_VERTTOOLBAR,	PrefsUnifiedDlg::OnCheckBoxChange)

	EVT_BUTTON(ID_PREFS_OK_TOP,		PrefsUnifiedDlg::OnOk)
	EVT_BUTTON(ID_OK,			PrefsUnifiedDlg::OnOk)

	EVT_BUTTON(ID_PREFS_CANCEL_TOP,		PrefsUnifiedDlg::OnCancel)

	// Browse buttons
	EVT_BUTTON(IDC_SELSKINFILE,		PrefsUnifiedDlg::OnButtonBrowseSkin)
	EVT_BUTTON(IDC_BTN_BROWSE_WAV,		PrefsUnifiedDlg::OnButtonBrowseWav)
	EVT_BUTTON(IDC_BROWSEV,			PrefsUnifiedDlg::OnButtonBrowseApplication)
	EVT_BUTTON(IDC_SELTEMPDIR,		PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELINCDIR,		PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELOSDIR,		PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELBROWSER,		PrefsUnifiedDlg::OnButtonBrowseApplication)

	EVT_SPINCTRL(IDC_TOOLTIPDELAY,		PrefsUnifiedDlg::OnToolTipDelayChange)

	EVT_BUTTON(IDC_EDITADR,			PrefsUnifiedDlg::OnButtonEditAddr)
	EVT_BUTTON(IDC_DESKTOPMODE,		PrefsUnifiedDlg::OnButtonSystray)
	EVT_BUTTON(IDC_IPFRELOAD,		PrefsUnifiedDlg::OnButtonIPFilterReload)
	EVT_BUTTON(IDC_COLOR_BUTTON,		PrefsUnifiedDlg::OnButtonColorChange)
	EVT_BUTTON(IDC_IPFILTERUPDATE,		PrefsUnifiedDlg::OnButtonIPFilterUpdate)
	EVT_CHOICE(IDC_COLORSELECTOR,		PrefsUnifiedDlg::OnColorCategorySelected)
	EVT_CHOICE(IDC_BROWSER,			PrefsUnifiedDlg::OnBrowserChange)
	EVT_LIST_ITEM_SELECTED(ID_PREFSLISTCTRL,PrefsUnifiedDlg::OnPrefsPageChange)

	EVT_INIT_DIALOG(PrefsUnifiedDlg::OnInitDialog)

	EVT_COMMAND_SCROLL(IDC_SLIDER,		PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_SLIDER3,		PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_SLIDER4,		PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_SLIDER2,		PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_FILEBUFFERSIZE,	PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_QUEUESIZE,	PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_SERVERKEEPALIVE,	PrefsUnifiedDlg::OnScrollBarChange)

	EVT_SPINCTRL(IDC_MAXUP,			PrefsUnifiedDlg::OnRateLimitChanged)
	
	EVT_CLOSE(PrefsUnifiedDlg::OnClose)
	
END_EVENT_TABLE()


// Static vars
int PrefsUnifiedDlg::s_ID;


/**
 * This struct provides a general way to represent config-tabs.
 */
struct PrefsPage
{
	//! The title of the page, used on the listctrl.
	wxString	m_title;
	//! Function pointer to the wxDesigner function creating the dialog.
	wxSizer*	(*m_function)(wxWindow*, bool, bool );
	//! The index of the image used on the list.
	int 		m_imageidx;
	//! The actual widget. To be set later.
	wxPanel*	m_widget;
};


PrefsPage pages[] =
{
	{ wxTRANSLATE("General"),		PreferencesGeneralTab,		13, NULL },
	{ wxTRANSLATE("Connection"),		PreferencesConnectionTab,	14, NULL },
	{ wxTRANSLATE("Proxy"),			PreferencesProxyTab,		24, NULL },
	{ wxTRANSLATE("Message Filter"),	PreferencesMessagesTab,		23, NULL },
	{ wxTRANSLATE("Remote Controls"),	PreferencesRemoteControlsTab,	11, NULL },
	{ wxTRANSLATE("Online Signature"),	PreferencesOnlineSigTab,	21, NULL },
	{ wxTRANSLATE("Server"),		PreferencesServerTab,		15, NULL },
	{ wxTRANSLATE("Files"),			PreferencesFilesTab,		16, NULL },
	{ wxTRANSLATE("Sources Dropping"),	PreferencesSourcesDroppingTab,	20, NULL },
	{ wxTRANSLATE("Directories"),		PreferencesDirectoriesTab,	17, NULL },
	{ wxTRANSLATE("Statistics"),		PreferencesStatisticsTab,	10, NULL },
	{ wxTRANSLATE("Security"),		PreferencesSecurityTab,		22, NULL },
	//Notications are disabled since they havent been implemented
	//{ wxTRANSLATE("Notifications"),	PreferencesNotifyTab,		18, NULL },
	{ wxTRANSLATE("Gui Tweaks"),		PreferencesGuiTweaksTab,	19, NULL },
	{ wxTRANSLATE("Core Tweaks"),		PreferencesaMuleTweaksTab,	12, NULL }
#ifdef __DEBUG__
	,{ wxTRANSLATE("Debugging"),		PreferencesDebug,			25, NULL }
#endif
};


PrefsUnifiedDlg *PrefsUnifiedDlg::NewPrefsDialog(wxWindow *parent)
{
	// Do not allow multiple dialogs
	if ( s_ID ) {
		return NULL;
	}

	return new PrefsUnifiedDlg( parent );
}


PrefsUnifiedDlg::PrefsUnifiedDlg(wxWindow *parent)
:
wxDialog(parent, -1, _("Preferences"), wxDefaultPosition, wxDefaultSize,
	wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	s_ID = GetId();

	preferencesDlgTop( this, FALSE );
	
	wxListCtrl* PrefsIcons = CastChild( ID_PREFSLISTCTRL, wxListCtrl );

	wxImageList* icon_list = new wxImageList(16, 16);
	PrefsIcons->AssignImageList( icon_list, wxIMAGE_LIST_SMALL);

	// Add the single column used
	PrefsIcons->InsertColumn(0, wxEmptyString, wxLIST_FORMAT_LEFT, PrefsIcons->GetSize().GetWidth()-5);

	// Temp variables for finding the smallest height and width needed
	int width = 0;
	int height = 0;

	// Add each page to the page-list
	for ( unsigned int i = 0; i < ELEMENT_COUNT(pages); i++ ) {
		// Add the icon and label assosiated with the page
		icon_list->Add( amuleSpecial(pages[i].m_imageidx) );
		PrefsIcons->InsertItem(i, wxGetTranslation(pages[i].m_title), i);
	}
	
	// Set list-width so that there arn't any scrollers
	PrefsIcons->SetColumnWidth( 0, wxLIST_AUTOSIZE );
	prefs_list_sizer->SetMinSize( PrefsIcons->GetColumnWidth( 0 ) + 10, 0 );

	// Now add the pages and calculate the minimum size	
	for ( unsigned int i = 0; i < ELEMENT_COUNT(pages); i++ ) {
		// Create a container widget and the contents of the page
		pages[i].m_widget = new wxPanel( this, -1 );
		pages[i].m_function( pages[i].m_widget, true, true );

		// Add it to the sizer
		prefs_sizer->Add( pages[i].m_widget, 0, wxGROW|wxEXPAND );

		if (pages[i].m_function == PreferencesGeneralTab) {
			// This must be done now or pages won't Fit();
			#if defined(USE_WX_TRAY) || defined(__SYSTRAY_DISABLED__)
					FindWindow(IDC_DESKTOPMODE)->Show(false);
					IDC_MISC_OPTIONS->Remove(FindWindow(IDC_DESKTOPMODE));
			#endif /* USE_WX_TRAY || __SYSTRAY_DISABLED__ */
			#ifdef __WXMSW__ 
				CastChild(IDC_BROWSERTABS, wxCheckBox)->Enable(false);
				wxChoice *browserCheck = CastChild(IDC_BROWSER, wxChoice);
				browserCheck->Clear();
				browserCheck->Append(_("System default"));
				browserCheck->Append(_("User Defined"));
			#endif /* __WXMSW__ */
		}
		
		// Align and resize the page
		Fit();
		Layout();

		// Find the greatest sizes
		wxSize size = prefs_sizer->GetSize();
		if ( size.GetWidth() > width ) {
			width = size.GetWidth();
		}

		if ( size.GetHeight() > height ) {
			height = size.GetHeight();
		}

		// Hide it for now
		prefs_sizer->Remove( pages[i].m_widget );
		pages[i].m_widget->Show( false );
	}
	
	// Default to the General tab
	m_CurrentPanel = pages[0].m_widget;
	prefs_sizer->Add( pages[0].m_widget, 0, wxGROW|wxEXPAND );
	m_CurrentPanel->Show( true );

	// Select the first item
	PrefsIcons->SetItemState( 0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );

	// We now have the needed minimum height and width
	prefs_sizer->SetMinSize( width, height );

	// Store some often used pointers
	m_ShareSelector = CastChild( IDC_SHARESELECTOR, CDirectoryTreeCtrl );
	m_buttonColor   = CastChild( IDC_COLOR_BUTTON, wxButton );
	m_choiceColor   = CastChild( IDC_COLORSELECTOR, wxChoice );

	// Connect the Cfgs with their widgets
	thePrefs::CFGMap::iterator it = thePrefs::s_CfgList.begin();
	for ( ; it != thePrefs::s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->ConnectToWidget( it->first, this ) ) {
			printf("Failed to connect Cfg to widget with the ID %d and key %s\n",
				it->first, (const char *)unicode2char(it->second->GetKey()));
		}
	}
	Fit();

	// It must not be resized to something smaller than what it currently is
	wxSize size = GetClientSize();
	SetSizeHints( size.GetWidth(), size.GetHeight() );
	
	#ifdef __SYSTRAY_DISABLED__
		FindWindow(IDC_ENABLETRAYICON)->Enable(false);
		FindWindow(IDC_MINTRAY)->Enable(false);
		#ifndef USE_WX_TRAY
			FindWindow(IDC_DESKTOPMODE)->Enable(false);
		#endif
	#endif
	
}


void PrefsUnifiedDlg::ClosePreferences()
{
	// Un-Connect the Cfgs
	thePrefs::CFGMap::iterator it = thePrefs::s_CfgList.begin();
	for ( ; it != thePrefs::s_CfgList.end(); ++it ) {
		// Checking for failures
		it->second->ConnectToWidget( 0 );
	}

	// Final actions:
	// Reset the ID so that a new dialog can be created
	s_ID = 0;

	// Hide the dialog since Destroy isn't instant
	Show(false);
}


Cfg_Base* PrefsUnifiedDlg::GetCfg(int id)
{
	thePrefs::CFGMap::iterator it = thePrefs::s_CfgList.find( id );

	if ( it != thePrefs::s_CfgList.end() ) {
		return it->second;
	}

	return NULL;
}


bool PrefsUnifiedDlg::TransferToWindow()
{
	// Connect the Cfgs with their widgets
	thePrefs::CFGMap::iterator it = thePrefs::s_CfgList.begin();
	for ( ; it != thePrefs::s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->TransferToWindow() ) {
			printf("Failed to transfer data from Cfg to Widget with the ID %d and key %s\n",
				it->first, (const char *)unicode2char(it->second->GetKey()));
		}
	}

	m_ShareSelector->SetSharedDirectories(&theApp.glob_prefs->shareddir_list);

	for ( int i = 0; i < cntStatColors; i++ ) {
		thePrefs::s_colors[i] = CStatisticsDlg::acrStat[i];
		thePrefs::s_colors_ref[i] = CStatisticsDlg::acrStat[i];
	}
	
	// Connection tab
	wxSpinEvent e;
	OnTCPClientPortChange(e);
	
	// Proxy tab initialization
	if (!CastChild(ID_PROXY_ENABLE_PROXY, wxCheckBox)->IsChecked()) {
		FindWindow(ID_PROXY_TYPE)->Enable(false);
		FindWindow(ID_PROXY_NAME)->Enable(false);
		FindWindow(ID_PROXY_PORT)->Enable(false);
	}
	if (!CastChild(ID_PROXY_ENABLE_PASSWORD, wxCheckBox)->IsChecked()) {
		FindWindow(ID_PROXY_USER)->Enable(false);
		FindWindow(ID_PROXY_PASSWORD)->Enable(false);
	}
	// This option from the proxy tab is currently unused
	FindWindow(ID_PROXY_AUTO_SERVER_CONNECT_WITHOUT_PROXY)->Enable(false);
	
	// Enable/Disable some controls
	bool customBrowser = CastChild( IDC_BROWSER, wxChoice )->GetSelection() == CastChild( IDC_BROWSER, wxChoice )->GetCount() - 1;
	FindWindow( IDC_BROWSERSELF )->Enable( customBrowser );
	FindWindow( IDC_SELBROWSER )->Enable( customBrowser );
	#ifndef __WXMSW__
		FindWindow( IDC_BROWSERTABS )->Enable( !customBrowser );
	#endif
	FindWindow( IDC_MINDISKSPACE )->Enable( thePrefs::IsCheckDiskspaceEnabled() );
	FindWindow( IDC_SKINFILE )->Enable( thePrefs::UseSkin() );
	FindWindow( IDC_OSDIR )->Enable( thePrefs::IsOnlineSignatureEnabled() );
	FindWindow( IDC_OSUPDATE )->Enable( thePrefs::IsOnlineSignatureEnabled() );
	FindWindow( IDC_UDPPORT )->Enable( !thePrefs::s_UDPDisable );
	FindWindow( IDC_SERVERRETRIES )->Enable( thePrefs::DeadServer() );
	FindWindow( IDC_HQR_VALUE )->Enable( thePrefs::DropHighQueueRankingSources() );
	FindWindow( IDC_IPFILTERURL )->Enable( thePrefs::IPFilterAutoLoad() );
	FindWindow( IDC_STARTNEXTFILE_SAME )->Enable(thePrefs::StartNextFile());
	
	FindWindow(IDC_MINTRAY)->Enable(thePrefs::UseTrayIcon());
	#ifndef USE_WX_TRAY
		FindWindow(IDC_DESKTOPMODE)->Enable(thePrefs::UseTrayIcon());
	#endif

	if (!CastChild(IDC_MSGFILTER, wxCheckBox)->IsChecked()) {
		FindWindow(IDC_MSGFILTER_ALL)->Enable(false);
		FindWindow(IDC_MSGFILTER_NONSECURE)->Enable(false);
		FindWindow(IDC_MSGFILTER_NONFRIENDS)->Enable(false);
		FindWindow(IDC_MSGFILTER_WORD)->Enable(false);		
		FindWindow(IDC_MSGWORD)->Enable(false);
	} else if (CastChild(IDC_MSGFILTER_ALL, wxCheckBox)->IsChecked()) {
		FindWindow(IDC_MSGFILTER_NONSECURE)->Enable(false);
		FindWindow(IDC_MSGFILTER_NONFRIENDS)->Enable(false);
		FindWindow(IDC_MSGFILTER_WORD)->Enable(false);		
		FindWindow(IDC_MSGWORD)->Enable(false);	
	}

	FindWindow(IDC_MSGWORD)->Enable(CastChild(IDC_MSGFILTER_WORD, wxCheckBox)->IsChecked());	
	
	// Set the permissions scrollers for files
	int perms = thePrefs::GetFilePermissions();
	CastChild( IDC_SPIN_PERM_FU, wxSpinCtrl )->SetValue( perms / 0100 );
	CastChild( IDC_SPIN_PERM_FG, wxSpinCtrl )->SetValue( perms % 0100 / 010 );
	CastChild( IDC_SPIN_PERM_FO, wxSpinCtrl )->SetValue( perms % 0100 % 010 / 01 );
	
	// Set the permissions scrollers for directories
	perms = thePrefs::GetDirPermissions();
	CastChild( IDC_SPIN_PERM_DU, wxSpinCtrl )->SetValue( perms / 0100 );
	CastChild( IDC_SPIN_PERM_DG, wxSpinCtrl )->SetValue( perms % 0100 / 010 );
	CastChild( IDC_SPIN_PERM_DO, wxSpinCtrl )->SetValue( perms % 0100 % 010 / 01 );

	// Set debugging toggles
#ifdef __DEBUG__
	int count = CLogger::GetDebugCategoryCount();
	wxCheckListBox* list = CastChild( ID_DEBUGCATS, wxCheckListBox );

	for ( int i = 0; i < count; i++ ) {
		const CDebugCategory& cat = CLogger::GetDebugCategory( i );
		
		list->Append( cat.GetName() );
		list->Check( i, cat.IsEnabled() );
	}
#endif
	
	return true;
}


bool PrefsUnifiedDlg::TransferFromWindow()
{
	// Connect the Cfgs with their widgets
	thePrefs::CFGMap::iterator it = thePrefs::s_CfgList.begin();
	for ( ; it != thePrefs::s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->TransferFromWindow() ) {
			printf("Failed to transfer data from Widget to Cfg with the ID %d and key %s\n",
				it->first, (const char *)unicode2char(it->second->GetKey()));
		}
	}

	theApp.glob_prefs->shareddir_list.Clear();
	m_ShareSelector->GetSharedDirectories(&theApp.glob_prefs->shareddir_list);

	for ( int i = 0; i < cntStatColors; i++ ) {
		if ( thePrefs::s_colors[i] != thePrefs::s_colors_ref[i] ) {
			CStatisticsDlg::acrStat[i] = thePrefs::s_colors[i];
			theApp.amuledlg->statisticswnd->ApplyStatsColor(i);
		}

		theApp.amuledlg->kademliawnd->SetGraphColors();
	}

	// Set the file-permissions value
	int file_perms = 0;
	file_perms |= 0100 * CastChild( IDC_SPIN_PERM_FU, wxSpinCtrl )->GetValue();
	file_perms |= 0010 * CastChild( IDC_SPIN_PERM_FG, wxSpinCtrl )->GetValue();
	file_perms |= 0001 * CastChild( IDC_SPIN_PERM_FO, wxSpinCtrl )->GetValue();
	thePrefs::SetFilePermissions( file_perms );

	int dir_perms = 0;
	dir_perms |= 0100 * CastChild( IDC_SPIN_PERM_DU, wxSpinCtrl )->GetValue();
	dir_perms |= 0010 * CastChild( IDC_SPIN_PERM_DG, wxSpinCtrl )->GetValue();
	dir_perms |= 0001 * CastChild( IDC_SPIN_PERM_DO, wxSpinCtrl )->GetValue();
	thePrefs::SetDirPermissions( dir_perms );

	// Get debugging toggles
#ifdef __DEBUG__
	int count = CLogger::GetDebugCategoryCount();
	wxCheckListBox* list = CastChild( ID_DEBUGCATS, wxCheckListBox );

	for ( int i = 0; i < count; i++ ) {
		const CDebugCategory& cat = CLogger::GetDebugCategory( i );
		
		CLogger::SetEnabled( cat.GetType(), list->IsChecked( i ) );
	}
#endif

	
	return true;
}


bool PrefsUnifiedDlg::CfgChanged(int ID)
{
	Cfg_Base* cfg = GetCfg(ID);

	if ( cfg ) {
		return cfg->HasChanged();
	}

	return false;
}


void PrefsUnifiedDlg::OnOk(wxCommandEvent& WXUNUSED(event))
{
	TransferFromWindow();

	// do sanity checking, special processing, and user notifications here
	thePrefs::CheckUlDlRatio();

	// Force port checking
	thePrefs::SetPort(thePrefs::GetPort());
	
	if (	IsEmptyFile(theApp.ConfigDir + wxT("addresses.dat")) && 
		CastChild(IDC_AUTOSERVER, wxCheckBox)->IsChecked() ) {
		thePrefs::UnsetAutoServerStart();
		wxMessageBox(wxString::wxString( _(
			"Your Auto-update servers list is in blank.\n"
			"'Auto-update serverlist at startup' will be disabled.")));
	}

	if ( thePrefs::AcceptExternalConnections() && thePrefs::ECPassword().IsEmpty() ) {
		thePrefs::EnableExternalConnections( false );

		wxMessageBox( _(
			"You have enabled external connections but have not specified a password.\n"
			"External connections cannot be enabled unless a valid password is specified.") );
	}
	
	// save the preferences on ok
	theApp.glob_prefs->Save();

	if ( CfgChanged(IDC_FED2KLH) && theApp.amuledlg->GetActiveDialog() != CamuleDlg::SearchWnd ) {
		theApp.amuledlg->ShowED2KLinksHandler( thePrefs::GetFED2KLH() );
	}

	if ( CfgChanged(IDC_LANGUAGE) ) {
		wxMessageBox(wxString::wxString(
			_("Language change will not be applied until aMule is restarted.")));
	}


	if (	CfgChanged(IDC_INCFILES) ||
		CfgChanged(IDC_TEMPFILES) ||
		m_ShareSelector->HasChanged ) {
		theApp.sharedfiles->Reload(false);
	}

	if ( CfgChanged(IDC_OSDIR) || CfgChanged(IDC_ONLINESIG) ) {
		wxTextCtrl* widget = CastChild( IDC_OSDIR, wxTextCtrl );

		// Build the filenames for the two OS files
		theApp.SetOSFiles( widget->GetValue() );
	}

	if (thePrefs::GetIPFilterOn()) {
		theApp.clientlist->FilterQueues();
	}

	if (thePrefs::GetShowRatesOnTitle()) {
		// This avoids a 5 seconds delay to show the title
		theApp.amuledlg->SetTitle(theApp.m_FrameTitle + wxT(" -- ") + _("Up: 0.0 | Down: 0.0"));
	} else {
		// This resets the title
		theApp.amuledlg->SetTitle(theApp.m_FrameTitle);
	}

	if (CfgChanged(IDC_EXTCATINFO)) {
		theApp.amuledlg->transferwnd->UpdateCatTabTitles();
	}

	// Changes related to the statistics-dlg
	if ( CfgChanged(IDC_SLIDER) ) {
		theApp.amuledlg->statisticswnd->SetUpdatePeriod();
		theApp.amuledlg->kademliawnd->SetUpdatePeriod();
	}

	if ( CfgChanged(IDC_SLIDER3) ) {
		theApp.amuledlg->statisticswnd->ResetAveragingTime();
	}
	
	if ( CfgChanged(IDC_DOWNLOAD_CAP) ) {
		theApp.amuledlg->statisticswnd->SetARange( true, thePrefs::GetMaxGraphDownloadRate() );
	}

	if ( CfgChanged(IDC_UPLOAD_CAP) ) {
		theApp.amuledlg->statisticswnd->SetARange( false, thePrefs::GetMaxGraphUploadRate() );
	}

	if (thePrefs::GetNetworkED2K() && theApp.IsConnectedED2K()) {
		theApp.serverconnect->Disconnect();
	}
	
	if (thePrefs::GetNetworkKademlia() && theApp.IsConnectedKad()) {
		theApp.StopKad();
	}	
	
	ClosePreferences();
}


void PrefsUnifiedDlg::OnClose(wxCloseEvent& WXUNUSED(event))
{
	ClosePreferences();
}


void PrefsUnifiedDlg::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	ClosePreferences();
}


void PrefsUnifiedDlg::OnCheckBoxChange(wxCommandEvent& event)
{
	bool		value = event.IsChecked();

	switch ( event.GetId() ) {
		case IDC_UDPDISABLE:
			// UDP is disable rather than enable, so we flip the value
			FindWindow( IDC_UDPPORT )->Enable(!value);
			break;
			
		case IDC_CHECKDISKSPACE:
			FindWindow( IDC_MINDISKSPACE )->Enable(value);
			break;	
		
		case IDC_USESKIN:
			FindWindow( IDC_SKINFILE )->Enable(value);;
			break;

		case IDC_ONLINESIG:
			FindWindow( IDC_OSDIR )->Enable(value);;
			FindWindow(IDC_OSUPDATE)->Enable(value);
			break;

		case IDC_REMOVEDEAD:
			FindWindow( IDC_SERVERRETRIES )->Enable(value);;
			break;

		case IDC_ENABLE_AUTO_HQRS:
			FindWindow( IDC_HQR_VALUE )->Enable(value);;
			break;

		case IDC_AUTOSERVER:
			if (	IsEmptyFile(theApp.ConfigDir + wxT("addresses.dat")) && 
				CastChild(event.GetId(), wxCheckBox)->IsChecked() ) {
				wxMessageBox(wxString::wxString( _(
					"Your Auto-update servers list is in blank.\n"
					"Please fill in at least one URL to point to a valid server.met file.\n"
					"Click on the button \"List\" by this checkbox to enter an URL.")));
				CastChild(event.GetId(), wxCheckBox)->SetValue(false);
			}
			break;

		case IDC_AUTOIPFILTER:
			FindWindow( IDC_IPFILTERURL )->Enable(value);;
			break;
		
		case IDC_MSGFILTER:
			// Toogle All filter options
			FindWindow(IDC_MSGFILTER_ALL)->Enable(value);
			FindWindow(IDC_MSGFILTER_NONSECURE)->Enable(value);
			FindWindow(IDC_MSGFILTER_NONFRIENDS)->Enable(value);
			FindWindow(IDC_MSGFILTER_WORD)->Enable(value);		
			if (value) {
				FindWindow(IDC_MSGWORD)->Enable(
					CastChild(IDC_MSGFILTER_WORD, wxCheckBox)->IsChecked());
			} else {
				FindWindow(IDC_MSGWORD)->Enable(false);
			}
			break;
		
		case IDC_MSGFILTER_ALL:
			// Toogle filtering by data.
			FindWindow(IDC_MSGFILTER_NONSECURE)->Enable(!value);
			FindWindow(IDC_MSGFILTER_NONFRIENDS)->Enable(!value);
			FindWindow(IDC_MSGFILTER_WORD)->Enable(!value);		
			if (!value) {
				FindWindow(IDC_MSGWORD)->Enable(
					CastChild(IDC_MSGFILTER_WORD, wxCheckBox)->IsChecked());
			} else {
				FindWindow(IDC_MSGWORD)->Enable(false);
			}
			break;
		
		case IDC_MSGFILTER_WORD:
			// Toogle filter word list.
			FindWindow(IDC_MSGWORD)->Enable(value);
			break;

		case ID_PROXY_ENABLE_PROXY:
			FindWindow(ID_PROXY_TYPE)->Enable(value);
			FindWindow(ID_PROXY_NAME)->Enable(value);
			FindWindow(ID_PROXY_PORT)->Enable(value);
			break;
			
		case ID_PROXY_ENABLE_PASSWORD:
			FindWindow(ID_PROXY_USER)->Enable(value);
			FindWindow(ID_PROXY_PASSWORD)->Enable(value);
			break;
			
		case IDC_STARTNEXTFILE:
			FindWindow(IDC_STARTNEXTFILE_SAME)->Enable(value);
			break;
		
		case IDC_ENABLETRAYICON:
			#ifndef __SYSTRAY_DISABLED__
				FindWindow(IDC_MINTRAY)->Enable(value);
				FindWindow(IDC_DESKTOPMODE)->Enable(value);
				if (value) {
					theApp.amuledlg->CreateSystray();
				} else {
					theApp.amuledlg->RemoveSystray();
				}
			#else
				// This should never happen as button is disabled
				wxASSERT(0);
			#endif
			break;
		
		case ID_PROXY_AUTO_SERVER_CONNECT_WITHOUT_PROXY:
			break;
		case IDC_VERTTOOLBAR:
			theApp.amuledlg->Create_Toolbar(wxEmptyString, value);
			break;
	}
}


void PrefsUnifiedDlg::OnButtonColorChange(wxCommandEvent& WXUNUSED(event))
{
	int index = m_choiceColor->GetSelection();
	wxColour col = WxColourFromCr( thePrefs::s_colors[index] );
	col = wxGetColourFromUser( this, col );
	if ( col.Ok() ) {
		m_buttonColor->SetBackgroundColour( col );
		thePrefs::s_colors[index] = CrFromWxColour(col);
	}
}


void PrefsUnifiedDlg::OnColorCategorySelected(wxCommandEvent& WXUNUSED(evt))
{
	m_buttonColor->SetBackgroundColour(
		WxColourFromCr( thePrefs::s_colors[ m_choiceColor->GetSelection() ] ) );
}


void PrefsUnifiedDlg::OnBrowserChange( wxCommandEvent& evt )
{
	wxTextCtrl* textctrl = CastChild( IDC_BROWSERSELF, wxTextCtrl );
	wxButton* btn = CastChild( IDC_SELBROWSER, wxButton );
	bool enable = evt.GetSelection() == CastChild( IDC_BROWSER, wxChoice )->GetCount() - 1;

	if (textctrl) {
		textctrl->Enable( enable );
	}
	if (btn) {
		btn->Enable( enable );
	}
#ifndef __WXMSW__
	FindWindow( IDC_BROWSERTABS )->Enable( !enable );
#endif
}

void PrefsUnifiedDlg::OnButtonSystray(wxCommandEvent& WXUNUSED(evt))
{
	#ifndef __SYSTRAY_DISABLED__
		#ifndef USE_WX_TRAY
			theApp.amuledlg->changeDesktopMode();

			// Ensure that the dialog is still visible afterwards
			Raise();
			SetFocus();
		#else
			// Should never happen, button is not shown.
			wxASSERT(0);
		#endif
	#else
			// Should never happen, button is not enabled.
			wxASSERT(0);	
	#endif
}


void PrefsUnifiedDlg::OnButtonDir(wxCommandEvent& event)
{
	wxString type;

	int id = 0;
	switch ( event.GetId() ) {
	case IDC_SELTEMPDIR:
		id = IDC_TEMPFILES;
		type = _("Temporary files");
		break;

	case IDC_SELINCDIR:
		id = IDC_INCFILES;
		type = _("Incoming files");
		break;

	case IDC_SELOSDIR:
		id = IDC_OSDIR;
		type = _("Online Signatures");
		break;

	default:
		wxASSERT( false );
		return;
	}

	type = CFormat(_("Choose a folder for %s")) % type;

	wxTextCtrl* widget = CastChild( id, wxTextCtrl );
	wxString dir = widget->GetValue();

	wxString str = wxDirSelector( type, dir );

	if ( !str.IsEmpty() ) {
		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonBrowseWav(wxCommandEvent& WXUNUSED(evt))
{
	wxString str = wxFileSelector( 
		_("Browse wav"), wxEmptyString, wxEmptyString,
		wxT("*.wav"), _("File wav (*.wav)|*.wav||") );
	
	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = CastChild( IDC_EDIT_TBN_WAVFILE, wxTextCtrl );

		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonBrowseSkin(wxCommandEvent& WXUNUSED(evt))
{
	wxString str = wxFileSelector(
		_("Browse skin file"), wxEmptyString, wxEmptyString, wxT("*") );

	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = CastChild( IDC_SKINFILE, wxTextCtrl );

		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonBrowseApplication(wxCommandEvent& event)
{
	wxString title;
	int id = 0;
	switch ( event.GetId() ) {
		case IDC_BROWSEV:
			id = IDC_VIDEOPLAYER;
			title = _("Browse for videoplayer");
			break;
		case IDC_SELBROWSER:
			id = IDC_BROWSERSELF;
			title = _("Select browser");
			break;
		default:
			wxASSERT( false );
			return;
	}
	wxString wildcard = CFormat(_("Executable%s"))
#ifdef __WINDOWS__
		% wxT(" (*.exe)|*.exe");
#else
		% wxT("|*");
#endif
	
	wxString str = wxFileSelector( title, wxEmptyString, wxEmptyString,
		wxEmptyString, wildcard );

	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = CastChild( id, wxTextCtrl );
		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonEditAddr(wxCommandEvent& WXUNUSED(evt))
{
	wxString fullpath( theApp.ConfigDir + wxT("addresses.dat") );

	EditServerListDlg* test = new EditServerListDlg(this, _("Edit Serverlist"),
		_("Add here URL's to download server.met files.\nOnly one url on each line."),
		fullpath );

	test->ShowModal();
	delete test;
}


void PrefsUnifiedDlg::OnButtonIPFilterReload(wxCommandEvent& WXUNUSED(event))
{
	theApp.ipfilter->Reload();
	theApp.clientlist->FilterQueues();
}


void PrefsUnifiedDlg::OnButtonIPFilterUpdate(wxCommandEvent& WXUNUSED(event))
{
	theApp.ipfilter->Update( CastChild( IDC_IPFILTERURL, wxTextCtrl )->GetValue() );
}


void PrefsUnifiedDlg::OnPrefsPageChange(wxListEvent& event)
{
	prefs_sizer->Remove( m_CurrentPanel );
	m_CurrentPanel->Show( false );

	m_CurrentPanel = pages[ event.GetIndex() ].m_widget;

	prefs_sizer->Add( m_CurrentPanel, 0, wxGROW|wxEXPAND );
	m_CurrentPanel->Show( true );

	Layout();
}


void PrefsUnifiedDlg::OnToolTipDelayChange(wxSpinEvent& event)
{
#ifdef __WXGTK__
	wxToolTip::SetDelay( event.GetPosition() * 1000 );
#else
	#warning NO TOOLTIPS FOR NON-GTK!
#endif
}


void PrefsUnifiedDlg::OnInitDialog( wxInitDialogEvent& WXUNUSED(evt) )
{
// This function exists solely to avoid automatic transfer-to-widget calls
}


void PrefsUnifiedDlg::OnScrollBarChange( wxScrollEvent& event )
{
	int id = 0;
	wxString label;

	switch ( event.GetId() ) {
	case IDC_SLIDER:
		id = IDC_SLIDERINFO;
		label = wxString::Format( _("Update delay: %d secs"), event.GetPosition() );
		break;

	case IDC_SLIDER3:
		id = IDC_SLIDERINFO3;
		label = wxString::Format( _("Time for average graph: %d mins"), event.GetPosition() );
		break;

	case IDC_SLIDER4:
		id = IDC_SLIDERINFO4;
		label = wxString::Format( _("Connections Graph Scale: %d"), event.GetPosition() );
		break;

	case IDC_SLIDER2:
		id = IDC_SLIDERINFO2;
		label = wxString::Format( _("Update delay : %d secs"), event.GetPosition() );
		break;

	case IDC_FILEBUFFERSIZE:
		id = IDC_FILEBUFFERSIZE_STATIC;
		label = wxString::Format( _("File Buffer Size: %d bytes"), event.GetPosition() * 15000 );
		break;

	case IDC_QUEUESIZE:
		id = IDC_QUEUESIZE_STATIC;
		label = wxString::Format( _("Upload Queue Size: %d clients"), event.GetPosition() * 100 );
		break;

	case IDC_SERVERKEEPALIVE:
		id = IDC_SERVERKEEPALIVE_LABEL;

		if ( event.GetPosition() ) {
			label = wxString::Format( _("Server connection refresh interval: %d minutes"),
				event.GetPosition() );
		} else {
			label = wxString::Format( _("Server connection refresh interval: Disabled") );
		}
		break;

	default:
		return;
	}

	wxStaticText* widget = CastChild( id, wxStaticText );

	if (widget) {
		widget->SetLabel( label );
		widget->GetParent()->Layout();
	}
}


void PrefsUnifiedDlg::OnRateLimitChanged( wxSpinEvent& event )
{
	// Here we do immediate sainity checking of the up/down ratio,
	// so that the user can see if his choice is illegal

	// We only do checks if the rate is limited
	if ( event.GetPosition() != (int)UNLIMITED ) {
		wxSpinCtrl* dlrate = CastChild( IDC_MAXDOWN, wxSpinCtrl );
	
		if ( event.GetPosition() < 4 ) {
			if (	( event.GetPosition() * 3 < dlrate->GetValue() ) ||
				( dlrate->GetValue() == (int)UNLIMITED ) ) {
				dlrate->SetValue( event.GetPosition() * 3 );
			}
		} else if ( event.GetPosition() < 10  ) {
			if (	( event.GetPosition() * 4 < dlrate->GetValue() ) ||
				( dlrate->GetValue() == (int)UNLIMITED ) ) {
				dlrate->SetValue( event.GetPosition() * 4 );
			}
		}
	}
}


void PrefsUnifiedDlg::OnTCPClientPortChange(wxSpinEvent& WXUNUSED(event))
{
	int port = CastChild(IDC_PORT, wxSpinCtrl)->GetValue();
	wxString txt;
	txt << wxT("UDP port for extended server requests (TCP+3):") << port + 3;
	CastChild(ID_TEXT_CLIENT_UDP_PORT, wxStaticText)->SetLabel(txt);
}
