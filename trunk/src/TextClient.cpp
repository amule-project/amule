// This file is part of the aMule Project
//
// aMule Copyright (C) 2003-2004 aMule Team ( http://www.amule-project.net )
// This file Copyright (C) 2003-2004 Kry (elkry@sourceforge.net  http://www.amule-project.net )
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
			

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for VERSION
#endif

#if !defined( __WXMSW__ )
	#include <unistd.h>
#endif

//-------------------------------------------------------------------

#include "TextClient.h"

//-------------------------------------------------------------------

#include <wx/intl.h>		// For _()
#if wxUSE_GUI
	#include <wx/menu.h>		// For wxMenu
	#include <wx/msgdlg.h>		// For wxMessageBox
#endif

//-------------------------------------------------------------------

#include "otherfunctions.h"

//-------------------------------------------------------------------
#define CMD_ID_HELP		1
#define CMD_ID_STATS		2
#define CMD_ID_SHOW		3
#define CMD_ID_RESUME		4
#define CMD_ID_PAUSE		5
#define CMD_ID_SRVSTAT		6
#define CMD_ID_CONN		7
#define CMD_ID_DISCONN		8
#define CMD_ID_CONN_TO_SRV	9
#define CMD_ID_RELOAD_IPFILTER	10
#define CMD_ID_SET_IPFILTER	11

#define APP_INIT_SIZE_X 640
#define APP_INIT_SIZE_Y 480

#define theApp (*((CamulecmdApp*)wxTheApp))

static CmdId commands[] = {
	{ wxT("quit"),		CMD_ID_QUIT },
	{ wxT("exit"),		CMD_ID_QUIT },
	{ wxT("help"),		CMD_ID_HELP },
	{ wxT("stats"),		CMD_ID_STATS },
	{ wxT("show"),		CMD_ID_SHOW },
	{ wxT("pause"),		CMD_ID_PAUSE },
	{ wxT("resume"),	CMD_ID_RESUME },
	{ wxT("serverstatus"),	CMD_ID_SRVSTAT },
	{ wxT("connect"),	CMD_ID_CONN },
	{ wxT("disconnect"),	CMD_ID_DISCONN },
	{ wxT("serverconnect"),	CMD_ID_CONN_TO_SRV },
	{ wxT("reloadipf"),	CMD_ID_RELOAD_IPFILTER },
	{ wxT("setipfilter"),	CMD_ID_SET_IPFILTER },
	{ wxEmptyString,	0 },
};

//-------------------------------------------------------------------
IMPLEMENT_APP(CamulecmdApp)
//-------------------------------------------------------------------

//-------------------------------------------------------------------
#if wxUSE_GUI
//-------------------------------------------------------------------
// IDs for the controls and the menu commands
enum
{
    // menu items
    amulecmd_Quit = 1,

    // it is important for the id corresponding to the "About" command
    // to have this standard value as otherwise it won't be handled 
    // properly under Mac (where it is special and put into the "Apple" 
    // menu)
    amulecmd_About = wxID_ABOUT,
    Event_Comand_ID = 32001,
    amuleFrame_ID = 32000 
};


BEGIN_EVENT_TABLE(CamulecmdFrame, wxFrame)
	EVT_MENU(amulecmd_Quit, CamulecmdFrame::OnQuit)
	EVT_MENU(amulecmd_About, CamulecmdFrame::OnAbout)
	EVT_TEXT_ENTER(Event_Comand_ID, CamulecmdFrame::OnComandEnter)
	EVT_SIZE(CamulecmdFrame::OnSize)
END_EVENT_TABLE()

//-------------------------------------------------------------------

CamulecmdFrame::CamulecmdFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
       : wxFrame(NULL, amuleFrame_ID, title, pos, size, style)
{

    wxMenu *menuFile = new wxMenu;
    menuFile->Append(amulecmd_Quit, _T("E&xit\tAlt-X"), _T("Quit amulecmd"));

    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(amulecmd_About, _T("&About...\tF1"), _T("Show about dialog"));

    // now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(menuFile, _T("&File"));
    menuBar->Append(helpMenu, _T("&Help"));

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);

    log_text = new wxTextCtrl(this, -1, _T(""), wxPoint(2, 2), wxSize(APP_INIT_SIZE_X-4,APP_INIT_SIZE_Y-30-4), wxTE_MULTILINE | wxTE_READONLY);
    log_text->SetBackgroundColour(wxT("wheat"));   

    cmd_control = new wxTextCtrl(this,Event_Comand_ID, _T(""), wxPoint(2,APP_INIT_SIZE_Y-30-4), wxSize(APP_INIT_SIZE_X-4,30), wxTE_PROCESS_ENTER);
    cmd_control->SetBackgroundColour(wxT("wheat"));
}

void CamulecmdFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    // TRUE is to force the frame to close
    Show(_("\nOk, exiting Text Client...\n"));
    Close(TRUE);
}

void CamulecmdFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxString msg;
    msg.Printf( _T("amulecmd DLG version\n")
                _T("Using %s\n(c) aMule Dev Team"), wxVERSION_STRING);

    wxMessageBox(msg, _T("About amulecmd"), wxOK | wxICON_INFORMATION, this);
}

void CamulecmdFrame::OnComandEnter(wxCommandEvent& WXUNUSED(event)) { 

	if (cmd_control->GetLineLength(0) == 0) {
		return;
	}
	
	wxString buffer = cmd_control->GetLineText(0);
	
	if (theApp.Parse_Command(buffer, commands)) {
		Close(TRUE);
	}
	cmd_control->Clear();
}

void CamulecmdFrame::OnSize( wxSizeEvent& WXUNUSED(event) )
{
    int x = 0;
    int y = 0;
    GetClientSize( &x, &y );

    if (log_text) log_text->SetSize( 2, 2, x-4, y-30 - 4 );
    if (cmd_control) cmd_control->SetSize( 2, y-30-2, x-4,30);
}

void CamulecmdFrame::OnIdle(wxIdleEvent &WXUNUSED(event))
{
	theApp.MainThreadIdleNow();
}
//-------------------------------------------------------------------
#endif // wxUSE_GUI
//-------------------------------------------------------------------

#if wxUSE_GUI
void CamulecmdApp::LocalShow(const wxString &s)
{
	if (!frame) {
		return;
	}
	frame->log_text->AppendText(s);
}
#endif

int CamulecmdApp::ProcessCommand(int CmdId)
{
	long FileId;
	wxString msg;
	wxString args = GetCmdArgs();
	switch (CmdId) {
		case CMD_ID_HELP:
			ShowHelp();
			break;
			
		case CMD_ID_STATS:
			msg = wxT("STATS");
			break;
			
 		case CMD_ID_SRVSTAT:
			msg = wxT("CONNSTAT");
			break;
			
 		case CMD_ID_CONN:
			msg = wxT("RECONN");
			break;
			
 		case CMD_ID_DISCONN:
			msg = wxT("DISCONN");
			break;
			
		case CMD_ID_CONN_TO_SRV:
			msg = wxT("SERVER CONNECT ") + args;
			break;
			
		case CMD_ID_RELOAD_IPFILTER:
			msg = wxT("RELOADIPF");
			break;
			
		case CMD_ID_SET_IPFILTER:
			msg = wxT("SET IPFILTER ") + args;
			break;
			
		case CMD_ID_PAUSE:
			if (args.IsNumber()) {
				args.ToLong(&FileId);
				msg.Printf(wxT("PAUSE%li"), FileId);
			} else {
				Show(_("Not a valid number\n"));
				return 0;
			}
			break;
			
		case CMD_ID_RESUME:
			if (args.IsNumber()) {
				args.ToLong(&FileId);
				msg.Printf(wxT("RESUME%li"), FileId);
			} else {
				Show(_("Not a valid number\n"));
				return 0;
			}
			break;
			
		case CMD_ID_SHOW:
			if ( args.Left(2) == wxT("dl") ) {
				msg = wxT("DL_QUEUE");
			} else if ( args.Left(2) == wxT("ul") ) {
				msg = wxT("UL_QUEUE");
			} else {
				Show(_("Hint: Use Show DL or Show UL\n"));
				return 0;
			}
			break;
			
		default:
			return -1;
	}
	Process_Answer(SendRecvMsg(msg));
	
	return 0;
}

void CamulecmdApp::ShowHelp() {
	Show(_("\n->Help: Avalaible commands (case insensitive):\n\n"));	
	Show(_("help:\n\tShows this help.\n"));	
	Show(_("quit, exit:\n\tExits Textclient.\n"));	
	Show(_("stats:\n\tShows statistics.\n"));	
	Show(_("show DL:\n\tShows Download queue.\n"));	
	Show(_("resume n:\n\tResume file number n.\n"));
	Show(_("pause n:\n\tPauses file number n.\n"));
	Show(_("ServerStatus:\n\tTell us if connected/not connected.\n"));
	Show(_("connect:\n\tTries to connect to any server. WARNING: Doesn't warn if failed\n"));
	Show(_("disconnect:\n\tDisconnect from server.\n"));
	Show(_("server connect 'name' 'port':\n\tConnect to specified server and port.\n"));
	Show(_("ReloadIPF:\n\tReload IPFilter table from file.\n"));
	Show(_("Setipfilter on/off:\n\tTurn on/of amule IPFilter.\n"));
	Show(_("\n->End of listing\n"));
}

void CamulecmdApp::ShowGreet() {
	Show(_("\n---------------------------------\n"));
	Show(_(  "|       aMule text client       |\n"));
	Show(_(  "---------------------------------\n\n"));
	Show(_("\nUse 'Help' for command list\n\n"));
}

#if wxUSE_GUI
bool CamulecmdApp::OnInit() {
	CaMuleExternalConnector::OnInit();
	frame = new CamulecmdFrame(_T("amulecmd DLG"), wxPoint(50, 50), wxSize(APP_INIT_SIZE_X, APP_INIT_SIZE_Y));
	frame->Show(TRUE);
#else
int CamulecmdApp::OnRun() {
#endif
	ConnectAndRun(wxT("aMulecmd"), commands);
	
	return TRUE;
}

