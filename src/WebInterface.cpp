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
	#include "config.h"		// Needed for VERSION
#endif

#if !defined( __WXMSW__ )
	#include <unistd.h>
#else
	#define AMULEWEBDLG 1
#endif

//-------------------------------------------------------------------

#include "WebInterface.h"

//-------------------------------------------------------------------

#include "otherfunctions.h"
#include "WebServer.h"

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
IMPLEMENT_APP(CamulewebApp)
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
    amuleFrame_ID = 32000 
};

BEGIN_EVENT_TABLE(CamulewebFrame, wxFrame)
	EVT_MENU(amuleweb_Quit,  CamulewebFrame::OnQuit)
	EVT_MENU(amuleweb_About, CamulewebFrame::OnAbout)
//	EVT_TEXT(Event_Comand_ID, CamulewebFrame::OnCommandChange)
	EVT_TEXT_ENTER(Event_Comand_ID, CamulewebFrame::OnCommandEnter)
	EVT_SIZE(CamulewebFrame::OnSize)
	EVT_IDLE(CamulewebFrame::OnIdle)
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

	log_text = new wxTextCtrl(this, -1, wxEmptyString,
		wxPoint(2, 2),
		wxSize(APP_INIT_SIZE_X-4, APP_INIT_SIZE_Y-30-4),
		wxTE_MULTILINE | wxTE_READONLY);
	log_text->SetBackgroundColour(wxT("wheat"));
	log_text->SetDefaultStyle(
		wxTextAttr(
			wxNullColour, 
			wxT("wheat"), 
			wxFont(10, wxMODERN, wxNORMAL, wxNORMAL)
		)
	);

	cmd_control = new wxTextCtrl(this, Event_Comand_ID, wxEmptyString,
		wxPoint(2, APP_INIT_SIZE_Y-30-4),
		wxSize(APP_INIT_SIZE_X-4,30),
		wxTE_PROCESS_ENTER);
	cmd_control->SetBackgroundColour(wxT("wheat"));
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

void CamulewebFrame::OnSize( wxSizeEvent& WXUNUSED(event) )
{
	int x = 0;
	int y = 0;
	GetClientSize( &x, &y );	
	if (log_text) log_text->SetSize( 2, 2, x-4, y-30 - 4 );
	if (cmd_control) cmd_control->SetSize( 2, y-30-2, x-4,30);
}

void CamulewebFrame::OnIdle(wxIdleEvent &WXUNUSED(event))
{
	theApp.MainThreadIdleNow();
}
//-------------------------------------------------------------------
#endif
//-------------------------------------------------------------------

#if wxUSE_GUI
void CamulewebApp::LocalShow(const wxString &s)
{
	if (!frame) {
		return;
	}
	frame->log_text->AppendText(s);
}
#endif

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
	Show(_("\n->Help: Avalaible commands (case insensitive):\n\n"));	
	Show(wxString(wxT("help:\n\t")) + wxString(_("Shows this help.\n")));	
	//Show(_("start: Start web server.\n"));
	//Show(_("stop: Stop web server.\n"));
	//Show(_("restart: Restart web server.\n"));	
	Show(wxString(wxT("quit, exit:\n\t")) + wxString(_("Exits aMuleWeb.\n")));	
	Show(_("\n->End of listing\n"));
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

#if !wxUSE_GUI
void CamulewebApp::Post_Shell() {
	webserver->StopServer();
}
#endif


#if wxUSE_GUI
int CamulewebApp::OnExit() {
	frame = NULL;
	if (webserver) {
		webserver->StopServer();
	}
	return 0;
}
#endif

#if wxUSE_GUI
bool CamulewebApp::OnInit() {
	CaMuleExternalConnector::OnInit();
	frame = new CamulewebFrame(_("amuleweb DLG"), wxPoint(50, 50), wxSize(APP_INIT_SIZE_X, APP_INIT_SIZE_Y));
	frame->Show(true);
#else
int CamulewebApp::OnRun() {
#endif
	ConnectAndRun(wxT("aMuleweb"), commands);

	return true;
}

