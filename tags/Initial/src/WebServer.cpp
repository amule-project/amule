//this file is part of aMule
//aMule Copyright (C)2003 aMule Team ( http://www.amule-project.net )
//This file Copyright (C)2003 Kry (elkry@sourceforge.net  http://www.amule-project.net )
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for VERSION
#endif

#include <cstdlib>
#include <cstdio>
#include <cstring>
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

#include "WebServer.h"
#include "MD5Sum.h"
#ifdef __FreeBSD__
#include "otherfunctions.h"	// Needed for atoll
#endif

#include "resource.h"		// Needed by strings.en
#include "strings.en"		// Strings
#include "WebSocket.h"		// Needed for StopSockets()
#include "otherstructs.h"	// Needed for TransferredData
#include "GetTickCount.h"	// Needed for GetTickCount

#define itemsof(x) (sizeof(x)/sizeof(x[0])) //shakraw

#define CMD_ID_QUIT		-1
#define CMD_ID_HELP		1
#define CMD_ID_NOP		2
#define CMD_ID_STOP		3
#define CMD_ID_START	4
#define CMD_ID_RESTART	5

#define APP_INIT_SIZE_X 640
#define APP_INIT_SIZE_Y 480

#if wxUSE_GUI && wxUSE_TIMER
class MyTimer *mytimer;
#endif

IMPLEMENT_APP(CamulewebApp)

wxSocketClient *m_ECClient = NULL;
CWebServer *webserver = NULL;

//shakraw - sends and receive string data to/from ECServer
wxString SendRecvMsg(const wxChar *msg) {
	wxString response("");

  	size_t len  = (wxStrlen(msg) + 1) * sizeof(wxChar);

	m_ECClient->SetFlags(wxSOCKET_WAITALL);
	m_ECClient->Write(&len, sizeof(size_t));
	m_ECClient->WriteMsg(msg, len);
	if (!m_ECClient->Error()) {
		// Wait until data available (will also return if the connection is lost)
		m_ECClient->WaitForRead(10);
	
		if (m_ECClient->IsData()) {
			m_ECClient->Read(&len, sizeof(size_t)); //read buffer size
			wxChar *result = new wxChar[len];
			m_ECClient->ReadMsg(result, len); // read buffer
 			if (!m_ECClient->Error()) {
				response.Append(result);
			}
			delete[] result;
		}
	}
	return(response);
}


#ifdef AMULEWEBDLG
// IDs for the controls and the menu commands
enum
{
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
//    EVT_TEXT(Event_Comand_ID, CamulewebFrame::OnComandChange)
    EVT_TEXT_ENTER(Event_Comand_ID, CamulewebFrame::OnComandEnter)
    EVT_SIZE      (CamulewebFrame::OnSize)
END_EVENT_TABLE()
#endif


void Show(char *Stringformat,...) {
	char bufferline[5000];

	va_list argptr;
	va_start(argptr, Stringformat);
	vsnprintf(bufferline, 5000, Stringformat, argptr);
	va_end(argptr);
#ifndef AMULEWEBDLG
	printf("%s",bufferline);
#else
	theApp.frame->log_text->AppendText(wxString::Format("%s",bufferline));
#endif
}


void ShowGreet() {
	Show("\n---------------------------------\n");
	Show("|       aMule Web Server        |\n");
	Show("---------------------------------\n\n");
	Show("\nUse 'Help' for command list\n\n");}

	
void ShowHelp() {
	Show("\n->Help: Avalaible commands :\n\n");	
	Show("Help: Shows this help.\t\t\t\t\tSyns: help\n");
	//Show("Start: Start web server.\t\t\t\tSyns: start\n");
	//Show("Stop: Stop web server.\t\t\t\t\tSyns: stop\n");
	//Show("Restart: Restart web server.\t\t\t\tSyns: restart\n");	
	Show("Quit: Exits Textclient.\t\t\t\t\tSyns: quit, Exit, exit\n");	
	Show("->End of listing\n");
}


#ifndef AMULEWEBDLG
void GetCommand(char* buffer, size_t buffer_size) {
	Show("aMule[web]$ "); 
	fflush(stdin);
	fgets(buffer, buffer_size, stdin);
}
#else
void GetCommand(char* buffer, size_t buffer_size) {
        char const* text = wxGetTextFromUser(_T("Command?"),_T("Enter Command")).c_str();
	size_t len = strlen(text);
	if (len > buffer_size - 2)
	  len = buffer_size - 2;
	strncpy(buffer, text, len);
	buffer[len] = '\n';
	buffer[len + 1] = 0;
}
#endif


int GetIDFromString(char* buffer) {
	if ((strncmp(buffer,"Quit",4)==0) || (strncmp(buffer,"quit",4)==0) || (strncmp(buffer,"Exit",4)==0) || (strncmp(buffer,"exit",4) ==0)) {
		return(CMD_ID_QUIT);
	} else if ((strncmp(buffer,"Help",4)==0) || (strncmp(buffer,"help",4)==0)) {
		return(CMD_ID_HELP);
	} else if ((strncmp(buffer,"Stop",4)==0) || (strncmp(buffer,"stop",4)==0)) {
		return(CMD_ID_STOP);
	} else if ((strncmp(buffer,"Start",5)==0) || (strncmp(buffer,"start",5)==0)) {
		return(CMD_ID_START);
	} else if ((strncmp(buffer,"Restart",4)==0) || (strncmp(buffer,"restart",4)==0)) {
		return(CMD_ID_RESTART);		
	} else if (strncmp(buffer, "",0)==0) {
		return(CMD_ID_NOP);
	}

	return(0);
}


void Process_Answer(int ID,char* answer) {
	char* t;
	t=strtok(answer,"\n");
	while (t!=NULL) {
		Show("%s",t);
		t=strtok(NULL,"\n");
	}
}


int ProcessCommand(int ID) {
	switch (ID) {
		case CMD_ID_HELP:
			ShowHelp();
			break;
		case CMD_ID_STOP:
			webserver->StopServer();
			break;
		case CMD_ID_START:
			webserver->StartServer();
			break;
		case CMD_ID_RESTART:
			webserver->RestartServer();
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
			} else if (cmd_ID==CMD_ID_NOP) {
				return false; //do nothing when hit enter
			} else {
					if (ProcessCommand(cmd_ID)<0) {
						Show("Error processing command - should never happen! Report bug, please");
					} 
			}
		} else Show("Syntax error!");
	return false;
}


void WebClientShell() {
	char buffer[256];

	bool The_End=false;
	do {
		GetCommand(buffer, sizeof(buffer));
		The_End = Parse_Command(buffer);
	} while (!The_End);
}


#ifndef AMULEWEBDLG
bool CamulewebApp::OnCmdLineParsed(wxCmdLineParser& amuleweb_parser) {
	
	bool result = true;
	
	result = wxApp::OnCmdLineParsed(amuleweb_parser);
	
	wxString TempStr;
	TempStr = "rh";
	if (!amuleweb_parser.Found(TempStr,&hostName)) {
		hostName = "localhost";
	}
	
	long port;
	TempStr = "p";
	if (!amuleweb_parser.Found(TempStr,&port)) {
		sPort="4712"; //get the default port
	} else {
		sPort=wxString::Format("%li", port);
	}

	return result;
}
#endif


#ifdef AMULEWEBDLG
int CamulewebApp::OnExit() {
	m_ECClient->Destroy();
	return 0;
}

bool CamulewebApp::OnInit() {
      frame = new CamulewebFrame(_T("amuleweb DLG"), wxPoint(50, 50), wxSize(APP_INIT_SIZE_X, APP_INIT_SIZE_Y));
      frame->Show(TRUE);
#else
int CamulewebApp::OnRun() {
#endif
	Show("\nThis is amuleweb (WebServer)\n\n");

	wxString *temp_wxpasswd;

#ifndef AMULEWEBDLG
	char* t_passwd;
	t_passwd = getpass("Enter password for mule connection (return if no pass defined): ");
	if (strlen(t_passwd)>0) {
		temp_wxpasswd = new wxString(MD5Sum(wxT(t_passwd)).GetHash());
	} else temp_wxpasswd = new wxString("");
	//printf("pass |%s| MD5HASH = |%s|\n",t_passwd,temp_wxpasswd->GetData());
#else
	hostName = wxGetTextFromUser(_T("Enter hostname or ip of the box running aMule"),_T("Enter Hostname"),_T("localhost"));
	sPort = wxGetTextFromUser(_T("Enter port for aMule's External Connection"),_T("Enter Port"),_T("4712"));
	temp_wxpasswd = new wxString(::wxGetPasswordFromUser(_T("Enter password for mule connection (OK if no pass defined)"),_T("Enter Password")));
	if (strlen(temp_wxpasswd->GetData())>0) {
		temp_wxpasswd = new wxString(MD5Sum(*temp_wxpasswd).GetHash());
	} else temp_wxpasswd = new wxString("");
#endif

	wxString passwd = wxString::Format("aMuleweb %s",temp_wxpasswd->GetData());
	delete temp_wxpasswd;

	Show("\nCreating client...\n");
	// Create the socket
	m_ECClient = new wxSocketClient();

	Show("Now doing connection...\n");

	wxIPV4address addr;
	addr.Hostname(hostName);
	addr.Service(sPort);

	Show("Using host %s port %d\n", addr.Hostname().GetData(), addr.Service());

	Show("\nTrying to connect (timeout = 10 sec) ...\n");
  	m_ECClient->Connect(addr, FALSE);
	m_ECClient->WaitOnConnect(10);

	if (!m_ECClient->IsConnected())
		// no connection => close gracefully
    	Show("Failed ! Unable to connect to the specified host\n");
	else {
		//Authenticate ourself
		if (SendRecvMsg(wxString::Format("AUTH %s", passwd.GetData())) == "Access Denied") {
			Show("ExternalConn: Access Denied.\n");
		} else {
    		Show("Succeeded ! Connection established\n");
			ShowGreet();

			//Creating the web server
			webserver = new CWebServer();
			webserver->StartServer();

#ifndef AMULEWEBDLG
			WebClientShell();
			Show("\nOk, exiting WebServer...\n");
#endif
		}
	}

	m_ECClient->Close();
	return TRUE;
}


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
    Show("\nOk, exiting Web Client...\n");
    Close(TRUE);
}


void CamulewebFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
    wxString msg;
    msg.Printf( _T("amuleweb DLG version\n")
                _T("Using %s\n(c) aMule Dev Team"), wxVERSION_STRING);

    wxMessageBox(msg, _T("About amuleweb"), wxOK | wxICON_INFORMATION, this);
}


void CamulewebFrame::OnComandEnter(wxCommandEvent& WXUNUSED(event)) {
	if (cmd_control->GetLineLength(0) == 0) {
		return;
	}
	
	char* buffer = (char *) cmd_control->GetLineText(0).c_str();
	
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


//---shakraw, webserver code below
#include "types.h"
#include "WebServer.h"
#include "ED2KLink.h"
#include "MD5Sum.h"
#include <stdlib.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

#define HTTPInit "Server: aMule\r\nPragma: no-cache\r\nExpires: 0\r\nCache-Control: no-cache, no-store, must-revalidate\r\nConnection: close\r\nContent-Type: text/html\r\n"
#define HTTPInitGZ "Server: aMule\r\nPragma: no-cache\r\nExpires: 0\r\nCache-Control: no-cache, no-store, must-revalidate\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Encoding: gzip\r\n"

#define WEB_SERVER_TEMPLATES_VERSION	4

static int dwThread = 0;


//shakraw, same as GetResString() from otherfunctions.h
wxString getResString(UINT nID) {
	for(unsigned int i=0; i < itemsof(_resStrings); ++i) {
		if(_resStrings[i].id==nID) {
			return(wxString(wxGetTranslation(_resStrings[i].resstr)));
		}
	}
	return("This String is Neat");
}

//shakraw, same as CastItoXBytes() from otherfunctions.h
wxString castItoXBytes(uint64 count) {
    wxString buffer;
	
    if (count < 1024)
        buffer.Printf("%.0f %s",(float)count,getResString(IDS_BYTES).GetData());
    else if (count < 1048576)
        buffer.Printf("%.0f %s",(float)count/1024,getResString(IDS_KBYTES).GetData());
    else if (count < 1073741824)
        buffer.Printf("%.2f %s",(float)count/1048576,getResString(IDS_MBYTES).GetData());
    else if (count < 1099511627776LL)
        buffer.Printf("%.2f %s",(float)count/1073741824,getResString(IDS_GBYTES).GetData());
    else
        buffer.Printf("%.3f %s",(float)count/1099511627776LL,getResString(IDS_TBYTES).GetData());
	
    return buffer;
}

//shakraw, same as toHex() from otherfunctions.h
inline BYTE toHex(const BYTE &x) {
	return x > 9 ? x + 55: x + 48;
}

//shakraw, same as URLEncode() from otherfunctions.h
wxString URLEncode(wxString sIn) {
    wxString sOut;
	
    const int nLen = sIn.Length() + 1;

    register LPBYTE pOutTmp = NULL;
    LPBYTE pOutBuf = NULL;
    register LPBYTE pInTmp = NULL;
    LPBYTE pInBuf =(LPBYTE)sIn.GetWriteBuf(nLen); //GetData(); //GetBuffer(nLen);
	
    //alloc out buffer
    pOutBuf = (LPBYTE)sOut.GetWriteBuf(nLen*3-2); //GetData(); //GetBuffer(nLen  * 3 - 2);//new BYTE [nLen  * 3];

    if(pOutBuf)
    {
        pInTmp	= pInBuf;
	pOutTmp = pOutBuf;
		
	// do encoding
	while (*pInTmp)
	{
	    if(isalnum(*pInTmp))
	        *pOutTmp++ = *pInTmp;
	    else
	        if(isspace(*pInTmp))
		    *pOutTmp++ = '+';
		else
		{
		    *pOutTmp++ = '%';
		    *pOutTmp++ = toHex(*pInTmp>>4);
		    *pOutTmp++ = toHex(*pInTmp%16);
		}
	    pInTmp++;
	}
	*pOutTmp = '\0';
	sOut=pOutBuf;
	//delete [] pOutBuf;
	//Out.ReleaseBuffer();
	sOut.UngetWriteBuf();
    }
    //sIn.ReleaseBuffer();
    sIn.UngetWriteBuf();
    return sOut;
}

//shakraw, same as LeadingZero() from otherfunctions.h
wxString leadingZero(uint32 units) {
	wxString temp;
	if (units<10) temp.Printf("0%i",units); else temp.Printf("%i",units);
	return temp;
}

//shakraw, same as CastSecondsToHM() from otherfunctions.h
wxString castSecondsToHM(sint32 count) {
	wxString buffer;
	if (count < 0)
		buffer = "?"; 
	else if (count < 60)
		buffer.Printf("%i %s",count,getResString(IDS_SECS).GetData()); 
	else if (count < 3600) 
		buffer.Printf("%i:%s %s",count/60,leadingZero(count-(count/60)*60).GetData(),getResString(IDS_MINS).GetData());
	else if (count < 86400) 
		buffer.Printf("%i:%s %s",count/3600,leadingZero((count-(count/3600)*3600)/60).GetData(),getResString(IDS_HOURS).GetData());
	else 
		buffer.Printf("%i %s %i %s",count/86400,getResString(IDS_DAYS).GetData(),(count-(count/86400)*86400)/3600,getResString(IDS_HOURS).GetData()); 
	return buffer;
} 


//returns web server listening port
int CWebServer::GetWSPort() {
	return(atoi(SendRecvMsg("PREFERENCES GETWSPORT").GetData()));
}


CWebServer::CWebServer(void) {
	m_Params.bShowUploadQueue = false;

	m_Params.DownloadSort = DOWN_SORT_NAME;
	m_Params.bDownloadSortReverse = false;
	m_Params.ServerSort = SERVER_SORT_NAME;
	m_Params.bServerSortReverse = false;
	m_Params.SharedSort = SHARED_SORT_NAME;
	m_Params.bSharedSortReverse = false;
		
	m_Params.sLastModified=wxString("");
	m_Params.sETag=wxString("");
	m_iSearchSortby=3;
	m_bSearchAsc=0;

	dwThread = 0; //GetCurrentThreadId();
	m_bServerWorking = false; // not running (listening) yet
}


void CWebServer::ReloadTemplates() {
	time_t t=time(NULL);
	char *s = new char[255];
	strftime(s, 255, "%a, %d %b %Y %H:%M:%S GMT", localtime(&t));
	m_Params.sLastModified = wxString(s);
	delete[] s;
	
	m_Params.sETag = MD5Sum(m_Params.sLastModified).GetHash();

	wxString sFile = getenv("HOME") + wxString("/.aMule/aMule.tmpl");
		
	if (!wxFileName::FileExists(sFile)) {
		// no file. do nothing.
		SendRecvMsg(wxString::Format("LOGGING ADDLOGLINE %d %s", true, getResString(IDS_WEB_ERR_CANTLOAD).GetData()));
		return;
	}

	wxFileInputStream input(sFile);
	if (input.Ok()) {
		wxTextInputStream file(input);
		wxString sAll;
		while (!input.Eof()) {
			wxString sLine;
			sLine=file.ReadString();

			sAll += sLine + "\n";
		}
		//file.Close();
		// it'll get closed sometime..
	
		wxString sVersion = _LoadTemplate(sAll,"TMPL_VERSION");
		long lVersion = atol(sVersion);
		if (lVersion < WEB_SERVER_TEMPLATES_VERSION) {
			SendRecvMsg(wxString::Format("LOGGING ADDLOGLINE %d %s", true, getResString(IDS_WEB_ERR_CANTLOAD).GetData()));
		} else {
			m_Templates.sHeader = _LoadTemplate(sAll,"TMPL_HEADER");
			m_Templates.sHeaderMetaRefresh = _LoadTemplate(sAll,"TMPL_HEADER_META_REFRESH");
			m_Templates.sHeaderStylesheet = _LoadTemplate(sAll,"TMPL_HEADER_STYLESHEET");
			m_Templates.sFooter = _LoadTemplate(sAll,"TMPL_FOOTER");
			m_Templates.sServerList = _LoadTemplate(sAll,"TMPL_SERVER_LIST");
			m_Templates.sServerLine = _LoadTemplate(sAll,"TMPL_SERVER_LINE");
			m_Templates.sTransferImages = _LoadTemplate(sAll,"TMPL_TRANSFER_IMAGES");
			m_Templates.sTransferList = _LoadTemplate(sAll,"TMPL_TRANSFER_LIST");
			m_Templates.sTransferDownHeader = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_HEADER");
			m_Templates.sTransferDownFooter = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_FOOTER");
			m_Templates.sTransferDownLine = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_LINE");
			m_Templates.sTransferDownLineGood = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_LINE_GOOD");
			m_Templates.sTransferUpHeader = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_HEADER");
			m_Templates.sTransferUpFooter = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_FOOTER");
			m_Templates.sTransferUpLine = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_LINE");
			m_Templates.sTransferUpQueueShow = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_QUEUE_SHOW");
			m_Templates.sTransferUpQueueHide = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_QUEUE_HIDE");
			m_Templates.sTransferUpQueueLine = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_QUEUE_LINE");
			m_Templates.sTransferBadLink = _LoadTemplate(sAll,"TMPL_TRANSFER_BAD_LINK");
			m_Templates.sDownloadLink = _LoadTemplate(sAll,"TMPL_DOWNLOAD_LINK");
			m_Templates.sSharedList = _LoadTemplate(sAll,"TMPL_SHARED_LIST");
			m_Templates.sSharedLine = _LoadTemplate(sAll,"TMPL_SHARED_LINE");
			m_Templates.sSharedLineChanged = _LoadTemplate(sAll,"TMPL_SHARED_LINE_CHANGED");
			m_Templates.sGraphs = _LoadTemplate(sAll,"TMPL_GRAPHS");
			m_Templates.sLog = _LoadTemplate(sAll,"TMPL_LOG");
			m_Templates.sServerInfo = _LoadTemplate(sAll,"TMPL_SERVERINFO");
			m_Templates.sDebugLog = _LoadTemplate(sAll,"TMPL_DEBUGLOG");
			m_Templates.sStats = _LoadTemplate(sAll,"TMPL_STATS");
			m_Templates.sPreferences = _LoadTemplate(sAll,"TMPL_PREFERENCES");
			m_Templates.sLogin = _LoadTemplate(sAll,"TMPL_LOGIN");
			m_Templates.sConnectedServer = _LoadTemplate(sAll,"TMPL_CONNECTED_SERVER");
			m_Templates.sAddServerBox = _LoadTemplate(sAll,"TMPL_ADDSERVERBOX");
			m_Templates.sWebSearch = _LoadTemplate(sAll,"TMPL_WEBSEARCH");
			m_Templates.sSearch = _LoadTemplate(sAll,"TMPL_SEARCH");
			m_Templates.iProgressbarWidth=atoi(_LoadTemplate(sAll,"PROGRESSBARWIDTH").GetData());
			m_Templates.sSearchHeader = _LoadTemplate(sAll,"TMPL_SEARCH_RESULT_HEADER");			
			m_Templates.sSearchResultLine = _LoadTemplate(sAll,"TMPL_SEARCH_RESULT_LINE");			
			m_Templates.sProgressbarImgs = _LoadTemplate(sAll,"PROGRESSBARIMGS");
			m_Templates.sProgressbarImgsPercent = _LoadTemplate(sAll,"PROGRESSBARPERCENTIMG");
			m_Templates.sClearCompleted = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_CLEARBUTTON");
			m_Templates.sCatArrow= _LoadTemplate(sAll,"TMPL_CATARROW");			

			m_Templates.sProgressbarImgsPercent.Replace("[PROGRESSGIFNAME]","%s");
			m_Templates.sProgressbarImgsPercent.Replace("[PROGRESSGIFINTERNAL]","%i");
			m_Templates.sProgressbarImgs.Replace("[PROGRESSGIFNAME]","%s");
			m_Templates.sProgressbarImgs.Replace("[PROGRESSGIFINTERNAL]","%i");
		}
	} else {
		SendRecvMsg(wxString::Format("LOGGING ADDLOGLINE %d %s", true, getResString(IDS_WEB_ERR_CANTLOAD).GetData()));
	}
}


CWebServer::~CWebServer(void) {
	if (m_bServerWorking)
		StopSockets();
}


wxString CWebServer::_LoadTemplate(wxString sAll, wxString sTemplateName) {
	wxString sRet = "";
	int nStart = sAll.Find("<--" + sTemplateName + "-->");
	int nEnd = sAll.Find("<--" + sTemplateName + "_END-->");
	if (nStart != -1 && nEnd != -1)	{
		nStart += sTemplateName.Length() + 7;
		sRet = sAll.Mid(nStart, nEnd - nStart - 1);
	}
	
	if (sRet.IsEmpty())	{
		if (sTemplateName=="TMPL_VERSION")
			printf("Can't find template version number!\nPlease replace aMule.tmpl with a newer version!");
			//theApp.amuledlg->AddLogLine(true,"Can't find template version number!\nPlease replace aMule.tmpl with a newer version!");
		printf("Failed to load template %s\n", sTemplateName.GetData());
		//theApp.amuledlg->AddDebugLogLine(false,"Failed to load template " + sTemplateName);
	}
	return sRet;
}


void CWebServer::RestartServer(void) { //shakraw - restart web socket listening (reload templates too)
	if (m_bServerWorking) {
		StopSockets();
	}
	StartSockets(this);
	Show("Web Server: Restarted\n");
}

void CWebServer::StopServer(void) { //shakraw - stop web socket listening
	if (m_bServerWorking) {
		m_bServerWorking = false;
		StopSockets();
		Show("Web Server: Stopped\n");
	} else
		Show("Web Server: not running\n");
}

void CWebServer::StartServer(void) { //shakraw - start web socket listening (reload templates)
	if (!m_bServerWorking) {
		ReloadTemplates();
		StartSockets(this);
		m_bServerWorking = true;
		Show("Web Server: Started\n");
	} else
		Show("Web Server: running\n");
}


void CWebServer::_RemoveServer(wxString sIP, wxString sPort) {
	wxString request = wxString("SERVER REMOVE ")+sIP+wxString(" ")+sPort;
	SendRecvMsg(request.GetData());
}


void CWebServer::_SetSharedFilePriority(wxString hash, uint8 priority) {	
	int prio = (int) priority;
	if (prio >= 0 && prio < 5) {
		SendRecvMsg(wxString::Format("SHAREDFILES SETAUTOUPPRIORITY %s %d", hash.GetData(), 0));
		SendRecvMsg(wxString::Format("SHAREDFILES SETUPPRIORITY %s %d", hash.GetData(), prio));
	} else if (prio == 5) {
		SendRecvMsg(wxString::Format("SHAREDFILES SETAUTOUPPRIORITY %s %d", hash.GetData(), 1));
		SendRecvMsg(wxString::Format("SHAREDFILES UPDATEAUTOUPPRIORITY %s", hash.GetData()));
	}
}

void CWebServer::AddStatsLine(UpDown* line) {
	m_Params.PointsForWeb.Add(line);
	if (m_Params.PointsForWeb.GetCount() > WEB_GRAPH_WIDTH) {
		delete m_Params.PointsForWeb[0];
		m_Params.PointsForWeb.RemoveAt(0);
	}
}

wxString CWebServer::_SpecialChars(wxString str) {
	str.Replace("&","&amp;");
	str.Replace("<","&lt;");
	str.Replace(">","&gt;");
	str.Replace("\"","&quot;");
	return str;
}

void CWebServer::_ConnectToServer(wxString sIP, wxString sPort) {
	wxString request = wxString("SERVER CONNECT ")+sIP+wxString(" ")+sPort;
	SendRecvMsg(request.GetData());
}

void CWebServer::ProcessImgFileReq(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL) return;
		
	wxString filename=Data.sURL;
	wxString contenttype;

	printf("inc. fname=%s\n",filename.GetData());
	if (filename.Right(4).MakeLower()==".gif") contenttype="Content-Type: image/gif\r\n";
	else if (filename.Right(4).MakeLower()==".jpg"  || filename.Right(5).MakeLower()==".jpeg") contenttype="Content-Type: image/jpg\r\n";
	else if (filename.Right(4).MakeLower()==".bmp") contenttype="Content-Type: image/bmp\r\n";
	else if (filename.Right(4).MakeLower()==".png") contenttype="Content-Type: image/png";
	//DonQ - additional filetypes
	else if (filename.Right(4).MakeLower()==".ico") contenttype="Content-Type: image/x-icon\r\n";
	else if (filename.Right(4).MakeLower()==".css") contenttype="Content-Type: text/css\r\n";
	else if (filename.Right(3).MakeLower()==".js") contenttype="Content-Type: text/javascript\r\n";
		
	contenttype += "Last-Modified: " + pThis->m_Params.sLastModified + "\r\n" + "ETag: " + pThis->m_Params.sETag + "\r\n";
		
	filename=filename.Right(filename.Length()-1);
	//filename.Replace("/","\\");
	//filename=wxString(theApp.glob_prefs->GetAppDir())+"webserver/"+filename;
	filename=getenv("HOME") + wxString("/.aMule/webserver/") + wxString(filename);
	printf("**** imgrequest: %s\n",filename.GetData());

	if (!wxFileName::FileExists(filename)) {
		printf("**** imgrequest: file %s does not exists\n", filename.GetData());
	}

	wxFileInputStream* fis = new wxFileInputStream(filename);
	if (fis->Ok()) {
		fis->SeekI(0,wxFromEnd);
		off_t koko=fis->TellI();
		fis->SeekI(0,wxFromStart);
		char* buffer=new char[koko];
		fis->Read((void*)buffer,koko);
		Data.pSocket->SendContent((char*)contenttype.GetData(),(void*)buffer,fis->LastRead());
		delete fis;
		delete[] buffer;
	}
}

void CWebServer::ProcessStyleFileReq(ThreadData Data) {
	wxString filename=Data.sURL;
	wxString contenttype;

	printf("inc. fname=%s\n",filename.GetData());
	contenttype="Content-Type: text/css\r\n";

	filename=filename.Right(filename.Length()-1);
	//filename.Replace("/","\\");
	//filename=wxString(theApp.glob_prefs->GetAppDir())+"webserver/"+filename;
	filename=getenv("HOME") + wxString("/.aMule/webserver/") + wxString(filename);
	printf("**** cssrequest: %s\n",filename.GetData());

	if (wxFileName::FileExists(filename)) {
		wxFileInputStream* fis = new wxFileInputStream(filename);
		if(fis->Ok()) {
			fis->SeekI(0,wxFromEnd);
			off_t koko=fis->TellI();
			fis->SeekI(0,wxFromStart);
			char* buffer=new char[koko];
			fis->Read((void*)buffer,koko);
			Data.pSocket->SendContent((char*)contenttype.GetData(),(void*)buffer,fis->LastRead());
			delete fis;
			delete[] buffer;
		}
	} else {
		printf("**** imgrequest: file %s does not exists\n", filename.GetData());
	}
}

void CWebServer::ProcessURL(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return;

	bool isUseGzip = (atoi(SendRecvMsg("PREFS GETWEBUSEGZIP").GetData()) == 0) ? false : true;
	wxString Out = "";
	wxString OutE = "";	// List Entry Templates
	wxString OutE2 = "";
	wxString OutS = "";	// ServerStatus Templates
	TCHAR *gzipOut = NULL;
	long gzipLen=0;

	wxString HTTPProcessData = "";
	wxString HTTPTemp = "";
	//char	HTTPTempC[100] = "";
	srand ( time(NULL) );

	long lSession = 0;
	wxString sSes = _ParseURL(Data.sURL, "ses");
	if (sSes.Cmp("") != 0) lSession = atol(sSes.GetData());
	wxString sSession; sSession.Printf("%ld", lSession);

	wxString sW = _ParseURL(Data.sURL, "w");

	// WE CANT TOUCH THE MAIN THREAD'S GUI!!!
	if (sW == "password") {
		bool login=false;
		wxString ip=inet_ntoa( Data.inadr );

		wxString strAuth = SendRecvMsg(wxString::Format("PREFS GETWSPASS %s", MD5Sum(_ParseURL(Data.sURL, "p")).GetHash().GetData()).GetData());
		if (strAuth == "AdminLogin") {
			Session* ses=new Session();
			ses->admin=true;
			ses->startTime = time(NULL);//CTime::GetCurrentTime();
			ses->lSession = lSession = rand() * 10000L + rand();
			pThis->m_Params.Sessions.Add(ses);
			login=true;
		} else if (strAuth == "GuestLogin") {
			Session* ses=new Session();
			ses->admin=false;
			ses->startTime = time(NULL);//CTime::GetCurrentTime();
			ses->lSession = lSession = rand() * 10000L + rand();
			pThis->m_Params.Sessions.Add(ses);
			login=true;
		} else {
			TransferredData newban={inet_addr(ip), ::GetTickCount()}; // save failed attempt (ip,time)
			pThis->m_Params.badlogins.Add(&newban);
			login=false;
		}
		isUseGzip = false; // [Julien]
		if (login) {
			uint32 ipn=inet_addr( ip) ;
			for (int i = 0; i < pThis->m_Params.badlogins.GetSize();) {
				if (ipn == pThis->m_Params.badlogins[i]->datalen) {
					pThis->m_Params.badlogins.RemoveAt(i);
				} else 
					i++;
			}
		}
	}

	if (sW == "logout")
		_RemoveSession(Data, lSession);
		
	if (_IsLoggedIn(Data, lSession)) {
		Out += _GetHeader(Data, lSession);
		
		wxString sPage = sW; //_ParseURL(Data.sURL, "w");
		printf("***** logged in, getting page %s\n", sPage.GetData());
		printf("***** session is %s\n", sSession.GetData());
		
		if (sPage == "server") {
			Out += _GetServerList(Data);
		} else if (sPage == "download") {
			Out += _GetDownloadLink(Data);
		} else if (sPage == "shared") { 
			Out += _GetSharedFilesList(Data);
		} else if (sPage == "transfer") {
			Out += _GetTransferList(Data);
		} else if (sPage == "websearch") {
			Out += _GetWebSearch(Data);
		} else if (sPage == "search") {
			Out += _GetSearch(Data);
		} else if (sPage == "graphs") {
			Out += _GetGraphs(Data);
		} else if (sPage == "log") {
			Out += _GetLog(Data);
		} else if (sPage == "sinfo") {
			Out += _GetServerInfo(Data);
		} else if (sPage == "debuglog") {
			Out += _GetDebugLog(Data);
		} else if (sPage == "stats") {
			Out += _GetStats(Data);
		} else if (sPage == "options") {
			Out += _GetPreferences(Data);
		}
		
		Out += _GetFooter(Data);

		if (sPage == "")
			isUseGzip = false;

		if (isUseGzip) {
			bool bOk = false;
			try {
				uLongf destLen = Out.Length() + 1024;
				gzipOut = new TCHAR[destLen];
				if(_GzipCompress((Bytef*)gzipOut, &destLen, (const Bytef*)(TCHAR*)Out.GetData(), Out.Length(), Z_DEFAULT_COMPRESSION) == Z_OK) {
					bOk = true;
					gzipLen = destLen;
				}
			} catch(...) {
			}
			
			if (!bOk) {
				isUseGzip = false;
				if (gzipOut != NULL) {
					delete[] gzipOut;
					gzipOut = NULL;
				}
			}
		}
	} else {
		isUseGzip = false;
		
		uint32 ip = inet_addr(inet_ntoa( Data.inadr ));
		uint32 faults=0;

		// check for bans
		for (int i = 0; i < pThis->m_Params.badlogins.GetSize();i++)
			if (pThis->m_Params.badlogins[i]->datalen==ip) faults++;

		if (faults>4) {
			Out += _GetPlainResString(IDS_ACCESSDENIED);
				
			// set 15 mins ban by using the badlist
			TransferredData preventive={ip, ::GetTickCount() + (15*60*1000) };
			for (int i=0;i<=5;i++)
				pThis->m_Params.badlogins.Add(&preventive);
		} else
			Out += _GetLoginScreen(Data);
	}

	// send answer ...
	if (!isUseGzip)	{
		Data.pSocket->SendContent(HTTPInit, Out, Out.Length());
	} else {
		Data.pSocket->SendContent(HTTPInitGZ, gzipOut, gzipLen);
	}
	if (gzipOut != NULL)
		delete[] gzipOut;
}

wxString CWebServer::_ParseURLArray(wxString URL, wxString fieldname) {
	wxString res,temp;

	while (URL.Length()>0) {
		int pos=URL.MakeLower().Find(fieldname.MakeLower() +"=");
		if (pos>-1) {
			temp=_ParseURL(URL,fieldname);
			if (temp=="") break;
			res.Append(temp+"|");
			URL.Remove(pos,10);
		} else break;
	}
	return res;
}

wxString CWebServer::_ParseURL(wxString URL, wxString fieldname) {
	wxString value = "";
	wxString Parameter = "";
	char fromReplace[4] = "";	// decode URL
	char toReplace[2] = "";		// decode URL
	int i = 0;
	int findPos = -1;
	int findLength = 0;

	printf("*** parsing url %s :: field %s\n",URL.GetData(),fieldname.GetData());
	if (URL.Find("?") > -1) {
		Parameter = URL.Mid(URL.Find("?")+1, URL.Length()-URL.Find("?")-1);

		// search the fieldname beginning / middle and strip the rest...
		if (Parameter.Find(fieldname + "=") == 0) {
			findPos = 0;
			findLength = fieldname.Length() + 1;
		}
		if (Parameter.Find("&" + fieldname + "=") > -1) {
			findPos = Parameter.Find("&" + fieldname + "=");
			findLength = fieldname.Length() + 2;
		}
		if (findPos > -1) {
			Parameter = Parameter.Mid(findPos + findLength, Parameter.Length());
			if (Parameter.Find("&") > -1) {
				Parameter = Parameter.Mid(0, Parameter.Find("&"));
			}
	
			value = Parameter;

			// decode value ...
			value.Replace("+", " ");
			for (i = 0 ; i <= 255 ; i++) {
				sprintf(fromReplace, "%%%02x", i);
				toReplace[0] = (char)i;
				toReplace[1] = 0;
				value.Replace(fromReplace, toReplace);
				sprintf(fromReplace, "%%%02X", i);
				value.Replace(fromReplace, toReplace);
			}
		}
	}
	printf("*** URL parsed. returning %s\n",value.GetData());
	return value;
}

wxString CWebServer::_GetHeader(ThreadData Data, long lSession) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	wxString sSession; sSession.Printf("%ld", lSession);

	wxString Out = pThis->m_Templates.sHeader;

	Out.Replace("[CharSet]", _GetWebCharSet());

	//shakraw - page header
	wxString sHeaderList = SendRecvMsg("WEBPAGE HEADER");
	int brk=sHeaderList.First("\t");
	
	int nRefresh = atoi(sHeaderList.Left(brk).GetData());
	sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
	
	if (nRefresh) {
		wxString sPage = _ParseURL(Data.sURL, "w");
		if ((sPage == "transfer") || (sPage == "server") ||
			(sPage == "graphs") || (sPage == "log") ||
			(sPage == "sinfo") || (sPage == "debuglog") ||
			(sPage == "stats")) {
			wxString sT = pThis->m_Templates.sHeaderMetaRefresh;
			wxString sRefresh; sRefresh.Printf("%d", nRefresh);
			sT.Replace("[RefreshVal]", sRefresh);
			
			wxString catadd="";
			if (sPage == "transfer")
				catadd="&cat="+_ParseURL(Data.sURL, "cat");
			sT.Replace("[wCommand]", sPage+catadd);
			
			Out.Replace("[HeaderMeta]", sT);
		}
	}
	Out.Replace("[Session]", sSession);
	printf("*** replaced session with %s\n",sSession.GetData());
	Out.Replace("[HeaderMeta]", ""); // In case there are no meta
	Out.Replace("[aMuleAppName]", "aMule");
	Out.Replace("[version]", VERSION); //shakraw - was CURRENT_VERSION_LONG);
	Out.Replace("[StyleSheet]", pThis->m_Templates.sHeaderStylesheet);
	Out.Replace("[WebControl]", _GetPlainResString(IDS_WEB_CONTROL));
	Out.Replace("[Transfer]", _GetPlainResString(IDS_CD_TRANS));
	Out.Replace("[Server]", _GetPlainResString(IDS_SV_SERVERLIST));
	Out.Replace("[Shared]", _GetPlainResString(IDS_SHAREDFILES));
	Out.Replace("[Download]", _GetPlainResString(IDS_SW_LINK));
	Out.Replace("[Graphs]", _GetPlainResString(IDS_GRAPHS));
	Out.Replace("[Log]", _GetPlainResString(IDS_SV_LOG));
	Out.Replace("[ServerInfo]", _GetPlainResString(IDS_SV_SERVERINFO));
	Out.Replace("[DebugLog]", _GetPlainResString(IDS_SV_DEBUGLOG));
	Out.Replace("[Stats]", _GetPlainResString(IDS_SF_STATISTICS));
	Out.Replace("[Options]", _GetPlainResString(IDS_EM_PREFS));
	Out.Replace("[Logout]", _GetPlainResString(IDS_WEB_LOGOUT));
	Out.Replace("[Search]", _GetPlainResString(IDS_SW_SEARCHBOX));

	char HTTPTempC[100] = "";
	wxString sConnected = "";
	
	if (sHeaderList.Left(brk) == "Connected") {
		sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
		if (sHeaderList.Left(brk) == "High ID") 
			sConnected = _GetPlainResString(IDS_CONNECTED);
		else
			sConnected = _GetPlainResString(IDS_CONNECTED) + " (" + _GetPlainResString(IDS_IDLOW) + ")";
		
		sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
		sConnected += ": " + sHeaderList.Left(brk);
		sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
		sprintf(HTTPTempC, "%s ", sHeaderList.Left(brk).GetData());
		sConnected += " [" + wxString(HTTPTempC) + _GetPlainResString(IDS_LUSERS) + "]";
	} else if (sHeaderList.Left(brk) == "Connecting") {
		sConnected = _GetPlainResString(IDS_CONNECTING);
	} else {
		sConnected = _GetPlainResString(IDS_DISCONNECTED);
		if (IsSessionAdmin(Data,sSession))
			sConnected+=" (<small><a href=\"?ses=" + sSession + "&w=server&c=connect\">"+_GetPlainResString(IDS_CONNECTTOANYSERVER)+"</a></small>)";
	}
	Out.Replace("[Connected]", "<b>"+_GetPlainResString(IDS_PW_CONNECTION)+":</b> "+sConnected);

	sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
	double ul_speed = atof(sHeaderList.Left(brk).GetData());
	sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
	double dl_speed = atof(sHeaderList.Left(brk).GetData());

	sprintf(HTTPTempC, _GetPlainResString(IDS_UPDOWNSMALL),ul_speed, dl_speed);

	wxString sLimits;
	// EC 25-12-2003
	sHeaderList = sHeaderList.Mid(brk+1); brk=sHeaderList.First("\t");
	wxString MaxUpload = sHeaderList.Left(brk);
	sHeaderList = sHeaderList.Mid(brk+1);
	wxString MaxDownload = sHeaderList.Left(brk);
	
	if (MaxUpload == "65535")  MaxUpload = getResString(IDS_PW_UNLIMITED);
	if (MaxDownload == "65535") MaxDownload = getResString(IDS_PW_UNLIMITED);
	sLimits.Printf("%s/%s", MaxUpload.GetData(), MaxDownload.GetData());
	// EC Ends
	Out.Replace("[Speed]", "<b>"+_GetPlainResString(IDS_DL_SPEED)+":</b> "+wxString(HTTPTempC) + "<small> (" + _GetPlainResString(IDS_PW_CON_LIMITFRM) + ": " + sLimits + ")</small>");

	return Out;
}

wxString CWebServer::_GetFooter(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	return pThis->m_Templates.sFooter;
}

wxString CWebServer::_GetServerList(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data.sURL, "ses");

	wxString sAddServerBox = "";

	wxString sCmd = _ParseURL(Data.sURL, "c");
	if (sCmd == "connect" && IsSessionAdmin(Data,sSession) ) {
		wxString sIP = _ParseURL(Data.sURL, "ip");
		if (sIP.IsEmpty()) {
			SendRecvMsg("SERVER RE-CONNECT");
		} else {
			wxString sPort = _ParseURL(Data.sURL, "port");
			if (sPort.IsEmpty()) sPort = "4661"; //try default port
			_ConnectToServer(sIP, sPort);
		}
	} else if (sCmd == "disconnect" && IsSessionAdmin(Data,sSession)) {
		SendRecvMsg("SERVER DISCONNECT");
	} else if (sCmd == "remove" && IsSessionAdmin(Data,sSession)) {
		wxString sIP = _ParseURL(Data.sURL, "ip");
		if (!sIP.IsEmpty()) {
			wxString sPort = _ParseURL(Data.sURL, "port");
			if (sPort.IsEmpty()) sPort = "4661"; //try default port
			_RemoveServer(sIP, sPort);
		}
	} else if (sCmd == "options") {
		sAddServerBox = _GetAddServerBox(Data);
	}
	
	wxString sSort = _ParseURL(Data.sURL, "sort");
	if (sSort != "") {
		if (sSort == "name")
			pThis->m_Params.ServerSort = SERVER_SORT_NAME;
		else if(sSort == "description")
			pThis->m_Params.ServerSort = SERVER_SORT_DESCRIPTION;
		else if(sSort == "ip")
			pThis->m_Params.ServerSort = SERVER_SORT_IP;
		else if(sSort == "users")
			pThis->m_Params.ServerSort = SERVER_SORT_USERS;
		else if(sSort == "files")
			pThis->m_Params.ServerSort = SERVER_SORT_FILES;

		if(_ParseURL(Data.sURL, "sortreverse") == "")
			pThis->m_Params.bServerSortReverse = false;
	}
	
	wxString sSortRev = _ParseURL(Data.sURL, "sortreverse");
	if (sSortRev != "") {
		pThis->m_Params.bServerSortReverse = (sSortRev == "true");
	}
	
	wxString sServerSortRev;
	if (pThis->m_Params.bServerSortReverse)
		sServerSortRev = "false";
	else
		sServerSortRev = "true";

	wxString Out = pThis->m_Templates.sServerList;
	Out.Replace("[ConnectedServerData]", _GetConnectedServer(Data));
	Out.Replace("[AddServerBox]", sAddServerBox);
	Out.Replace("[Session]", sSession);
	
	if (pThis->m_Params.ServerSort == SERVER_SORT_NAME)
		Out.Replace("[SortName]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortName]", "");
	
	if (pThis->m_Params.ServerSort == SERVER_SORT_DESCRIPTION)
		Out.Replace("[SortDescription]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortDescription]", "");
	
	if (pThis->m_Params.ServerSort == SERVER_SORT_IP)
		Out.Replace("[SortIP]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortIP]", "");
	
	if (pThis->m_Params.ServerSort == SERVER_SORT_USERS)
		Out.Replace("[SortUsers]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortUsers]", "");
	
	if (pThis->m_Params.ServerSort == SERVER_SORT_FILES)
		Out.Replace("[SortFiles]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortFiles]", "");
	
	Out.Replace("[ServerList]", _GetPlainResString(IDS_SV_SERVERLIST));
	Out.Replace("[Servername]", _GetPlainResString(IDS_SL_SERVERNAME));
	Out.Replace("[Description]", _GetPlainResString(IDS_DESCRIPTION));
	Out.Replace("[Address]", _GetPlainResString(IDS_IP));
	Out.Replace("[Connect]", _GetPlainResString(IDS_IRC_CONNECT));
	Out.Replace("[Users]", _GetPlainResString(IDS_LUSERS));
	Out.Replace("[Files]", _GetPlainResString(IDS_LFILES));
	Out.Replace("[Actions]", _GetPlainResString(IDS_WEB_ACTIONS));
		
	wxString OutE = pThis->m_Templates.sServerLine;
	OutE.Replace("[Connect]", _GetPlainResString(IDS_IRC_CONNECT));
	OutE.Replace("[RemoveServer]", _GetPlainResString(IDS_REMOVETHIS));
	OutE.Replace("[ConfirmRemove]", _GetPlainResString(IDS_WEB_CONFIRM_REMOVE_SERVER));

	CArray<ServerEntry*, ServerEntry*> ServerArray;

	// Populating array
	wxString sServerList = SendRecvMsg("SERVER LIST");
	wxString sEntry;
	int brk=0, newLinePos;
	while (sServerList.Length() > 0) {
		newLinePos=sServerList.Find("\n");
		
		sEntry = sServerList.Left(newLinePos);
		sServerList = sServerList.Mid(newLinePos+1);

		ServerEntry* Entry = new ServerEntry;

		brk=sEntry.First("\t");
		Entry->sServerName = _SpecialChars(sEntry.Left(brk));
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		Entry->sServerDescription = _SpecialChars(sEntry.Left(brk));
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		Entry->nServerPort = atoi(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		Entry->sServerIP = sEntry.Left(brk);
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		Entry->nServerUsers = atoi(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		Entry->nServerMaxUsers = atoi(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1);
		Entry->nServerFiles = atoi(sEntry.GetData());
		
		ServerArray.Add(Entry);
	}

	// Sorting (simple bubble sort, we don't have tons of data here)
	bool bSorted = true;
	for (int nMax = 0;bSorted && nMax < ServerArray.GetCount()*2; nMax++) {
		bSorted = false;
		for (int i = 0; i < ServerArray.GetCount() - 1; i++) {
			bool bSwap = false;
			switch(pThis->m_Params.ServerSort) {
				case SERVER_SORT_NAME:
					bSwap = ServerArray[i]->sServerName.CmpNoCase(ServerArray[i+1]->sServerName) > 0;
					break;
				case SERVER_SORT_DESCRIPTION:
					bSwap = ServerArray[i]->sServerDescription.CmpNoCase(ServerArray[i+1]->sServerDescription) < 0;
					break;
				case SERVER_SORT_IP:
					bSwap = ServerArray[i]->sServerIP.CmpNoCase(ServerArray[i+1]->sServerIP) > 0;
					break;
				case SERVER_SORT_USERS:
					bSwap = ServerArray[i]->nServerUsers < ServerArray[i+1]->nServerUsers;
					break;
				case SERVER_SORT_FILES:
					bSwap = ServerArray[i]->nServerFiles < ServerArray[i+1]->nServerFiles;
					break;
			}
			
			if (pThis->m_Params.bServerSortReverse) {
				bSwap = !bSwap;
			}
			
			if (bSwap) {
				bSorted = true;
				ServerEntry* TmpEntry = ServerArray[i];
				ServerArray[i] = ServerArray[i+1];
				ServerArray[i+1] = TmpEntry;
			}
		}
	}
	
	// Displaying
	wxString sList = "";
	for (int i = 0; i < ServerArray.GetCount(); i++) {
		wxString HTTPProcessData = OutE;	// Copy Entry Line to Temp
		HTTPProcessData.Replace("[1]", ServerArray[i]->sServerName);
		HTTPProcessData.Replace("[2]", ServerArray[i]->sServerDescription);
		wxString sPort; sPort.Printf(":%d", ServerArray[i]->nServerPort);
		HTTPProcessData.Replace("[3]", ServerArray[i]->sServerIP + sPort);
		
		wxString sT;
		if (ServerArray[i]->nServerUsers > 0) {
			if (ServerArray[i]->nServerMaxUsers > 0)
				sT.Printf("%d (%d)", ServerArray[i]->nServerUsers, ServerArray[i]->nServerMaxUsers);
			else
				sT.Printf("%d", ServerArray[i]->nServerUsers);
		}
		
		HTTPProcessData.Replace("[4]", sT);
		sT = "";
		if (ServerArray[i]->nServerFiles > 0)
			sT.Printf("%d", ServerArray[i]->nServerFiles);
		
		HTTPProcessData.Replace("[5]", sT);
		
		wxString sServerPort; sServerPort.Printf("%d", ServerArray[i]->nServerPort);
		
		HTTPProcessData.Replace("[6]", IsSessionAdmin(Data,sSession)? wxString("?ses=" + sSession + "&w=server&c=connect&ip=" + ServerArray[i]->sServerIP+"&port="+sServerPort):GetPermissionDenied());
		HTTPProcessData.Replace("[LinkRemove]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=server&c=remove&ip=" + ServerArray[i]->sServerIP+"&port="+sServerPort):GetPermissionDenied());

		sList += HTTPProcessData;
	}
	Out.Replace("[ServersList]", sList);

	return Out;
}

wxString CWebServer::_GetTransferList(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data.sURL, "ses");
	int cat=atoi(_ParseURL(Data.sURL,"cat"));

	bool clcompl=(_ParseURL(Data.sURL,"ClCompl")=="yes" );
	wxString sCat = "";	if (cat!=0) sCat.Printf("&cat=%i",cat);

	wxString Out = "";

	if (clcompl && IsSessionAdmin(Data,sSession)) {
		SendRecvMsg("TRANSFER CLEARCOMPLETE");
	}
	
	if (_ParseURL(Data.sURL, "c") != "" && IsSessionAdmin(Data,sSession)) {
		wxString HTTPTemp = _ParseURL(Data.sURL, "c");
		if (HTTPTemp.Right(1) != "/")
			HTTPTemp += "/";

		wxString request = wxString("TRANSFER ADDFILELINK ") + HTTPTemp;
		if (SendRecvMsg(request) == "Bad Link") {
			char HTTPTempC[100] = "";
			sprintf(HTTPTempC,_GetPlainResString(IDS_ERR_INVALIDLINK), "Bad Link");//error.GetData());
			Out += pThis->m_Templates.sTransferBadLink;
			Out.Replace("[InvalidLink]", HTTPTempC);
			Out.Replace("[Link]", HTTPTemp);
		}
	}

	if (_ParseURL(Data.sURL, "op") != "" && _ParseURL(Data.sURL, "file") != "") {
		uchar FileHash[16];
		_GetFileHash(_ParseURL(Data.sURL, "file"), FileHash);

		int found_file_index = -1;

		int dlFilePosition=0;
		//shakraw - sFileHashes formatted as: %s\t%s\t....\t%s
		wxString sFileHashes = SendRecvMsg("TRANSFER DL_FILEHASH");
		wxString sEntry;
		int tabPos;
		while (sFileHashes.Length()>0) {
			tabPos=sFileHashes.First("\t");
			sEntry = sFileHashes.Left(tabPos);
			sFileHashes = sFileHashes.Mid(tabPos+1);
			
			uchar filehash[16];
			for (int k = 0; k < 16; k++)
				filehash[k] = (uchar) sEntry.GetChar(k);

			bool bGood = true;
			for (int i = 0; i < 16; i++) {
				if ( filehash[i] != FileHash[i]) {
					bGood = false;
					break;
				}
			}
			
			if (bGood) {
				found_file_index = dlFilePosition;
				break;
			}			
			
			dlFilePosition++;
		}
		
		if(_ParseURL(Data.sURL, "op") == "pause" && IsSessionAdmin(Data,sSession)) {
			if (found_file_index >= 0) 
				SendRecvMsg(wxString::Format("TRANSFER DL_FILEPAUSE %i", found_file_index));
		} else if(_ParseURL(Data.sURL, "op") == "resume" && IsSessionAdmin(Data,sSession)) {
			if (found_file_index >= 0)
				SendRecvMsg(wxString::Format("TRANSFER DL_FILERESUME %i", found_file_index));
		} else if(_ParseURL(Data.sURL, "op") == "cancel" && IsSessionAdmin(Data,sSession)) {
			if (found_file_index >= 0)
				SendRecvMsg(wxString::Format("TRANSFER DL_FILEDELETE %i", found_file_index));
		} else if(_ParseURL(Data.sURL, "op") == "prioup" && IsSessionAdmin(Data,sSession)) {
			if (found_file_index >= 0) {
				SendRecvMsg(wxString::Format("TRANSFER DL_FILEPRIOUP %d", found_file_index));
			}
		} else if(_ParseURL(Data.sURL, "op") == "priodown" && IsSessionAdmin(Data,sSession)) {
			if (found_file_index >= 0) {
				SendRecvMsg(wxString::Format("TRANSFER DL_FILEPRIODOWN %d", found_file_index));
			}
		}
	}

	if (_ParseURL(Data.sURL, "sort") != "") {
		if(_ParseURL(Data.sURL, "sort") == "name")
			pThis->m_Params.DownloadSort = DOWN_SORT_NAME;
		else if(_ParseURL(Data.sURL, "sort") == "size")
			pThis->m_Params.DownloadSort = DOWN_SORT_SIZE;
		else if(_ParseURL(Data.sURL, "sort") == "transferred")
			pThis->m_Params.DownloadSort = DOWN_SORT_TRANSFERRED;
		else if(_ParseURL(Data.sURL, "sort") == "speed")
			pThis->m_Params.DownloadSort = DOWN_SORT_SPEED;
		else if(_ParseURL(Data.sURL, "sort") == "progress")
			pThis->m_Params.DownloadSort = DOWN_SORT_PROGRESS;

		if(_ParseURL(Data.sURL, "sortreverse") == "")
			pThis->m_Params.bDownloadSortReverse = false;
	}
	if (_ParseURL(Data.sURL, "sortreverse") != "") {
		pThis->m_Params.bDownloadSortReverse = (_ParseURL(Data.sURL, "sortreverse") == "true");
	} 
	if(_ParseURL(Data.sURL, "showuploadqueue") == "true") {
		pThis->m_Params.bShowUploadQueue = true;
	}
	if(_ParseURL(Data.sURL, "showuploadqueue") == "false") {
		pThis->m_Params.bShowUploadQueue = false;
	}
	
	wxString sDownloadSortRev;
	if (pThis->m_Params.bDownloadSortReverse)
		sDownloadSortRev = "false";
	else
		sDownloadSortRev = "true";

	Out += pThis->m_Templates.sTransferImages;
	Out += pThis->m_Templates.sTransferList;
	Out.Replace("[DownloadHeader]", pThis->m_Templates.sTransferDownHeader);
	Out.Replace("[DownloadFooter]", pThis->m_Templates.sTransferDownFooter);
	Out.Replace("[UploadHeader]", pThis->m_Templates.sTransferUpHeader);
	Out.Replace("[UploadFooter]", pThis->m_Templates.sTransferUpFooter);
	Out.Replace("[Session]", sSession);

	InsertCatBox(Out,cat,"",true,true);
	
	if (pThis->m_Params.DownloadSort == DOWN_SORT_NAME)
		Out.Replace("[SortName]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortName]", "");
	
	if (pThis->m_Params.DownloadSort == DOWN_SORT_SIZE)
		Out.Replace("[SortSize]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortSize]", "");
	
	if (pThis->m_Params.DownloadSort == DOWN_SORT_TRANSFERRED)
		Out.Replace("[SortTransferred]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortTransferred]", "");
	
	if (pThis->m_Params.DownloadSort == DOWN_SORT_SPEED)
		Out.Replace("[SortSpeed]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortSpeed]", "");
	
	if (pThis->m_Params.DownloadSort == DOWN_SORT_PROGRESS)
		Out.Replace("[SortProgress]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortProgress]", "");
	
	Out.Replace("[Filename]", _GetPlainResString(IDS_DL_FILENAME));
	Out.Replace("[Size]", _GetPlainResString(IDS_DL_SIZE));
	Out.Replace("[Transferred]", _GetPlainResString(IDS_COMPLETE));
	Out.Replace("[Progress]", _GetPlainResString(IDS_DL_PROGRESS));
	Out.Replace("[Speed]", _GetPlainResString(IDS_DL_SPEED));
	Out.Replace("[Sources]", _GetPlainResString(IDS_DL_SOURCES));
	Out.Replace("[Actions]", _GetPlainResString(IDS_WEB_ACTIONS));
	Out.Replace("[User]", _GetPlainResString(IDS_QL_USERNAME));
	Out.Replace("[TotalDown]", _GetPlainResString(IDS_INFLST_USER_TOTALDOWNLOAD));
	Out.Replace("[TotalUp]", _GetPlainResString(IDS_INFLST_USER_TOTALUPLOAD));
	Out.Replace("[Prio]", _GetPlainResString(IDS_PRIORITY));
	Out.Replace("[CatSel]",sCat);
	wxString OutE = pThis->m_Templates.sTransferDownLine;
	wxString OutE2 = pThis->m_Templates.sTransferDownLineGood;

	float fTotalSize = 0, fTotalTransferred = 0, fTotalSpeed = 0;
	CArray<DownloadFiles*, DownloadFiles*> FilesArray;

	// Populating array
	wxString sTransferDLList = SendRecvMsg("TRANSFER DL_LIST");
	bool completedAv=false;
	wxString sEntry;
	int newLinePos, brk=0;
	while (sTransferDLList.Length()>0) {
		newLinePos=sTransferDLList.First("\n");

		sEntry = sTransferDLList.Left(newLinePos);
		sTransferDLList = sTransferDLList.Mid(newLinePos+1);

		DownloadFiles *dFile = new DownloadFiles;

		brk=sEntry.First("\t");
		dFile->sFileName = _SpecialChars(sEntry.Left(brk));
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lFileSize = atol(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lFileTransferred = atol(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->fCompleted = atof(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lFileSpeed = atol(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileStatus = atoi(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sFileStatus = sEntry.Left(brk);
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFilePrio = atoi(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sFileHash = sEntry.Left(brk);
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lSourceCount = atol(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lNotCurrentSourceCount = atol(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lTransferringSourceCount = atol(sEntry.Left(brk).GetData());
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sED2kLink = sEntry.Left(brk);
		sEntry = sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sFileInfo = _SpecialChars(sEntry.Left(brk));
		sEntry = sEntry.Mid(brk+1);
		completedAv = (atoi(sEntry.GetData()) == 0) ? false : true;

		FilesArray.Add(dFile);

#if 0
			if (cat>0 && cur_file->GetCategory()!=cat) continue;
			if (cat<0) {
				switch (cat) {
					case -1 : if (cur_file->GetCategory()!=0) continue; break;
					case -2 : if (!cur_file->IsPartFile()) continue; break;
					case -3 : if (cur_file->IsPartFile()) continue; break;
					case -4 : if (!((cur_file->GetStatus()==PS_READY|| cur_file->GetStatus()==PS_EMPTY) && cur_file->GetTransferingSrcCount()==0)) continue; break;
					case -5 : if (!((cur_file->GetStatus()==PS_READY|| cur_file->GetStatus()==PS_EMPTY) && cur_file->GetTransferingSrcCount()>0)) continue; break;
					case -6 : if (cur_file->GetStatus()!=PS_ERROR) continue; break;
					case -7 : if (cur_file->GetStatus()!=PS_PAUSED) continue; break;
					case -8 : if (!cur_file->IsStopped()) continue; break;
					case -9 : if (!cur_file->IsMovie()) continue; break;
					case -10 : if (ED2KFT_AUDIO != GetED2KFileTypeID(cur_file->GetFileName())) continue; break;
					case -11 : if (!cur_file->IsArchive()) continue; break;
					case -12 : if (ED2KFT_CDIMAGE != GetED2KFileTypeID(cur_file->GetFileName())) continue; break;
				}
			}
#endif			

	}
	
	// Sorting (simple bubble sort, we don't have tons of data here)
	bool bSorted = true;
	for (int nMax = 0;bSorted && nMax < FilesArray.GetCount()*2; nMax++) {
		bSorted = false;
		for (int i = 0; i < FilesArray.GetCount() - 1; i++) {
			bool bSwap = false;
			switch (pThis->m_Params.DownloadSort) {
				case DOWN_SORT_NAME:
					bSwap = FilesArray[i]->sFileName.CmpNoCase(FilesArray[i+1]->sFileName) > 0;
					break;
				case DOWN_SORT_SIZE:
					bSwap = FilesArray[i]->lFileSize < FilesArray[i+1]->lFileSize;
					break;
				case DOWN_SORT_TRANSFERRED:
					bSwap = FilesArray[i]->lFileTransferred < FilesArray[i+1]->lFileTransferred;
					break;
				case DOWN_SORT_SPEED:
					bSwap = FilesArray[i]->lFileSpeed < FilesArray[i+1]->lFileSpeed;
					break;
				case DOWN_SORT_PROGRESS:
					bSwap = FilesArray[i]->fCompleted  < FilesArray[i+1]->fCompleted ;
					break;
			}
			if (pThis->m_Params.bDownloadSortReverse) {
				bSwap = !bSwap;
			}
			if (bSwap) {
				bSorted = true;
				DownloadFiles* TmpFile = FilesArray[i];
				FilesArray[i] = FilesArray[i+1];
				FilesArray[i+1] = TmpFile;
			}
		}
	}
	// Displaying
	wxString sDownList = "";
	wxString HTTPTemp;

	for (int i = 0; i < FilesArray.GetCount(); i++) {
		wxString JSfileinfo=FilesArray[i]->sFileInfo;
		//JSfileinfo.Replace("\n","\\n");
		JSfileinfo.Replace("|","\\n");
		wxString sActions = "<acronym title=\"" + FilesArray[i]->sFileStatus + "\"><a href=\"javascript:alert(\'"+ JSfileinfo+"')\"><img src=\"l_info.gif\" alt=\"" + FilesArray[i]->sFileStatus + "\"></a></acronym> ";

		wxString sED2kLink;
		sED2kLink.Printf("<acronym title=\"[Ed2klink]\"><a href=\""+ FilesArray[i]->sED2kLink +"\"><img src=\"l_ed2klink.gif\" alt=\"[Ed2klink]\"></a></acronym> ");
		sED2kLink.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
		sActions += sED2kLink;

		bool bCanBeDeleted = true;
		switch (FilesArray[i]->nFileStatus) {
			case PS_COMPLETING:
			case PS_COMPLETE:
				bCanBeDeleted = false;
				break;
			case PS_HASHING: 
			case PS_WAITINGFORHASH:
			case PS_ERROR:
				break;
			case PS_PAUSED:
				if (IsSessionAdmin(Data,sSession)) {
					wxString sResume;
					sResume.Printf("<acronym title=\"[Resume]\"><a href=\"[Link]\"><img src=\"l_resume.gif\" alt=\"[Resume]\"></a></acronym> ");
					sResume.Replace("[Link]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=transfer&op=resume&file=" + FilesArray[i]->sFileHash):GetPermissionDenied()) ;
					sActions += sResume;
				}
				break; 
			default: // waiting or downloading
				if (IsSessionAdmin(Data,sSession)) {
					wxString sPause;
					sPause.Printf("<acronym title=\"[Pause]\"><a href=\"[Link]\"><img src=\"l_pause.gif\" alt=\"[Pause]\"></a></acronym> ");
					sPause.Replace("[Link]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=transfer&op=pause&file=" + FilesArray[i]->sFileHash):GetPermissionDenied());
					sActions += sPause;
				}
				break;
		}
		if (bCanBeDeleted) {
			if (IsSessionAdmin(Data,sSession)) {
				wxString sCancel;
				sCancel.Printf("<acronym title=\"[Cancel]\"><a href=\"[Link]\" onclick=\"return confirm(\'[ConfirmCancel]\')\"><img src=\"l_cancel.gif\" alt=\"[Cancel]\"></a></acronym> ");
				sCancel.Replace("[Link]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=transfer&op=cancel&file=" + FilesArray[i]->sFileHash):GetPermissionDenied());
				sActions += sCancel;
			}
		}
		
		if (IsSessionAdmin(Data,sSession)) {
			sActions.Replace("[Resume]", _GetPlainResString(IDS_DL_RESUME));
			sActions.Replace("[Pause]", _GetPlainResString(IDS_DL_PAUSE));
			sActions.Replace("[Cancel]", _GetPlainResString(IDS_MAIN_BTN_CANCEL));
			sActions.Replace("[ConfirmCancel]", _GetPlainResString(IDS_Q_CANCELDL2, true));

			if (FilesArray[i]->nFileStatus!=PS_COMPLETE && FilesArray[i]->nFileStatus!=PS_COMPLETING) {
				sActions.Append("<acronym title=\"[PriorityUp]\"><a href=\"?ses=[Session]&amp;w=transfer&op=prioup&file=" + FilesArray[i]->sFileHash+sCat+"\"><img src=\"l_up.gif\" alt=\"[PriorityUp]\"></a></acronym>");
				sActions.Append("&nbsp;<acronym title=\"[PriorityDown]\"><a href=\"?ses=[Session]&amp;w=transfer&op=priodown&file=" + FilesArray[i]->sFileHash+sCat +"\"><img src=\"l_down.gif\" alt=\"[PriorityDown]\"></a></acronym>");
			}
		}
		
		wxString HTTPProcessData;
		// if downloading, draw in other color
		if (FilesArray[i]->lFileSpeed > 0)
			HTTPProcessData = OutE2;
		else
			HTTPProcessData = OutE;

		if (FilesArray[i]->sFileName.Length() > SHORT_FILENAME_LENGTH)
			HTTPProcessData.Replace("[ShortFileName]", FilesArray[i]->sFileName.Left(SHORT_FILENAME_LENGTH) + "...");
		else
			HTTPProcessData.Replace("[ShortFileName]", FilesArray[i]->sFileName);

		HTTPProcessData.Replace("[FileInfo]", FilesArray[i]->sFileInfo);

		fTotalSize += FilesArray[i]->lFileSize;

		HTTPProcessData.Replace("[2]", castItoXBytes(FilesArray[i]->lFileSize));

		if (FilesArray[i]->lFileTransferred > 0) {
			fTotalTransferred += FilesArray[i]->lFileTransferred;

			HTTPProcessData.Replace("[3]", castItoXBytes(FilesArray[i]->lFileTransferred));
		} else
			HTTPProcessData.Replace("[3]", "-");

		HTTPProcessData.Replace("[DownloadBar]", _GetDownloadGraph(Data,FilesArray[i]->sFileHash));

		if (FilesArray[i]->lFileSpeed > 0.0f) {
			fTotalSpeed += FilesArray[i]->lFileSpeed;

			HTTPTemp.Printf("%8.2f %s", FilesArray[i]->lFileSpeed/1024.0 ,_GetPlainResString(IDS_KBYTESEC).GetData() );
			HTTPProcessData.Replace("[4]", HTTPTemp);
		} else
			HTTPProcessData.Replace("[4]", "-");
		
		if (FilesArray[i]->lSourceCount > 0) {
			HTTPTemp.Printf("%li&nbsp;/&nbsp;%8li&nbsp;(%li)",
				FilesArray[i]->lSourceCount-FilesArray[i]->lNotCurrentSourceCount,
				FilesArray[i]->lSourceCount,
				FilesArray[i]->lTransferringSourceCount
			);
			HTTPProcessData.Replace("[5]", HTTPTemp);
		} else
			HTTPProcessData.Replace("[5]", "-");
		
		switch (FilesArray[i]->nFilePrio) {
			case 0: HTTPTemp=getResString(IDS_PRIOLOW);break;
			case 10: HTTPTemp=getResString(IDS_PRIOAUTOLOW);break;

			case 1: HTTPTemp=getResString(IDS_PRIONORMAL);break;
			case 11: HTTPTemp=getResString(IDS_PRIOAUTONORMAL);break;

			case 2: HTTPTemp=getResString(IDS_PRIOHIGH);break;
			case 12: HTTPTemp=getResString(IDS_PRIOAUTOHIGH);break;
			//shakraw - it seems there is a problem with dl file priorities
			//i.e. I've got fileprio=3, VERYHIGH, but we can't set this priority 
			//in dl window. so, why fileprio=3?
			default: HTTPTemp="-"; break; 
		}
	
		HTTPProcessData.Replace("[PrioVal]", HTTPTemp);
		HTTPProcessData.Replace("[6]", sActions);

		sDownList += HTTPProcessData;
	}

	Out.Replace("[DownloadFilesList]", sDownList);
	Out.Replace("[PriorityUp]", _GetPlainResString(IDS_PRIORITY_UP));
    Out.Replace("[PriorityDown]", _GetPlainResString(IDS_PRIORITY_DOWN));
	// Elandal: cast from float to integral type always drops fractions.
	// avoids implicit cast warning
	Out.Replace("[TotalDownSize]", castItoXBytes((uint64)fTotalSize));
	Out.Replace("[TotalDownTransferred]", castItoXBytes((uint64)fTotalTransferred));
	
	Out.Replace("[ClearCompletedButton]",(completedAv && IsSessionAdmin(Data,sSession))?pThis->m_Templates.sClearCompleted :wxString(wxT("")));

	HTTPTemp.Printf("%8.2f %s", fTotalSpeed/1024.0,_GetPlainResString(IDS_KBYTESEC).GetData());
	Out.Replace("[TotalDownSpeed]", HTTPTemp);
	OutE = pThis->m_Templates.sTransferUpLine;
	
	HTTPTemp.Printf("%i",pThis->m_Templates.iProgressbarWidth);
	Out.Replace("[PROGRESSBARWIDTHVAL]",HTTPTemp);

	fTotalSize = 0;
	fTotalTransferred = 0;
	fTotalSpeed = 0;

	wxString sUpList = "";

	//shakraw - upload list
	wxString sTransferULList = SendRecvMsg("TRANSFER UL_LIST");
	wxString HTTPProcessData;
	float transfDown, transfUp, transfDatarate; 
	while (sTransferULList.Length()>0) {
		newLinePos=sTransferULList.First("\n");

		sEntry = sTransferULList.Left(newLinePos);
		sTransferULList = sTransferULList.Mid(newLinePos+1);

		HTTPProcessData = OutE;

		brk=sEntry.First("\t");
		HTTPProcessData.Replace("[1]", _SpecialChars(sEntry.Left(brk)));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		HTTPProcessData.Replace("[FileInfo]", _SpecialChars(sEntry.Left(brk)));		
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		HTTPProcessData.Replace("[2]", _SpecialChars(sEntry.Left(brk)));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		
		transfDown = atoi(sEntry.Left(brk).GetData());
		fTotalSize += transfDown;
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		
		transfUp = (float) atoi(sEntry.Left(brk).GetData());
		fTotalTransferred += transfUp;
		sEntry=sEntry.Mid(brk+1);

		wxString HTTPTemp;
		HTTPTemp.Printf("%s / %s", castItoXBytes((uint64)transfDown).GetData(),castItoXBytes((uint64)transfUp).GetData());
		HTTPProcessData.Replace("[3]", HTTPTemp);

		transfDatarate = (float) atoi(sEntry.GetData());
		fTotalSpeed += transfDatarate;
		
		HTTPTemp.Printf("%8.2f " + _GetPlainResString(IDS_KBYTESEC), transfDatarate/1024.0);
		HTTPProcessData.Replace("[4]", HTTPTemp);
		
		sUpList += HTTPProcessData;
	}
	
	Out.Replace("[UploadFilesList]", sUpList);
	// Elandal: cast from float to integral type always drops fractions.
	// avoids implicit cast warning
	HTTPTemp.Printf("%s / %s", castItoXBytes((uint64)fTotalSize).GetData(), castItoXBytes((uint64)fTotalTransferred).GetData());
	Out.Replace("[TotalUpTransferred]", HTTPTemp);
	HTTPTemp.Printf("%8.2f " + _GetPlainResString(IDS_KBYTESEC), fTotalSpeed/1024.0);
	Out.Replace("[TotalUpSpeed]", HTTPTemp);

	if (pThis->m_Params.bShowUploadQueue) {
		Out.Replace("[UploadQueue]", pThis->m_Templates.sTransferUpQueueShow);
		Out.Replace("[UploadQueueList]", _GetPlainResString(IDS_ONQUEUE));
		Out.Replace("[UserNameTitle]", _GetPlainResString(IDS_QL_USERNAME));
		Out.Replace("[FileNameTitle]", _GetPlainResString(IDS_DL_FILENAME));
		Out.Replace("[ScoreTitle]", _GetPlainResString(IDS_SCORE));
		Out.Replace("[BannedTitle]", _GetPlainResString(IDS_BANNED));

		OutE = pThis->m_Templates.sTransferUpQueueLine;
		// Replace [xx]
		wxString sQueue = "";

		//shakraw - waiting list
		wxString sTransferWList = SendRecvMsg("TRANSFER W_LIST");
		while (sTransferWList.Length()>0) {
			newLinePos=sTransferWList.First("\n");

			sEntry = sTransferWList.Left(newLinePos);
			sTransferWList = sTransferWList.Mid(newLinePos+1);

            char HTTPTempC[100] = "";
			HTTPProcessData = OutE;

			brk=sTransferWList.First("\t");
			if (brk==-1) {
				HTTPProcessData.Replace("[UserName]", _SpecialChars(sEntry));
				continue;
			}
			HTTPProcessData.Replace("[UserName]", _SpecialChars(sEntry.Left(brk)));
			sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
			HTTPProcessData.Replace("[FileName]", _SpecialChars(sEntry.Left(brk)));
			sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
			sprintf(HTTPTempC, "%i" , atoi(sEntry.Left(brk).GetData()));
			sEntry=sEntry.Mid(brk+1);
			
			wxString HTTPTemp = HTTPTempC;
			HTTPProcessData.Replace("[Score]", HTTPTemp);

			if (atoi(sEntry.GetData()))
				HTTPProcessData.Replace("[Banned]", _GetPlainResString(IDS_YES));
			else
				HTTPProcessData.Replace("[Banned]", _GetPlainResString(IDS_NO));
		}
		
		Out.Replace("[QueueList]", sQueue);
	} else {
		Out.Replace("[UploadQueue]", pThis->m_Templates.sTransferUpQueueHide);
	}

	Out.Replace("[ShowQueue]", _GetPlainResString(IDS_WEB_SHOW_UPLOAD_QUEUE));
	Out.Replace("[HideQueue]", _GetPlainResString(IDS_WEB_HIDE_UPLOAD_QUEUE));
	Out.Replace("[Session]", sSession);
	Out.Replace("[CLEARCOMPLETED]",_GetPlainResString(IDS_DL_CLEAR));

	wxString buffer;
	buffer.Printf("%s (%i)", _GetPlainResString(IDS_TW_DOWNLOADS).GetData(),FilesArray.GetCount());
	Out.Replace("[DownloadList]",buffer);
	buffer.Printf("%s (%i)",_GetPlainResString(IDS_PW_CON_UPLBL).GetData(),atoi(SendRecvMsg("QUEUE UL_GETLENGTH")));
	Out.Replace("[UploadList]", buffer );
	Out.Replace("[CatSel]",sCat);

	return Out;
}

wxString CWebServer::_GetDownloadLink(ThreadData Data) {
#if 0 //shakraw
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data.sURL, "ses");

	if (!IsSessionAdmin(Data,sSession)) {
		wxString ad="<br><br><div align=\"center\" class=\"message\">[Message]</div>";
		ad.Replace("[Message]",_GetPlainResString(IDS_ACCESSDENIED));
		return ad;
	}
	
	wxString Out = pThis->m_Templates.sDownloadLink;

	Out.Replace("[Download]", _GetPlainResString(IDS_SW_DOWNLOAD));
	Out.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
	Out.Replace("[Start]", _GetPlainResString(IDS_SW_START));
	Out.Replace("[Session]", sSession);

	if (theApp.glob_prefs->GetCatCount()>1)
		InsertCatBox(Out,0, pThis->m_Templates.sCatArrow );
	else Out.Replace("[CATBOX]","");

	return Out;
#endif
	return "";
}

wxString CWebServer::_GetSharedFilesList(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
    wxString sSession = _ParseURL(Data.sURL, "ses");
	
	if (pThis == NULL)
		return "";
	
	if (_ParseURL(Data.sURL, "sort") != "")  {
		if(_ParseURL(Data.sURL, "sort") == "name")
			pThis->m_Params.SharedSort = SHARED_SORT_NAME;
		else if(_ParseURL(Data.sURL, "sort") == "size")
			pThis->m_Params.SharedSort = SHARED_SORT_SIZE;
		else if(_ParseURL(Data.sURL, "sort") == "transferred")
			pThis->m_Params.SharedSort = SHARED_SORT_TRANSFERRED;
		else if(_ParseURL(Data.sURL, "sort") == "alltimetransferred")
			pThis->m_Params.SharedSort = SHARED_SORT_ALL_TIME_TRANSFERRED;
		else if(_ParseURL(Data.sURL, "sort") == "requests")
			pThis->m_Params.SharedSort = SHARED_SORT_REQUESTS;
		else if(_ParseURL(Data.sURL, "sort") == "alltimerequests")
			pThis->m_Params.SharedSort = SHARED_SORT_ALL_TIME_REQUESTS;
		else if(_ParseURL(Data.sURL, "sort") == "accepts")
			pThis->m_Params.SharedSort = SHARED_SORT_ACCEPTS;
		else if(_ParseURL(Data.sURL, "sort") == "alltimeaccepts")
			pThis->m_Params.SharedSort = SHARED_SORT_ALL_TIME_ACCEPTS;
		else if(_ParseURL(Data.sURL, "sort") == "priority")
			pThis->m_Params.SharedSort = SHARED_SORT_PRIORITY;

		if(_ParseURL(Data.sURL, "sortreverse") == "")
			pThis->m_Params.bSharedSortReverse = false;
	}
	if (_ParseURL(Data.sURL, "sortreverse") != "") 
		pThis->m_Params.bSharedSortReverse = (_ParseURL(Data.sURL, "sortreverse") == "true");

	if (_ParseURL(Data.sURL, "hash") != "" && _ParseURL(Data.sURL, "setpriority") != "" && IsSessionAdmin(Data,sSession)) 
		_SetSharedFilePriority(_ParseURL(Data.sURL, "hash"),atoi(_ParseURL(Data.sURL, "setpriority")));

	if (_ParseURL(Data.sURL, "reload") == "true") {
		SendRecvMsg("SHAREDFILES RELOAD");
	}

	wxString sSharedSortRev;
	if (pThis->m_Params.bSharedSortReverse) 
		sSharedSortRev = "false";
	else
		sSharedSortRev = "true";
	
	//Name sorting link
	wxString Out = pThis->m_Templates.sSharedList;
	if (pThis->m_Params.SharedSort == SHARED_SORT_NAME)
		Out.Replace("[SortName]", "sort=name&sortreverse=" + sSharedSortRev);
	else
		Out.Replace("[SortName]", "sort=name");
	
	//Size sorting Link
	if (pThis->m_Params.SharedSort == SHARED_SORT_SIZE)
		Out.Replace("[SortSize]", "sort=size&sortreverse=" + sSharedSortRev);
	else
		Out.Replace("[SortSize]", "sort=size");
	
	//Priority sorting Link
	if (pThis->m_Params.SharedSort == SHARED_SORT_PRIORITY)
		Out.Replace("[SortPriority]", "sort=priority&sortreverse=" + sSharedSortRev);
	else
		Out.Replace("[SortPriority]", "sort=priority");
	
	//Transferred sorting link
	if (pThis->m_Params.SharedSort == SHARED_SORT_TRANSFERRED) {
		if (pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortTransferred]", "sort=alltimetransferred&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortTransferred]", "sort=transferred&sortreverse=" + sSharedSortRev);
	} else if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_TRANSFERRED) {
		if (pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortTransferred]", "sort=transferred&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortTransferred]", "sort=alltimetransferred&sortreverse=" + sSharedSortRev);
	} else
        Out.Replace("[SortTransferred]", "&sort=transferred&sortreverse=false");
	
	//Request sorting link
	if (pThis->m_Params.SharedSort == SHARED_SORT_REQUESTS) {
		if (pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortRequests]", "sort=alltimerequests&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortRequests]", "sort=requests&sortreverse=" + sSharedSortRev);
	} else if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_REQUESTS) {
		if (pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortRequests]", "sort=requests&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortRequests]", "sort=alltimerequests&sortreverse=" + sSharedSortRev);
	} else
        Out.Replace("[SortRequests]", "&sort=requests&sortreverse=false");
	
	//Accepts sorting link
	if (pThis->m_Params.SharedSort == SHARED_SORT_ACCEPTS) {
		if (pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortAccepts]", "sort=alltimeaccepts&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortAccepts]", "sort=accepts&sortreverse=" + sSharedSortRev);
	} else if (pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_ACCEPTS) {
		if (pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortAccepts]", "sort=accepts&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortAccepts]", "sort=alltimeaccepts&sortreverse=" + sSharedSortRev);
	} else
        Out.Replace("[SortAccepts]", "&sort=accepts&sortreverse=false");

	if (_ParseURL(Data.sURL, "reload") == "true") {
#warning fix GetLastLogEntry()
		wxString resultlog = ""; //_SpecialChars(theApp.amuledlg->GetLastLogEntry());
		//resultlog = resultlog.TrimRight('\n');
		//resultlog = resultlog.Mid(resultlog.ReverseFind('\n'));
		Out.Replace("[Message]",resultlog);
	} else
        Out.Replace("[Message]","");

	Out.Replace("[Filename]", _GetPlainResString(IDS_DL_FILENAME));
	Out.Replace("[Priority]",  _GetPlainResString(IDS_PRIORITY));
	Out.Replace("[FileTransferred]",  _GetPlainResString(IDS_SF_TRANSFERRED));
	Out.Replace("[FileRequests]",  _GetPlainResString(IDS_SF_REQUESTS));
	Out.Replace("[FileAccepts]",  _GetPlainResString(IDS_SF_ACCEPTS));
	Out.Replace("[Size]", _GetPlainResString(IDS_DL_SIZE));
	Out.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
	Out.Replace("[Reload]", _GetPlainResString(IDS_SF_RELOAD));
	Out.Replace("[Session]", sSession);

	wxString OutE = pThis->m_Templates.sSharedLine; 
	OutE.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
	OutE.Replace("[PriorityUp]", _GetPlainResString(IDS_PRIORITY_UP));
	OutE.Replace("[PriorityDown]", _GetPlainResString(IDS_PRIORITY_DOWN));

	wxString OutE2 = pThis->m_Templates.sSharedLineChanged; 
	OutE2.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
	OutE2.Replace("[PriorityUp]", _GetPlainResString(IDS_PRIORITY_UP));
	OutE2.Replace("[PriorityUp]", _GetPlainResString(IDS_PRIORITY_DOWN));

	CArray<SharedFiles*, SharedFiles*> SharedArray;

	// Populating array
	// sSharedFilesList as:
	// %s\t%ld\t%s\t%ld\t%ll\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%d\n
	wxString sSharedFilesList = SendRecvMsg("SHAREDFILES LIST");
	wxString sEntry;
	int brk=0, newLinePos;
	while (sSharedFilesList.Length()>0) {
		newLinePos=sSharedFilesList.First("\n");

		sEntry = sSharedFilesList.Left(newLinePos);
		
		sSharedFilesList = sSharedFilesList.Mid(newLinePos+1);

		SharedFiles* dFile=new SharedFiles();
		//%s\t%ld\t%s\t%ld\t%ll\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%d\n
		brk=sEntry.First("\t");
		dFile->sFileName = _SpecialChars(sEntry.Left(brk));
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->lFileSize = atol(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sED2kLink = sEntry.Left(brk);
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileTransferred = atol(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileAllTimeTransferred = atoll(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileRequests = atoi(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileAllTimeRequests = atol(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileAccepts = atoi(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFileAllTimeAccepts = atol(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sFileHash = sEntry.Left(brk);
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->sFilePriority = sEntry.Left(brk);
		sEntry=sEntry.Mid(brk+1); brk=sEntry.First("\t");
		dFile->nFilePriority = atoi(sEntry.Left(brk).GetData());
		sEntry=sEntry.Mid(brk+1);
		if (atoi(sEntry.GetData())==0) {
			dFile->bFileAutoPriority = false;
		} else {
			dFile->bFileAutoPriority = true;
		}

		SharedArray.Add(dFile);
	}
	
	// Sorting (simple bubble sort, we don't have tons of data here)
	bool bSorted = true;
	
	for (int nMax = 0;bSorted && nMax < SharedArray.GetCount()*2; nMax++) {
		bSorted = false;
		for (int i = 0; i < SharedArray.GetCount() - 1; i++) {
			bool bSwap = false;
			switch (pThis->m_Params.SharedSort) {
				case SHARED_SORT_NAME:
					bSwap = SharedArray[i]->sFileName.CmpNoCase(SharedArray[i+1]->sFileName) > 0;
					break;
				case SHARED_SORT_SIZE:
					bSwap = SharedArray[i]->lFileSize < SharedArray[i+1]->lFileSize;
					break;
				case SHARED_SORT_TRANSFERRED:
					bSwap = SharedArray[i]->nFileTransferred < SharedArray[i+1]->nFileTransferred;
					break;
				case SHARED_SORT_ALL_TIME_TRANSFERRED:
					bSwap = SharedArray[i]->nFileAllTimeTransferred < SharedArray[i+1]->nFileAllTimeTransferred;
					break;
				case SHARED_SORT_REQUESTS:
					bSwap = SharedArray[i]->nFileRequests < SharedArray[i+1]->nFileRequests;
					break;
				case SHARED_SORT_ALL_TIME_REQUESTS:
					bSwap = SharedArray[i]->nFileAllTimeRequests < SharedArray[i+1]->nFileAllTimeRequests;
					break;
				case SHARED_SORT_ACCEPTS:
					bSwap = SharedArray[i]->nFileAccepts < SharedArray[i+1]->nFileAccepts;
					break;
				case SHARED_SORT_ALL_TIME_ACCEPTS:
					bSwap = SharedArray[i]->nFileAllTimeAccepts < SharedArray[i+1]->nFileAllTimeAccepts;
					break;
				case SHARED_SORT_PRIORITY:
					//Very low priority is define equal to 4 ! Must adapte sorting code
					if (SharedArray[i]->nFilePriority == 4) {
						if (SharedArray[i+1]->nFilePriority == 4)
							bSwap = false;
						else
							bSwap = true;
					} else {
						if (SharedArray[i+1]->nFilePriority == 4) {
							if (SharedArray[i]->nFilePriority == 4)
								bSwap = true;
							else
								bSwap = false;
						} else
							bSwap = SharedArray[i]->nFilePriority < SharedArray[i+1]->nFilePriority;
					}
					break;
			}
			if (pThis->m_Params.bSharedSortReverse) {
				bSwap = !bSwap;
			}
			if (bSwap) {
				bSorted = true;
				SharedFiles* TmpFile = SharedArray[i];
				SharedArray[i] = SharedArray[i+1];
				SharedArray[i+1] = TmpFile;
			}
		}
	}

	// Displaying
	wxString sSharedList = "";
	for (int i = 0; i < SharedArray.GetCount(); i++) {
		char HTTPTempC[100] = "";
		wxString HTTPProcessData;
		if (SharedArray[i]->sFileHash == _ParseURL(Data.sURL,"hash") )
            HTTPProcessData = OutE2;
		else
            HTTPProcessData = OutE;

		HTTPProcessData.Replace("[FileName]", _SpecialChars(SharedArray[i]->sFileName));
		if (SharedArray[i]->sFileName.Length() > SHORT_FILENAME_LENGTH)
            HTTPProcessData.Replace("[ShortFileName]", _SpecialChars(SharedArray[i]->sFileName.Left(SHORT_FILENAME_LENGTH)) + "...");
		else
			HTTPProcessData.Replace("[ShortFileName]", _SpecialChars(SharedArray[i]->sFileName));

		sprintf(HTTPTempC, "%s",castItoXBytes(SharedArray[i]->lFileSize).GetData());
		HTTPProcessData.Replace("[FileSize]", wxString(HTTPTempC));
		HTTPProcessData.Replace("[FileLink]", SharedArray[i]->sED2kLink);

		sprintf(HTTPTempC, "%s",castItoXBytes(SharedArray[i]->nFileTransferred).GetData());
		HTTPProcessData.Replace("[FileTransferred]", wxString(HTTPTempC));

		sprintf(HTTPTempC, "%s",castItoXBytes(SharedArray[i]->nFileAllTimeTransferred).GetData());
		HTTPProcessData.Replace("[FileAllTimeTransferred]", wxString(HTTPTempC));

		sprintf(HTTPTempC, "%i", SharedArray[i]->nFileRequests);
		HTTPProcessData.Replace("[FileRequests]", wxString(HTTPTempC));

		sprintf(HTTPTempC, "%i", SharedArray[i]->nFileAllTimeRequests);
		HTTPProcessData.Replace("[FileAllTimeRequests]", wxString(HTTPTempC));

		sprintf(HTTPTempC, "%i", SharedArray[i]->nFileAccepts);
		HTTPProcessData.Replace("[FileAccepts]", wxString(HTTPTempC));

		sprintf(HTTPTempC, "%i", SharedArray[i]->nFileAllTimeAccepts);
		HTTPProcessData.Replace("[FileAllTimeAccepts]", wxString(HTTPTempC));

		HTTPProcessData.Replace("[Priority]", SharedArray[i]->sFilePriority);

		HTTPProcessData.Replace("[FileHash]", SharedArray[i]->sFileHash);

		uint8 upperpriority=0, lesserpriority=0;
		if(SharedArray[i]->nFilePriority == 4) {
			upperpriority = 0;	lesserpriority = 4;
		} else if (SharedArray[i]->nFilePriority == 0) {
			upperpriority = 1;	lesserpriority = 4;
		} else if (SharedArray[i]->nFilePriority == 1) {
			upperpriority = 2;	lesserpriority = 0;
		} else if (SharedArray[i]->nFilePriority == 2) {
			upperpriority = 3;	lesserpriority = 1;
		} else if (SharedArray[i]->nFilePriority == 3) {
			upperpriority = 5;	lesserpriority = 2;
		} else if (SharedArray[i]->nFilePriority == 5) {
			upperpriority = 5;	lesserpriority = 3;
		} if (SharedArray[i]->bFileAutoPriority) {
			upperpriority = 5;	lesserpriority = 3;
		}
		
		sprintf(HTTPTempC, "%i", upperpriority);
		HTTPProcessData.Replace("[PriorityUpLink]", "hash=" + SharedArray[i]->sFileHash +"&setpriority=" + wxString(HTTPTempC));
		sprintf(HTTPTempC, "%i", lesserpriority);
		HTTPProcessData.Replace("[PriorityDownLink]", "hash=" + SharedArray[i]->sFileHash +"&setpriority=" + wxString(HTTPTempC)); 

		sSharedList += HTTPProcessData;
	}
	Out.Replace("[SharedFilesList]", sSharedList);
	Out.Replace("[Session]", sSession);

	return Out;
}

wxString CWebServer::_GetGraphs(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString Out = pThis->m_Templates.sGraphs;
	
	wxString sGraphDownload = "", sGraphUpload = "", sGraphCons = "";
	wxString sTmp = "";
	
	for (int i = 0; i < WEB_GRAPH_WIDTH; i++) {
		if (i < pThis->m_Params.PointsForWeb.GetCount()) {
			if (i != 0) {
				sGraphDownload += ",";
				sGraphUpload += ",";
				sGraphCons += ",";
			}
			// download
			sTmp.Format("%d" , (uint32) (pThis->m_Params.PointsForWeb[i]->download*1024));
			sGraphDownload += sTmp;
			// upload
			sTmp.Format("%d" , (uint32) (pThis->m_Params.PointsForWeb[i]->upload*1024));
			sGraphUpload += sTmp;
			// connections
			sTmp.Format("%d" , (uint32) (pThis->m_Params.PointsForWeb[i]->connections));
			sGraphCons += sTmp;
		}
	}
	
	Out.Replace("[GraphDownload]", sGraphDownload);
	Out.Replace("[GraphUpload]", sGraphUpload);
	Out.Replace("[GraphConnections]", sGraphCons);
	
	Out.Replace("[TxtDownload]", _GetPlainResString(IDS_DOWNLOAD));
	Out.Replace("[TxtUpload]", _GetPlainResString(IDS_PW_CON_UPLBL));
	Out.Replace("[TxtTime]", _GetPlainResString(IDS_TIME));
	Out.Replace("[TxtConnections]", _GetPlainResString(IDS_SP_ACTCON));
	Out.Replace("[KByteSec]", _GetPlainResString(IDS_KBYTESEC));
	Out.Replace("[TxtTime]", _GetPlainResString(IDS_TIME));

	//sGraphs formatted as: %d\t%d\t%d\t%d
	wxString sGraphs = SendRecvMsg("WEBPAGE GETGRAPH");
	int brk = sGraphs.First("\t");
	
	wxString sScale;
	sScale.Printf("%s", castSecondsToHM(atoi(sGraphs.Left(brk).GetData()) * WEB_GRAPH_WIDTH).GetData() );
	sGraphs = sGraphs.Mid(brk+1); brk=sGraphs.First("\t");

	wxString s1, s2, s3;
	s1.Printf("%d", atoi(sGraphs.Left(brk).GetData()) + 4);
	sGraphs = sGraphs.Mid(brk+1); brk=sGraphs.First("\t");
	s2.Printf("%d", atoi(sGraphs.Left(brk).GetData()) + 4);
	sGraphs = sGraphs.Mid(brk+1);
	s3.Printf("%d", atoi(sGraphs.GetData()) + 20);
	
	Out.Replace("[ScaleTime]", sScale);
	Out.Replace("[MaxDownload]", s1);
	Out.Replace("[MaxUpload]", s2);
	Out.Replace("[MaxConnections]", s3);

	return Out;
}

wxString CWebServer::_GetAddServerBox(ThreadData Data) {	
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data.sURL, "ses");

	if (!IsSessionAdmin(Data,sSession)) return "";

	wxString Out = pThis->m_Templates.sAddServerBox;
	if (_ParseURL(Data.sURL, "addserver") == "true") {
		wxString sIP = _ParseURL(Data.sURL, "serveraddr");
		wxString sPort = _ParseURL(Data.sURL, "serverport");
		wxString sName = _ParseURL(Data.sURL, "servername");
		
		wxString request = wxString("SERVER ADD ")+sIP+wxString(" ")+sPort+wxString(" ")+sName;
		SendRecvMsg(request.GetData());

#warning fix GetLastLogEntry
		wxString resultlog = ""; //_SpecialChars(theApp.amuledlg->GetLastLogEntry());
		//resultlog = resultlog.TrimRight('\n');
		//resultlog = resultlog.Mid(resultlog.ReverseFind('\n'));
		Out.Replace("[Message]",resultlog);
	} else if (_ParseURL(Data.sURL, "updateservermetfromurl") == "true") {
		wxString request = wxString("SERVER UPDATEMET ") + wxString(_ParseURL(Data.sURL, "servermeturl"));
		SendRecvMsg(request);
		
#warning fix GetLastLogEntry
		wxString resultlog = ""; //_SpecialChars(theApp.amuledlg->GetLastLogEntry());
		//resultlog = resultlog.TrimRight('\n');
		//resultlog = resultlog.Mid(resultlog.ReverseFind('\n'));
		Out.Replace("[Message]",resultlog);
	} else
		Out.Replace("[Message]", "");
	
	Out.Replace("[AddServer]", _GetPlainResString(IDS_SV_NEWSERVER));
	Out.Replace("[IP]", _GetPlainResString(IDS_SV_ADDRESS));
	Out.Replace("[Port]", _GetPlainResString(IDS_SV_PORT));
	Out.Replace("[Name]", _GetPlainResString(IDS_SW_NAME));
	Out.Replace("[Add]", _GetPlainResString(IDS_SV_ADD));
	Out.Replace("[UpdateServerMetFromURL]", _GetPlainResString(IDS_SV_MET));
	Out.Replace("[URL]", _GetPlainResString(IDS_SV_URL));
	Out.Replace("[Apply]", _GetPlainResString(IDS_PW_APPLY));

	return Out;
}

wxString CWebServer::_GetWebSearch(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data.sURL, "ses");
    
	wxString Out = pThis->m_Templates.sWebSearch;
	if (_ParseURL(Data.sURL, "tosearch") != "") {
		wxString query;
		wxString tosearch = _ParseURL(Data.sURL, "tosearch");
		query = "http://www.filedonkey.com/fdsearch/index.php?media=";
		query += _ParseURL(Data.sURL, "media");
		tosearch = URLEncode(tosearch);
		tosearch.Replace("%20","+");
		query += "&pattern=";
		query += _ParseURL(Data.sURL, "tosearch");
		query += "&action=search&name=FD-Search&op=modload&file=index&requestby=amule";
		Out += "\n<script language=\"javascript\">";
		Out += "\n searchwindow=window.open('"+ query + "','searchwindow');";
		Out += "\n</script>";
	}
	
	Out.Replace("[Session]", sSession);
	Out.Replace("[Name]", _GetPlainResString(IDS_SW_NAME));
	Out.Replace("[Type]", _GetPlainResString(IDS_TYPE));
	Out.Replace("[Any]", _GetPlainResString(IDS_SEARCH_ANY));
	Out.Replace("[Archives]", _GetPlainResString(IDS_SEARCH_ARC));
	Out.Replace("[Audio]", _GetPlainResString(IDS_SEARCH_AUDIO));
	Out.Replace("[CD-Images]", _GetPlainResString(IDS_SEARCH_CDIMG));
	Out.Replace("[Pictures]", _GetPlainResString(IDS_SEARCH_PICS));
	Out.Replace("[Programs]", _GetPlainResString(IDS_SEARCH_PRG));
	Out.Replace("[Videos]", _GetPlainResString(IDS_SEARCH_VIDEO));
	Out.Replace("[Search]", _GetPlainResString(IDS_SW_START));
	Out.Replace("[WebSearch]", _GetPlainResString(IDS_SW_WEBBASED));
	
	return Out;
}

wxString CWebServer::_GetLog(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data.sURL, "ses");

	wxString Out = pThis->m_Templates.sLog;

	if (_ParseURL(Data.sURL, "clear") == "yes" && IsSessionAdmin(Data,sSession)) {
		SendRecvMsg("LOG RESETLOG");
	}
	Out.Replace("[Clear]", _GetPlainResString(IDS_PW_RESET));
	Out.Replace("[Log]", _SpecialChars(SendRecvMsg("LOG GETALLLOGENTRIES"))+"<br><a name=\"end\"></a>" );
	Out.Replace("[Session]", sSession);

	return Out;
}

wxString CWebServer::_GetServerInfo(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data.sURL, "ses");

	wxString Out = pThis->m_Templates.sServerInfo;

	if (_ParseURL(Data.sURL, "clear") == "yes") {
		SendRecvMsg("LOG CLEARSERVERINFO");
	}
	Out.Replace("[Clear]", _GetPlainResString(IDS_PW_RESET));
	Out.Replace("[ServerInfo]", _SpecialChars(wxString(SendRecvMsg("LOG GETSERVERINFO"))));
	Out.Replace("[Session]", sSession);

	return Out;
}

//shakraw, this is useless in amule 'cause debuglog and log windows are the same.
//so, at the moment, GETALLDEBUGLOGENTRIES has the same behaviour of GETALLLOGENTRIES.
//Note that, when clearing, the log file ~/.aMule/logfile will not be removed here.
wxString CWebServer::_GetDebugLog(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data.sURL, "ses");

	wxString Out = pThis->m_Templates.sDebugLog;

	if (_ParseURL(Data.sURL, "clear") == "yes" && IsSessionAdmin(Data,sSession)) {
		SendRecvMsg("LOG RESETDEBUGLOG");
	}
	Out.Replace("[Clear]", _GetPlainResString(IDS_PW_RESET));

	Out.Replace("[DebugLog]", _SpecialChars(SendRecvMsg("LOG GETALLDEBUGLOGENTRIES"))+"<br><a name=\"end\"></a>" );
	Out.Replace("[Session]", sSession);

	return Out;
}

wxString CWebServer::_GetStats(ThreadData Data) {
	printf("***_GetStats arrived\n");
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data.sURL, "ses");

	// refresh statistics.. ARGH. NO NO NO NO
	// (it will be done in statisticsdlg and in main thread)
	//theApp.amuledlg->statisticswnd.ShowStatistics(true);
	
	wxString Out = pThis->m_Templates.sStats;
	
	wxString sStats = SendRecvMsg("WEBPAGE STATISTICS");
	int brk = sStats.First("\t");
	
	Out.Replace("[STATSDATA]", sStats.Left(brk));
	Out.Replace("[Stats]", sStats.Mid(brk+1));

	return Out;
}

wxString CWebServer::_GetPreferences(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	int brk;
	
	wxString sSession = _ParseURL(Data.sURL, "ses");

	wxString Out = pThis->m_Templates.sPreferences;
	Out.Replace("[Session]", sSession);

	if ((_ParseURL(Data.sURL, "saveprefs") == "true") && IsSessionAdmin(Data,sSession) ) {
		wxString prefList("");
		if (_ParseURL(Data.sURL, "gzip") == "true" || _ParseURL(Data.sURL, "gzip") == "on") {
			prefList.Append("1\t");
		}
		if (_ParseURL(Data.sURL, "gzip") == "false" || _ParseURL(Data.sURL, "gzip") == "") {
			prefList.Append("0\t");
		}
		if (_ParseURL(Data.sURL, "showuploadqueue") == "true" || _ParseURL(Data.sURL, "showuploadqueue") == "on" ) {
			pThis->m_Params.bShowUploadQueue = true;
		}
		if(_ParseURL(Data.sURL, "showuploadqueue") == "false" || _ParseURL(Data.sURL, "showuploadqueue") == "") {
			pThis->m_Params.bShowUploadQueue = false;
		}
		if (_ParseURL(Data.sURL, "refresh") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data.sURL, "refresh")));
		}
		if (_ParseURL(Data.sURL, "maxdown") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data.sURL, "maxdown")));
		}
		if (_ParseURL(Data.sURL, "maxup") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data.sURL, "maxup")));
		}
		
		if (_ParseURL(Data.sURL, "maxcapdown") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data.sURL, "maxcapdown")));
		}
		if (_ParseURL(Data.sURL, "maxcapup") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data.sURL, "maxcapup")));
		}

		if (_ParseURL(Data.sURL, "maxsources") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data.sURL, "maxsources")));
		}
		if (_ParseURL(Data.sURL, "maxconnections") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data.sURL, "maxconnections")));
		}
		if (_ParseURL(Data.sURL, "maxconnectionsperfive") != "") {
			prefList+=wxString::Format("%d\t", atoi(_ParseURL(Data.sURL, "maxconnectionsperfive")));
		}

		prefList+=wxString::Format("%d\t", ((_ParseURL(Data.sURL, "fullchunks").MakeLower() == "on") ? 1 : 0));
		prefList+=wxString::Format("%d\t", ((_ParseURL(Data.sURL, "firstandlast").MakeLower() == "on") ? 1 : 0));
		
		SendRecvMsg(wxString::Format("WEBPAGE SETPREFERENCES %s", prefList.GetData()).GetData());
	}

	// Fill form
	//sPreferencesList formatted as: %d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d
	wxString sPreferences = SendRecvMsg("WEBPAGE GETPREFERENCES");
	brk = sPreferences.First("\t");
	if (atoi(sPreferences.Left(brk).GetData())) {
		Out.Replace("[UseGzipVal]", "checked");
	} else {
		Out.Replace("[UseGzipVal]", "");
	}
    if(pThis->m_Params.bShowUploadQueue) {
		Out.Replace("[ShowUploadQueueVal]", "checked");
	} else {
		Out.Replace("[ShowUploadQueueVal]", "");
	}
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	if (atoi(sPreferences.Left(brk).GetData())) {
		Out.Replace("[FirstAndLastVal]", "checked");
	} else {
		Out.Replace("[FirstAndLastVal]", "");
	}
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	if (atoi(sPreferences.Left(brk).GetData())) {
		Out.Replace("[FullChunksVal]", "checked");
	} else {
		Out.Replace("[FullChunksVal]", "");
	}
	
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	wxString sRefresh; sRefresh.Printf("%d", atoi(sPreferences.Left(brk).GetData()));
	Out.Replace("[RefreshVal]", sRefresh);
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	sRefresh.Printf("%d", atoi(sPreferences.Left(brk).GetData()));
	Out.Replace("[MaxSourcesVal]", sRefresh);
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	sRefresh.Printf("%d", atoi(sPreferences.Left(brk).GetData()));
	Out.Replace("[MaxConnectionsVal]", sRefresh);
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	sRefresh.Printf("%d", atoi(sPreferences.Left(brk).GetData()));
	Out.Replace("[MaxConnectionsPer5Val]", sRefresh);

	Out.Replace("[KBS]", _GetPlainResString(IDS_KBYTESEC)+":");
	Out.Replace("[FileSettings]", wxString(_GetPlainResString(IDS_WEB_FILESETTINGS)+":"));
	Out.Replace("[LimitForm]", _GetPlainResString(IDS_WEB_CONLIMITS)+":");
	Out.Replace("[MaxSources]", _GetPlainResString(IDS_PW_MAXSOURCES)+":");
	Out.Replace("[MaxConnections]", _GetPlainResString(IDS_PW_MAXC)+":");
	Out.Replace("[MaxConnectionsPer5]", _GetPlainResString(IDS_MAXCON5SECLABEL)+":");
	Out.Replace("[UseGzipForm]", _GetPlainResString(IDS_WEB_GZIP_COMPRESSION));
	Out.Replace("[UseGzipComment]", _GetPlainResString(IDS_WEB_GZIP_COMMENT));
	Out.Replace("[ShowUploadQueueForm]", _GetPlainResString(IDS_WEB_SHOW_UPLOAD_QUEUE));
	Out.Replace("[ShowUploadQueueComment]", _GetPlainResString(IDS_WEB_UPLOAD_QUEUE_COMMENT));
	Out.Replace("[ShowQueue]", _GetPlainResString(IDS_WEB_SHOW_UPLOAD_QUEUE));
	Out.Replace("[HideQueue]", _GetPlainResString(IDS_WEB_HIDE_UPLOAD_QUEUE));
	Out.Replace("[RefreshTimeForm]", _GetPlainResString(IDS_WEB_REFRESH_TIME));
	Out.Replace("[RefreshTimeComment]", _GetPlainResString(IDS_WEB_REFRESH_COMMENT));
	Out.Replace("[SpeedForm]", _GetPlainResString(IDS_SPEED_LIMITS));
	Out.Replace("[MaxDown]", _GetPlainResString(IDS_DOWNLOAD));
	Out.Replace("[MaxUp]", _GetPlainResString(IDS_PW_CON_UPLBL));
	Out.Replace("[SpeedCapForm]", _GetPlainResString(IDS_CAPACITY_LIMITS));
	Out.Replace("[MaxCapDown]", _GetPlainResString(IDS_DOWNLOAD));
	Out.Replace("[MaxCapUp]", _GetPlainResString(IDS_PW_CON_UPLBL));
	Out.Replace("[TryFullChunks]", _GetPlainResString(IDS_FULLCHUNKTRANS));
	Out.Replace("[FirstAndLast]", _GetPlainResString(IDS_DOWNLOADMOVIECHUNKS));
	Out.Replace("[WebControl]", _GetPlainResString(IDS_WEB_CONTROL));
	Out.Replace("[aMuleAppName]", "aMule");
	Out.Replace("[Apply]", _GetPlainResString(IDS_PW_APPLY));

	wxString sT;
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	int n = atoi(sPreferences.Left(brk).GetData());
	if (n < 0 || n == 65535) n = 0;
	sT.Printf("%d", n);
	Out.Replace("[MaxDownVal]", sT);
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	n = atoi(sPreferences.Left(brk).GetData());
	if (n < 0 || n == 65535) n = 0;
	sT.Printf("%d", n);
	Out.Replace("[MaxUpVal]", sT);
	sPreferences=sPreferences.Mid(brk+1); brk=sPreferences.First("\t");
	sT.Printf("%d", atoi(sPreferences.Left(brk).GetData()));
	Out.Replace("[MaxCapDownVal]", sT);
	sPreferences=sPreferences.Mid(brk+1);
	sT.Printf("%d", atoi(sPreferences.GetData()));
	Out.Replace("[MaxCapUpVal]", sT);

	return Out;
}

wxString CWebServer::_GetLoginScreen(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data.sURL, "ses");

	wxString Out = "";

	Out += pThis->m_Templates.sLogin;

	Out.Replace("[CharSet]", _GetWebCharSet());
	Out.Replace("[aMulePlus]", "aMule");
	Out.Replace("[aMuleAppName]", "aMule");
	Out.Replace("[version]", VERSION); //shakraw - was CURRENT_VERSION_LONG);
	Out.Replace("[Login]", _GetPlainResString(IDS_WEB_LOGIN));
	Out.Replace("[EnterPassword]", _GetPlainResString(IDS_WEB_ENTER_PASSWORD));
	Out.Replace("[LoginNow]", _GetPlainResString(IDS_WEB_LOGIN_NOW));
	Out.Replace("[WebControl]", _GetPlainResString(IDS_WEB_CONTROL));

	return Out;
}

wxString CWebServer::_GetConnectedServer(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data.sURL, "ses");

	wxString HTTPTemp = "";
	char	HTTPTempC[100] = "";
	wxString OutS = pThis->m_Templates.sConnectedServer;
	OutS.Replace("[ConnectedServer]", _GetPlainResString(IDS_PW_SERVER));
	OutS.Replace("[Servername]", _GetPlainResString(IDS_SL_SERVERNAME));
	OutS.Replace("[Status]", _GetPlainResString(IDS_STATUS));
	OutS.Replace("[Usercount]", _GetPlainResString(IDS_LUSERS));
	OutS.Replace("[Action]", _GetPlainResString(IDS_CONNECTING));
	OutS.Replace("[URL_Disconnect]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=server&c=disconnect"):GetPermissionDenied());
	OutS.Replace("[URL_Connect]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=server&c=connect"):GetPermissionDenied());
	OutS.Replace("[Disconnect]", _GetPlainResString(IDS_IRC_DISCONNECT));
	OutS.Replace("[Connect]", _GetPlainResString(IDS_CONNECTTOANYSERVER));
	OutS.Replace("[URL_ServerOptions]", IsSessionAdmin(Data,sSession)?wxString("?ses=" + sSession + "&w=server&c=options"):GetPermissionDenied());
	OutS.Replace("[ServerOptions]", wxString(_GetPlainResString(IDS_SERVER)+_GetPlainResString(IDS_EM_PREFS)));
	OutS.Replace("[WebSearch]", _GetPlainResString(IDS_SW_WEBBASED));

	wxString sServerStat = SendRecvMsg("SERVER STAT");
	int brk=sServerStat.First("\t");
	if (sServerStat.Left(brk) == "Connected") {
		sServerStat=sServerStat.Mid(brk+1);brk=sServerStat.First("\t");
		if (sServerStat.Left(brk) == "High ID")
			OutS.Replace("[1]", _GetPlainResString(IDS_CONNECTED));
		else
			OutS.Replace("[1]", _GetPlainResString(IDS_CONNECTED) + " (" + _GetPlainResString(IDS_IDLOW) + ")");

		sServerStat=sServerStat.Mid(brk+1);brk=sServerStat.First("\t");
		OutS.Replace("[2]", sServerStat.Left(brk));
		
		sServerStat=sServerStat.Mid(brk+1);brk=sServerStat.First("\t");
		sprintf(HTTPTempC, "%10i", atoi(sServerStat.Left(brk).GetData()));
		HTTPTemp = HTTPTempC;												
		OutS.Replace("[3]", HTTPTemp);

	} else if (sServerStat.Left(brk) == "Connecting") {
		OutS.Replace("[1]", _GetPlainResString(IDS_CONNECTING));
		OutS.Replace("[2]", "");
		OutS.Replace("[3]", "");
	} else {
		OutS.Replace("[1]", _GetPlainResString(IDS_DISCONNECTED));
		OutS.Replace("[2]", "");
		OutS.Replace("[3]", "");
	}

	return OutS;
}


// We have to add gz-header and some other stuff
// to standard zlib functions
// in order to use gzip in web pages
int CWebServer::_GzipCompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level) { 
	const static int gz_magic[2] = {0x1f, 0x8b}; // gzip magic header
	int err;
	uLong crc;
	z_stream stream = {0};
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;
	crc = crc32(0L, Z_NULL, 0);
	// init Zlib stream
	// NOTE windowBits is passed < 0 to suppress zlib header
	err = deflateInit2(&stream, level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (err != Z_OK)
		return err;

	sprintf((char*)dest , "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
		Z_DEFLATED, 0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/, 255);
	// wire buffers
	stream.next_in = (Bytef*) source ;
	stream.avail_in = (uInt)sourceLen;
	stream.next_out = ((Bytef*) dest) + 10;
	stream.avail_out = *destLen - 18;
	// doit
	err = deflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		deflateEnd(&stream);
		return err;
	}
	err = deflateEnd(&stream);
	crc = crc32(crc, (const Bytef *) source ,  sourceLen );
	//CRC
	*(((Bytef*) dest)+10+stream.total_out) = (Bytef)(crc & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+1) = (Bytef)((crc>>8) & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+2) = (Bytef)((crc>>16) & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+3) = (Bytef)((crc>>24) & 0xFF);
	// Length
	*(((Bytef*) dest)+10+stream.total_out+4) = (Bytef)( sourceLen  & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+5) = (Bytef)(( sourceLen >>8) & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+6) = (Bytef)(( sourceLen >>16) &	0xFF);
	*(((Bytef*) dest)+10+stream.total_out+7) = (Bytef)(( sourceLen >>24) &	0xFF);
	// return  destLength
	*destLen = 10 + stream.total_out + 8;
	return err;
}

bool CWebServer::_IsLoggedIn(ThreadData Data, long lSession) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	_RemoveTimeOuts(Data,lSession);

	// find our session
	// i should have used CMap there, but i like CArray more ;-)
	for (int i = 0; i < pThis->m_Params.Sessions.GetSize(); i++) {
		if (pThis->m_Params.Sessions[i]->lSession == lSession && lSession != 0) {
			// if found, also reset expiration time
			pThis->m_Params.Sessions[i]->startTime = time(NULL);
			return true;
		}
	}

	return false;
}

void CWebServer::_RemoveTimeOuts(ThreadData Data, long lSession) {
	// remove expired sessions
	CWebServer *pThis = (CWebServer *)Data.pThis;
	pThis->UpdateSessionCount();
}

bool CWebServer::_RemoveSession(ThreadData Data, long lSession) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	// find our session
	for(int i = 0; i < pThis->m_Params.Sessions.GetSize(); i++) {
		if (pThis->m_Params.Sessions[i]->lSession == lSession && lSession != 0) {
			pThis->m_Params.Sessions.RemoveAt(i);
			SendRecvMsg(wxString::Format("LOG ADDLOGLINE %s", getResString(IDS_WEB_SESSIONEND).GetData()));
			return true;
		}
	}
	return false;
}

Session CWebServer::GetSessionByID(ThreadData Data,long sessionID) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	
	if (pThis != NULL) {
		for (int i = 0; i < pThis->m_Params.Sessions.GetSize(); i++) {
			if (pThis->m_Params.Sessions[i]->lSession == sessionID && sessionID != 0)
				return *(pThis->m_Params.Sessions.GetAt(i));
		}
	}

	Session ses;
	ses.admin=false;
	ses.startTime = 0;

	return ses;
}

bool CWebServer::IsSessionAdmin(ThreadData Data,wxString SsessionID) {
	long sessionID=atoll(SsessionID);
	CWebServer *pThis = (CWebServer *)Data.pThis;
	
	if (pThis != NULL) {
		for (int i = 0; i < pThis->m_Params.Sessions.GetSize(); i++) {
			if (pThis->m_Params.Sessions[i]->lSession == sessionID && sessionID != 0)
				return pThis->m_Params.Sessions[i]->admin;
		}
	}
	return false;
}

wxString CWebServer::GetPermissionDenied() {
	return "javascript:alert(\'"+_GetPlainResString(IDS_ACCESSDENIED)+"\')";
}

bool CWebServer::_GetFileHash(wxString sHash, uchar *FileHash) {
	char hex_byte[3];
	int byte;
	hex_byte[2] = '\0';
	for (int i = 0; i < 16; i++) {
		hex_byte[0] = sHash.GetChar(i*2);
		hex_byte[1] = sHash.GetChar((i*2 + 1));
		sscanf(hex_byte, "%02x", &byte);
		FileHash[i] = (uchar)byte;
	}
	return true;
}

wxString CWebServer::_GetPlainResString(UINT nID, bool noquot) {
	wxString sRet = getResString(nID);
	sRet.Replace("&", "");
	if (noquot) {
        sRet.Replace("'", "\\'");
		sRet.Replace("\n", "\\n");
	}

	return sRet;
}

// EC + kuchin
wxString CWebServer::_GetWebCharSet() {
#if 0
	switch (theApp.glob_prefs->GetLanguageID())
	{
		case MAKELANGID(LANG_POLISH,SUBLANG_DEFAULT):				return "windows-1250";
		case MAKELANGID(LANG_RUSSIAN,SUBLANG_DEFAULT):				return "windows-1251";
		case MAKELANGID(LANG_GREEK,SUBLANG_DEFAULT):				return "ISO-8859-7";
		case MAKELANGID(LANG_HEBREW,SUBLANG_DEFAULT):				return "ISO-8859-8";
		case MAKELANGID(LANG_KOREAN,SUBLANG_DEFAULT):				return "EUC-KR";
		case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED):	return "GB2312";
		case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL):	return "Big5";
		case MAKELANGID(LANG_LITHUANIAN,SUBLANG_DEFAULT):			return "windows-1257";
		case MAKELANGID(LANG_TURKISH,SUBLANG_DEFAULT):				return "windows-1254";
	}
#endif
	// Western (Latin) includes Catalan, Danish, Dutch, English, Faeroese, Finnish, French,
	// German, Galician, Irish, Icelandic, Italian, Norwegian, Portuguese, Spanish and Swedish
	return "ISO-8859-1";
}

// Ornis: creating the progressbar. colored if ressources are given/available
wxString CWebServer::_GetDownloadGraph(ThreadData Data,wxString filehash) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";
	
	// cool style
	wxString progresscolor[12];
	progresscolor[0]="transparent.gif";
	progresscolor[1]="black.gif";
	progresscolor[2]="yellow.gif";
	progresscolor[3]="red.gif";

	progresscolor[4]="blue1.gif";
	progresscolor[5]="blue2.gif";
	progresscolor[6]="blue3.gif";
	progresscolor[7]="blue4.gif";
	progresscolor[8]="blue5.gif";
	progresscolor[9]="blue6.gif";

	progresscolor[10]="green.gif";
	progresscolor[11]="greenpercent.gif";

	wxString Out = "";
	wxString temp;

	wxString response = SendRecvMsg(wxString::Format("WEBPAGE PROGRESSBAR %d %s", pThis->m_Templates.iProgressbarWidth, filehash.GetData()).GetData());
	int brk=response.First("\t");
	
	if (atoi(response.Left(brk).GetData())) {
		temp.Printf(wxString(pThis->m_Templates.sProgressbarImgsPercent+"<br>").GetData(),progresscolor[11].GetData(),pThis->m_Templates.iProgressbarWidth);
		Out+=temp;
		temp.Printf(pThis->m_Templates.sProgressbarImgs.GetData(),progresscolor[10].GetData(),pThis->m_Templates.iProgressbarWidth);
		Out+=temp;
	} else {
		response=response.Mid(brk+1); brk=response.First("\t");
		wxString s_ChunkBar = response.Left(brk);
		// and now make a graph out of the array - need to be in a progressive way
		uint8 lastcolor=1;
		uint16 lastindex=0;
		for (uint16 i=0;i<pThis->m_Templates.iProgressbarWidth;i++) {
			if (lastcolor!= atoi(s_ChunkBar.Mid(i,1).GetData())) {
				if (i>lastindex) {
					temp.Printf(pThis->m_Templates.sProgressbarImgs.GetData() ,progresscolor[lastcolor].GetData(),i-lastindex);

					Out+=temp;
				}
				lastcolor=atoi(s_ChunkBar.Mid(i,1).GetData());
				lastindex=i;
			}
		}

		temp.Printf(pThis->m_Templates.sProgressbarImgs.GetData(), progresscolor[lastcolor].GetData(), pThis->m_Templates.iProgressbarWidth-lastindex);
		Out+=temp;

		response=response.Mid(brk+1);
		double percentComplete = atof(response.GetData());
		int complx=(int)((pThis->m_Templates.iProgressbarWidth/100.0)*percentComplete);
		(complx>0)?temp.Printf(wxString(pThis->m_Templates.sProgressbarImgsPercent+"<br>").GetData(), progresscolor[11].GetData(),complx):temp.Printf(wxString(pThis->m_Templates.sProgressbarImgsPercent+"<br>").GetData(),progresscolor[0].GetData(),5);
		Out=temp+Out;
	}

	return Out;
}

wxString CWebServer::_GetSearch(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL)
		return "";

	wxString sSession = _ParseURL(Data.sURL, "ses");
	wxString Out = pThis->m_Templates.sSearch;

	wxString downloads=_ParseURLArray(Data.sURL,"downloads");
	if (downloads != "" && IsSessionAdmin(Data,sSession) ) {
		int brk;
		while (downloads.Length()>0) {
			brk=downloads.First("|");
			SendRecvMsg(wxString::Format("SEARCH DOWNLOADFILE %s", downloads.Left(brk).GetData()));
			downloads=downloads.Mid(brk+1);
		}
	}

	wxString sToSearch = _ParseURL(Data.sURL, "tosearch");
	if (sToSearch != "" && IsSessionAdmin(Data,sSession)) {
		wxString sParams;
		sParams.Printf(sToSearch+"\n");
		sParams.Append(_ParseURL(Data.sURL, "type")+"\n");
		sParams.Append(wxString::Format("%ld\n", atol(_ParseURL(Data.sURL, "min").GetData())*1048576));
		sParams.Append(wxString::Format("%ld\n", atol(_ParseURL(Data.sURL, "max").GetData())*1048576));
		sParams.Append(_ParseURL(Data.sURL, "avail")+"\n");
		sParams.Append(_ParseURL(Data.sURL, "ext")+"\n");
		sParams.Append(_ParseURL(Data.sURL, "method")+"\n");

		SendRecvMsg(wxString::Format("SEARCH DONEWSEARCH %s", sParams.GetData()));
		Out.Replace("[Message]",_GetPlainResString(IDS_SW_SEARCHINGINFO));
	} else if (sToSearch != "" && !IsSessionAdmin(Data,sSession) ) {
		Out.Replace("[Message]",_GetPlainResString(IDS_ACCESSDENIED));
	} else 
		Out.Replace("[Message]","");

	wxString sSort = _ParseURL(Data.sURL, "sort");
	if (sSort.Length()>0) pThis->m_iSearchSortby=atoi(sSort);
	sSort = _ParseURL(Data.sURL, "sortAsc");
	if (sSort.Length()>0) pThis->m_bSearchAsc=atoi(sSort);

	wxString result = pThis->m_Templates.sSearchHeader + wxString(SendRecvMsg(wxString::Format("SEARCH WEBLIST %s\t%d\t%d", pThis->m_Templates.sSearchResultLine.GetData(), pThis->m_iSearchSortby, pThis->m_bSearchAsc)));
	
	if (atoi(SendRecvMsg("SEARCH GETCATCOUNT").GetData()) > 1)
		InsertCatBox(Out,0,pThis->m_Templates.sCatArrow);
	else
		Out.Replace("[CATBOX]","");

	Out.Replace("[SEARCHINFOMSG]","");
	Out.Replace("[RESULTLIST]", result);
	Out.Replace("[Result]", getResString(IDS_SW_RESULT) );
	Out.Replace("[Session]", sSession);
	Out.Replace("[WebSearch]", _GetPlainResString(IDS_SW_WEBBASED));
	Out.Replace("[Name]", _GetPlainResString(IDS_SW_NAME));
	Out.Replace("[Type]", _GetPlainResString(IDS_TYPE));
	Out.Replace("[Any]", _GetPlainResString(IDS_SEARCH_ANY));
	Out.Replace("[Archives]", _GetPlainResString(IDS_SEARCH_ARC));
	Out.Replace("[Audio]", _GetPlainResString(IDS_SEARCH_AUDIO));
	Out.Replace("[CD-Images]", _GetPlainResString(IDS_SEARCH_CDIMG));
	Out.Replace("[Pictures]", _GetPlainResString(IDS_SEARCH_PICS));
	Out.Replace("[Programs]", _GetPlainResString(IDS_SEARCH_PRG));
	Out.Replace("[Videos]", _GetPlainResString(IDS_SEARCH_VIDEO));
	Out.Replace("[Search]", _GetPlainResString(IDS_SW_SEARCHBOX));
	Out.Replace("[RefetchResults]", _GetPlainResString(IDS_SW_REFETCHRES));
	Out.Replace("[Download]", _GetPlainResString(IDS_DOWNLOAD));
	
	Out.Replace("[Filesize]", _GetPlainResString(IDS_DL_SIZE));
	Out.Replace("[Sources]", _GetPlainResString(IDS_DL_SOURCES));
	Out.Replace("[Filehash]", _GetPlainResString(IDS_FILEHASH));
	Out.Replace("[Filename]", _GetPlainResString(IDS_DL_FILENAME));
	Out.Replace("[WebSearch]", _GetPlainResString(IDS_SW_WEBBASED));

	Out.Replace("[SizeMin]", _GetPlainResString(IDS_SEARCHMINSIZE));
	Out.Replace("[SizeMax]", _GetPlainResString(IDS_SEARCHMAXSIZE));
	Out.Replace("[Availabl]", _GetPlainResString(IDS_SEARCHAVAIL));
	Out.Replace("[Extention]", _GetPlainResString(IDS_SEARCHEXTENTION));
	Out.Replace("[Global]", _GetPlainResString(IDS_GLOBALSEARCH2));
	Out.Replace("[MB]", _GetPlainResString(IDS_MBYTES));
		
	Out.Replace("[METHOD]", _GetPlainResString(IDS_METHOD));
	Out.Replace("[USESSERVER]", _GetPlainResString(IDS_SERVER));
	Out.Replace("[Global]", _GetPlainResString(IDS_GLOBALSEARCH));

	wxString val;
	val.Printf("%i",(pThis->m_iSearchSortby!=0 || (pThis->m_iSearchSortby==0 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace("[SORTASCVALUE0]", val);
	val.Printf("%i",(pThis->m_iSearchSortby!=1 || (pThis->m_iSearchSortby==1 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace("[SORTASCVALUE1]", val);
	val.Printf("%i",(pThis->m_iSearchSortby!=2 || (pThis->m_iSearchSortby==2 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace("[SORTASCVALUE2]", val);
	val.Printf("%i",(pThis->m_iSearchSortby!=3 || (pThis->m_iSearchSortby==3 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace("[SORTASCVALUE3]", val);
	
	return Out;
}

int CWebServer::UpdateSessionCount() {
	// remove old bans
	for (int i = 0; i < m_Params.badlogins.GetSize();) {
		uint32 diff= ::GetTickCount() - m_Params.badlogins[i]->timestamp ;
		if (diff >1000U*60U*15U && (::GetTickCount() > m_Params.badlogins[i]->timestamp)) {
			m_Params.badlogins.RemoveAt(i);
		} else 
			i++;
	}

	// count & remove old session
    m_Params.Sessions.GetSize();
	for (int i = 0; i < m_Params.Sessions.GetSize();) {
	  time_t ts=time(NULL)-m_Params.Sessions[i]->startTime;
	  if (ts > SESSION_TIMEOUT_SECS) {
	    m_Params.Sessions.RemoveAt(i);
	  } else
	  	i++;
	}

	return m_Params.Sessions.GetCount();
}

void CWebServer::InsertCatBox(wxString &Out,int preselect,wxString boxlabel,bool jump,bool extraCats) {
#if 0 //shakraw
	wxString tempBuf2,tempBuf3;
	if (jump) tempBuf2="onchange=GotoCat(this.form.cat.options[this.form.cat.selectedIndex].value)>";
	else tempBuf2=">";
	wxString tempBuf="<form><select name=\"cat\" size=\"1\""+tempBuf2;
	for (int i=0;i< theApp.glob_prefs->GetCatCount();i++) {
		tempBuf3= (i==preselect)? " selected":"";
		printf("before\n");
		tempBuf2.Printf("<option%s value=\"%i\">%s</option>",tempBuf3,i, (i==0)?_GetPlainResString(IDS_ALL).GetData():theApp.glob_prefs->GetCategory(i)->title );
		printf("after\n");
		tempBuf.Append(tempBuf2);
	}
	printf("hello 4\n");
	if (extraCats) {
		if (theApp.glob_prefs->GetCatCount()>1){
			tempBuf2.Printf("<option>------------</option>");
			tempBuf.Append(tempBuf2);
		}
	
		for (int i=(theApp.glob_prefs->GetCatCount()>1)?1:2;i<=12;i++) {
			tempBuf3= ( (-i)==preselect)? " selected":"";
			tempBuf2.Printf("<option%s value=\"%i\">%s</option>",tempBuf3,-i, GetSubCatLabel(-i) );
			tempBuf.Append(tempBuf2);
		}
	}
	tempBuf.Append("</select></form>");
	Out.Replace("[CATBOX]",boxlabel+tempBuf);
#endif
}

wxString CWebServer::GetSubCatLabel(int cat) {
	switch (cat) {
		case -1: return _GetPlainResString(IDS_ALLOTHERS);
		case -2: return _GetPlainResString(IDS_STATUS_NOTCOMPLETED);
		case -3: return _GetPlainResString(IDS_DL_TRANSFCOMPL);
		case -4: return _GetPlainResString(IDS_WAITING);
		case -5: return _GetPlainResString(IDS_DOWNLOADING);
		case -6: return _GetPlainResString(IDS_ERRORLIKE);
		case -7: return _GetPlainResString(IDS_PAUSED);
		case -8: return _GetPlainResString(IDS_STOPPED);
		case -9: return _GetPlainResString(IDS_VIDEO);
		case -10: return _GetPlainResString(IDS_AUDIO);
		case -11: return _GetPlainResString(IDS_SEARCH_ARC);
		case -12: return _GetPlainResString(IDS_SEARCH_CDIMG);
	}
	return "?";
}

// Elandal: moved from CUpDownClient
// Webserber [kuchin]
//shakraw, I think this should be re-moved to CUpDownClient...
/*wxString CWebServer::GetUploadFileInfo(CUpDownClient* client)
{
#if 0 //shakraw
	if(client == NULL) return "";
	wxString sRet;

	// build info text and display it
	sRet.Printf(CString(_("NickName: %s\n")), client->GetUserName(), client->GetUserID());
	if (client->reqfile)
	{
		sRet += CString(_("Requested:")) + wxString(client->reqfile->GetFileName()) + "\n";
		wxString stat;
		stat.Printf(CString(_("Filestats for this session: Accepted %d of %d requests"))+CString(_("Filestats for all sessions: Accepted %d of %d requests")),
			client->reqfile->statistic.GetAccepts(),
			client->reqfile->statistic.GetRequests(),
			CastItoXBytes(client->reqfile->statistic.GetTransfered()).GetData(),
			client->reqfile->statistic.GetAllTimeAccepts(),
			client->reqfile->statistic.GetAllTimeRequests(),
			CastItoXBytes(client->reqfile->statistic.GetAllTimeTransfered()).GetData());
		sRet += stat;
	}
	else
	{
		sRet += CString(_("Requested unknown file"));
	}
	return sRet;
#endif
	return "TODO: _GetUploadFileInfo";
}
*/
