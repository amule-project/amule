//
// This file is part of the aMule Project
//
// Copyright (c) 2004 aMule Project ( http://www.amule-project.net )
// Original author: Emilio Sandoz
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

#ifdef __WXGTK__
	#include <wx/gtk/tooltip.h>
#endif


#include "amule.h"			// Needed for theApp
#include "amuleDlg.h"
#include "color.h"
#include "otherfunctions.h"		// Needed for MakeFoldername
#include "EditServerListDlg.h"
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "StatisticsDlg.h"		// Needed for graph parameters, colors
#include "IPFilter.h"			// Needed for CIPFilter
#include "SearchList.h"
#include "DownloadQueue.h"
#include "ClientList.h"
#include "DirectoryTreeCtrl.h"		// Needed for CDirectoryTreeCtrl
#include "Preferences.h"
#include "muuli_wdr.h"


BEGIN_EVENT_TABLE(PrefsUnifiedDlg,wxDialog)
	EVT_CHECKBOX( IDC_UDPDISABLE,		PrefsUnifiedDlg::OnCheckBoxChange )
	EVT_CHECKBOX( IDC_CHECKDISKSPACE,	PrefsUnifiedDlg::OnCheckBoxChange )
	EVT_CHECKBOX( IDC_USESKIN,			PrefsUnifiedDlg::OnCheckBoxChange )
	EVT_CHECKBOX( IDC_ONLINESIG,		PrefsUnifiedDlg::OnCheckBoxChange )
	EVT_CHECKBOX( IDC_REMOVEDEAD,		PrefsUnifiedDlg::OnCheckBoxChange )
	EVT_CHECKBOX( IDC_ENABLE_AUTO_HQRS,	PrefsUnifiedDlg::OnCheckBoxChange )
	
	EVT_BUTTON(ID_PREFS_OK_TOP, PrefsUnifiedDlg::OnOk)
	EVT_BUTTON(ID_OK, PrefsUnifiedDlg::OnOk)

	EVT_BUTTON(ID_PREFS_CANCEL_TOP, PrefsUnifiedDlg::OnCancel)
	EVT_BUTTON(ID_CANCEL, PrefsUnifiedDlg::OnCancel)

	// Browse buttons
	EVT_BUTTON(IDC_SELSKINFILE,  PrefsUnifiedDlg::OnButtonBrowseSkin)
	EVT_BUTTON(IDC_BTN_BROWSE_WAV, PrefsUnifiedDlg::OnButtonBrowseWav)
	EVT_BUTTON(IDC_BROWSEV, PrefsUnifiedDlg::OnButtonBrowseVideoplayer)
	EVT_BUTTON(IDC_SELTEMPDIR, PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELINCDIR,  PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELOSDIR,  PrefsUnifiedDlg::OnButtonDir)

	EVT_SPINCTRL( IDC_TOOLTIPDELAY, PrefsUnifiedDlg::OnToolTipDelayChange)

	EVT_BUTTON(IDC_EDITADR, PrefsUnifiedDlg::OnButtonEditAddr)
	EVT_BUTTON(ID_DESKTOPMODE, PrefsUnifiedDlg::OnButtonSystray)
	EVT_BUTTON(IDC_IPFRELOAD, PrefsUnifiedDlg::OnButtonIPFilterReload)
	EVT_BUTTON(IDC_COLOR_BUTTON, PrefsUnifiedDlg::OnButtonColorChange)
	EVT_CHOICE(IDC_COLORSELECTOR, PrefsUnifiedDlg::OnColorCategorySelected)
	EVT_CHOICE(IDC_FCHECK, PrefsUnifiedDlg::OnFakeBrowserChange)
	EVT_LIST_ITEM_SELECTED(ID_PREFSLISTCTRL, PrefsUnifiedDlg::OnPrefsPageChange)

	EVT_INIT_DIALOG(PrefsUnifiedDlg::OnInitDialog)

	EVT_COMMAND_SCROLL(IDC_SLIDER,			PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_SLIDER3,			PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_SLIDER4,			PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_SLIDER2,			PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_FILEBUFFERSIZE,	PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_QUEUESIZE,		PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_SERVERKEEPALIVE,	PrefsUnifiedDlg::OnScrollBarChange)
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
	//! The actual widget, to be set later.
	wxPanel*	m_widget;
};


PrefsPage pages[] =
    {
        { _("General"),				PreferencesGeneralTab,			13,	NULL },
        { _("Connection"),			PreferencesConnectionTab,		14,	NULL },
        { _("Remote Controls"),		PreferencesRemoteControlsTab,	11,	NULL },
        { _("Online Signature"),	PreferencesOnlineSigTab,	 	21,	NULL },
        { _("Server"),				PreferencesServerTab,			15,	NULL },
        { _("Files"),				PreferencesFilesTab,			16,	NULL },
        { _("Sources Dropping"),	PreferencesSourcesDroppingTab,	20,	NULL },
        { _("Directories"),			PreferencesDirectoriesTab,		17,	NULL },
        { _("Statistics"),			PreferencesStatisticsTab,		10,	NULL },
        { _("Security"),			PreferencesSecurityTab,		 	22,	NULL },
        //	Notications are disabled since they havent been implemented
        //	{ _("Notifications"),	PreferencesNotifyTab,			18,	NULL },
        { _("Gui Tweaks"),			PreferencesGuiTweaksTab,		19,	NULL },
        { _("Core Tweaks"),			PreferencesaMuleTweaksTab,		12,	NULL }
    };




PrefsUnifiedDlg* PrefsUnifiedDlg::NewPrefsDialog(wxWindow* parent)
{
	// Do not allow multiple dialogs
	if ( s_ID )
		return NULL;

	return new PrefsUnifiedDlg( parent );
}


PrefsUnifiedDlg::PrefsUnifiedDlg(wxWindow* parent)
		: wxDialog(parent, -1, _("Preferences"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	s_ID = GetId();

	preferencesDlgTop( this, FALSE );
	wxListCtrl* PrefsIcons = (wxListCtrl*) FindWindowById(ID_PREFSLISTCTRL,this);

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
		PrefsIcons->InsertItem(i, pages[i].m_title, i);
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

		// Align and resize the page
		Fit();
		Layout();

		// Find the greatest sizes
		wxSize size = prefs_sizer->GetSize();
		if ( size.GetWidth() > width )
			width = size.GetWidth();

		if ( size.GetHeight() > height )
			height = size.GetHeight();

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
	m_ShareSelector = ((CDirectoryTreeCtrl*)FindWindowById(IDC_SHARESELECTOR, this));
	m_buttonColor = (wxButton*)FindWindowById(IDC_COLOR_BUTTON, this);
	m_choiceColor = (wxChoice*)FindWindowById(IDC_COLORSELECTOR, this);

	// Connect the Cfgs with their widgets
	CPreferences::CFGMap::iterator it = CPreferences::s_CfgList.begin();
	for ( ; it != CPreferences::s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->ConnectToWidget( it->first, this ) ) {
			printf("Failed to connect Cfg to widget with the ID %d and key %s\n", it->first, unicode2char(it->second->GetKey()));
		}
	}


	Fit();

	// Place the window centrally
	CentreOnScreen();

	// It must not be resized to something smaller than what it currently is
	wxSize size = GetClientSize();

	SetSizeHints( size.GetWidth(), size.GetHeight() );
}


PrefsUnifiedDlg::~PrefsUnifiedDlg()
{
	// Un-Connect the Cfgs
	CPreferences::CFGMap::iterator it = CPreferences::s_CfgList.begin();
	for ( ; it != CPreferences::s_CfgList.end(); ++it ) {
		// Checking for failures
		it->second->ConnectToWidget( 0 );
	}
}


Cfg_Base* PrefsUnifiedDlg::GetCfg(int id)
{
	CPreferences::CFGMap::iterator it = CPreferences::s_CfgList.find( id );

	if ( it != CPreferences::s_CfgList.end() )
		return it->second;

	return NULL;
}


bool PrefsUnifiedDlg::TransferToWindow()
{
	// Connect the Cfgs with their widgets
	CPreferences::CFGMap::iterator it = CPreferences::s_CfgList.begin();
	for ( ; it != CPreferences::s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->TransferToWindow() ) {
			printf("Failed to transfer data from Cfg to Widget with the ID %d and key %s\n", it->first, unicode2char(it->second->GetKey()));
		}
	}

	m_ShareSelector->SetSharedDirectories(&theApp.glob_prefs->shareddir_list);


	for ( int i = 0; i < cntStatColors; i++ ) {
		CPreferences::s_colors[i] = CStatisticsDlg::acrStat[i];
		CPreferences::s_colors_ref[i] = CStatisticsDlg::acrStat[i];
	}


	// Enable/Disable some controls
	FindWindow( IDC_FCHECKSELF )->Enable( ((wxChoice*)FindWindow( IDC_FCHECK ))->GetSelection() == 8 );
	FindWindow( IDC_MINDISKSPACE )->Enable( CPreferences::IsCheckDiskspaceEnabled() );
	FindWindow( IDC_SKINFILE )->Enable( CPreferences::UseSkin() );
	FindWindow( IDC_OSDIR )->Enable( CPreferences::IsOnlineSignatureEnabled() );
#warning UDPDisable isnt implemented!
	FindWindow( IDC_UDPPORT )->Enable( !CPreferences::s_UDPDisable );
	FindWindow( IDC_SERVERRETRIES )->Enable( CPreferences::DeadServer() );
	FindWindow( IDC_HQR_VALUE )->Enable( CPreferences::DropHighQueueRankingSources() );


	// Set the permissions scrollers for files
	int perms = CPreferences::GetFilePermissions();
	((wxSpinCtrl*)FindWindow(IDC_SPIN_PERM_FU))->SetValue( perms / 0100 );
	((wxSpinCtrl*)FindWindow(IDC_SPIN_PERM_FG))->SetValue( perms % 0100 / 010 );
	((wxSpinCtrl*)FindWindow(IDC_SPIN_PERM_FO))->SetValue( perms % 0100 % 010 / 01 );
	
	// Set the permissions scrollers for directories
	perms = CPreferences::GetDirPermissions();
	((wxSpinCtrl*)FindWindow(IDC_SPIN_PERM_DU))->SetValue( perms / 0100 );
	((wxSpinCtrl*)FindWindow(IDC_SPIN_PERM_DG))->SetValue( perms % 0100 / 010 );
	((wxSpinCtrl*)FindWindow(IDC_SPIN_PERM_DO))->SetValue( perms % 0100 % 010 / 01 );

	return true;
}


bool PrefsUnifiedDlg::TransferFromWindow()
{
	// Connect the Cfgs with their widgets
	CPreferences::CFGMap::iterator it = CPreferences::s_CfgList.begin();
	for ( ; it != CPreferences::s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->TransferFromWindow() ) {
			printf("Failed to transfer data from Widget to Cfg with the ID %d and key %s\n", it->first, unicode2char(it->second->GetKey()));
		}
	}

	theApp.glob_prefs->shareddir_list.Clear();
	m_ShareSelector->GetSharedDirectories(&theApp.glob_prefs->shareddir_list);

	for ( int i = 0; i < cntStatColors; i++ ) {
		if ( CPreferences::s_colors[i] != CPreferences::s_colors_ref[i] ) {
			CStatisticsDlg::acrStat[i] = CPreferences::s_colors[i];
			theApp.amuledlg->statisticswnd->ApplyStatsColor(i);
		}

	}


	// Set the file-permissions value
	int file_perms = 0;
	file_perms |= 0100 * ((wxSpinCtrl*)FindWindow(IDC_SPIN_PERM_FU))->GetValue();
	file_perms |= 0010 * ((wxSpinCtrl*)FindWindow(IDC_SPIN_PERM_FG))->GetValue();
	file_perms |= 0001 * ((wxSpinCtrl*)FindWindow(IDC_SPIN_PERM_FO))->GetValue();
	CPreferences::SetFilePermissions( file_perms );

	int dir_perms = 0;
	dir_perms |= 0100 * ((wxSpinCtrl*)FindWindow(IDC_SPIN_PERM_FU))->GetValue();
	dir_perms |= 0010 * ((wxSpinCtrl*)FindWindow(IDC_SPIN_PERM_FG))->GetValue();
	dir_perms |= 0001 * ((wxSpinCtrl*)FindWindow(IDC_SPIN_PERM_FO))->GetValue();
	CPreferences::SetDirPermissions( dir_perms );


	return true;
}


bool PrefsUnifiedDlg::CfgChanged(int ID)
{
	Cfg_Base* cfg = GetCfg(ID);

	if ( cfg )
		return cfg->HasChanged();

	return false;
}


void PrefsUnifiedDlg::OnOk(wxCommandEvent& WXUNUSED(event))
{
	TransferFromWindow();

	// do sanity checking, special processing, and user notifications here
	theApp.glob_prefs->CheckUlDlRatio();

	// save the preferences on ok
	theApp.glob_prefs->Save();


	if ( CfgChanged(IDC_FED2KLH) && theApp.amuledlg->GetActiveDialog() != CamuleDlg::SearchWnd )
		theApp.amuledlg->ShowED2KLinksHandler( theApp.glob_prefs->GetFED2KLH() );

	if ( CfgChanged(IDC_LANGUAGE) )
		wxMessageBox(wxString::wxString(_("Language change will not be applied until aMule is restarted.")));


	if ( CfgChanged(IDC_INCFILES) || CfgChanged(IDC_TEMPFILES) || m_ShareSelector->HasChanged )
		theApp.sharedfiles->Reload(true, false);


	if ( CfgChanged(IDC_OSDIR) ) {
		wxTextCtrl* widget = (wxTextCtrl*)FindWindow( IDC_OSDIR );

		// Build the filenames for the two OS files
		theApp.SetOSFiles( widget->GetValue() );
	}

	if (theApp.glob_prefs->GetIPFilterOn()) {
		theApp.clientlist->FilterQueues();
	}

	if (theApp.glob_prefs->GetShowRatesOnTitle()) {
		// This avoids a 5 seconds delay to show the title
		theApp.amuledlg->SetTitle(theApp.m_FrameTitle + wxT(" -- Up: 0.0 | Down: 0.0"));
	} else {
		// This resets the title
		theApp.amuledlg->SetTitle(theApp.m_FrameTitle);
	}

	// Final actions:
	// Reset the ID so that a new dialog can be created
	s_ID = 0;

	// Hide the dialog since Destroy isn't instant
	Show( false );

	// Destory the dialog
	Destroy();
}


void PrefsUnifiedDlg::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	// Final actions:
	// Reset the ID so that a new dialog can be created
	s_ID = 0;

	// Hide the dialog since Destroy isn't instant
	Show( false );

	// Destory the dialog
	Destroy();
}


void PrefsUnifiedDlg::OnCheckBoxChange(wxCommandEvent& event)
{
	bool		value = event.IsChecked();
	wxWindow*	widget = NULL;

	switch ( event.GetId() ) {
		case IDC_UDPDISABLE:
			// UDP is disable rather than enable, so we flip the value
			value = !value;
			widget = FindWindow( IDC_UDPPORT ); 
			break;
			
		case IDC_CHECKDISKSPACE:
			widget = FindWindow( IDC_MINDISKSPACE );
			break;	
		
		case IDC_USESKIN:
			widget = FindWindow( IDC_SKINFILE );
			break;

		case IDC_ONLINESIG:
			widget = FindWindow( IDC_OSDIR );
			break;

		case IDC_REMOVEDEAD:
			widget = FindWindow( IDC_SERVERRETRIES );
			break;

		case IDC_ENABLE_AUTO_HQRS:
			widget = FindWindow( IDC_HQR_VALUE );
			break;
	}

	if ( widget )
		widget->Enable( value );
}


void PrefsUnifiedDlg::OnButtonColorChange(wxCommandEvent& WXUNUSED(event))
{
	int index = m_choiceColor->GetSelection();
	wxColour col = WxColourFromCr( CPreferences::s_colors[index] );
	col = wxGetColourFromUser( this, col );
	if ( col.Ok() ) {
		m_buttonColor->SetBackgroundColour( col );
		CPreferences::s_colors[index] = CrFromWxColour(col);
	}
}


void PrefsUnifiedDlg::OnColorCategorySelected(wxCommandEvent& WXUNUSED(evt))
{
	m_buttonColor->SetBackgroundColour( WxColourFromCr( CPreferences::s_colors[ m_choiceColor->GetSelection() ] ) );
}


void PrefsUnifiedDlg::OnFakeBrowserChange( wxCommandEvent& evt )
{
	wxTextCtrl* widget = (wxTextCtrl*)FindWindow( IDC_FCHECKSELF );

	if ( widget )
		widget->Enable( evt.GetSelection() == 8 );
}


void PrefsUnifiedDlg::OnButtonSystray(wxCommandEvent& WXUNUSED(evt))
{
	theApp.amuledlg->changeDesktopMode();

	// Ensure that the dialog is still visible afterwards
	Raise();
	SetFocus();
}


void PrefsUnifiedDlg::OnButtonDir(wxCommandEvent& event)
{
	wxString type = _("Choose a folder for ");

	int id = 0;
	switch ( event.GetId() ) {
	case IDC_SELTEMPDIR:
		id = IDC_TEMPFILES;
		type += _("Temporary files");
		break;

	case IDC_SELINCDIR:
		id = IDC_INCFILES;
		type += _("Incomming files");
		break;

	case IDC_SELOSDIR:
		id = IDC_OSDIR;
		type += _("Online Signatures");
		break;

	default:
		wxASSERT( false );
		return;
	}

	wxTextCtrl* widget	= (wxTextCtrl*)FindWindow( id );
	wxString dir		= widget->GetValue();

	wxString str = wxDirSelector( type, dir );

	if ( !str.IsEmpty() ) {
		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonBrowseWav(wxCommandEvent& WXUNUSED(evt))
{
	wxString str = wxFileSelector( _("Browse wav"), wxEmptyString, wxEmptyString,
		wxT("*.wav"), _("File wav (*.wav)|*.wav||") );
	
	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = (wxTextCtrl*)FindWindow( IDC_EDIT_TBN_WAVFILE );

		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonBrowseSkin(wxCommandEvent& WXUNUSED(evt))
{
	wxString str = wxFileSelector( _("Browse skin file"), wxEmptyString, wxEmptyString, wxT("*") );

	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = (wxTextCtrl*)FindWindow( IDC_SKINFILE );

		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonBrowseVideoplayer(wxCommandEvent& WXUNUSED(e))
{
	wxString str = wxFileSelector( _("Browse for videoplayer"), wxT(""), wxT(""),
	                               wxT(""), _("Executable (*)|*||") );

	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = (wxTextCtrl*)FindWindow( IDC_VIDEOPLAYER );

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
		label.Printf( _("Update delay: %d secs"), event.GetPosition() );
		break;

	case IDC_SLIDER3:
		id = IDC_SLIDERINFO3;
		label.Printf( _("Time for average graph: %d mins"), event.GetPosition() );
		break;

	case IDC_SLIDER4:
		id = IDC_SLIDERINFO4;
		label.Printf( _("Connections Graph Scale: %d"), event.GetPosition() );
		break;

	case IDC_SLIDER2:
		id = IDC_SLIDERINFO2;
		label.Printf( _("Update delay : %d secs"), event.GetPosition() );
		break;

	case IDC_FILEBUFFERSIZE:
		id = IDC_FILEBUFFERSIZE_STATIC;
		label.Printf( _("File Buffer Size: %d bytes"), event.GetPosition() * 15000 );
		break;

	case IDC_QUEUESIZE:
		id = IDC_QUEUESIZE_STATIC;
		label.Printf( _("Upload Queue Size: %d clients"), event.GetPosition() * 100 );
		break;

	case IDC_SERVERKEEPALIVE:
		id = IDC_SERVERKEEPALIVE_LABEL;

		if ( event.GetPosition() )
			label.Printf( _("Server connection refresh interval: %d minutes"), event.GetPosition() );
		else
			label.Printf( _("Server connection refresh interval: Disabled") );

		break;

	default:
		return;
	}

	wxStaticText* widget = (wxStaticText*)FindWindow( id );

	if ( widget )
		widget->SetLabel( label );
}

