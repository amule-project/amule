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
			

#include <cstdlib>
#include <cstdio>
#include <cstring>
#if !defined( __WXMSW__ )
	#include <unistd.h>
#else
	#define AMULECMDDLG 1
#endif
#include <wx/tokenzr.h>

#include "TextClient.h"

#include "MD5Sum.h"
#include "endianfix.h"
#include "ECSocket.h"

#define CMD_ID_QUIT		-1
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

#define char2unicode(x) wxConvCurrent->cMB2WX(x)
#define unicode2char(x) (const char*)wxConvCurrent->cWX2MB(x)

IMPLEMENT_APP(CamulecmdApp)

#ifdef AMULECMDDLG
// IDs for the controls and the menu commands
enum
{
    // menu items
    amulecmd_Quit = 1,

    // it is important for the id corresponding to the "About" command to have
    // this standard value as otherwise it won't be handled properly under Mac
    // (where it is special and put into the "Apple" menu)
    amulecmd_About = wxID_ABOUT,
    Event_Comand_ID = 32001,
    amuleFrame_ID = 32000 
};


BEGIN_EVENT_TABLE(CamulecmdFrame, wxFrame)
    EVT_MENU(amulecmd_Quit,  CamulecmdFrame::OnQuit)
    EVT_MENU(amulecmd_About, CamulecmdFrame::OnAbout)
    EVT_TEXT_ENTER(Event_Comand_ID, CamulecmdFrame::OnComandEnter)
    EVT_SIZE      (CamulecmdFrame::OnSize)
END_EVENT_TABLE()
#endif

wxString cmdargs;
ECSocket *m_ECClient = NULL;
bool isConnected = false;

void Show(const wxString& line) {
#ifndef AMULECMDDLG
	printf("%s", unicode2char(line));
#else
	theApp.frame->log_text->AppendText(line);
#endif
}

void ShowGreet() {
	Show(_("\n---------------------------------\n"));
	Show(_(  "|       aMule text client       |\n"));
	Show(_(  "---------------------------------\n\n"));
	Show(_("\nUse 'Help' for command list\n\n"));
}

void ShowHelp() {
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

typedef struct s_CmdId {
	const wxString cmd;
	int id;
} CmdId;

int GetIDFromString(wxString &buffer) {
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
		{ wxT("reloadipf"),	CMD_ID_RELOAD_IPFILTER  },
		{ wxT("setipfilter"),	CMD_ID_SET_IPFILTER },
		{ wxEmptyString, 0 },
	};
	wxStringTokenizer tokens(buffer);
	wxString cmd = tokens.GetNextToken().MakeLower();
	cmdargs = wxEmptyString;
	while ( tokens.HasMoreTokens() ) {
		cmdargs += tokens.GetNextToken().MakeLower();
	}
	
	register int i = 0;
	bool found = false;
	while ( !found && commands[i].cmd != wxEmptyString ) {
		found = commands[i].cmd == cmd;
		if (!found) {
			i++;
		}
	}
	
	return commands[i].id;
}

void Process_Answer(const wxString& answer) {
	wxStringTokenizer tokens(answer, wxT("\n"));
	wxString t;
	while ( tokens.HasMoreTokens() ) {
		Show(tokens.GetNextToken() + wxT("\n"));
	}
}

int ProcessCommand(int CmdId) {
	long FileId;
	wxString msg;
	switch (CmdId) {
		case CMD_ID_HELP:
			ShowHelp();
			return 0;
			
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
			msg = wxT("SERVER CONNECT ") + cmdargs;
			break;
			
		case CMD_ID_RELOAD_IPFILTER:
			msg = wxT("RELOADIPF");
			break;
			
		case CMD_ID_SET_IPFILTER:
			msg = wxT("SET IPFILTER ") + cmdargs;
			break;
			
		case CMD_ID_PAUSE:
			if (cmdargs.IsNumber()) {
				cmdargs.ToLong(&FileId);
				msg.Printf(wxT("PAUSE%li"), FileId);
			} else {
				Show(_("Not a valid number\n"));
				return -1;
			}
			break;
			
		case CMD_ID_RESUME:
			if (cmdargs.IsNumber()) {
				cmdargs.ToLong(&FileId);
				msg.Printf(wxT("RESUME%li"), FileId);
			} else {
				Show(_("Not a valid number\n"));
				return -1;
			}
			break;
			
		case CMD_ID_SHOW:
			if ( cmdargs.Left(2) == wxT("DL") ) {
				msg = wxT("DL_QUEUE");
			} else if ( cmdargs.Left(2) == wxT("UL") ) {
				msg = wxT("UL_QUEUE");
			} else {
				Show(_("Hint: Use Show DL or Show UL\n"));
				return -1;
			}
			break;
			
		default:
			return -1;
	}
	Process_Answer(m_ECClient->SendRecvMsg(msg));
	
	return 0;
}

bool Parse_Command(wxString &buffer) {
	int cmd_ID = GetIDFromString(buffer);
	bool quit = cmd_ID == CMD_ID_QUIT;
	if ( cmd_ID && !quit ) {
		if (ProcessCommand(cmd_ID) < 0) {
			Show(_("Error processing command - should never happen! Report bug, please\n"));
		}
	} else if (!cmd_ID) {
		Show(_("Syntax error!\n"));
	}
	
	return quit;
}

#ifndef AMULECMDDLG

void TextClientShell() {
	char buffer[256];
	wxString buf;

	bool The_End = false;
	do {
		Show(wxT("\naMule$ "));
		fflush(stdin);
		fgets(buffer, 256, stdin);
		buf = char2unicode(buffer);
		The_End = Parse_Command(buf);
	} while ((!The_End) && (isConnected));
}


bool CamulecmdApp::OnCmdLineParsed(wxCmdLineParser& amulecmd_parser) {
	
	bool result = true;
	
	// Call base class version to process standard command line options
	//result = wxApp::OnCmdLineParsed(amulecmd_parser);
	
	if ( !amulecmd_parser.Found(wxT("rh"), &hostName) ) {
		hostName = wxT("localhost");
	}
	
	long port;
	if (!amulecmd_parser.Found(wxT("p"),&port)) {
		//server = getenv("HOME") + wxString("/.aMule/muleconn");	;
		sPort = wxT("4712"); // default port
	} else {
		//server = wxString::Format("%li",port);
		sPort = wxString::Format(wxT("%li"),port);
	}
	
	m_HasCommandLinePassword = amulecmd_parser.Found(wxT("password"), &m_CommandLinePassword);
	
	return result;
}
#endif

#ifdef AMULECMDDLG
int CamulecmdApp::OnExit() {
/*
	if (conn) {
		delete conn;
	}
	if (client) {
    	delete client;
	}
*/
	m_ECClient->Destroy();
	return 0;
}

bool CamulecmdApp::OnInit() {

      frame = new CamulecmdFrame(_T("amulecmd DLG"),
                                 wxPoint(50, 50), wxSize(APP_INIT_SIZE_X, APP_INIT_SIZE_Y));

      // and show it (the frames, unlike simple controls, are not shown when
      // created initially)
      frame->Show(TRUE);
#else
int CamulecmdApp::OnRun() {
#endif
	Show(_("\nThis is amulecmd (TextClient)\n\n"));

	wxString * temp_wxpasswd;

#ifndef AMULECMDDLG
	const char* t_passwd;
	if ( m_HasCommandLinePassword ) {
		t_passwd = unicode2char(m_CommandLinePassword);
	} else {
		t_passwd = getpass("Enter password for mule connection (return if no pass defined): ");
	}
	if ( strlen(t_passwd) > 0 ) {
		temp_wxpasswd = new wxString(MD5Sum(char2unicode(t_passwd)).GetHash());
#else
	hostName = wxGetTextFromUser(_T("Enter hostname or ip of the box running aMule"),_T("Enter Hostname"),_T("localhost"));
	//server = wxGetTextFromUser(_T("Enter port for aMule's External Connection"),_T("Enter Port"),_T("4712"));
	sPort = wxGetTextFromUser(_T("Enter port for aMule's External Connection"),_T("Enter Port"),_T("4712"));
	temp_wxpasswd = new wxString(::wxGetPasswordFromUser(_T("Enter password for mule connection (OK if no pass defined)"),_T("Enter Password")));
	if (strlen(unicode2char(*temp_wxpasswd))>0) {
		temp_wxpasswd = new wxString(MD5Sum(*temp_wxpasswd).GetHash());
#endif
	} else 
		temp_wxpasswd = new wxString(wxT(""));

	wxString passwd = wxString::Format(wxT("aMulecmd %s"),temp_wxpasswd->GetData());
	//printf("pass |%s| MD5HASH = |%s|\n",t_passwd,temp_wxpasswd->GetData());;
	delete temp_wxpasswd;

	Show(_("\nCreating client...\n"));
	//client = new MuleClient;
	// Create the socket
	m_ECClient = new ECSocket();
	
	Show(_("Now, doing connection....\n"));

	wxIPV4address addr;
	addr.Hostname(hostName);
	addr.Service(sPort);
  
	Show(wxString::Format(_("Using host %s port %d\n"), addr.Hostname().GetData(), addr.Service()));
	m_ECClient->Connect(addr, FALSE);
	m_ECClient->WaitOnConnect(10);
	
	if (!m_ECClient->IsConnected())
		// no connection => close gracefully
		Show(_("Connection Failed. Unable to connect to the specified host\n"));
	else {
		//Authenticate ourself
		if (m_ECClient->SendRecvMsg(wxString::Format(wxT("AUTH %s"), passwd.GetData())) == wxT("Access Denied")) {
			Show(_("ExternalConn: Access Denied.\n"));
		} else {
			isConnected=true;
	    	Show(_("Succeeded ! Connection established\n"));
			ShowGreet();
#ifndef AMULECMDDLG
			TextClientShell();		
			Show(_("\nOk, exiting Text Client...\n"));
#endif
		}
	}
	
	m_ECClient->Close();
	
	return TRUE;
}

#ifdef AMULECMDDLG

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
	
	char* buffer = (char*) unicode2char(cmd_control->GetLineText(0));
	
	if (Parse_Command(buffer)) {
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
#endif

