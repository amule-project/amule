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
#pragma implementation
#include "TextClient.h"
//-------------------------------------------------------------------

#include <wx/intl.h>			// For _()
#if wxUSE_GUI
	#include <wx/menu.h>		// For wxMenu
	#include <wx/msgdlg.h>		// For wxMessageBox
	#include <wx/sizer.h>		// For wxBoxSizer
	#include <wx/statline.h>	// For wxStaticLine
#endif

#include <list>

//-------------------------------------------------------------------

#include "otherfunctions.h"
#include "ECcodes.h"
#include "ECPacket.h"
#include "ECSpecialTags.h"

#define APP_INIT_SIZE_X 640
#define APP_INIT_SIZE_Y 480

#define theApp (*((CamulecmdApp*)wxTheApp))

static CmdId commands[] = {
	{ wxT("quit"),		CMD_ID_QUIT },
	{ wxT("exit"),		CMD_ID_QUIT },
	{ wxT("help"),		CMD_ID_HELP },
	{ wxT("stats"),		CMD_ID_STATS },
	{ wxT("pause"),		CMD_ID_PAUSE },
	{ wxT("resume"),	CMD_ID_RESUME },
	{ wxT("connect"),	CMD_ID_CONN },
	{ wxT("disconnect"),	CMD_ID_DISCONN },
	{ wxT("reloadipf"),	CMD_ID_RELOAD_IPFILTER },
	{ wxT("setiplevel"),	CMD_ID_SET_IPLEVEL },
	{ wxT("iplevel"),	CMD_ID_IPLEVEL },
	{ wxT("list"),		CMD_ID_DLOAD_QUEUE },
	//{ wxT("find"),	CMD_ID_CMDSEARCH },
	{ wxT("shutdown"),	CMD_ID_SHUTDOWN },
	{ wxT("servers"),	CMD_ID_SERVERLIST },
	{ wxT("add"),		CMD_ID_ADDLINK },
	// backward compat commands
	{ wxT("connectto"),	CMD_ID_CONN_TO_SRV },
	{ wxT("serverstatus"),	CMD_ID_SRVSTAT },
	{ wxT("setipfilter"),	CMD_ID_SET_IPFILTER },
	{ wxT("getiplevel"),	CMD_ID_GET_IPLEVEL },
	{ wxT("show"),		CMD_ID_SHOW },
	{ wxEmptyString,	0 },
};

//-------------------------------------------------------------------
IMPLEMENT_APP (CamulecmdApp);
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
    amuleFrame_ID = 32000,
    Timer_ID
};


BEGIN_EVENT_TABLE(CamulecmdFrame, wxFrame)
	EVT_MENU(amulecmd_Quit, CamulecmdFrame::OnQuit)
	EVT_MENU(amulecmd_About, CamulecmdFrame::OnAbout)
	EVT_TEXT_ENTER(Event_Comand_ID, CamulecmdFrame::OnComandEnter)
	EVT_IDLE(CamulecmdFrame::OnIdle)
	EVT_TIMER(Timer_ID, CamulecmdFrame::OnTimerEvent)
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

	m_timer = new wxTimer(this, Timer_ID);
	m_timer->Start(5000);
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

void CamulecmdFrame::OnTimerEvent(wxTimerEvent &WXUNUSED(event))
{
	wxWakeUpIdle();
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
#else
void CamulecmdApp::OnInitCmdLine(wxCmdLineParser& amuleweb_parser)
{
	CaMuleExternalConnector::OnInitCmdLine(amuleweb_parser);
	amuleweb_parser.AddOption(wxT("c"), wxT("command"), 
		wxT("execute <str> and exit"), 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
}

bool CamulecmdApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	m_HasCmdOnCmdLine = parser.Found(wxT("command"), &m_CmdString);
	return CaMuleExternalConnector::OnCmdLineParsed(parser);
}

void CamulecmdApp::TextShell(const wxString& prompt, CmdId commands[])
{
	if (m_HasCmdOnCmdLine)
		Parse_Command(m_CmdString, commands);
	else
		CaMuleExternalConnector::TextShell(prompt, commands);
}
#endif

int CamulecmdApp::ProcessCommand(int CmdId)
{
	uint32 FileId;
	wxString args = GetCmdArgs();
	CECPacket *request = 0;
	std::list<CECPacket *> request_list;
	
	switch (CmdId) {
		case CMD_ID_HELP:
			ShowHelp();
			return 0; // No need to contact core to display help ;)
			
 		case CMD_ID_SRVSTAT:
		case CMD_ID_STATS:
			request = new CECPacket(EC_OP_STAT_REQ);
			request->AddTag(CECTag(EC_TAG_DETAIL_LEVEL, (uint8)EC_DETAIL_CMD));
			request_list.push_back(request);
			break;
			
		case CMD_ID_SHUTDOWN:
			request = new CECPacket(EC_OP_SHUTDOWN);
			request_list.push_back(request);
			break;
			
		case CMD_ID_CONN_TO_SRV:
 		case CMD_ID_CONN:
			if ( ! args.IsEmpty() ) {
				unsigned int ip[4];
				unsigned int port;
				int result = sscanf(unicode2char(args), "%d.%d.%d.%d:%d", &ip[0], &ip[1], &ip[2], &ip[3], &port);
				if (result == 5) {
					EC_IPv4_t addr;
					addr.ip[0] = ip[0];
					addr.ip[1] = ip[1];
					addr.ip[2] = ip[2];
					addr.ip[3] = ip[3];
					addr.port = port;
					request = new CECPacket(EC_OP_SERVER_CONNECT);
					request->AddTag(CECTag(EC_TAG_SERVER, addr));
					request_list.push_back(request);
				} else {
					Show(_("Invalid IP format. Use xxx.xxx.xxx.xxx:xxxx\n"));
					return 0;
				}
			} else {
				request = new CECPacket(EC_OP_SERVER_CONNECT);
				request_list.push_back(request);
			}
			break;

 		case CMD_ID_DISCONN:
			request = new CECPacket(EC_OP_SERVER_DISCONNECT);
			request_list.push_back(request);
			break;

		case CMD_ID_SERVERLIST:
			request = new CECPacket(EC_OP_GET_SERVER_LIST);
			request->AddTag(CECTag(EC_TAG_DETAIL_LEVEL, (uint8)EC_DETAIL_CMD));
			request_list.push_back(request);
			break;

			
		case CMD_ID_RELOAD_IPFILTER:
			request = new CECPacket(EC_OP_IPFILTER_RELOAD);
			request_list.push_back(request);
			break;
			
		case CMD_ID_SET_IPFILTER:
			if ( ! args.IsEmpty() ) {
				CECTag *reqTag = NULL;
				if (args.IsSameAs(wxT("ON"), false)) {
					reqTag = new CECTag(EC_TAG_IPFILTER_ENABLED, (uint8)1);
				} else if (args.IsSameAs(wxT("OFF"), false)) {
					reqTag = new CECTag(EC_TAG_IPFILTER_ENABLED, (uint8)0);
				} else {
					Show(_("This command requieres an argument. Valid arguments: 'on', 'off'\n"));
					return 0;
				}
				if (reqTag) {
					request = new CECPacket(EC_OP_SET_PREFERENCES);
					CECEmptyTag prefs(EC_TAG_PREFS_SECURITY);
					prefs.AddTag(*reqTag);
					request->AddTag(prefs);
					request_list.push_back(request);
				}
			} else {
				Show(_("This command requieres an argument. Valid arguments: 'on', 'off'\n"));
				return 0;
			}
			request = new CECPacket(EC_OP_GET_PREFERENCES);
			request->AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)EC_PREFS_SECURITY));
			request_list.push_back(request);
			break;

		case CMD_ID_DLOAD_QUEUE:
			request = new CECPacket(EC_OP_GET_DLOAD_QUEUE);
			request_list.push_back(request);
			break;
			
		case CMD_ID_PAUSE:
			if ( args.IsEmpty() ) {
				Show(_("This command requieres an argument. Valid arguments: 'all', a number.\n"));
				return 0;
			} else if (args.ToULong((unsigned long *)&FileId, 16)) {
				request = new CECPacket(EC_OP_PARTFILE_PAUSE);
				request->AddTag(CECTag(EC_TAG_ITEM_ID, FileId));
				request_list.push_back(request);
			} else if ( args.Left(3) == wxT("all") ) {
				CECPacket request_all (EC_OP_GET_DLOAD_QUEUE);
				CECPacket *reply_all = SendRecvMsg_v2(&request_all);
				
				for(int i = 0;i < reply_all->GetTagCount();i++) {
					CECTag *tag = reply_all->GetTagByIndex(i);
					FileId = tag->GetTagByName(EC_TAG_ITEM_ID)->GetInt32Data();
					request = new CECPacket(EC_OP_PARTFILE_PAUSE);
					request->AddTag(CECTag(EC_TAG_ITEM_ID, FileId));
					request_list.push_back(request);
				}
			} else {
				Show(_("Not a valid number\n"));
				return 0;
			}
			break;
			
		case CMD_ID_RESUME:
			if ( args.IsEmpty() ) {
				Show(_("This command requieres an argument. Valid arguments: 'all' or a number.\n"));
				return 0;
			} else if (args.ToULong((unsigned long *)&FileId, 16)) {
				request = new CECPacket(EC_OP_PARTFILE_RESUME);
				request->AddTag(CECTag(EC_TAG_ITEM_ID, FileId));
				request_list.push_back(request);
			} else if ( args.Left(3) == wxT("all") ) {
				CECPacket request_all (EC_OP_GET_DLOAD_QUEUE);
				CECPacket *reply_all = SendRecvMsg_v2(&request_all);
				
				for(int i = 0;i < reply_all->GetTagCount();i++) {
					CECTag *tag = reply_all->GetTagByIndex(i);
					FileId = tag->GetTagByName(EC_TAG_ITEM_ID)->GetInt32Data();
					request = new CECPacket(EC_OP_PARTFILE_RESUME);
					request->AddTag(CECTag(EC_TAG_ITEM_ID, FileId));
					request_list.push_back(request);
				}
			} else {
				Show(_("Not a valid number\n"));
				return 0;
			}
			break;
			
		case CMD_ID_SHOW:
			// kept for backwards compatibility. Now 'list'
			if ( args.Left(2) == wxT("dl") ) {
				request = new CECPacket(EC_OP_GET_DLOAD_QUEUE);
				request_list.push_back(request);
			} else if ( args.Left(2) == wxT("ul") ) {
				request = new CECPacket(EC_OP_GET_ULOAD_QUEUE);
				request_list.push_back(request);
			} else {
				Show(_("Hint: Use Show DL or Show UL\n"));
				return 0;
			}
			break;

		case CMD_ID_GET_IPLEVEL:
			// kept for backwards compatibility only
		case CMD_ID_SET_IPLEVEL:
			// kept for backwards compatibility only
		case CMD_ID_IPLEVEL:
			if ( !args.IsEmpty() ) {
				unsigned long int level = 0;
				if (args.ToULong(&level) == true && level < 256) {
					request = new CECPacket(EC_OP_SET_PREFERENCES);
					CECEmptyTag prefs(EC_TAG_PREFS_SECURITY);
					prefs.AddTag(CECTag(EC_TAG_IPFILTER_LEVEL, (uint8)level));
					request->AddTag(prefs);
					request_list.push_back(request);
				} else {
					Show(_("IPLevel parameter must be in the range of 0-255.\n"));
					return 0;
				}
			}
			request = new CECPacket(EC_OP_GET_PREFERENCES);
			request->AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)EC_PREFS_SECURITY));
			request_list.push_back(request);
			break;
		case CMD_ID_ADDLINK:
			if ( ! args.IsEmpty() ) {
				request = new CECPacket(EC_OP_ED2K_LINK);
				request->AddTag(CECTag(EC_TAG_STRING, args));
				request_list.push_back(request);
			} else {
				Show(_("This option requires an argument."));
			}
			break;

		default:
			return -1;
	}
	
	if ( ! request_list.empty() ) {
		std::list<CECPacket *>::iterator it = request_list.begin();
		while ( it != request_list.end() ) {
			CECPacket *curr = *it++;
			CECPacket *reply = SendRecvMsg_v2(curr);
			delete curr;
			if ( reply ) {
				Process_Answer_v2(reply);
				delete reply;
			}
		}
		request_list.resize(0);
	}
	
	return 0;
}

// Formats a filesize in bytes to make it suitable for displaying
wxString CastItoXBytes( uint64 count )
{

        if (count < 1024)
                return wxString::Format( wxT("%.0f %s"), (float)count, _("Bytes") );
        else if (count < 1048576)
                return wxString::Format( wxT("%.0f %s"), (float)count/1024, _("KB") );
        else if (count < 1073741824)
                return wxString::Format( wxT("%.2f %s"), (float)count/1048576, _("MB") );
        else if (count < 1099511627776LL)
                return wxString::Format( wxT("%.2f %s"), (float)count/1073741824, _("GB") );
        else
                return wxString::Format( wxT("%.3f %s"), (float)count/1099511627776LL, _("TB") );

        return _("Error");
}

/*
 * Format EC packet into text form for output to console
 * 
 */
void CamulecmdApp::Process_Answer_v2(CECPacket *response)
{
	wxString s;

	wxASSERT(response);

	switch (response->GetOpCode()) {
		case EC_OP_NOOP:
			s += _("Operation was successful.");
			break;
		case EC_OP_FAILED:
			if (response->GetTagCount()) {
				s += wxString(_("Request failed with the following error: ")) + wxString(wxGetTranslation(response->GetTagByIndex(0)->GetStringData())) + wxT(".");
			} else {
				s += _("Request failed with an unknown error.");
			}
			break;
		case EC_OP_PREFERENCES:
			{
				CECTag *tab = response->GetTagByName(EC_TAG_PREFS_SECURITY);
				if (tab) {
					s += wxString::Format(_("IPFilter is %s.\n"), (tab->GetTagByName(EC_TAG_IPFILTER_ENABLED) == NULL) ? _("OFF") : _("ON"));
					s += wxString::Format(_("Current IPFilter Level is %d.\n"), tab->GetTagByName(EC_TAG_IPFILTER_LEVEL)->GetInt8Data());
				}
			}
			break;		
		case EC_OP_STRINGS:
			for (int i = 0; i < response->GetTagCount(); ++i) {
				s += response->GetTagByIndex(i)->GetStringData();
			}
			break;
		case EC_OP_STATS:
			switch (response->GetTagByName(EC_TAG_CONNSTATE)->GetInt8Data()) {
				case 0:
					s = _("Not connected");
					break;
				case 1:
					s = _("Now connecting");
					break;
				case 2:
				case 3: {
						CECTag *server = response->GetTagByName(EC_TAG_CONNSTATE)->GetTagByIndex(0);
						s = _("Connected to ");
						s += server->GetTagByName(EC_TAG_SERVER_NAME)->GetStringData();
						s += wxT(" ") + server->GetIPv4Data().StringIP() + wxT(" ");
						s += response->GetTagByName(EC_TAG_CONNSTATE)->GetInt8Data() == 2 ? _("with LowID") : _("with HighID");
					}
					break;
			}
			s += _("\nDownload:\t") +
				CastItoXBytes(response->GetTagByName(EC_TAG_STATS_DL_SPEED)->GetInt32Data()) + _("/sec");
			s += _("\nUpload:\t") +
				CastItoXBytes(response->GetTagByName(EC_TAG_STATS_UL_SPEED)->GetInt32Data()) + _("/sec");

			s += wxString::Format(_("\nClients in queue: \t%d\n"),
				response->GetTagByName(EC_TAG_STATS_UL_QUEUE_LEN)->GetInt32Data());
			break;
		case EC_OP_DLOAD_QUEUE:
			for(int i = 0; i < response->GetTagCount(); i ++) {
				CEC_PartFile_Tag *tag = (CEC_PartFile_Tag *)response->GetTagByIndex(i);
				unsigned long filesize, donesize;
				filesize = tag->SizeFull();
				donesize = tag->SizeDone();
					
				s += wxString::Format(wxT("%08x "), tag->FileID()) +
					tag->FileName() +
					wxString::Format(wxT("\t [%.1f%%] %i/%i - "),
						((float)donesize) / ((float)filesize)*100.0,
						(int)tag->SourceXferCount(),
						(int)tag->SourceCount()) +
					tag->GetFileStatusString();
					if ( tag->SourceXferCount() > 0) {
						s += wxT(" ") + CastItoXBytes(tag->Speed()) + _("/sec");
					}
				s += wxT("\n");
			}
			break;
		case EC_OP_ULOAD_QUEUE:
			for(int i = 0; i < response->GetTagCount(); i ++) {
				CECTag *tag = response->GetTagByIndex(i);
				s += wxT("\n");
				s += wxString::Format(wxT("%08x "), tag->GetTagByName(EC_TAG_ITEM_ID)->GetInt32Data()) +
					tag->GetStringData() + wxT(" ") +
					tag->GetTagByName(EC_TAG_PARTFILE)->GetStringData() + wxT(" ") +
					CastItoXBytes(tag->GetTagByName(EC_TAG_PARTFILE_SIZE_XFER)->GetInt32Data()) + wxT(" ") +
					CastItoXBytes(tag->GetTagByName(EC_TAG_PARTFILE_SPEED)->GetInt32Data()) + _("/sec");
			}
			break;
		case EC_OP_SERVER_LIST:
			for(int i = 0; i < response->GetTagCount(); i ++) {
				CECTag *tag = response->GetTagByIndex(i);
				wxString ip = tag->GetIPv4Data().StringIP();
				ip.Append(' ', 24 - ip.Length());
				s += ip;
				s += tag->GetTagByName(EC_TAG_SERVER_NAME)->GetStringData();
				s += wxT("\n");
			}
			break;
		default:
			s += wxString::Format(_("Received an unknown reply from the server, OpCode = %#x."), response->GetOpCode());
	}
	Process_Answer(s);
}


void CamulecmdApp::ShowHelp() {
//                                  1         2         3         4         5         6         7         8
//                         12345678901234567890123456789012345678901234567890123456789012345678901234567890
	Show(_("\n----------------> Help: Available commands (case insensitive): <----------------\n\n"));
	Show(wxString(wxT("Connect [")) + wxString(_("server IP")) + wxString(wxT("]\t")) + wxString(_("Connect to given/random server. No warn if failed!\n")));
//	Show(wxString(wxT("ConnectTo [")) + wxString(_("name")) + wxString(wxT("] [")) + wxString(_("port")) + wxString(wxT("]:\t")) + wxString(_("Connect to specified server and port.\n")));
	Show(wxString(wxT("Disconnect:\t\t")) + wxString(_("Disconnect from server.\n")));
	Show(wxString(wxT("Servers:\t\t")) + wxString(_("Show server list.\n")));
//	Show(wxString(wxT("ServerStatus:\t\t")) + wxString(_("Tell us if connected/not connected.\n")));
	Show(wxString(wxT("Stats:\t\t\t")) + wxString(_("Shows status and statistics.\n")));
	Show(wxString(wxT("Show DL | UL:\t\t")) + wxString(_("Shows Download/Upload queue.\n")));
	Show(wxString(wxT("List <")) + wxString(_("pattern")) + wxString(wxT(">:\t\t")) + wxString(_("Lists or finds downloads by name or number.\n")));
	Show(wxString(wxT("Resume [n | all]:\t")) + wxString(_("Resume file number n (or 'all').\n")));
	Show(wxString(wxT("Pause [n | all]:\t")) + wxString(_("Pauses file number n (or 'all').\n")));
	Show(wxString(wxT("SetIPFilter <on | off>:\t")) + wxString(_("Turn on/off amule IPFilter.\n")));
	Show(wxString(wxT("ReloadIPF:\t\t")) + wxString(_("Reload IPFilter table from file.\n")));
//	Show(wxString(wxT("GetIPLevel:\t\t")) + wxString(_("Shows current IP Filter level.\n")));
//	Show(wxString(wxT("SetIPLevel <")) + wxString(_("new level")) + wxString(wxT(">:\t")) + wxString(_("Changes current IP Filter level.\n")));
	Show(wxString(wxT("IPLevel [")) + wxString(_("level")) + wxString(wxT("]:\t")) + wxString(_("Shows/Sets current IP Filter level.\n")));
	Show(wxString(wxT("Add <")) + wxString(_("ED2k Link")) + wxString(wxT(">\t\t")) + wxString(_("Adds <ED2k Link> to aMule.\n\t\t\t\tCurrently file and server links are supported.\n")));
	Show(wxString(wxT("Help:\t\t\t")) + wxString(_("Shows this help.\n")));	
	Show(wxString(wxT("Quit:\t\t\t")) + wxString(_("Exits Textclient.\n")));
	Show(wxString(wxT("Shutdown:\t\t\t")) + wxString(_("Shutdown amule\n")));
	Show(_("\n----------------------------> End of listing <----------------------------------\n"));
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
	frame->Show(true);
#else
int CamulecmdApp::OnRun() {
#endif
	ConnectAndRun(wxT("aMulecmd"), commands);
	
	return true;
}

