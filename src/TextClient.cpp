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
#include "ECcodes.h"
#include "ECPacket.h"

#define APP_INIT_SIZE_X 640
#define APP_INIT_SIZE_Y 480

#define theApp (*((CamulecmdApp*)wxTheApp))

static CmdId commands[] = {
	{ wxT("quit"),		CMD_ID_QUIT },
	{ wxT("exit"),		CMD_ID_QUIT },
	{ wxT("help"),		CMD_ID_HELP },
	{ wxT("stats"),		CMD_ID_STATS },
	{ wxT("status"),	CMD_ID_STATS },
	{ wxT("show"),		CMD_ID_SHOW },
	{ wxT("pause"),		CMD_ID_PAUSE },
	{ wxT("resume"),	CMD_ID_RESUME },
	{ wxT("serverstatus"),	CMD_ID_SRVSTAT },
	{ wxT("connect"),	CMD_ID_CONN },
	{ wxT("connectto"),	CMD_ID_CONN_TO_SRV },
	{ wxT("disconnect"),	CMD_ID_DISCONN },
	{ wxT("reloadipf"),	CMD_ID_RELOAD_IPFILTER },
	{ wxT("setipfilter"),	CMD_ID_SET_IPFILTER },
	{ wxT("getiplevel"),	CMD_ID_GET_IPLEVEL },
	{ wxT("setiplevel"),	CMD_ID_SET_IPLEVEL },
	{ wxT("iplevel"),	CMD_ID_IPLEVEL },
	{ wxT("list"),		CMD_ID_CMDSEARCH },
	{ wxT("find"),		CMD_ID_CMDSEARCH },
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
	amuleweb_parser.AddOption(wxT("c"), wxT("command"), wxT("execute <str> and exit"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
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
	long FileId;
	wxString msg;
	wxString args = GetCmdArgs();
	CECPacket *request = 0;
	switch (CmdId) {
		case CMD_ID_HELP:
			ShowHelp();
			return 0; // No need to contact core to display help ;)
			
		case CMD_ID_STATS:
			request = new CECPacket(EC_OP_STAT_REQ);
			break;
			
 		case CMD_ID_SRVSTAT:
		// kept for backwards compatibility only (now in Stats)
			msg = wxT("CONNSTAT");
			break;
			
 		case CMD_ID_CONN:
			if ( ! args.IsEmpty() ) {
				msg = wxT("SERVER CONNECT ") + args;
			} else {
				msg = wxT("RECONN");
			}
			break;
			
 		case CMD_ID_DISCONN:
			msg = wxT("DISCONN");
			break;

		case CMD_ID_CONN_TO_SRV:
		// kept for backwards compatibility only
			msg = wxT("SERVER CONNECT ") + args;
			break;
			
		case CMD_ID_RELOAD_IPFILTER:
			request = new CECPacket(EC_OP_IPFILTER_CMD);
			request->AddTag(CECTag(EC_TAG_STRING, wxT("RELOAD"));
			break;
			
		case CMD_ID_SET_IPFILTER:
			if ( ! args.IsEmpty() ) {
				request = new CECPacket(EC_OP_IPFILTER_CMD);
				request->AddTag(CECTag(EC_TAG_STRING, args));
			} else {
				Show(_("This command requieres an argument. Valid arguments: 'yes', 'no'\n"));
				return 0;
			}
			break;

		case CMD_ID_CMDSEARCH:
			msg = wxT("CMDSEARCH") + args;
			break;
			
		case CMD_ID_PAUSE:
			if ( args.IsEmpty() ) {
				Show(_("This command requieres an argument. Valid arguments: 'all', a number.\n"));
				return 0;
			} else if (args.IsNumber()) {
				args.ToLong(&FileId);
				msg.Printf(wxT("PAUSE%li"), FileId);
				//request = new CEECPacket(EC_OP_Q_FILE_CMD);
				//request->AddTag(CECTag(EC_TAG_PARTFILE, args));
			} else if ( args.Left(3) == wxT("all") ) {
				msg = wxT("PAUSEALL");
			} else {
				Show(_("Not a valid number\n"));
				return 0;
			}
			break;
			
		case CMD_ID_RESUME:
			if ( args.IsEmpty() ) {
				Show(_("This command requieres an argument. Valid arguments: 'all' or a number.\n"));
				return 0;
			} else if (args.IsNumber()) {
				args.ToLong(&FileId);
				msg.Printf(wxT("RESUME%li"), FileId);
			} else if ( args.Left(3) == wxT("all") ) {
				msg = wxT("RESUMEALL");
			} else {
				Show(_("Not a valid number\n"));
				return 0;
			}
			break;
			
		case CMD_ID_SHOW:
		// kept for backwards compatibility. Now 'list'
			if ( args.Left(2) == wxT("dl") ) {
				request = new CECPacket(EC_OP_GET_DLOAD_QUEUE);
			} else if ( args.Left(2) == wxT("ul") ) {
				msg = wxT("UL_QUEUE");
			} else {
				Show(_("Hint: Use Show DL or Show UL\n"));
				return 0;
			}
			break;

		case CMD_ID_IPLEVEL:
			if ( !args.IsEmpty() ) {
				request = new CECPacket(EC_OP_IPFILTER_CMD);
				request->AddTag(CECTag(EC_TAG_STRING, args));
			}
			break;

		case CMD_ID_GET_IPLEVEL:
		// kept for backwards compatibility only
			request = new CECPacket(EC_OP_IPFILTER_CMD);
			break;

		case CMD_ID_SET_IPLEVEL:
		// kept for backwards compatibility only
			if ( ! args.IsEmpty() ) {
				request = new CECPacket(EC_OP_IPFILTER_CMD);
				request->AddTag(CECTag(EC_TAG_STRING, args));
			} else {
				Show(_("This command requieres an argument. Valid arguments: 0 - 255\n"));
				return 0;
			}
			break;
			
		default:
			return -1;
	}
	
	// v1 or v2 command format
	if ( request ) {
		wxString answer;
		CECPacket *reply = SendRecvMsg_v2(request);
		if ( reply ) {
			answer = ECv2_Response2String(reply);
			delete reply;
		}
		Process_Answer(answer);
	} else {
		Process_Answer(SendRecvMsg(msg));
	}
	
	return 0;
}

/*
 * Format EC packet into text form for output to consol
 * 
 */
wxString ECv2_Response2String(CECPacket *response)
{
	wxString s;
	if ( !response ) {
		return wxEmptyString;
	}
	switch(response->GetOpCode()) {
		case EC_OP_STRINGS:
			s = response->GetTagByIndex(0)->GetTagString();
			break;
		case EC_OP_DLOAD_QUEUE:
			for(int i = 0; i < response->GetTagCount(); i ++) {
				CECTag *tag = response->GetTagByIndex(i);
				unsigned long filesize, donesize, src_count, src_xfer_count;
				tag->GetTagByName(EC_TAG_PARTFILE_SIZE_FULL)->GetTagString().ToULong(&filesize);
				tag->GetTagByName(EC_TAG_PARTFILE_SIZE_DONE)->GetTagString().ToULong(&donesize);
				tag->GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT)->GetTagString().ToULong(&src_count);
				tag->GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT_XFER)->GetTagString().ToULong(&src_xfer_count);
					
				s += tag->GetTagByName(EC_TAG_ITEM_ID)->GetTagString() + _(" ") +
					tag->GetTagString () +
					wxString::Format(wxT("\t [%.1f%%] %i/%i - "),
						((float)donesize) / ((float)filesize)*100.0, (int)src_xfer_count, (int)src_count) +
					tag->GetTagByName(EC_TAG_PARTFILE_STATUS)->GetTagString();
					
				s += _("\n");
			}
			break;
			
	}
	return s;
}


void CamulecmdApp::ShowHelp() {
//                                  1         2         3         4         5         6         7         8
//                         12345678901234567890123456789012345678901234567890123456789012345678901234567890
	Show(_("\n----------------> Help: Avalaible commands (case insensitive): <----------------\n\n"));
	Show(wxString(wxT("Connect [")) + wxString(_("ip")) + wxString(wxT("] [")) + wxString(_("port")) + wxString(wxT("]:\t")) + wxString(_("Connect to given/random server. No warn if failed!\n")));
//	Show(wxString(wxT("ConnectTo [")) + wxString(_("name")) + wxString(wxT("] [")) + wxString(_("port")) + wxString(wxT("]:\t")) + wxString(_("Connect to specified server and port.\n")));
	Show(wxString(wxT("Disconnect:\t\t")) + wxString(_("Disconnect from server.\n")));
//	Show(wxString(wxT("ServerStatus:\t\t")) + wxString(_("Tell us if connected/not connected.\n")));
	Show(wxString(wxT("Stats:\t\t\t")) + wxString(_("Shows status and statistics.\n")));
//	Show(wxString(wxT("Show DL:\t\t")) + wxString(_("Shows Download queue.\n")));
	Show(wxString(wxT("List <")) + wxString(_("pattern")) + wxString(wxT(">:\t\t")) + wxString(_("Lists or finds downloads by name or number.\n")));
	Show(wxString(wxT("Resume [n | all]:\t")) + wxString(_("Resume file number n (or 'all').\n")));
	Show(wxString(wxT("Pause [n | all]:\t")) + wxString(_("Pauses file number n (or 'all').\n")));
	Show(wxString(wxT("SetIPFilter <on | off>:\t")) + wxString(_("Turn on/of amule IPFilter.\n")));
	Show(wxString(wxT("ReloadIPF:\t\t")) + wxString(_("Reload IPFilter table from file.\n")));
//	Show(wxString(wxT("GetIPLevel:\t\t")) + wxString(_("Shows current IP Filter level.\n")));
//	Show(wxString(wxT("SetIPLevel <")) + wxString(_("new level")) + wxString(wxT(">:\t")) + wxString(_("Changes current IP Filter level.\n")));
	Show(wxString(wxT("IPLevel [")) + wxString(_("level")) + wxString(_("]:\t")) + wxString(_("Shows/Sets current IP Filter level.\n")));
	Show(wxString(wxT("Help:\t\t\t")) + wxString(_("Shows this help.\n")));	
	Show(wxString(wxT("Quit:\t\t\t")) + wxString(_("Exits Textclient.\n")));
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

