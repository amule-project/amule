//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         wxCasFrame Class
///
/// Purpose:      wxCas main frame
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
#include "wx/wxprec.h"

#ifdef __BORLANDC__
 #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
 #include "wx/wx.h"
#endif

#include <wx/image.h>
#include <wx/version.h>
#include <wx/config.h>
#include <wx/datetime.h>
#include <wx/wfstream.h>
#include <wx/protocol/ftp.h>

#include "wxcasframe.h"
#include "wxcasprint.h"
#include "wxcasprefs.h"
#include "wxcascte.h"
#include "wxcaspix.h"

// Constructor
WxCasFrame::WxCasFrame ( const wxString & title ) :
		wxFrame ( ( wxFrame * ) NULL, -1, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE )
{
	// Give it an icon
#ifdef __WXMSW__
	wxIcon icon(wxT("wxcas"));
#else
	wxIcon icon;
	icon.CopyFromBitmap( WxCasPix::getPixmap( wxT( "wxcas" ) ) );
#endif
	SetIcon ( icon );

	// Prefs
	wxConfigBase * prefs = wxConfigBase::Get();

	m_maxLineCount = 0;

	// Check if we have a previous DL max hit
	double absoluteMaxDL = ( double ) ( prefs->Read ( WxCasCte::ABSOLUTE_MAX_DL_KEY, 0L ) ) / 1024.0; // Stored in bytes
	wxDateTime absoluteMaxDlDate( ( time_t ) ( prefs->Read ( WxCasCte::ABSOLUTE_MAX_DL_DATE_KEY,
	                              ( long ) ( wxDateTime::Now().GetTicks() ) ) ) ); // Stored in Ticks

	// Add Online Sig file
	m_aMuleSig = new OnLineSig ( wxFileName( prefs->
	                             Read ( WxCasCte::AMULESIG_PATH_KEY,
	                                    WxCasCte::DEFAULT_AMULESIG_PATH ),
	                             WxCasCte::AMULESIG_FILENAME ),
	                             absoluteMaxDL, absoluteMaxDlDate );

	//Save absolute hit if we reach it during constructing
	if ( m_aMuleSig->IsSessionMaxDlChanged() ) {
		SaveAbsoluteHits();
	}

#ifdef __LINUX__		// System monitoring on Linux

	m_sysMonitor = new LinuxMon ();
#endif

	// Status Bar
	CreateStatusBar ();
	SetStatusText ( _( "Welcome!" ) );

	// Frame Vertical sizer
	m_frameVBox = new wxBoxSizer ( wxVERTICAL );

	// Add Main panel to frame (needed by win32 for padding sub panels)
	m_mainPanel = new wxPanel ( this, -1 );

	// Main Panel Vertical Sizer
	m_mainPanelVBox = new wxBoxSizer ( wxVERTICAL );

	// Main Panel static line
	m_staticLine = new wxStaticLine ( m_mainPanel, -1 );

#ifdef __WXMSW__

	m_BottomStaticLine = new wxStaticLine ( m_mainPanel, -1 );
#endif

	// Statistics Static Vertical Box Sizer
	m_sigPanelSBox = new wxStaticBox ( m_mainPanel, -1, _( "aMule" ) );
	m_sigPanelSBoxSizer = new wxStaticBoxSizer ( m_sigPanelSBox, wxVERTICAL );

	// Hit Static Horizontal Box Sizer
	m_hitPanelSBox = new wxStaticBox ( m_mainPanel, -1, _( "Maximum DL rate since wxCas is running" ) );
	m_hitPanelSBoxSizer = new wxStaticBoxSizer ( m_hitPanelSBox, wxHORIZONTAL );

	// Hit Static Horizontal Box Sizer
	m_absHitPanelSBox = new wxStaticBox ( m_mainPanel, -1, _( "Absolute Maximum DL rate during wxCas previous runs" ) );
	m_absHitPanelSBoxSizer = new wxStaticBoxSizer ( m_absHitPanelSBox, wxHORIZONTAL );

	// Statistic labels
	m_statLine_1 = new wxStaticText ( m_mainPanel, -1, MakeStatLine_1() );
	m_statLine_2 = new wxStaticText ( m_mainPanel, -1, MakeStatLine_2() );
	m_statLine_3 = new wxStaticText ( m_mainPanel, -1, MakeStatLine_3() );
	m_statLine_4 = new wxStaticText ( m_mainPanel, -1, MakeStatLine_4() );
	m_statLine_5 = new wxStaticText ( m_mainPanel, -1, MakeStatLine_5() );
	m_statLine_6 = new wxStaticText ( m_mainPanel, -1, MakeStatLine_6() );

	m_hitLine = new wxStaticText ( m_mainPanel, -1, MakeHitsLine_1() );
	m_hitButton =
	    new wxButton ( m_mainPanel, ID_HIT_BUTTON, wxString ( _( "Reset" ) ) );

	m_absHitLine = new wxStaticText ( m_mainPanel, -1, MakeHitsLine_2() );
	m_absHitButton =
	    new wxButton ( m_mainPanel, ID_ABS_HIT_BUTTON, wxString ( _( "Reset" ) ) );

#ifdef __LINUX__		// System monitoring on Linux

	// Monitoring Static Vertical Box Sizer
	m_monPanelSBox = new wxStaticBox ( m_mainPanel, -1, _( "System" ) );
	m_monPanelSBoxSizer = new wxStaticBoxSizer ( m_monPanelSBox, wxVERTICAL );

	m_sysLine_1 = new wxStaticText ( m_mainPanel, -1, MakeSysLine_1() );
	m_sysLine_2 = new wxStaticText ( m_mainPanel, -1, MakeSysLine_2() );
#endif

	// Statistic Panel Layout
	m_sigPanelSBoxSizer->Add ( m_statLine_1, 0, wxALL | wxALIGN_CENTER | wxGROW, 5 );
	m_sigPanelSBoxSizer->Add ( m_statLine_2, 0, wxALL | wxALIGN_CENTER | wxGROW, 5 );
	m_sigPanelSBoxSizer->Add ( m_statLine_3, 0, wxALL | wxALIGN_CENTER | wxGROW, 5 );
	m_sigPanelSBoxSizer->Add ( m_statLine_4, 0, wxALL | wxALIGN_CENTER | wxGROW, 5 );
	m_sigPanelSBoxSizer->Add ( m_statLine_5, 0, wxALL | wxALIGN_CENTER | wxGROW, 5 );
	m_sigPanelSBoxSizer->Add ( m_statLine_6, 0, wxALL | wxALIGN_CENTER | wxGROW, 5 );

	m_hitPanelSBoxSizer->Add ( m_hitLine, 0, wxALL | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxGROW, 5 );
	m_hitPanelSBoxSizer->Add ( m_hitButton, 0, wxALL | wxALIGN_RIGHT, 5 );

	m_absHitPanelSBoxSizer->Add ( m_absHitLine, 0, wxALL | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxGROW, 5 );
	m_absHitPanelSBoxSizer->Add ( m_absHitButton, 0, wxALL | wxALIGN_RIGHT, 5 );

#ifdef __LINUX__		// System monitoring on Linux

	m_monPanelSBoxSizer->Add ( m_sysLine_1, 0, wxALL | wxALIGN_CENTER | wxGROW, 5 );
	m_monPanelSBoxSizer->Add ( m_sysLine_2, 0, wxALL | wxALIGN_CENTER | wxGROW, 5 );
#endif

	// Main panel Layout
	m_mainPanelVBox->Add ( m_staticLine, 0, wxALL | wxALIGN_CENTER | wxGROW );

	m_mainPanelVBox->Add ( m_sigPanelSBoxSizer, 0, wxALL | wxALIGN_CENTER | wxGROW, 10 );

#ifdef __LINUX__		// System monitoring on Linux

	m_mainPanelVBox->Add ( m_monPanelSBoxSizer, 0, wxALL | wxALIGN_CENTER | wxGROW, 10 );
#endif

	m_mainPanelVBox->Add ( m_hitPanelSBoxSizer, 0, wxALL | wxALIGN_CENTER | wxGROW, 10 );

	m_mainPanelVBox->Add ( m_absHitPanelSBoxSizer, 0, wxALL | wxALIGN_CENTER | wxGROW, 10 );

#ifdef __WXMSW__

	m_mainPanelVBox->Add ( m_BottomStaticLine, 0, wxALL | wxALIGN_CENTER | wxGROW );
#endif

	// Toolbar Pixmaps
	m_toolBarBitmaps[ 0 ] = WxCasPix::getPixmap( wxT( "refresh" ) );
	m_toolBarBitmaps[ 1 ] = WxCasPix::getPixmap( wxT( "save" ) );
	m_toolBarBitmaps[ 2 ] = WxCasPix::getPixmap( wxT( "print" ) );
	m_toolBarBitmaps[ 3 ] = WxCasPix::getPixmap( wxT( "about" ) );
	m_toolBarBitmaps[ 4 ] = WxCasPix::getPixmap( wxT( "stop" ) );
	m_toolBarBitmaps[ 5 ] = WxCasPix::getPixmap( wxT( "prefs" ) );

	// Constructing toolbar
	m_toolbar =
	    new wxToolBar ( this, -1, wxDefaultPosition, wxDefaultSize,
	                    wxTB_HORIZONTAL | wxTB_FLAT );

	m_toolbar->SetToolBitmapSize ( wxSize ( 32, 32 ) );
	m_toolbar->SetMargins ( 2, 2 );

	m_toolbar->AddTool ( ID_BAR_REFRESH, wxT( "Refresh" ), m_toolBarBitmaps[ 0 ],
	                     _( "Stop Auto Refresh" ) );

	m_toolbar->AddSeparator ();

	m_toolbar->AddTool ( ID_BAR_SAVE, wxT( "Save" ), m_toolBarBitmaps[ 1 ],
	                     _( "Save Online Statistics image" ) );

	m_toolbar->AddTool ( ID_BAR_PRINT, wxT( "Print" ), m_toolBarBitmaps[ 2 ],
	                     _( "Print Online Statistics image" ) );

	m_toolbar->AddTool ( ID_BAR_PREFS, wxT( "Prefs" ), m_toolBarBitmaps[ 5 ],
	                     _( "Preferences setting" ) );

	m_toolbar->AddSeparator ();

	m_toolbar->AddTool ( ID_BAR_ABOUT, wxT( "About" ), m_toolBarBitmaps[ 3 ],
	                     _( "About wxCas" ) );

	m_toolbar->Realize ();

	SetToolBar ( m_toolbar );

	// Panel Layout
	m_mainPanel->SetAutoLayout( true );
	m_mainPanel->SetSizer ( m_mainPanelVBox );

	// Frame Layout
	m_frameVBox->Add ( m_mainPanel, 1, wxALL | wxGROW );
	SetAutoLayout ( TRUE );
	SetSizerAndFit ( m_frameVBox );

	// Add refresh timer
	m_refresh_timer = new wxTimer ( this, ID_REFRESH_TIMER );
	m_refresh_timer->Start ( 1000 * prefs->Read ( WxCasCte::REFRESH_RATE_KEY, WxCasCte::DEFAULT_REFRESH_RATE ) );	// s to ms

	// Add FTP update timer
	m_ftp_update_timer = new wxTimer ( this, ID_FTP_UPDATE_TIMER );
	m_ftp_update_timer->Start ( 60000 * prefs->Read ( WxCasCte::FTP_UPDATE_RATE_KEY, WxCasCte::DEFAULT_FTP_UPDATE_RATE ) );	// min to ms
}

// Destructor
WxCasFrame::~WxCasFrame ()
{
	delete m_aMuleSig;

#ifdef __LINUX__		// System monitoring on Linux

	delete m_sysMonitor;
#endif
}

// Events table
BEGIN_EVENT_TABLE ( WxCasFrame, wxFrame )
EVT_TOOL ( ID_BAR_REFRESH, WxCasFrame::OnBarRefresh )
EVT_TOOL ( ID_BAR_SAVE, WxCasFrame::OnBarSave )
EVT_TOOL ( ID_BAR_PRINT, WxCasFrame::OnBarPrint )
EVT_TOOL ( ID_BAR_PREFS, WxCasFrame::OnBarPrefs )
EVT_TOOL ( ID_BAR_ABOUT, WxCasFrame::OnBarAbout )
EVT_TIMER ( ID_REFRESH_TIMER, WxCasFrame::OnRefreshTimer )
EVT_TIMER ( ID_FTP_UPDATE_TIMER, WxCasFrame::OnFtpUpdateTimer )
EVT_BUTTON ( ID_HIT_BUTTON, WxCasFrame::OnHitButton )
EVT_BUTTON ( ID_ABS_HIT_BUTTON, WxCasFrame::OnAbsHitButton )
END_EVENT_TABLE ()

// Get Stat Bitmap
wxImage *
WxCasFrame::GetStatImage () const
{
	wxBitmap
	statBitmap = WxCasPix::getPixmap( wxT( "stat" ) );

	wxMemoryDC memdc;
	memdc.SelectObject ( statBitmap );

#ifdef __WXMSW__

	memdc.
	SetFont ( wxFont::wxFont ( 6, wxSWISS, wxNORMAL, wxBOLD ) );
#else

	memdc.
	SetFont ( wxFont::wxFont ( 8, wxSWISS, wxNORMAL, wxBOLD ) );
#endif

	memdc.
	SetTextForeground ( *wxWHITE );
	memdc.
	DrawText ( m_statLine_1->GetLabel (), 25, 8 );
	memdc.
	DrawText ( m_statLine_2->GetLabel (), 25, 26 );
	memdc.
	DrawText ( m_statLine_3->GetLabel (), 25, 43 );
	memdc.
	DrawText ( m_statLine_4->GetLabel (), 25, 60 );
	memdc.
	DrawText ( m_statLine_5->GetLabel (), 25, 77 );
	memdc.
	DrawText ( m_statLine_6->GetLabel (), 25, 94 );
	memdc.
	SelectObject ( wxNullBitmap );

	wxImage *
	statImage = new wxImage ( statBitmap.ConvertToImage() );

	return
	    ( statImage );
}

// Refresh button
void
WxCasFrame::OnBarRefresh ( wxCommandEvent& WXUNUSED( event ) )
{
	if ( m_refresh_timer->IsRunning () ) {
		m_refresh_timer->Stop ();
		m_ftp_update_timer->Stop ();
		m_toolbar->DeleteTool ( ID_BAR_REFRESH );
		m_toolbar->InsertTool ( 0, ID_BAR_REFRESH, wxT( "Refresh" ),
		                        m_toolBarBitmaps[ 4 ], wxNullBitmap,
		                        wxITEM_NORMAL, _( "Start Auto Refresh" ) );
		m_toolbar->Realize ();
		SetStatusText ( _( "Auto Refresh stopped" ) );
	} else {
		m_refresh_timer->Start ();
		m_ftp_update_timer->Start ();
		m_toolbar->DeleteTool ( ID_BAR_REFRESH );
		m_toolbar->InsertTool ( 0, ID_BAR_REFRESH, wxT( "Refresh" ),
		                        m_toolBarBitmaps[ 0 ], wxNullBitmap,
		                        wxITEM_NORMAL, _( "Stop Auto Refresh" ) );
		m_toolbar->Realize ();
		SetStatusText ( _( "Auto Refresh started" ) );
	}
}

// Save button
void
WxCasFrame::OnBarSave ( wxCommandEvent& WXUNUSED( event ) )
{
	wxImage * statImage = GetStatImage ();

	wxString saveFileName = wxFileSelector ( _( "Save Statistics Image" ),
	                        wxFileName::GetHomeDir (),
	                        WxCasCte::AMULESIG_IMG_NAME,
	                        ( const wxChar * ) NULL,
	                        wxT ( "PNG files (*.png)|*.png|" )
	                        wxT ( "JPEG files (*.jpg)|*.jpg|" )
	                        wxT ( "BMP files (*.bmp)|*.bmp|" ),
	                        wxSAVE );

	if ( !saveFileName.empty () ) {
		// This one guesses image format from filename extension
		// (it may fail if the extension is not recognized):

		if ( !statImage->SaveFile ( saveFileName ) ) {
			wxMessageBox ( _( "No handler for this file type." ),
			               _( "File was not saved" ), wxOK | wxCENTRE, this );
		}
	}
	delete statImage;
}

// Print button
void
WxCasFrame::OnBarPrint ( wxCommandEvent& WXUNUSED( event ) )
{
	wxPrinter printer;
	WxCasPrint printout ( _( "aMule Online Statistics" ) );
	if ( !printer.Print ( this, &printout, TRUE ) ) {
		if ( wxPrinter::GetLastError () == wxPRINTER_ERROR ) {
			wxMessageBox ( _
			               ( "There was a problem printing.\nPerhaps your current printer is not set correctly?" ),
			               _( "Printing" ), wxOK );
		}
	}
}

// Prefs button
void
WxCasFrame::OnBarPrefs ( wxCommandEvent& WXUNUSED( event ) )
{
	WxCasPrefs dlg ( this );
	dlg.ShowModal ();
}

// About button
void
WxCasFrame::OnBarAbout ( wxCommandEvent& WXUNUSED( event ) )
{
	wxMessageBox ( _
	               ( "wxCas, aMule OnLine Signature Statistics\n\n"
	                 "(c) 2004 ThePolish <thepolish@vipmail.ru>\n\n"
	                 "Based on CAS by Pedro de Oliveira <falso@rdk.homeip.net>\n\n"
	                 "Distributed under GPL" ),
	               _( "About wxCas" ), wxOK | wxCENTRE | wxICON_INFORMATION );
}

// Refresh timer
void
WxCasFrame::OnRefreshTimer ( wxTimerEvent& WXUNUSED( event ) )
{
	// Prefs
	wxConfigBase * prefs = wxConfigBase::Get();

	UpdateAll ();

	// Generate stat image if asked in config
	if ( ( bool )
	        ( prefs->
	          Read ( WxCasCte::ENABLE_AUTOSTATIMG_KEY,
	                 WxCasCte::DEFAULT_AUTOSTATIMG_ISENABLED ) ) ) {
		wxImage * statImage = GetStatImage ();

		wxFileName fileName ( prefs->
		                      Read ( WxCasCte::AUTOSTATIMG_DIR_KEY,
		                             WxCasCte::DEFAULT_AUTOSTATIMG_PATH ),
		                      WxCasCte::AMULESIG_IMG_NAME,
		                      prefs->
		                      Read ( WxCasCte::AUTOSTATIMG_TYPE_KEY,
		                             WxCasCte::DEFAULT_AUTOSTATIMG_TYPE ).
		                      Lower () );

		if ( !statImage->SaveFile ( fileName.GetFullPath () ) ) {
			wxLogError ( wxT( "No handler for this file type. File was not saved" ) );
		}
		delete statImage;
	}
}

// Ftp update timer
void
WxCasFrame::OnFtpUpdateTimer ( wxTimerEvent& WXUNUSED( event ) )
{
	// Prefs
	wxConfigBase * prefs = wxConfigBase::Get();

	// Image must be autogenerated to be uploaded
	if ( ( bool ) ( prefs->Read ( WxCasCte::ENABLE_AUTOSTATIMG_KEY,
	                              WxCasCte::DEFAULT_AUTOSTATIMG_ISENABLED ) ) &&
	        ( bool ) ( prefs->Read ( WxCasCte::ENABLE_FTP_UPDATE_KEY,
	                                 WxCasCte::DEFAULT_FTP_UPDATE_ISENABLED ) ) ) {
		// Get image file
		wxFileName fileName ( prefs->
		                      Read ( WxCasCte::AUTOSTATIMG_DIR_KEY,
		                             WxCasCte::DEFAULT_AUTOSTATIMG_PATH ),
		                      WxCasCte::AMULESIG_IMG_NAME,
		                      prefs->
		                      Read ( WxCasCte::AUTOSTATIMG_TYPE_KEY,
		                             WxCasCte::DEFAULT_AUTOSTATIMG_TYPE ).
		                      Lower () );

		// If img doenst exist, return
		if ( ! fileName.FileExists( fileName.GetFullPath () ) ) {
			wxLogError( wxT( "Image file " ) + fileName.GetFullPath () + wxT( " doesn't exist" ) );
			return ;
		}

		// Connect to ftp
		wxFTP ftp;

		ftp.SetUser( prefs->Read ( WxCasCte::FTP_USER_KEY,
		                           WxCasCte::DEFAULT_FTP_USER ) );
		ftp.SetPassword( prefs->Read ( WxCasCte::FTP_PASSWD_KEY,
		                               WxCasCte::DEFAULT_FTP_PASSWD ) );

		if ( ! ftp.Connect( prefs->Read ( WxCasCte::FTP_URL_KEY,
		                                  WxCasCte::DEFAULT_FTP_URL ) ) ) {
			wxLogError( wxT( "Cannot connect to FTP server " ) + prefs->Read ( WxCasCte::FTP_URL_KEY,
			            WxCasCte::DEFAULT_FTP_URL ) );
			return ;
		}

		// Chdir
		if ( ! ftp.ChDir( prefs->Read ( WxCasCte::FTP_PATH_KEY,
		                                WxCasCte::DEFAULT_FTP_PATH ) ) ) {
			wxLogError( wxT( "Cannot chdir to " ) + prefs->Read ( WxCasCte::FTP_PATH_KEY,
			            WxCasCte::DEFAULT_FTP_PATH ) );
			ftp.Close();
			return ;
		}

		// Upload image
		ftp.SetBinary();
		wxFileInputStream in( fileName.GetFullPath () );
		if ( in.Ok() ) {
			wxOutputStream * out = ftp.GetOutputStream( fileName.GetFullName () );
			if ( out ) {
				out->Write( in );
				delete out;
			} else {
				wxLogError( wxT( "Cannot open FTP upload stream" ) );
			}
		} else {
			wxLogError( wxT( "Cannot open file stream to read image file" ) );
		}

		// Close connexion
		ftp.Close();
	}
}

// Reset wxcas session hit
void
WxCasFrame::OnHitButton ( wxCommandEvent& WXUNUSED( event ) )
{
	m_aMuleSig->ResetSessionMaxDL();
	UpdateStatsPanel ();
}

// Reset wxcas absolute hit
void
WxCasFrame::OnAbsHitButton ( wxCommandEvent& WXUNUSED( event ) )
{
	m_aMuleSig->ResetAbsoluteMaxDL();
	SaveAbsoluteHits();
	UpdateStatsPanel ();
}

// Update all panels and frame, call Fit if needed
void
WxCasFrame::UpdateAll ( bool forceFitting )
{
	bool needFit = UpdateStatsPanel ();

	if ( needFit || forceFitting ) {
		// Fit stats pannel
		m_mainPanel->Fit();

		// Fit main frame
		SetClientSize( m_mainPanel->GetSize() );
	}
}

// Update stat panel
bool
WxCasFrame::UpdateStatsPanel ()
{
	// Set labels
	m_aMuleSig->Refresh ();

#ifdef __LINUX__		// System monitoring on Linux

	m_sysMonitor->Refresh ();
#endif

	wxString newline;
	wxString status;
	unsigned int newMaxLineCount = 0;

	Freeze ();

	// Stat line 1
	newline = MakeStatLine_1();
	m_statLine_1->SetLabel ( newline );

	newMaxLineCount = GetMaxUInt( newline.Length (), newMaxLineCount );

	// aMule is running
	if ( m_aMuleSig->GetAmuleState () == 1 ) {
		// Stat line 2
		newline = MakeStatLine_2();
		m_statLine_2->SetLabel ( newline );

		newMaxLineCount = GetMaxUInt( newline.Length (), newMaxLineCount );

		// Stat line 3
		newline = MakeStatLine_3();
		m_statLine_3->SetLabel ( newline );

		newMaxLineCount = GetMaxUInt( newline.Length (), newMaxLineCount );

		// Stat line 4
		newline = MakeStatLine_4();
		m_statLine_4->SetLabel ( newline );

		newMaxLineCount = GetMaxUInt( newline.Length (), newMaxLineCount );

		// Stat line 5
		newline = MakeStatLine_5();
		m_statLine_5->SetLabel ( newline );

		newMaxLineCount = GetMaxUInt( newline.Length (), newMaxLineCount );

		// Stat line 6
		newline = MakeStatLine_6();
		m_statLine_6->SetLabel ( newline );

		newMaxLineCount = GetMaxUInt( newline.Length (), newMaxLineCount );

		// Hits line 1

		if ( m_aMuleSig->IsSessionMaxDlChanged() ) {
			newline = MakeHitsLine_1();
			m_hitLine->SetLabel ( newline );

			newMaxLineCount = GetMaxUInt( newline.Length (), newMaxLineCount );
		}

		// Hits line 2
		if ( m_aMuleSig->IsAbsoluteMaxDlChanged() ) {
			newline = MakeHitsLine_2();
			m_absHitLine->SetLabel ( newline );

			newMaxLineCount = GetMaxUInt( newline.Length (), newMaxLineCount );

			// Save new records
			SaveAbsoluteHits();
		}

#ifdef __LINUX__		// System monitoring on Linux
		newline = MakeSysLine_1();
		m_sysLine_1->SetLabel ( newline );

		newMaxLineCount = GetMaxUInt( newline.Length (), newMaxLineCount );

		newline = MakeSysLine_2();
		m_sysLine_2->SetLabel ( newline );

		newMaxLineCount = GetMaxUInt( newline.Length (), newMaxLineCount );
#endif

		status = _( "aMule is running" );
	}
	// aMule is stopped
	else if ( m_aMuleSig->GetAmuleState () == 0 ) {
		status = _( "WARNING: aMule is stopped !" );
	}
	// aMule is connecting
	else if ( m_aMuleSig->GetAmuleState () == 2 ) {
		status = _( "aMule is connecting..." );
	}
	// aMule status is unknown
	else {
		status = _( "Oh Oh, aMule status is unknown..." );
	}

	Thaw ();

	// Set status bar
	SetStatusText ( status );

	// Resize only if needed
	if ( m_maxLineCount != newMaxLineCount ) {
		m_maxLineCount = newMaxLineCount;
		return ( TRUE );
	} else {
		return ( FALSE );
	}
}

// Refresh period changing
bool
WxCasFrame::ChangeRefreshPeriod( const int newPeriod )
{
	// As the user can stop it, we must let it in the same state
	// it was before changing period
	bool wasRunning = FALSE;

	if ( m_refresh_timer->IsRunning() ) {
		wasRunning = TRUE;
	}

	bool ok = m_refresh_timer->Start( newPeriod );

	if ( ! wasRunning ) {
		m_refresh_timer->Stop();
	}

	return ( ok );
}

// Ftp update period changing
bool
WxCasFrame::ChangeFtpUpdatePeriod( const int newPeriod )
{
	// As the user can stop it, we must let it in the same state
	// it was before changing period
	bool wasRunning = FALSE;

	if ( m_ftp_update_timer->IsRunning() ) {
		wasRunning = TRUE;
	}

	bool ok = m_ftp_update_timer->Start( newPeriod );

	if ( ! wasRunning ) {
		m_ftp_update_timer->Stop();
	}

	return ( ok );
}

// Set amulesig.dat file
void
WxCasFrame::SetAmuleSigFile( const wxFileName& file )
{
	m_aMuleSig->SetAmuleSig ( file );
}

wxString
WxCasFrame::MakeStatLine_1() const
{
	wxString newline;
	if ( m_aMuleSig->GetAmuleState () == 1 ) {
		newline = _( "aMule " )
		          + m_aMuleSig->GetVersion ()
		          + _( " has been running for " )
		          + m_aMuleSig->GetRunTime ();
	} else if ( m_aMuleSig->GetAmuleState () == 0 ) {

		newline = _( "aMule " )
		          + m_aMuleSig->GetVersion ()
		          + _( " is STOPPED !" );
	} else if ( m_aMuleSig->GetAmuleState () == 2 ) {
		newline = _( "aMule " )
		          + m_aMuleSig->GetVersion ()
		          + _( " is connecting..." );
	} else {
		newline = _( "aMule " )
		          + m_aMuleSig->GetVersion ()
		          + _( " is doing something strange, check it !" );
	}

	return ( newline );
}

wxString
WxCasFrame::MakeStatLine_2() const
{
	wxString notTooLongName ( m_aMuleSig->GetServerName () );
	if ( notTooLongName.Length() > 32 ) {
		notTooLongName = notTooLongName.Left( 32 ) + wxT( "..." );
	}

	wxString newline = m_aMuleSig->GetUser ()
	                   + _( " is on " )
	                   + notTooLongName
			   + _( " [" )
			   + m_aMuleSig->GetServerIP ()
			   + _( ":" )
			   + m_aMuleSig->GetServerPort ()
			   + _( "] with " )
			   + m_aMuleSig->GetConnexionIDType ()
			   + _( " ( Kad: " );
	if(m_aMuleSig->GetKadState() == 2) {
		newline += _( "on");
	} else if (m_aMuleSig->GetKadState() == 1) {
		newline += _( "firewalled");
	} else {
		newline += _( "off");
	}
	newline += _( " )");
	return ( newline );
}

wxString
WxCasFrame::MakeStatLine_3() const
{
	wxString newline = _( "Total Download: " )
	                   + m_aMuleSig->GetConvertedTotalDL ()
	                   + _( ", Upload: " )
	                   + m_aMuleSig->GetConvertedTotalUL ();

	return ( newline );
}

wxString
WxCasFrame::MakeStatLine_4() const
{
	wxString newline = _( "Session Download: " )
	                   + m_aMuleSig->GetConvertedSessionDL ()
	                   + _( ", Upload: " )
	                   + m_aMuleSig->GetConvertedSessionUL ();

	return ( newline );
}

wxString
WxCasFrame::MakeStatLine_5() const
{
	wxString newline = _( "Download: " )
	                   + m_aMuleSig->GetDLRate ()
	                   + _( " kB/s, Upload: " )
	                   + m_aMuleSig->GetULRate ()
	                   + _( "kB/s" );

	return ( newline );
}

wxString
WxCasFrame::MakeStatLine_6() const
{
	wxString newline = _( "Sharing: " )
	                   + m_aMuleSig->GetSharedFiles ()
	                   + _( " file(s), Clients on queue: " )
	                   + m_aMuleSig->GetQueue ();

	return ( newline );
}

wxString
WxCasFrame::MakeHitsLine_1() const
{
	wxString newline = wxString::Format ( _( "%.2f kB/s" ), m_aMuleSig->GetSessionMaxDL () )
	                   + _( " on " )
	                   + m_aMuleSig->GetSessionMaxDlDate().Format( wxT( "%c" ) );

	return ( newline );
}

wxString
WxCasFrame::MakeHitsLine_2() const
{
	wxString newline = wxString::Format ( _( "%.2f kB/s" ), m_aMuleSig->GetAbsoluteMaxDL() )
	                   + _( " on " )
	                   + m_aMuleSig->GetAbsoluteMaxDlDate().Format( wxT( "%c" ) );

	return ( newline );
}

#ifdef __LINUX__		// System monitoring on Linux
wxString
WxCasFrame::MakeSysLine_1() const
{
	wxString newline = _( "System Load Average (1-5-15 min): " )
	                   + m_sysMonitor->GetSysLoad_1 () + wxT( " " )
	                   + m_sysMonitor->GetSysLoad_5 () + wxT( " " )
	                   + m_sysMonitor->GetSysLoad_15 ();

	return ( newline );
}

wxString
WxCasFrame::MakeSysLine_2() const
{
	wxString newline = _( "System uptime: " )
	                   + m_sysMonitor->GetUptime ();

	return ( newline );
}
#endif

void WxCasFrame::SaveAbsoluteHits()
{
	wxConfigBase * prefs = wxConfigBase::Get();
	prefs->Write( WxCasCte::ABSOLUTE_MAX_DL_KEY, ( long ) ( 1024.0 * m_aMuleSig->GetAbsoluteMaxDL() ) );
	prefs->Write( WxCasCte::ABSOLUTE_MAX_DL_DATE_KEY, ( long ) ( m_aMuleSig->GetAbsoluteMaxDlDate().GetTicks() ) );
	prefs->Flush();
}
