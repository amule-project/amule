//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 Angel Vidal (Kry) ( kry@amule.org / http://www.amule.org )
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//
	

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for VERSION
#endif

#ifndef __WXMSW__
	#include <unistd.h>
#endif

//-------------------------------------------------------------------
#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "TextClient.h"
#endif
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

#include "OtherFunctions.h"
#include "ECcodes.h"
#include "ECPacket.h"
#include "ECSpecialTags.h"
#include "Format.h"		// Needed for CFormat

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
//	{ wxT("list"),		CMD_ID_DLOAD_QUEUE }, // TODO: FIXME!
	//{ wxT("find"),	CMD_ID_CMDSEARCH },
	{ wxT("shutdown"),	CMD_ID_SHUTDOWN },
	{ wxT("servers"),	CMD_ID_SERVERLIST },
	{ wxT("add"),		CMD_ID_ADDLINK },
	// backward compat commands
	{ wxT("serverstatus"),	CMD_ID_SRVSTAT },
	{ wxT("setipfilter"),	CMD_ID_SET_IPFILTER },
	{ wxT("getiplevel"),	CMD_ID_GET_IPLEVEL },
	{ wxT("show"),		CMD_ID_SHOW },
	{ wxT("setupbwlimit"),	CMD_ID_SETUPBWLIMIT },
	{ wxT("setdownbwlimit"),	CMD_ID_SETDOWNBWLIMIT },
	{ wxT("getbwlimits"),	CMD_ID_GETBWLIMITS },
	{ wxT("statistics"),	CMD_ID_STATTREE },
	{ wxT("reloadshared"),	CMD_ID_RELOADSHARED },
	{ wxEmptyString,	0 },
};

//-------------------------------------------------------------------
IMPLEMENT_APP (CamulecmdApp)
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
	#ifdef CVSDATE
		msg = wxString::Format(
			_("amulecmd [DLG version] %s %s\n"
			"Using %s\n"
			"(c) aMule Dev Team"),
			wxT(VERSION), wxT(CVSDATE), wxVERSION_STRING);
	#else
		msg = wxString::Format(
			_("amulecmd [DLG version] %s\n"
			"Using %s\n"
			"(c) aMule Dev Team"),
			wxT(VERSION), wxVERSION_STRING);
	#endif
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
void CamulecmdApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	CaMuleExternalConnector::OnInitCmdLine(parser);
	parser.AddOption(wxT("c"), wxT("command"), 
		_("Execute <str> and exit."), 
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
	wxString args = GetCmdArgs();
	CECPacket *request = 0;
	std::list<CECPacket *> request_list;

	switch (CmdId) {
		case CMD_ID_HELP:
			ShowHelp();
			return 0; // No need to contact core to display help ;)

 		case CMD_ID_SRVSTAT:
		case CMD_ID_STATS:
			request = new CECPacket(EC_OP_STAT_REQ, EC_DETAIL_CMD);
			request_list.push_back(request);
			break;

		case CMD_ID_SHUTDOWN:
			request = new CECPacket(EC_OP_SHUTDOWN);
			request_list.push_back(request);
			break;

 		case CMD_ID_CONN:
			if ( !args.IsEmpty() ) {
				if ( args == wxT("kad") ) {
					request_list.push_back(new CECPacket(EC_OP_KAD_START));
				} else if ( args == wxT("ed2k") ) {
					request_list.push_back(new CECPacket(EC_OP_SERVER_CONNECT));
				} else {
					unsigned int ip[4];
					unsigned int port;
					// Not much we can do against this unicode2char.
					int result = sscanf(unicode2char(args), "%d.%d.%d.%d:%d", &ip[0], &ip[1], &ip[2], &ip[3], &port);
					if (result != 5) {
						// Try to resolve DNS -- good for dynamic IP servers
						wxString serverName(args.BeforeFirst(wxT(':')));
						long lPort;
						bool ok = args.AfterFirst(wxT(':')).ToLong(&lPort);
						port = (unsigned int)lPort;
						wxIPV4address a;
						a.Hostname(serverName);
						a.Service(port);
						result = sscanf(unicode2char(a.IPAddress()), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
						if (serverName.IsEmpty() || !ok || (result != 4)) {
							Show(_("Invalid IP format. Use xxx.xxx.xxx.xxx:xxxx\n"));
							return 0;
						}
					}
					EC_IPv4_t addr;
					addr.ip[0] = ip[0];
					addr.ip[1] = ip[1];
					addr.ip[2] = ip[2];
					addr.ip[3] = ip[3];
					addr.port = port;
					request = new CECPacket(EC_OP_SERVER_CONNECT);
					request->AddTag(CECTag(EC_TAG_SERVER, addr));
					request_list.push_back(request);
				}
			} else {
				request_list.push_back(new CECPacket(EC_OP_CONNECT));
			}
			break;

 		case CMD_ID_DISCONN:
			if ( !args.IsEmpty() ) {
				if ( args == wxT("kad") ) {
					request_list.push_back(new CECPacket(EC_OP_KAD_STOP));
				} else if ( args == wxT("ed2k") ) {
					request_list.push_back(new CECPacket(EC_OP_SERVER_DISCONNECT));
				} else {
					Show(_("Invalid argument. Valid arguments: 'ed2k', 'kad'.\n"));
					return 0;
				}
			} else {
				request_list.push_back(new CECPacket(EC_OP_DISCONNECT));
			}
			break;

		case CMD_ID_SERVERLIST:
			request = new CECPacket(EC_OP_GET_SERVER_LIST, EC_DETAIL_CMD);
			request_list.push_back(request);
			break;

		case CMD_ID_RELOAD_IPFILTER:
			request = new CECPacket(EC_OP_IPFILTER_RELOAD);
			request_list.push_back(request);
			break;

		case CMD_ID_SET_IPFILTER:
			if ( ! args.IsEmpty() ) {
				uint8 enabledFlag;
				if (args.IsSameAs(wxT("ON"), false)) {
					enabledFlag = 1;
				} else if (args.IsSameAs(wxT("OFF"), false)) {
					enabledFlag = 0;
				} else {
					Show(_("This command requieres an argument. Valid arguments: 'on', 'off'\n"));
					return 0;
				}
				request = new CECPacket(EC_OP_SET_PREFERENCES);
				CECEmptyTag prefs(EC_TAG_PREFS_SECURITY);
				prefs.AddTag(CECTag(EC_TAG_IPFILTER_ENABLED, enabledFlag));
				request->AddTag(prefs);
				request_list.push_back(request);
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
			} else if ( args.Left(3) == wxT("all") ) {
				CECPacket request_all(EC_OP_GET_DLOAD_QUEUE, EC_DETAIL_CMD);
				CECPacket *reply_all = SendRecvMsg_v2(&request_all);
				if (reply_all) {
					request = new CECPacket(EC_OP_PARTFILE_PAUSE);
					for(int i = 0;i < reply_all->GetTagCount();i++) {
						CECTag *tag = reply_all->GetTagByIndex(i);
						if (tag) {
							request->AddTag(CECTag(EC_TAG_PARTFILE, tag->GetMD4Data()));
						}
					}
					request_list.push_back(request);
					delete reply_all;
				}
			} else {
				CMD4Hash hash(args);
				if (!hash.IsEmpty()) {
					request = new CECPacket(EC_OP_PARTFILE_PAUSE);
					request->AddTag(CECTag(EC_TAG_PARTFILE, hash));
					request_list.push_back(request);
				} else {
					Show(_("Not a valid number\n"));
					return 0;
				}
			}
			break;

		case CMD_ID_RESUME:
			if ( args.IsEmpty() ) {
				Show(_("This command requieres an argument. Valid arguments: 'all' or a number.\n"));
				return 0;
			} else if ( args.Left(3) == wxT("all") ) {
				CECPacket request_all(EC_OP_GET_DLOAD_QUEUE, EC_DETAIL_CMD);
				CECPacket *reply_all = SendRecvMsg_v2(&request_all);
				if (reply_all) {
					request = new CECPacket(EC_OP_PARTFILE_RESUME);
					for(int i = 0;i < reply_all->GetTagCount();i++) {
						CECTag *tag = reply_all->GetTagByIndex(i);
						if (tag) {
							request->AddTag(CECTag(EC_TAG_PARTFILE, tag->GetMD4Data()));
						}
					}
					request_list.push_back(request);
					delete reply_all;
				}
			} else {
				CMD4Hash hash(args);
				if (!hash.IsEmpty()) {
					request = new CECPacket(EC_OP_PARTFILE_RESUME);
					request->AddTag(CECTag(EC_TAG_PARTFILE, hash));
					request_list.push_back(request);
				} else {
					Show(_("Not a valid number\n"));
					return 0;
				}
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
				//aMule doesn't like AICH links without |/| in front of h=
				if (args.Find(wxT("|h=")) > -1 && args.Find(wxT("|/|h=")) == -1) {
					args.Replace(wxT("|h="),wxT("|/|h="));
				}
				request = new CECPacket(EC_OP_ED2K_LINK);
				request->AddTag(CECTag(EC_TAG_STRING, args));
				request_list.push_back(request);
			} else {
				Show(_("This option requires an argument."));
			}
			break;
		case CMD_ID_SETUPBWLIMIT:
			if ( ! args.IsEmpty() ) {
				unsigned long int limit;
				if (args.ToULong(&limit)) {
					request = new CECPacket(EC_OP_SET_PREFERENCES);
					CECEmptyTag prefs(EC_TAG_PREFS_CONNECTIONS);
					prefs.AddTag(CECTag(EC_TAG_CONN_MAX_UL, (uint16)limit));
					request->AddTag(prefs);
					request_list.push_back(request);
				} else {
					Show(_("Invalid argument."));
				}
			} else {
				Show(_("This option requires an argument."));
			}
			request = new CECPacket(EC_OP_GET_PREFERENCES);
			request->AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)EC_PREFS_CONNECTIONS));
			request_list.push_back(request);
			break;
		case CMD_ID_SETDOWNBWLIMIT:
			if ( ! args.IsEmpty() ) {
				unsigned long int limit;
				if (args.ToULong(&limit)) {
					request = new CECPacket(EC_OP_SET_PREFERENCES);
					CECEmptyTag prefs(EC_TAG_PREFS_CONNECTIONS);
					prefs.AddTag(CECTag(EC_TAG_CONN_MAX_DL, (uint16)limit));
					request->AddTag(prefs);
					request_list.push_back(request);
				} else {
					Show(_("Invalid argument."));
				}
			} else {
				Show(_("This option requires an argument."));
			}
		case CMD_ID_GETBWLIMITS:
			request = new CECPacket(EC_OP_GET_PREFERENCES);
			request->AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)EC_PREFS_CONNECTIONS));
			request_list.push_back(request);
			break;
		case CMD_ID_STATTREE:
			request = new CECPacket(EC_OP_GET_STATSTREE);
			if (!args.IsEmpty()) {
				unsigned long int max_versions;
				if (args.ToULong(&max_versions)) {
					if (max_versions < 256) {
						request->AddTag(CECTag(EC_TAG_STATTREE_CAPPING, (uint8)max_versions));
					} else {
						delete request;
						Show(wxString(_("Invalid argument.")) + wxT(" (1-255)"));
						return 0;
					}
				} else {
					delete request;
					Show(wxString(_("Invalid argument.")) + wxT(" (1-255)"));
					return 0;
				}
			}
			request_list.push_back(request);
			break;
		case CMD_ID_RELOADSHARED:
			request = new CECPacket(EC_OP_SHAREDFILES_RELOAD);
			request_list.push_back(request);
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

// Formats a statistics (sub)tree to text
wxString StatTree2Text(CEC_StatTree_Node_Tag *tree, int depth)
{
	if (!tree) {
		return wxEmptyString;
	}
	wxString result = wxString(wxChar(' '), depth) + tree->GetDisplayString() + wxT("\n");
	for (int i = 0; i < tree->GetTagCount(); ++i) {
		CEC_StatTree_Node_Tag *tmp = (CEC_StatTree_Node_Tag*)tree->GetTagByIndex(i);
		if (tmp->GetTagName() == EC_TAG_STATTREE_NODE) {
			result += StatTree2Text(tmp, depth + 1);
		}
	}
	return result;
}

/*
 * Format EC packet into text form for output to console
 */
void CamulecmdApp::Process_Answer_v2(CECPacket *response)
{
	wxString s;
	wxString msgFailedUnknown(_("Request failed with an unknown error."));
	wxASSERT(response);
	switch (response->GetOpCode()) {
		case EC_OP_NOOP:
			s << _("Operation was successful.");
			break;
		case EC_OP_FAILED:
			if (response->GetTagCount()) {
				CECTag *tag = response->GetTagByIndex(0);
				if (tag) {
					s <<	CFormat(_("Request failed with the following error: %s")) % wxGetTranslation(tag->GetStringData());
				} else {
					s << msgFailedUnknown;
				}
			} else {
				s << msgFailedUnknown;
			}
			break;
		case EC_OP_SET_PREFERENCES:
			{
				const CECTag *tab = response->GetTagByNameSafe(EC_TAG_PREFS_SECURITY);
				const CECTag *ipfilterLevel = tab->GetTagByName(EC_TAG_IPFILTER_LEVEL);
				if (ipfilterLevel) {
					s << wxString::Format(_("IPFilter is %s.\n"),
						(tab->GetTagByName(EC_TAG_IPFILTER_ENABLED) == NULL) ? _("OFF") : _("ON"));
					s << wxString::Format(_("Current IPFilter Level is %d.\n"),
						ipfilterLevel->GetInt8Data());
				}
				tab = response->GetTagByNameSafe(EC_TAG_PREFS_CONNECTIONS);
				const CECTag *connMaxUL = tab->GetTagByName(EC_TAG_CONN_MAX_UL);
				const CECTag *connMaxDL = tab->GetTagByName(EC_TAG_CONN_MAX_DL);
				if (connMaxUL && connMaxDL) {
					s << wxString::Format(_("Bandwidth Limits: Up: %u kB/s, Down: %u kB/s.\n"),
						connMaxUL->GetInt16Data(),
						connMaxDL->GetInt16Data());
				}
			}
			break;
		case EC_OP_STRINGS:
			for (int i = 0; i < response->GetTagCount(); ++i) {
				CECTag *tag = response->GetTagByIndex(i);
				if (tag) {
					s << tag->GetStringData() << wxT("\n");
				} else {
				}
			}
			break;
		case EC_OP_STATS: {
			CEC_ConnState_Tag *connState = (CEC_ConnState_Tag*)response->GetTagByName(EC_TAG_CONNSTATE);
			if (connState) {
				switch (connState->ClientID()) {
				case 0:
					s = _("Not connected");
					break;
				case 0xffffffff:
					s = _("Now connecting");
					break;
				default:
					CECTag *server = connState ? connState->GetTagByIndex(0) : NULL;
					CECTag *serverName = server ? server->GetTagByName(EC_TAG_SERVER_NAME) : NULL;
					if (server && serverName) {
						s << CFormat(_("Connected to %s %s %s")) %
						 serverName->GetStringData() %
						 server->GetIPv4Data().StringIP() %
						 (connState->HaveLowID() ? _("with LowID") : _("with HighID"));
					}
					break;
				}
			}
			CECTag *tmpTag;
			if ((tmpTag = response->GetTagByName(EC_TAG_STATS_DL_SPEED)) != 0) {
				s <<	CFormat(_("\nDownload:\t%s")) % CastItoSpeed(tmpTag->GetInt32Data());
			}
			if ((tmpTag = response->GetTagByName(EC_TAG_STATS_UL_SPEED)) != 0) {
				s <<	CFormat(_("\nUpload:\t%s")) % CastItoSpeed(tmpTag->GetInt32Data());
			}
			if ((tmpTag = response->GetTagByName(EC_TAG_STATS_UL_QUEUE_LEN)) != 0) {
				s << 	wxString::Format(_("\nClients in queue:\t%d\n"), tmpTag->GetInt32Data());
			}
			if ((tmpTag = response->GetTagByName(EC_TAG_STATS_TOTAL_SRC_COUNT)) != 0) {
				s << 	wxString::Format(_("\nTotal sources:\t%d\n"), tmpTag->GetInt32Data());
			}
			break;
		}
		case EC_OP_DLOAD_QUEUE:
			for(int i = 0; i < response->GetTagCount(); ++i) {
				CEC_PartFile_Tag *tag =
					(CEC_PartFile_Tag *)response->GetTagByIndex(i);
				if (tag) {
					unsigned long filesize, donesize;
					filesize = tag->SizeFull();
					donesize = tag->SizeDone();
					s <<	tag->FileHashString() << wxT(" ") <<
						tag->FileName() <<
						wxString::Format(wxT("\t [%.1f%%] %i/%i - "),
							((float)donesize) / ((float)filesize)*100.0,
							(int)tag->SourceXferCount(),
							(int)tag->SourceCount()) <<
						tag->GetFileStatusString();
						if ( tag->SourceXferCount() > 0) {
							s << wxT(" ") + CastItoSpeed(tag->Speed());
						}
					s << wxT("\n");
				}
			}
			break;
		case EC_OP_ULOAD_QUEUE:
			for(int i = 0; i < response->GetTagCount(); ++i) {
				CECTag *tag = response->GetTagByIndex(i);
				CECTag *clientName = tag ? tag->GetTagByName(EC_TAG_CLIENT_NAME) : NULL;
				CECTag *partfileName = tag ? tag->GetTagByName(EC_TAG_PARTFILE_NAME) : NULL;
				CECTag *partfileSizeXfer = tag ? tag->GetTagByName(EC_TAG_PARTFILE_SIZE_XFER) : NULL;
				CECTag *partfileSpeed = tag ? tag->GetTagByName(EC_TAG_CLIENT_UP_SPEED) : NULL;
				if (tag && clientName && partfileName && partfileSizeXfer && partfileSpeed) {
					s <<	wxT("\n") <<
						wxString::Format(wxT("%10u "), tag->GetInt32Data()) <<
						clientName->GetStringData() << wxT(" ") <<
						partfileName->GetStringData() << wxT(" ") <<
						CastItoXBytes(partfileSizeXfer->GetInt32Data()) << wxT(" ") <<
						CastItoSpeed(partfileSpeed->GetInt32Data());
				}
			}
			break;
		case EC_OP_SERVER_LIST:
			for(int i = 0; i < response->GetTagCount(); i ++) {
				CECTag *tag = response->GetTagByIndex(i);
				CECTag *serverName = tag ? tag->GetTagByName(EC_TAG_SERVER_NAME) : NULL;
				if (tag && serverName) {
					wxString ip = tag->GetIPv4Data().StringIP();
					ip.Append(' ', 24 - ip.Length());
					s << ip << serverName->GetStringData() << wxT("\n");
				}
			}
			break;
		case EC_OP_STATSTREE:
			s << StatTree2Text((CEC_StatTree_Node_Tag*)response->GetTagByName(EC_TAG_STATTREE_NODE), 0);
			break;
		default:
			s << wxString::Format(_("Received an unknown reply from the server, OpCode = %#x."), response->GetOpCode());
	}
	Process_Answer(s);
}


void CamulecmdApp::ShowHelp() {
//                                  1         2         3         4         5         6         7         8
//                         12345678901234567890123456789012345678901234567890123456789012345678901234567890
	Show(_("\n--------------------> Available commands (case insensitive): <------------------\n\n"));
	Show(wxT("Connect [<") + wxString(_("server IP:Port")) + wxT(">]:\t") + _("Connect to given server.\n"));
	Show(wxT("Connect [ed2k|kad]:\t") + wxString(_("Connect to random server/kad.\n")));
	Show(wxT("Connect:\t\t") + wxString(_("Connect to whatever is set in preferences.\n")));
	Show(wxT("Disconnect [ed2k|kad]:\t") + wxString(_("Disconnect from server/kad.\n")));
	Show(wxT("Disconnect:\t\t") + wxString(_("Disconnect from whatever aMule is connected to.\n")));
	Show(wxT("Servers:\t\t") + wxString(_("Show server list.\n")));
//	Show(wxT("ServerStatus:\t\t") + _("Tell us if connected/not connected.\n"));
	Show(wxT("Stats:\t\t\t") + wxString(_("Shows status and statistics.\n")));
	Show(wxT("Show DL | UL:\t\t") + wxString(_("Shows Download/Upload queue.\n")));
//	Show(wxT("List <") + _("pattern") + wxT(">:\t\t") + _("Lists or finds downloads by name or number.\n"));
	Show(wxT("Resume [n | all]:\t") + wxString(_("Resume file number n (or 'all').\n")));
	Show(wxT("Pause [n | all]:\t") + wxString(_("Pauses file number n (or 'all').\n")));
	Show(wxT("SetIPFilter <on | off>:\t") + wxString(_("Turn on/off amule IPFilter.\n")));
	Show(wxT("ReloadIPF:\t\t") + wxString(_("Reload IPFilter table from file.\n")));
//	Show(wxT("GetIPLevel:\t\t") + _("Shows current IP Filter level.\n"));
//	Show(wxT("SetIPLevel <") + _("new level") + wxT(">:\t") + _("Changes current IP Filter level.\n"));
	Show(wxT("IPLevel [") + wxString(_("level")) + wxT("]:\t") + _("Shows/Sets current IP Filter level.\n"));
	Show(wxT("Add <") + wxString(_("ED2k_Link")) + wxT(">:\t") + _("Adds <ED2k_Link> (file or server) to aMule.\n"));
	Show(wxT("SetUpBWLimit <") + wxString(_("limit")) + wxT(">:\t") + _("Sets maximum upload bandwidth.\n"));
	Show(wxT("SetDownBWLimit <") + wxString(_("limit")) + wxT(">:\t") + _("Sets maximum download bandwidth.\n"));
	Show(wxT("GetBWLimits:\t\t") + wxString(_("Displays bandwidth limits.\n")));
	Show(wxT("Statistics [") + wxString(_("detail")) + wxT("]:\t") + wxString(_("Displays full statistics tree.\n"
		"\t\t\tOptional 'detail' specifies how many client version\n"
		"\t\t\tentries should be shown (0=unlimited)\n")));
	Show(wxT("ReloadShared:\t\t") + wxString(_("Reload shared files list.\n")));
	Show(wxT("Help:\t\t\t") + wxString(_("Shows this help.\n")));	
	Show(wxT("Quit, exit:\t\t") + wxString(_("Exits aMulecmd.\n")));
	Show(wxT("Shutdown:\t\t") + wxString(_("Shutdown aMule\n")));
	Show(_("\n----------------------------> End of listing <----------------------------------\n"));
}

void CamulecmdApp::ShowGreet() {
	Show(wxT("\n---------------------------------\n"));
	Show(wxT("|       ") + wxString(_("aMule text client")) + wxT("       |\n"));
	Show(wxT("---------------------------------\n\n"));
	// Do not merge the line below, or translators could translate "Help"
	Show(CFormat(_("\nUse '%s' for command list\n\n")) % wxT("Help"));
}

#if wxUSE_GUI
bool CamulecmdApp::OnInit() {
	CaMuleExternalConnector::OnInit();
	frame = new CamulecmdFrame(wxT("amulecmd DLG"), wxPoint(50, 50), wxSize(APP_INIT_SIZE_X, APP_INIT_SIZE_Y));
	frame->Show(true);
#else
int CamulecmdApp::OnRun() {
#endif
	ConnectAndRun(wxT("aMulecmd"), wxT(VERSION), commands);
	
	return true;
}
