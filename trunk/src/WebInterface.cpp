/*  This file is part of aMule project
 *  
 *  aMule Copyright (C)2003-2004 aMule Team ( http://www.amule-project.net )
 *  This file Copyright (C)2003 Kry (elkry@sourceforge.net  http://www.amule-project.net )
 *  This file Copyright (C)2004 shakraw <shakraw@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"	// For VERSION
#endif

#if !defined( __WXMSW__ )
	#include <unistd.h>
#endif

//-------------------------------------------------------------------
#include "MD5Sum.h"

#include "WebInterface.h"

//-------------------------------------------------------------------

#if wxUSE_GUI
	#include <wx/statline.h>
#endif

//-------------------------------------------------------------------

#include "otherfunctions.h"
#include "WebServer.h"

#if wxCHECK_VERSION(2,4,2)
	#include <wx/config.h>	// For wxFileConfig in wx-2.4.2
#endif
#include <wx/fileconf.h>	// For wxFileConfig

//-------------------------------------------------------------------

#define CMD_ID_HELP	1
#define CMD_ID_STOP	2
#define CMD_ID_START	3
#define CMD_ID_RESTART	4

#define APP_INIT_SIZE_X 640
#define APP_INIT_SIZE_Y 480

#define theApp (*((CamulewebApp*)wxTheApp))

static CmdId commands[] = {
	{ wxT("quit"),		CMD_ID_QUIT },
	{ wxT("exit"),		CMD_ID_QUIT },
	{ wxT("help"),		CMD_ID_HELP },
	{ wxT("stop"),		CMD_ID_STOP },
	{ wxT("start"),		CMD_ID_START },
	{ wxT("restart"),	CMD_ID_RESTART },
	{ wxEmptyString,	0 },
};

static CWebServer *webserver = NULL;

#if wxUSE_GUI && wxUSE_TIMER
	class MyTimer *mytimer;
#endif

//-------------------------------------------------------------------
IMPLEMENT_APP(CamulewebApp);
//-------------------------------------------------------------------

//-------------------------------------------------------------------
#if wxUSE_GUI
//-------------------------------------------------------------------
// IDs for the controls and the menu commands
enum {
    // menu items
    amuleweb_Quit = 1,

    // it is important for the id corresponding to the "About" command to have
    // this standard value as otherwise it won't be handled properly under Mac
    // (where it is special and put into the "Apple" menu)
    amuleweb_About = wxID_ABOUT,
    Event_Comand_ID = 32001,
    amuleFrame_ID = 32000,
    Timer_ID
};

BEGIN_EVENT_TABLE(CamulewebFrame, wxFrame)
	EVT_MENU(amuleweb_Quit,  CamulewebFrame::OnQuit)
	EVT_MENU(amuleweb_About, CamulewebFrame::OnAbout)
	EVT_TEXT_ENTER(Event_Comand_ID, CamulewebFrame::OnCommandEnter)
	EVT_IDLE(CamulewebFrame::OnIdle)
	EVT_TIMER(Timer_ID, CamulewebFrame::OnTimerEvent)
END_EVENT_TABLE()


CamulewebFrame::CamulewebFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
       : wxFrame(NULL, amuleFrame_ID, title, pos, size, style)
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(amuleweb_Quit, _("E&xit\tAlt-X"), _("Quit amuleweb"));

	wxMenu *helpMenu = new wxMenu;
	helpMenu->Append(amuleweb_About, _("&About...\tF1"), _("Show about dialog"));

	// now append the freshly created menu to the menu bar...
	wxMenuBar *menuBar = new wxMenuBar();
	menuBar->Append(menuFile, _("&File"));
	menuBar->Append(helpMenu, _("&Help"));

	// ... and attach this menu bar to the frame
	SetMenuBar(menuBar);

	// Text controls and sizer
	wxBoxSizer *vsizer = new wxBoxSizer(wxVERTICAL);
	log_text = new wxTextCtrl(this, -1, wxEmptyString,
		wxDefaultPosition, wxSize(APP_INIT_SIZE_X,APP_INIT_SIZE_Y),
		wxTE_MULTILINE|wxVSCROLL|wxTE_READONLY);
	log_text->SetBackgroundColour(wxT("wheat"));
	log_text->SetDefaultStyle(
		wxTextAttr(
			wxNullColour, 
			wxT("wheat"), 
			wxFont(10, wxMODERN, wxNORMAL, wxNORMAL)
		)
	);
	vsizer->Add( log_text, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );
	wxStaticLine *line = new wxStaticLine( this, -1,
		wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	vsizer->Add( line, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	cmd_control = new wxTextCtrl(this, Event_Comand_ID, wxEmptyString,
			wxDefaultPosition, wxSize(APP_INIT_SIZE_X,-1), wxTE_PROCESS_ENTER);
	cmd_control->SetBackgroundColour(wxT("wheat"));
	vsizer->Add(cmd_control, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

	SetSizer(vsizer);
	vsizer->SetSizeHints(this);

	m_timer = new wxTimer(this, Timer_ID);
	m_timer->Start(5000);
}

void CamulewebFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	// true is to force the frame to close
	Show(_("\nOk, exiting Web Client...\n"));
	Close(true);
}

void CamulewebFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxString msg;
	msg.Printf( 
		_("amuleweb [DLG version]\n"
		"Using %s\n(c) aMule Dev Team"),
		wxVERSION_STRING);
	wxMessageBox(msg, _("About amuleweb"), wxOK | wxICON_INFORMATION, this);
}

void CamulewebFrame::OnCommandEnter(wxCommandEvent& WXUNUSED(event)){
	if (cmd_control->GetLineLength(0) == 0) {
		return; 
	}
	wxString buffer = cmd_control->GetLineText(0);
	if (theApp.Parse_Command(buffer, commands)) {
		Close(true);
	}
	cmd_control->Clear();
}

void CamulewebFrame::OnIdle(wxIdleEvent &WXUNUSED(event))
{
	theApp.MainThreadIdleNow();
}

void CamulewebFrame::OnTimerEvent(wxTimerEvent &WXUNUSED(event))
{
	wxWakeUpIdle();
}

void CamulewebApp::LocalShow(const wxString &s)
{
	if (!frame) {
		return;
	}
	frame->log_text->AppendText(s);
}

int CamulewebApp::OnExit() {
	frame = NULL;
	if (webserver) {
		webserver->StopServer();
	}
	return 0;
}

bool CamulewebApp::OnInit() {
	CaMuleExternalConnector::OnInit();
	frame = new CamulewebFrame(_("amuleweb DLG"), wxPoint(50, 50), wxSize(APP_INIT_SIZE_X, APP_INIT_SIZE_Y));
	frame->Show(true);
	ConnectAndRun(wxT("aMuleweb"), commands);

	return true;
}

#else

int CamulewebApp::OnRun() {
	ConnectAndRun(wxT("aMuleweb"), commands);

	return true;
}

void CamulewebApp::Post_Shell() {
	webserver->StopServer();
}

#endif

void CamulewebApp::OnInitCmdLine(wxCmdLineParser& amuleweb_parser)
{
	CaMuleExternalConnector::OnInitCmdLine(amuleweb_parser);
	amuleweb_parser.AddOption(wxT("t"), wxT("template"), 
		wxT("loads template from file <template file>"), 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
		
	amuleweb_parser.AddSwitch(wxT("z"), wxT("gzip"), 
		wxT("Use gzip compression"));
	
	amuleweb_parser.AddOption(wxT("apw"), wxT("admin-pass"), 
		wxT("Full access password for webserver"), 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddOption(wxT("gpw"), wxT("guest-pass"), 
		wxT("Guest password for webserver"), 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddSwitch(wxT("g"), wxT("guest"), 
		wxT("Allow guest access"));
}

bool CamulewebApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	m_HasTemplate = parser.Found(wxT("template"), &m_TemplateFileName);
	m_bForcedUseGzip = m_UseGzip = parser.Found(wxT("gzip"));
	m_bForcedAllowGuest = m_AllowGuest = parser.Found(wxT("guest"));
	if ( parser.Found(wxT("file-config")) ) {
		wxFileConfig eMuleIni(
			wxT("eMule"),
			wxT("eMule-project"),
			wxT(".eMule")
		);
		m_AdminPass = eMuleIni.Read(wxT("/WebServer/Password"));
		m_GuestPass = eMuleIni.Read(wxT("/WebServer/PasswordLow"));
		wxString AllowGuest = eMuleIni.Read(wxT("/WebServer/UseLowRightsUser"));
		m_AllowGuest = (AllowGuest == wxT("1")) ? true : false;
	}
	// file already contain password in hashed form
	if ( !parser.Found(wxT("admin-pass"), &m_AdminPass) ) {
		m_bForcedAdminPassword = false;
	} else {
		m_AdminPass = MD5Sum(m_AdminPass).GetHash();
		m_bForcedAdminPassword = true;
	}
	if ( !parser.Found(wxT("guest-pass"), &m_GuestPass) ) {
		m_bForcedGuestPassword = false;
	} else {
		m_GuestPass = MD5Sum(m_GuestPass).GetHash();
		m_bForcedGuestPassword = true;
	}
	return CaMuleExternalConnector::OnCmdLineParsed(parser);
}

int CamulewebApp::ProcessCommand(int ID) {
	switch (ID) {
		case CMD_ID_HELP:
			ShowHelp();
			break;
		case CMD_ID_STOP:
			//webserver->StopServer();
			break;
		case CMD_ID_START:
			//webserver->StartServer();
			break;
		case CMD_ID_RESTART:
			//webserver->RestartServer();
			break;
		default:
			return -1;
			break;
	}

	return 0;
}

void CamulewebApp::ShowHelp() {
//                                  1         2         3         4         5         6         7         8
//                         12345678901234567890123456789012345678901234567890123456789012345678901234567890
	Show(         _("\n----------------> Help: Avalaible commands (case insensitive): <----------------\n\n"));	
	Show(wxString(wxT("Help:\t\t\t")) + wxString(_("Shows this help.\n")));
	//Show(wxString(wxT("Start:\t\t\t)) + wxString(_("Start web server.\n")));
	//Show(wxString(wxT("Stop:\t\t\t))  + wxString(_("Stop web server.\n")));
	//Show(wxString(wxT("Restart:\t\t\t)) + wxString(_("Restart web server.\n")));
	Show(wxString(wxT("Quit, Exit:\t\t")) + wxString(_("Exits aMuleWeb.\n")));
	Show(         _("\n----------------------------> End of listing <----------------------------------\n"));
}

void CamulewebApp::ShowGreet() {
	Show(wxT("\n---------------------------------\n"));
	Show(wxString(wxT("|       ")) + wxString(_("aMule Web Server")) + wxString(wxT("        |\n")));
	Show(wxT("---------------------------------\n\n"));
	Show(_("\nUse 'Help' for command list\n\n"));
}

void CamulewebApp::Pre_Shell() {
	//Creating the web server
	webserver = new CWebServer(this);
	webserver->StartServer();
}
