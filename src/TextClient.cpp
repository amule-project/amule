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

#include <wx/intl.h>			// For _()
#if wxUSE_GUI
	#include <wx/menu.h>		// For wxMenu
	#include <wx/msgdlg.h>		// For wxMessageBox
	#include <wx/sizer.h>		// For wxBoxSizer
	#include <wx/statline.h>	// For wxStaticLine
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
#define CMD_ID_GET_IPLEVEL	12
#define CMD_ID_SET_IPLEVEL	13

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
	{ wxT("getiplevel"),	CMD_ID_GET_IPLEVEL },
	{ wxT("setiplevel"),	CMD_ID_SET_IPLEVEL },
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
END_EVENT_TABLE()

//-------------------------------------------------------------------

CamulecmdFrame::CamulecmdFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
       : wxFrame(NULL, amuleFrame_ID, title, pos, size, style)
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(amulecmd_Quit, _("E&xit\tAlt-X"), _("Quit amulecmd"));

	wxMenu *helpMenu = new wxMenu;
	helpMenu->Append(amulecmd_About, _("&About...\tF1"), _("Show about dialog"));

	// now append the freshly created menu to the menu bar...
	wxMenuBar *menuBar = new wxMenuBar();
	menuBar->Append(menuFile, _("&File"));
	menuBar->Append(helpMenu, _("&Help"));

	// ... and attach this menu bar to the frame
	SetMenuBar(menuBar);

	// Text controls and sizer
	wxBoxSizer *vsizer = new wxBoxSizer(wxVERTICAL);
	log_text = new wxTextCtrl(this, -1, wxEmptyString,
		wxDefaultPosition, wxSize(APP_INIT_SIZE_X, APP_INIT_SIZE_Y),
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
	cmd_control = new wxTextCtrl(this,Event_Comand_ID, wxEmptyString,
		wxPoint(2, APP_INIT_SIZE_Y-30-4),
		wxSize(APP_INIT_SIZE_X-4,30),
		wxTE_PROCESS_ENTER);
	cmd_control->SetBackgroundColour(wxT("wheat"));
vsizer->Add(cmd_control, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

	SetSizer(vsizer);
	vsizer->SetSizeHints(this);
}

void CamulecmdFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	// true is to force the frame to close
	Show(_("\nOk, exiting Text Client...\n"));
	Close(true);
}

void CamulecmdFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxString msg;
	msg.Printf(
		_("amulecmd DLG version\n"
		"Using %s\n(c) aMule Dev Team"),
		wxVERSION_STRING);
	wxMessageBox(msg, _("About amulecmd"), wxOK | wxICON_INFORMATION, this);
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

		case CMD_ID_GET_IPLEVEL:
			msg = wxT("GETIPLEVEL");
			break;

		case CMD_ID_SET_IPLEVEL:
			msg = wxT("SETIPLEVEL ") + args;
			break;
			
		default:
			return -1;
	}
	Process_Answer(SendRecvMsg(msg));
	
	return 0;
}

void CamulecmdApp::ShowHelp() {
	Show(_("\n->Help: Avalaible commands (case insensitive):\n\n"));	
	Show(wxString(wxT("help:\n\t")) + wxString(_("Shows this help.\n")));	
	Show(wxString(wxT("quit, exit:\n\t")) + wxString(_("Exits Textclient.\n")));	
	Show(wxString(wxT("stats:\n\t")) + wxString(_("Shows statistics.\n")));	
	Show(wxString(wxT("show DL:\n\t")) + wxString(_("Shows Download queue.\n")));	
	Show(wxString(wxT("resume n:\n\t")) + wxString(_("Resume file number n.\n")));
	Show(wxString(wxT("pause n:\n\t")) + wxString(_("Pauses file number n.\n")));
	Show(wxString(wxT("ServerStatus:\n\t")) + wxString(_("Tell us if connected/not connected.\n")));
	Show(wxString(wxT("connect:\n\t")) + wxString(_("Tries to connect to any server. WARNING: Doesn't warn if failed\n")));
	Show(wxString(wxT("disconnect:\n\t")) + wxString(_("Disconnect from server.\n")));
	Show(wxString(wxT("server connect 'name' 'port':\n\t")) + wxString(_("Connect to specified server and port.\n")));
	Show(wxString(wxT("ReloadIPF:\n\t")) + wxString(_("Reload IPFilter table from file.\n")));
	Show(wxString(wxT("Setipfilter on/off:\n\t")) + wxString(_("Turn on/of amule IPFilter.\n")));
	Show(wxString(wxT("GetIPLevel:\n\t")) + wxString(_("Shows current IP Filter level.\n")));
	Show(wxString(wxT("SetIPLevel <new level>:\n\t")) + wxString(_("Changes current IP Filter level.\n")));
	Show(_("\n->End of listing\n"));
}

void CamulecmdApp::ShowGreet() {
	Show(wxT("\n---------------------------------\n"));
	Show(wxString(wxT("|       ")) + wxString(_("aMule text client")) + wxString(wxT("       |\n")));
	Show(wxT("---------------------------------\n\n"));
	Show(_("\nUse 'Help' for command list\n\n"));
}

#if wxUSE_GUI
bool CamulecmdApp::OnInit() {
	CaMuleExternalConnector::OnInit();
	frame = new CamulecmdFrame(wxT("amulecmd DLG"), wxPoint(50, 50), wxSize(APP_INIT_SIZE_X, APP_INIT_SIZE_Y));
	frame->Show(TRUE);
#else
int CamulecmdApp::OnRun() {
#endif
	ConnectAndRun(wxT("aMulecmd"), commands);
	
	return TRUE;
}

