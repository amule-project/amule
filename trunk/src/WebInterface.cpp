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


#include "WebInterface.h"
#include "WebServer.h"
#include "ECSocket.h"
#include "MD5Sum.h"

#define CMD_ID_QUIT	-1
#define CMD_ID_HELP	1
#define CMD_ID_NOP	2
#define CMD_ID_STOP	3
#define CMD_ID_START	4
#define CMD_ID_RESTART	5


#define APP_INIT_SIZE_X 640
#define APP_INIT_SIZE_Y 480

#define char2unicode(x) wxConvCurrent->cMB2WX(x)
#define unicode2char(x) (const char*)wxConvCurrent->cWX2MB(x)

#if wxUSE_GUI && wxUSE_TIMER
	class MyTimer *mytimer;
#endif


IMPLEMENT_APP(CamulewebApp)


ECSocket *m_ECClient = NULL;
CWebServer *webserver = NULL;


#ifdef AMULEWEBDLG
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
//    EVT_TEXT(Event_Comand_ID, CamulewebFrame::OnCommandChange)
    EVT_TEXT_ENTER(Event_Comand_ID, CamulewebFrame::OnCommandEnter)
    EVT_SIZE      (CamulewebFrame::OnSize)
END_EVENT_TABLE()
#endif


//common functions -- start
#ifndef AMULEWEBDLG
void GetCommand(char* buffer, size_t buffer_size) {
	theApp.Print("amuleweb$ "); 
	fflush(stdin);
	fgets(buffer, buffer_size, stdin);
}
#else
void GetCommand(char* buffer, size_t buffer_size) {
        char const *text = wxGetTextFromUser(_T("Command?"), _T("Enter Command")).c_str();
	size_t len = strlen(text);
	if (len > buffer_size - 2) len = buffer_size - 2;
	strncpy(buffer, text, len);
	buffer[len] = '\n';
	buffer[len + 1] = 0;
}
#endif


int GetIDFromString(char* buffer) {
	if ((strncmp(buffer,"Quit",4) == 0) || (strncmp(buffer,"quit",4) == 0) || (strncmp(buffer,"Exit",4) == 0) || (strncmp(buffer,"exit",4) == 0)) {
		return(CMD_ID_QUIT);
	} else if ((strncmp(buffer,"Help",4) == 0) || (strncmp(buffer,"help",4) == 0)) {
		return(CMD_ID_HELP);
	} else if ((strncmp(buffer,"Stop",4) == 0) || (strncmp(buffer,"stop",4) == 0)) {
		return(CMD_ID_STOP);
	} else if ((strncmp(buffer,"Start",5) == 0) || (strncmp(buffer,"start",5) == 0)) {
		return(CMD_ID_START);
	} else if ((strncmp(buffer,"Restart",4) == 0) || (strncmp(buffer,"restart",4) == 0)) {
		return(CMD_ID_RESTART);		
	} else if (strncmp(buffer, "",0)==0) {
		return(CMD_ID_NOP);
	}

	return(0);
}


int ProcessCommand(int ID) {
	switch (ID) {
		case CMD_ID_HELP:
			theApp.Print("\n->Help: Avalaible commands :\n\n");	
			theApp.Print("Help: Shows this help.\t\t\t\t\tSyns: help\n");
			//theApp.Print("Start: Start web server.\t\t\t\tSyns: start\n");
			//theApp.Print("Stop: Stop web server.\t\t\t\t\tSyns: stop\n");
			//theApp.Print("Restart: Restart web server.\t\t\t\tSyns: restart\n");	
			theApp.Print("Quit: Exits Textclient.\t\t\t\t\tSyns: quit, Exit, exit\n");	
			theApp.Print("->End of listing\n");
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
			return(-1);
			break;
	}
	return(0);
}


bool Parse_Command(char *buffer) {
	int cmd_ID;
	cmd_ID = GetIDFromString(buffer);
	if (cmd_ID) {
		if (cmd_ID == CMD_ID_QUIT) {
			return true;
		} else if (cmd_ID == CMD_ID_NOP) {
			return false; //do nothing when hit enter
		} else {
			if (ProcessCommand(cmd_ID)<0) {
				theApp.Print("Error processing command - should never happen! Report bug, please");
			} 
		}
	} else 
		theApp.Print("Syntax error!");
	return false;
}


void WebInterfaceShell(void) {
	char buffer[256];
	bool The_End=false;
	
	while (!The_End) {
		GetCommand(buffer, sizeof(buffer));
		The_End = Parse_Command(buffer);
	}
}
//common functions -- end


#ifdef AMULEWEBDLG
CamulewebFrame::CamulewebFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
       : wxFrame(NULL, amuleFrame_ID, title, pos, size, style) {

    wxMenu *menuFile = new wxMenu;
    menuFile->Append(amuleweb_Quit, _T("E&xit\tAlt-X"), _T("Quit amuleweb"));

    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(amuleweb_About, _T("&About...\tF1"), _T("Show about dialog"));

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


void CamulewebFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
    // TRUE is to force the frame to close
    theApp.Print("\nOk, exiting Web Client...\n");
    Close(TRUE);
}


void CamulewebFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
    wxString msg;
    msg.Printf( _T("amuleweb [DLG version]\n")
                _T("Using %s\n(c) aMule Dev Team"), wxVERSION_STRING);

    wxMessageBox(msg, _T("About amuleweb"), wxOK | wxICON_INFORMATION, this);
}


void CamulewebFrame::OnCommandEnter(wxCommandEvent& WXUNUSED(event)) {
	if (cmd_control->GetLineLength(0) == 0) {
		return; 
	}
	
	char *buffer = (char *) cmd_control->GetLineText(0).c_str();
	
	if (Parse_Command(buffer)) {
		Close(TRUE);
	}

	cmd_control->Clear();
}


void CamulewebFrame::OnSize( wxSizeEvent& WXUNUSED(event) ) {
	int x = 0;
	int y = 0;
	GetClientSize( &x, &y );
	
	if (log_text) log_text->SetSize( 2, 2, x-4, y-30 - 4 );
	if (cmd_control) cmd_control->SetSize( 2, y-30-2, x-4,30);
}
#endif


#ifdef AMULEWEBDLG
int CamulewebApp::OnExit() {
	m_ECClient->Destroy();
	if (webserver) webserver->StopServer();
	return 0;
}
#endif


//prints output information
void CamulewebApp::Print(char *sFormat, ...) {
	char buffer[5000];

	va_list argptr;
	va_start(argptr, sFormat);
	vsnprintf(buffer, 5000, sFormat, argptr);
	va_end(argptr);
#ifndef AMULEWEBDLG
	printf("%s",buffer);
#else
	theApp.frame->log_text->AppendText(wxString::Format("%s",buffer));
#endif
}


#ifdef AMULEWEBDLG
bool CamulewebApp::OnInit() {
      frame = new CamulewebFrame(_T("amuleweb DLG"), wxPoint(50, 50), wxSize(APP_INIT_SIZE_X, APP_INIT_SIZE_Y));
      frame->Show(TRUE);
#else
int CamulewebApp::OnRun() {
#endif
	Print("\nThis is amuleweb (WebInterface)\n\n");

	wxString *temp_wxpasswd;

#ifndef AMULEWEBDLG
	char *t_passwd = getpass("Enter password for mule connection (return if no pass defined): ");
	if (strlen(t_passwd)>0) {
		temp_wxpasswd = new wxString(MD5Sum(char2unicode(t_passwd)).GetHash());
#else
	hostName = wxGetTextFromUser(_T("Enter hostname or ip of the box running aMule"), _T("Enter Hostname"), _T("localhost"));
	sPort = wxGetTextFromUser(_T("Enter port for aMule's External Connection"), _T("Enter Port"), _T("4712"));
	temp_wxpasswd = new wxString(::wxGetPasswordFromUser(_T("Enter password for mule connection (OK if no pass defined)"), _T("Enter Password")));
	if (strlen(temp_wxpasswd->GetData())>0) {
		temp_wxpasswd = new wxString(MD5Sum(*temp_wxpasswd).GetHash());
#endif
	} else 
		temp_wxpasswd = new wxString(wxT(""));

	wxString passwd = wxString::Format(wxT("aMuleweb %s"), temp_wxpasswd->GetData());
	delete temp_wxpasswd;

	Print("\nCreating client...\n");
	
	// Create the socket
	m_ECClient = new ECSocket();

	Print("Now doing connection...\n");

	wxIPV4address addr;
	addr.Hostname(hostName);
	addr.Service(sPort);

	Print("Using host %s port %d\n\n", addr.Hostname().GetData(), addr.Service());
	Print("Trying to connect (timeout = 10 sec) ...\n");
	
  	m_ECClient->Connect(addr, FALSE);
	m_ECClient->WaitOnConnect(10);

	if (!m_ECClient->IsConnected()) {
		// no connection => close gracefully
    		Print("Failed ! Unable to connect to the specified host\n");
	} else {
		//Authenticate ourself
		if (m_ECClient->SendRecvMsg(wxString::Format(wxT("AUTH %s"), passwd.GetData())) == wxT("Access Denied")) {
			Print("ExternalConn: Access Denied.\n");
		} else {
    		Print("Succeeded ! Connection established\n\n");
			Print("---------------------------------\n");
			Print("|       aMule Web Server        |\n");
			Print("---------------------------------\n\n\n");
			Print("Use 'Help' for command list\n\n");

			//Creating the web server
			webserver = new CWebServer(this);
			webserver->StartServer();

#ifndef AMULEWEBDLG
			WebInterfaceShell();
			Print("Ok, exiting WebServer...\n");
			
			webserver->StopServer();
#endif
		}
	}

#ifndef AMULEWEBDLG
	m_ECClient->Destroy();
#endif
	return TRUE;
}


#ifndef AMULEWEBDLG
bool CamulewebApp::OnCmdLineParsed(wxCmdLineParser& amuleweb_parser) {
	
	bool result = true;
	
	result = wxApp::OnCmdLineParsed(amuleweb_parser);
	
	wxString TempStr;
	TempStr = wxT("rh");
	if (!amuleweb_parser.Found(TempStr,&hostName)) {
		hostName = wxT("localhost");
	}
	
	long port;
	TempStr = wxT("p");
	if (!amuleweb_parser.Found(TempStr,&port)) {
		sPort=wxT("4712"); //get the default port
	} else {
		sPort=wxString::Format(wxT("%li"), port);
	}

	return result;
}
#endif


wxString CamulewebApp::SendRecvMsg(const wxChar *msg) {
	return m_ECClient->SendRecvMsg(msg);
}
