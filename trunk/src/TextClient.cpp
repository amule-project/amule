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

// This is kludgy test to warn people about the well-known wxBase rpm bug.
#if !defined(WXBASE) && !defined(__WXUNIVERSAL__) && !defined(__WXMSW__) && !defined(__WXMOTIF__) && \
    !defined(__WXGTK__) && !defined(__WXMAC__) && !defined(__WXPM__) && !defined(__WXSTUBS__)
#error ============================================================================
#error There is a problem with the WX_CXXFLAGS.
#error This is probably caused by having a link from wx-config to wxbase-*-config.
#error PLEASE MAKE SURE THAT YOUR wx-config POINTS TO THE CORRECT CONFIGURE SCRIPT!
#error Then run: config.status --recheck; config.status
#error ============================================================================
#endif

#include "TextClient.h"
#include "MD5Sum.h"
#include "endianfix.h"
#include "ECSocket.h"

#define CMD_ID_QUIT -1
#define CMD_ID_HELP 1
#define CMD_ID_STATS 2
#define CMD_ID_SHOW 3
#define CMD_ID_RESUME 4
#define CMD_ID_PAUSE 5
#define CMD_ID_SRVSTAT 6
#define CMD_ID_CONN 7
#define CMD_ID_DISCONN 8

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
//    EVT_TEXT(Event_Comand_ID, CamulecmdFrame::OnComandChange)
    EVT_TEXT_ENTER(Event_Comand_ID, CamulecmdFrame::OnComandEnter)
    EVT_SIZE      (CamulecmdFrame::OnSize)
END_EVENT_TABLE()
#endif

//MuleConnection *	conn = NULL;
//MuleClient *				client;
char *cmdargs = NULL;
ECSocket *m_ECClient = NULL;
bool isConnected = false;

/*
wxConnectionBase *MuleClient::OnMakeConnection()
{
    return new MuleConnection;
}

bool MuleConnection::OnAdvise(const wxString& topic, const wxString& item, wxChar *data, int size, wxIPCFormat format)
{
    return TRUE;
}
*/

void Show(char *Stringformat,...) {
	char bufferline[5000];

	va_list argptr;
	va_start(argptr, Stringformat);
	vsnprintf(bufferline, 5000, Stringformat, argptr);
	va_end(argptr);
	#ifndef AMULECMDDLG
	printf("%s",bufferline);
	#else
	theApp.frame->log_text->AppendText(wxString::Format(wxT("%s"),bufferline));
	//wxLogMessage(_T(bufferline));
	#endif
}

/*
bool MuleConnection::OnDisconnect()
{
	Show("Disconnected: aMule might have exited\n");
	#ifndef AMULECMDDLG
	theApp.OnExit();
	#else
	if (theApp.frame) {	
		wxCommandEvent Temp;
		theApp.frame->OnQuit(Temp);
	}
	#endif
 	conn = NULL;
    return wxConnection::OnDisconnect();
}
*/

void ShowGreet() {
	Show("\n---------------------------------\n");
	Show("|       aMule text client       |\n");
	Show("---------------------------------\n\n");
	Show("\nUse 'Help' for command list\n\n");
}

void ShowHelp() {
	Show("\n->Help: Avalaible commands :\n\n");	
	Show("Quit: Exits Textclient.\t\t\t\t\tSyns: quit, Exit, exit\n");	
	Show("Help: Shows this help.\t\t\t\t\tSyns: help\n");	
	Show("Stats: Shows statistics.\t\t\t\tSyns: stats\n");	
	Show("Show DL: Shows Download queue\n");	
	Show("Resume n: Resume file number n.\t\t\t\tSyns: resume\n");
	Show("Pause n: Pauses file number n.\t\t\t\tSyns: pause\n");
	Show("ServerStatus: Tell us if connected/not connected. \tSyns: serverstatus\n");
	Show("Connect: Tries to connect to any server.\t\tSyns: connect\n");
	Show("\tWARNING: Doesn't warn if failed\n.");
	Show("Disconnect: Disconnect from server. \t\t\tSyns: disconnect\n\n");
	Show("->End of listing\n");
}
#ifndef AMULECMDDLG
void GetCommand(char* buffer, size_t buffer_size) {
	Show("\naMule$ "); 
	fflush(stdin);
	fgets(buffer, buffer_size, stdin);
}
#endif

int GetIDFromString(char* buffer) {
	if ((strncmp(buffer,"Quit",4)==0) || (strncmp(buffer,"quit",4)==0) || (strncmp(buffer,"Exit",4)==0) || (strncmp(buffer,"exit",4) ==0)) {
		return(CMD_ID_QUIT);
	} else if ((strncmp(buffer,"Help",4)==0) || (strncmp(buffer,"help",4)==0)) {
		return(CMD_ID_HELP);
	} else if ((strncmp(buffer,"Stats",5)==0) || (strncmp(buffer,"stats",5)==0)) {
		return(CMD_ID_STATS);
	} else if ((strncmp(buffer,"Show",4)==0) || (strncmp(buffer,"show",4)==0)) {
		cmdargs = (buffer + 5);
		return(CMD_ID_SHOW);
	} else if ((strncmp(buffer,"pause",5)==0) || (strncmp(buffer,"Pause",5)==0)) {
		cmdargs = (buffer + 6);
		return(CMD_ID_PAUSE);
	} else if ((strncmp(buffer,"resume",6)==0) || (strncmp(buffer,"Resume",6)==0)) {
		cmdargs = (buffer + 7);
		return(CMD_ID_RESUME);
	} else if ((strncmp(buffer,"serverstatus",12)==0) || (strncmp(buffer,"ServerStatus",12)==0)) {
		return(CMD_ID_SRVSTAT);
	} else if ((strncmp(buffer,"connect",7)==0) || (strncmp(buffer,"Connect",7)==0)) {
		return(CMD_ID_CONN);
	} else if ((strncmp(buffer,"disconnect",10)==0) || (strncmp(buffer,"Disconnect",10)==0)) {
		return(CMD_ID_DISCONN);
	}
	return(0);
}

void Process_Answer(int ID,char* answer) {
	char* t;
	t=strtok(answer,"\n");
	while (t!=NULL) {
		Show("%s\n",t);
		t=strtok(NULL,"\n");
	}
}


int ProcessCommand(int ID) {
	int fileID;
	char reqbuffer [256];	
	switch (ID) {
				case CMD_ID_HELP:
					ShowHelp();
				    break;
 				case CMD_ID_STATS:
					//Process_Answer(CMD_ID_STATS, conn->Request("STATS", NULL));
					Process_Answer(CMD_ID_STATS, (char*) m_ECClient->SendRecvMsg(wxT("STATS")).GetData());
				break;
 				case CMD_ID_SRVSTAT:
					//Process_Answer(CMD_ID_SRVSTAT, conn->Request("CONNSTAT", NULL));
					Process_Answer(CMD_ID_SRVSTAT, (char*) m_ECClient->SendRecvMsg(wxT("CONNSTAT")).GetData());
				break;
 				case CMD_ID_CONN:
					//Process_Answer(CMD_ID_CONN, conn->Request("RECONN", NULL));
					Process_Answer(CMD_ID_CONN, (char*) m_ECClient->SendRecvMsg(wxT("RECONN")).GetData());
				break;
 				case CMD_ID_DISCONN:
					//Process_Answer(CMD_ID_DISCONN, conn->Request("DISCONN", NULL));
					Process_Answer(CMD_ID_DISCONN, (char*) m_ECClient->SendRecvMsg(wxT("DISCONN")).GetData());
				break;
 				case CMD_ID_PAUSE:
					if (sscanf(cmdargs,"%i",&fileID)) {
						sprintf(reqbuffer,"PAUSE%i",fileID);
				    	//Process_Answer(CMD_ID_PAUSE, conn->Request(reqbuffer, NULL));
						Process_Answer(CMD_ID_PAUSE, (char*) unicode2char(m_ECClient->SendRecvMsg(char2unicode(reqbuffer)))); 
					} else Show("Not a valid number\n");
				break;
 				case CMD_ID_RESUME:
					if (sscanf(cmdargs,"%i",&fileID)) {
						sprintf(reqbuffer,"RESUME%i",fileID);
				    	//Process_Answer(CMD_ID_RESUME, conn->Request(reqbuffer, NULL));
						Process_Answer(CMD_ID_RESUME, (char*) unicode2char(m_ECClient->SendRecvMsg(char2unicode(reqbuffer))));
					} else Show("Not a valid number\n");
				break;
 				case CMD_ID_SHOW:
					if (strncmp(cmdargs,"DL",2)==0) {
							//Process_Answer(CMD_ID_SHOW, conn->Request("DL_QUEUE", NULL));
						Process_Answer(CMD_ID_SHOW, (char*) m_ECClient->SendRecvMsg(wxT("DL_QUEUE")).GetData());
					} else if (strncmp(cmdargs,"UL",2)==0) {
							//Process_Answer(CMD_ID_SHOW, conn->Request("UL_QUEUE", NULL));
						Process_Answer(CMD_ID_SHOW, (char*) m_ECClient->SendRecvMsg(wxT("UL_QUEUE")).GetData());
					} else Show("Hint: Use Show DL or Show UL\n");
				break;
				default:
					return(-1);
					break;
			}
	return(0);
}
bool Parse_Command(char* buffer) {
	int cmd_ID;
	cmd_ID=GetIDFromString(buffer);
		if (cmd_ID) {
			if (cmd_ID==CMD_ID_QUIT) {
				return true;
			} else {
					if (ProcessCommand(cmd_ID)<0) {
						Show("Error processing command - should never happen! Report bug, please\n");
					} 
			}
		} else Show("Syntax error!\n");
	return false;
}

#ifndef AMULECMDDLG

void TextClientShell() {
	char buffer[256];

	bool The_End=false;
	do {
		GetCommand(buffer, sizeof(buffer));
		The_End = Parse_Command(buffer);
	} while ((!The_End) && (isConnected));
}


bool CamulecmdApp::OnCmdLineParsed(wxCmdLineParser& amulecmd_parser) {
	
	bool result = true;
	
	result = wxApp::OnCmdLineParsed(amulecmd_parser);
	
	wxString TempStr;
	TempStr = wxT("rh");
	if (!amulecmd_parser.Found(TempStr,&hostName)) {
		hostName = wxT("localhost");
	}
	
	long port;
	TempStr = wxT("p");
	if (!amulecmd_parser.Found(TempStr,&port)) {
		//server = getenv("HOME") + wxString("/.aMule/muleconn");	;
		sPort = wxT("4712"); // default port
	} else {
		//server = wxString::Format("%li",port);
		sPort = wxString::Format(wxT("%li"),port);
	}
	
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
	
	Show("\nThis is amulecmd (TextClient)\n\n");

	wxString * temp_wxpasswd;

	#ifndef AMULECMDDLG
	char* t_passwd;
	t_passwd = getpass("Enter password for mule connection (return if no pass defined): ");
	if (strlen(t_passwd)>0) {
		temp_wxpasswd = new wxString(MD5Sum(char2unicode(t_passwd)).GetHash());
	} else temp_wxpasswd = new wxString(wxT(""));
	#else
	hostName = wxGetTextFromUser(_T("Enter hostname or ip of the box running aMule"),_T("Enter Hostname"),_T("localhost"));
	//server = wxGetTextFromUser(_T("Enter port for aMule's External Connection"),_T("Enter Port"),_T("4713"));
	sPort = wxGetTextFromUser(_T("Enter port for aMule's External Connection"),_T("Enter Port"),_T("4713"));
	temp_wxpasswd = new wxString(::wxGetPasswordFromUser(_T("Enter password for mule connection (OK if no pass defined)"),_T("Enter Password")));
	if (strlen(unicode2char(temp_wxpasswd->GetData()))>0) {
		temp_wxpasswd = new wxString(MD5Sum(*temp_wxpasswd).GetHash());
	} else temp_wxpasswd = new wxString(wxT(""));
	#endif


	wxString passwd = wxString::Format(wxT("aMulecmd %s"),temp_wxpasswd->GetData());
	//printf("pass |%s| MD5HASH = |%s|\n",t_passwd,temp_wxpasswd->GetData());;
	delete temp_wxpasswd;

	Show("\nCreating client...\n");
	//client = new MuleClient;
	// Create the socket
	m_ECClient = new ECSocket();
	
	Show("Now, doing connection....\n");

	wxIPV4address addr;
	addr.Hostname(hostName);
	addr.Service(sPort);
  
	//Show("Using host %s port %s\n",hostName.c_str(), server.c_str());
	Show("Using host %s port %d\n", addr.Hostname().GetData(), addr.Service());
	m_ECClient->Connect(addr, FALSE);
	m_ECClient->WaitOnConnect(10);
	
	/*
	conn = (MuleConnection *) client->MakeConnection(hostName, server, passwd);
	if (!conn) {
		// no connection => close gracefully
		Show("aMule is not running, not accepting connections or wrong password supplied\n");
		} else {
		ShowGreet();

#ifndef AMULECMDDLG			
		TextClientShell();
		Show("\nOk, exiting Text Client...\n");
		conn->Disconnect();
	}
	
	if (conn) {
		delete conn;
	}
	if (client) {
    	delete client;
	}
#else
	}
#endif
	*/
	
	if (!m_ECClient->IsConnected())
		// no connection => close gracefully
		Show("Connection Failed. Unable to connect to the specified host\n");
	else {
		//Authenticate ourself
		if (m_ECClient->SendRecvMsg(wxString::Format(wxT("AUTH %s"), passwd.GetData())) == wxT("Access Denied")) {
			Show("ExternalConn: Access Denied.\n");
		} else {
			isConnected=true;
	    	Show("Succeeded ! Connection established\n");
			ShowGreet();
#ifndef AMULECMDDLG
			TextClientShell();		
			Show("\nOk, exiting Text Client...\n");
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
    Show("\nOk, exiting Text Client...\n");
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
	
	char* buffer = (char *) cmd_control->GetLineText(0).c_str();
	
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
