//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         wxCasPrefs Class
///
/// Purpose:      Display user preferences dialog and manage configuration storage system
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
///
/// Pixmats from aMule http://www.amule.org
///
/// This program is free software; you can redistribute it and/or modify
///  it under the terms of the GNU General Public License as published by
/// the Free Software Foundation; either version 2 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the
/// Free Software Foundation, Inc.,
/// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h"

#ifdef __BORLANDC__
 #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
 #include "wx/wx.h"
#endif

#include <wx/config.h>

#include "wxcasprefs.h"
#include "wxcascte.h"
#include "wxcasframe.h"

// Constructor
WxCasPrefs::WxCasPrefs ( wxWindow * parent ) : wxDialog ( parent, -1,
		        wxString ( _
		                   ( "Preferences" ) ) )
{
	// Prefs
	wxConfigBase * prefs = wxConfigBase::Get();

	// Main vertical Sizer
	m_mainVBox = new wxBoxSizer ( wxVERTICAL );

	// OS Path
	m_osPathSBox =
	    new wxStaticBox ( this, -1, _( "Directory containing amulesig.dat file" ) );
	m_osPathSBoxSizer = new wxStaticBoxSizer ( m_osPathSBox, wxHORIZONTAL );

	m_osPathTextCtrl = new wxTextCtrl ( this, -1, wxEmptyString );
	m_osPathBrowseButton =
	    new wxButton ( this, ID_OSPATH_BROWSE_BUTTON, wxString ( _( "Browse" ) ) );

	wxString str;

	prefs->Read ( WxCasCte::AMULESIG_PATH_KEY, &str,
	              WxCasCte::DEFAULT_AMULESIG_PATH );

	// Text extent
	wxInt32 charExtent, y;
	m_osPathTextCtrl->GetTextExtent ( wxT( "8" ), &charExtent, &y );
	m_osPathTextCtrl->SetSize ( wxSize ( charExtent * ( str.Length () + 1 ), -1 ) );

	m_osPathTextCtrl->SetValue ( str );
	m_osPathTextCtrl->
	SetToolTip ( _
	             ( "Enter here the directory where your amulesig.dat file is" ) );

	m_osPathSBoxSizer->Add ( m_osPathTextCtrl, 1, wxALL | wxALIGN_CENTER, 5 );
	m_osPathSBoxSizer->Add ( m_osPathBrowseButton, 0, wxALL | wxALIGN_CENTER, 5 );

	m_mainVBox->Add ( m_osPathSBoxSizer, 0, wxGROW | wxALIGN_CENTER | wxALL, 10 );

	// Refresh rate
	m_refreshSBox = new wxStaticBox ( this, -1, wxEmptyString );
	m_refreshSBoxSizer = new wxStaticBoxSizer ( m_refreshSBox, wxHORIZONTAL );


	m_refreshSpinButton = new wxSpinCtrl ( this, -1 );
	m_refreshSpinButton->SetRange ( WxCasCte::MIN_REFRESH_RATE,
	                                WxCasCte::MAX_REFRESH_RATE );
	m_refreshSpinButton->SetValue ( prefs->
	                                Read ( WxCasCte::REFRESH_RATE_KEY,
	                                       WxCasCte::DEFAULT_REFRESH_RATE ) );
	m_refreshStaticText =
	    new wxStaticText ( this, -1, _( "Refresh rate interval in seconds" ), wxDefaultPosition,
	                       wxDefaultSize, wxALIGN_CENTRE );
	m_refreshSBoxSizer->Add ( m_refreshSpinButton, 0, wxALL | wxALIGN_CENTER, 5 );
	m_refreshSBoxSizer->Add ( m_refreshStaticText, 1, wxALL | wxALIGN_CENTER, 5 );

	m_mainVBox->Add ( m_refreshSBoxSizer, 0, wxGROW | wxALIGN_CENTER | wxALL,
	                  10 );

	// Auto generate stat image
	m_autoStatImgSBox = new wxStaticBox ( this, -1, wxEmptyString );
	m_autoStatImgSBoxSizer =
	    new wxStaticBoxSizer ( m_autoStatImgSBox, wxVERTICAL );

	m_autoStatImgCheck =
	    new wxCheckBox ( this, ID_AUTOSTATIMG_CHECK,
	                     _
	                     ( "Generate a stat image at every refresh event" ) );
	m_autoStatImgSBoxSizer->Add ( m_autoStatImgCheck, 0,
	                              wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5 );

	m_autoStatImgHBoxSizer = new wxBoxSizer ( wxHORIZONTAL );

	wxString strs[] =
	    {
	        wxT ( "PNG" ), wxT ( "JPG" ), wxT ( "BMP" ) };

	m_autoStatImgCombo =
	    new wxComboBox ( this, ID_AUTOSTATIMG_COMBO,
	                     prefs->
	                     Read ( WxCasCte::AUTOSTATIMG_TYPE_KEY,
	                            WxCasCte::DEFAULT_AUTOSTATIMG_TYPE ),
	                     wxDefaultPosition, wxDefaultSize, 3, strs,
	                     wxCB_DROPDOWN | wxCB_READONLY );

	m_autoStatImgTextCtrl = new wxTextCtrl ( this, -1, wxEmptyString );
	m_autoStatImgTextCtrl->SetValue ( prefs->
	                                  Read ( WxCasCte::AUTOSTATIMG_DIR_KEY,
	                                         WxCasCte::DEFAULT_AUTOSTATIMG_PATH ) );
	m_autoStatImgTextCtrl->
	SetToolTip ( _
	             ( "Enter here the directory where you want to generate the statistic image" ) );

	m_autoStatImgButton =
	    new wxButton ( this, ID_AUTOSTATIMG_BROWSE_BUTTON, wxString ( _( "Browse" ) ) );

	m_autoStatImgHBoxSizer->Add ( m_autoStatImgCombo, 0, wxALIGN_CENTER | wxALL,
	                              5 );
	m_autoStatImgHBoxSizer->Add ( m_autoStatImgTextCtrl, 1,
	                              wxALIGN_CENTER | wxALL, 5 );
	m_autoStatImgHBoxSizer->Add ( m_autoStatImgButton, 0, wxALIGN_CENTER | wxALL,
	                              5 );

	m_autoStatImgSBoxSizer->Add ( m_autoStatImgHBoxSizer, 0,
	                              wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5 );

	m_mainVBox->Add ( m_autoStatImgSBoxSizer, 0, wxGROW | wxALIGN_CENTER | wxALL,
	                  5 );

	// Auto FTP update stat image
	m_ftpUpdateSBox = new wxStaticBox ( this, -1, wxEmptyString );
	m_ftpUpdateSBoxSizer =
	    new wxStaticBoxSizer ( m_ftpUpdateSBox, wxVERTICAL );

	// Check
	m_ftpUpdateCheck =
	    new wxCheckBox ( this, ID_FTP_UPDATE_CHECK,
	                     _
	                     ( "Upload periodicaly your stat image to FTP server" ) );
	m_ftpUpdateSBoxSizer->Add ( m_ftpUpdateCheck, 0,
	                            wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5 );

	// Grid size
	m_ftpUpdateGridSizer = new wxGridSizer( 2 );

	// FTP Static text
	m_ftpUrlStaticText = new wxStaticText ( this, -1, _( "FTP Url" ) );
	m_ftpUpdateGridSizer->Add ( m_ftpUrlStaticText, 1,
	                            wxALIGN_LEFT | wxALIGN_BOTTOM | wxALL, 5 );

	m_ftpPathStaticText = new wxStaticText ( this, -1, _( "FTP Path" ) );
	m_ftpUpdateGridSizer->Add ( m_ftpPathStaticText, 1,
	                            wxALIGN_LEFT | wxALIGN_BOTTOM | wxALL, 5 );
	// Url
	m_ftpUrlTextCtrl = new wxTextCtrl ( this, -1, wxEmptyString );
	m_ftpUrlTextCtrl->SetValue ( prefs->
	                             Read ( WxCasCte::FTP_URL_KEY,
	                                    WxCasCte::DEFAULT_FTP_URL ) );
	m_ftpUrlTextCtrl->
	SetToolTip ( _
	             ( "Enter here the URL of your FTP server" ) );

	m_ftpUpdateGridSizer->Add ( m_ftpUrlTextCtrl, 1,
	                            wxGROW | wxALIGN_LEFT | wxALIGN_TOP | wxALL, 5 );

	// Path
	m_ftpPathTextCtrl = new wxTextCtrl ( this, -1, wxEmptyString );
	m_ftpPathTextCtrl->SetValue ( prefs->
	                              Read ( WxCasCte::FTP_PATH_KEY,
	                                     WxCasCte::DEFAULT_FTP_PATH ) );
	m_ftpPathTextCtrl->
	SetToolTip ( _
	             ( "Enter here the directory where putting your stat image on FTP server" ) );

	m_ftpUpdateGridSizer->Add ( m_ftpPathTextCtrl, 1,
	                            wxGROW | wxALIGN_LEFT | wxALIGN_TOP | wxALL, 5 );

	// Login Static text
	m_ftpUserStaticText = new wxStaticText ( this, -1, _( "User" ) );
	m_ftpUpdateGridSizer->Add ( m_ftpUserStaticText, 1,
	                            wxALIGN_LEFT | wxALIGN_BOTTOM | wxALL, 5 );

	m_ftpPasswdStaticText = new wxStaticText ( this, -1, _( "Password" ) );
	m_ftpUpdateGridSizer->Add ( m_ftpPasswdStaticText, 1,
	                            wxALIGN_LEFT | wxALIGN_BOTTOM | wxALL, 5 );

	// User
	m_ftpUserTextCtrl = new wxTextCtrl ( this, -1, wxEmptyString );
	m_ftpUserTextCtrl->SetValue ( prefs->
	                              Read ( WxCasCte::FTP_USER_KEY,
	                                     WxCasCte::DEFAULT_FTP_USER ) );
	m_ftpUserTextCtrl->
	SetToolTip ( _
	             ( "Enter here the User name to log into your FTP server" ) );

	m_ftpUpdateGridSizer->Add ( m_ftpUserTextCtrl, 1,
	                            wxGROW | wxALIGN_LEFT | wxALIGN_TOP | wxALL, 5 );

	// Passwd
	m_ftpPasswdTextCtrl = new wxTextCtrl ( this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
	m_ftpPasswdTextCtrl->SetValue ( prefs->
	                                Read ( WxCasCte::FTP_PASSWD_KEY,
	                                       WxCasCte::DEFAULT_FTP_PASSWD ) );
	m_ftpPasswdTextCtrl->
	SetToolTip ( _
	             ( "Enter here the User password to log into your FTP server" ) );

	m_ftpUpdateGridSizer->Add ( m_ftpPasswdTextCtrl, 1,
	                            wxGROW | wxALIGN_LEFT | wxALIGN_TOP | wxALL, 5 );


	// Add to static sizer
	m_ftpUpdateSBoxSizer->Add ( m_ftpUpdateGridSizer, 1,
	                            wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5 );

	// Upload rate
	m_ftpRateHBoxSizer = new wxBoxSizer( wxHORIZONTAL );
	m_ftpUpdateSpinButton = new wxSpinCtrl ( this, -1 );
	m_ftpUpdateSpinButton->SetRange ( WxCasCte::MIN_FTP_RATE,
	                                  WxCasCte::MAX_FTP_RATE );
	m_ftpUpdateSpinButton->SetValue ( prefs->
	                                  Read ( WxCasCte::FTP_UPDATE_RATE_KEY,
	                                         WxCasCte::DEFAULT_FTP_UPDATE_RATE ) );
	m_ftpUpdateStaticText =
	    new wxStaticText ( this, -1, _( "FTP update rate interval in minutes" ), wxDefaultPosition,
	                       wxDefaultSize, wxALIGN_CENTRE );
	m_ftpRateHBoxSizer->Add ( m_ftpUpdateSpinButton, 0, wxALL | wxALIGN_CENTER, 5 );
	m_ftpRateHBoxSizer->Add ( m_ftpUpdateStaticText, 1, wxALL | wxALIGN_CENTER, 5 );

	m_ftpUpdateSBoxSizer->Add ( m_ftpRateHBoxSizer, 0,
	                            wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5 );

	// Add to main sizer
	m_mainVBox->Add ( m_ftpUpdateSBoxSizer, 0, wxGROW | wxALIGN_CENTER | wxALL,
	                  5 );

	// Mask auto stat img disabled controls
	if ( ( bool )
	        ( prefs->
	          Read ( WxCasCte::ENABLE_AUTOSTATIMG_KEY,
	                 WxCasCte::DEFAULT_AUTOSTATIMG_ISENABLED ) ) ) {
		m_autoStatImgCheck->SetValue( TRUE );
	} else {
		m_autoStatImgCheck->SetValue ( FALSE );
		EnableAutoStatImgCtrls( FALSE );

		m_ftpUpdateCheck->Enable ( FALSE );
		EnableFtpUpdateCtrls( FALSE );
	}

	// Mask Ftp update disabled controls
	if ( ( bool )
	        ( prefs->
	          Read ( WxCasCte::ENABLE_FTP_UPDATE_KEY,
	                 WxCasCte::DEFAULT_FTP_UPDATE_ISENABLED ) ) ) {
		m_ftpUpdateCheck->SetValue( TRUE );
	} else {
		m_ftpUpdateCheck->SetValue ( FALSE );
		EnableFtpUpdateCtrls( FALSE );
	}

	// Separator line
	m_staticLine = new wxStaticLine ( this, -1 );
	m_mainVBox->Add ( m_staticLine, 0, wxGROW | wxALIGN_CENTER | wxALL );

	// Button bar
	m_buttonHBox = new wxBoxSizer ( wxHORIZONTAL );
	m_validateButton =
	    new wxButton ( this, ID_VALIDATE_BUTTON, wxString ( _( "Validate" ) ) );
	m_cancelButton =
	    new wxButton ( this, wxID_CANCEL, wxString ( _( "Cancel" ) ) );

	m_buttonHBox->Add ( m_validateButton, 0, wxALIGN_CENTER | wxALL, 5 );
	m_buttonHBox->Add ( m_cancelButton, 0, wxALIGN_CENTER | wxALL, 5 );

	m_mainVBox->Add ( m_buttonHBox, 0, wxALIGN_CENTER | wxALL, 10 );

	// Layout
	SetAutoLayout ( TRUE );
	SetSizerAndFit ( m_mainVBox );

	m_validateButton->SetFocus ();
	m_validateButton->SetDefault ();
}

// Destructor
WxCasPrefs::~WxCasPrefs ()
{}

// Events table
BEGIN_EVENT_TABLE ( WxCasPrefs, wxDialog )
EVT_BUTTON ( ID_OSPATH_BROWSE_BUTTON, WxCasPrefs::OnOSPathBrowseButton )
EVT_BUTTON ( ID_AUTOSTATIMG_BROWSE_BUTTON, WxCasPrefs::OnAutoStatImgBrowseButton )
EVT_BUTTON ( ID_VALIDATE_BUTTON, WxCasPrefs::OnValidateButton )
EVT_CHECKBOX ( ID_AUTOSTATIMG_CHECK, WxCasPrefs::OnAutoStatImgCheck )
EVT_CHECKBOX ( ID_FTP_UPDATE_CHECK, WxCasPrefs::OnFtpUpdateCheck )
END_EVENT_TABLE ()

// Browse for OS Path
void WxCasPrefs::OnOSPathBrowseButton ( wxCommandEvent& WXUNUSED( event ) )
{
	const wxString &dir = wxDirSelector(_
		("Folder containing your signature file"),
	        WxCasCte::DEFAULT_AMULESIG_PATH,
		wxDD_DEFAULT_STYLE,
		wxDefaultPosition, this);
	if ( !dir.empty () ) {
		m_osPathTextCtrl->SetValue ( dir );
	}
}

// Browse for stat image Path
void
WxCasPrefs::OnAutoStatImgBrowseButton ( wxCommandEvent& WXUNUSED( event ) )
{
	const wxString & dir = wxDirSelector(
		_("Folder where generating the statistic image"),
	        WxCasCte::DEFAULT_AUTOSTATIMG_PATH,
		wxDD_DEFAULT_STYLE,
		wxDefaultPosition, this);

	if ( !dir.empty () ) {
		m_autoStatImgTextCtrl->SetValue ( dir );
	}
}

// Auto Generate Stat Image Check Button
void
WxCasPrefs::OnAutoStatImgCheck ( wxCommandEvent& WXUNUSED( event ) )
{
	if ( m_autoStatImgCheck->GetValue () ) {
		EnableAutoStatImgCtrls( TRUE );

		m_ftpUpdateCheck->Enable ( TRUE );
		if ( m_ftpUpdateCheck->GetValue () ) {
			EnableFtpUpdateCtrls( TRUE );
		}
	} else {
		EnableAutoStatImgCtrls( FALSE );

		m_ftpUpdateCheck->Enable ( FALSE );
		EnableFtpUpdateCtrls( FALSE );
	}
}

// Ftp update Check Button
void
WxCasPrefs::OnFtpUpdateCheck ( wxCommandEvent& WXUNUSED( event ) )
{
	if ( m_ftpUpdateCheck->GetValue () ) {
		EnableFtpUpdateCtrls( TRUE );
	} else {
		EnableFtpUpdateCtrls( FALSE );
	}
}
// Validate Prefs
void
WxCasPrefs::OnValidateButton ( wxCommandEvent& WXUNUSED( event ) )
{
	// Prefs
	wxConfigBase * prefs = wxConfigBase::Get();

	// Write amulesig dir
	if ( prefs->Read ( WxCasCte::AMULESIG_PATH_KEY,
	                   WxCasCte::DEFAULT_AMULESIG_PATH ) !=
	        m_osPathTextCtrl->GetValue () ) {
		// Reload amulesig.dat
		wxFileName amulesig( m_osPathTextCtrl->GetValue (),
		                     WxCasCte::AMULESIG_FILENAME );
		( ( WxCasFrame* ) GetParent() ) ->SetAmuleSigFile( amulesig );

		prefs->Write ( WxCasCte::AMULESIG_PATH_KEY,
		               m_osPathTextCtrl->GetValue () );
	}

	// Restart timer if refresh interval has changed
	if ( prefs->Read ( WxCasCte::REFRESH_RATE_KEY, WxCasCte::DEFAULT_REFRESH_RATE ) !=
	        m_refreshSpinButton->GetValue () ) {
		( ( WxCasFrame* ) GetParent() ) ->ChangeRefreshPeriod( 1000 * m_refreshSpinButton->GetValue () );

		// Write refresh interval
		prefs->Write ( WxCasCte::REFRESH_RATE_KEY,
		               m_refreshSpinButton->GetValue () );
	}

	// Write auto stat img state
	prefs->Write ( WxCasCte::ENABLE_AUTOSTATIMG_KEY,
	               m_autoStatImgCheck->GetValue () );

	// If auto stat img is enabled
	if ( m_autoStatImgCheck->GetValue () ) {
		prefs->Write ( WxCasCte::AUTOSTATIMG_DIR_KEY,
		               m_autoStatImgTextCtrl->GetValue () );

		prefs->Write ( WxCasCte::AUTOSTATIMG_TYPE_KEY,
		               m_autoStatImgCombo->GetValue () );

		// Write Ftp update state
		prefs->Write ( WxCasCte::ENABLE_FTP_UPDATE_KEY,
		               m_ftpUpdateCheck->GetValue () );

		// If Ftp update is enabled
		if ( m_ftpUpdateCheck->GetValue () ) {
			// Restart timer if update interval has changed
			if ( prefs->Read ( WxCasCte::FTP_UPDATE_RATE_KEY, WxCasCte::DEFAULT_FTP_UPDATE_RATE ) !=
			        m_ftpUpdateSpinButton->GetValue () ) {
				( ( WxCasFrame* ) GetParent() ) ->ChangeFtpUpdatePeriod( 60000 * m_refreshSpinButton->GetValue () );
				prefs->Write ( WxCasCte::FTP_UPDATE_RATE_KEY,
				               m_ftpUpdateSpinButton->GetValue () );
			}
			// Write Ftp parameters
			prefs->Write ( WxCasCte::FTP_URL_KEY,
			               m_ftpUrlTextCtrl->GetValue () );

			prefs->Write ( WxCasCte::FTP_PATH_KEY,
			               m_ftpPathTextCtrl->GetValue () );

			prefs->Write ( WxCasCte::FTP_USER_KEY,
			               m_ftpUserTextCtrl->GetValue () );

			prefs->Write ( WxCasCte::FTP_PASSWD_KEY,
			               m_ftpPasswdTextCtrl->GetValue () );
		}
	}

	// Force config writing
	prefs->Flush();

	// Close window
	this->EndModal ( this->GetReturnCode () );
}

// Enable/Disable auto img ctrls
void
WxCasPrefs::EnableAutoStatImgCtrls( bool state )
{
	m_autoStatImgTextCtrl->Enable ( state );
	m_autoStatImgButton->Enable ( state );
	m_autoStatImgCombo->Enable ( state );
}

// Enable/Disable Ftp update ctrls
void
WxCasPrefs::EnableFtpUpdateCtrls( bool state )
{
	m_ftpUpdateSpinButton->Enable ( state );
	m_ftpUpdateStaticText->Enable ( state );
	m_ftpUrlTextCtrl->Enable ( state );
	m_ftpUrlStaticText->Enable ( state );
	m_ftpPathTextCtrl->Enable ( state );
	m_ftpPathStaticText->Enable ( state );
	m_ftpUserTextCtrl->Enable ( state );
	m_ftpUserStaticText->Enable ( state );
	m_ftpPasswdTextCtrl->Enable ( state );
	m_ftpPasswdStaticText->Enable ( state );
}
// File_checked_for_headers
