//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 Angel Vidal (Kry) ( kry@amule.org / http://www.amule.org )
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//
	

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for VERSION
#endif

#ifndef __WXMSW__
	#include <unistd.h>
#endif

#include "TextClient.h"

//-------------------------------------------------------------------

#include <wx/intl.h>			// For _()
#include <list>

//-------------------------------------------------------------------

#include <ec/ECCodes.h>
#include <ec/ECPacket.h>
#include <ec/ECSpecialTags.h>

#include <common/Format.h>		// Needed for CFormat

#include "OtherFunctions.h"


#define APP_INIT_SIZE_X 640
#define APP_INIT_SIZE_Y 480

#define theApp (*((CamulecmdApp*)wxTheApp))

//-------------------------------------------------------------------

enum {
	CMD_ID_STATUS,
	CMD_ID_RESUME,
	CMD_ID_PAUSE,
	CMD_ID_CANCEL,
	CMD_ID_CONNECT,
	CMD_ID_CONNECT_ED2K,
	CMD_ID_CONNECT_KAD,
	CMD_ID_DISCONNECT,
	CMD_ID_DISCONNECT_ED2K,
	CMD_ID_DISCONNECT_KAD,
	CMD_ID_RELOAD_SHARED,
	CMD_ID_RELOAD_IPFILTER,
 	CMD_ID_SET_IPFILTER_ON,
	CMD_ID_SET_IPFILTER_OFF,
	CMD_ID_SET_IPFILTER_LEVEL,
 	CMD_ID_GET_IPFILTER,
 	CMD_ID_GET_IPFILTER_STATE,
 	CMD_ID_GET_IPFILTER_LEVEL,
	CMD_ID_SHOW_UL,
	CMD_ID_SHOW_DL,
	CMD_ID_SHOW_LOG,
	CMD_ID_SHOW_SERVERS,
	CMD_ID_SHOW_SHARED,
	CMD_ID_RESET_LOG,
	CMD_ID_SHUTDOWN,
 	CMD_ID_ADDLINK,
 	CMD_ID_SET_BWLIMIT_UP,
 	CMD_ID_SET_BWLIMIT_DOWN,
 	CMD_ID_GET_BWLIMITS,
	CMD_ID_STATTREE,
	// IDs for deprecated commands
	CMD_ID_SET_IPFILTER
};


//-------------------------------------------------------------------
IMPLEMENT_APP (CamulecmdApp)
//-------------------------------------------------------------------

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

void CamulecmdApp::TextShell(const wxString& prompt)
{
	if (m_HasCmdOnCmdLine)
		Parse_Command(m_CmdString);
	else
		CaMuleExternalConnector::TextShell(prompt);
}

int CamulecmdApp::ProcessCommand(int CmdId)
{
	wxString args = GetCmdArgs();
	CECPacket *request = 0;
	std::list<CECPacket *> request_list;
	int tmp_int = 0;

	// Implementation of the deprecated command 'SetIPFilter'.
	if (CmdId == CMD_ID_SET_IPFILTER) {
		if ( ! args.IsEmpty() ) {
			if (args.IsSameAs(wxT("ON"), false)) {
				CmdId = CMD_ID_SET_IPFILTER_ON;
			} else if (args.IsSameAs(wxT("OFF"), false)) {
				CmdId = CMD_ID_SET_IPFILTER_OFF;
			} else {
				return CMD_ERR_INVALID_ARG;
			}
		} else {
			CmdId = CMD_ID_GET_IPFILTER_STATE;
		}
	}

	switch (CmdId) {
		case CMD_ID_STATUS:
			request_list.push_back(new CECPacket(EC_OP_STAT_REQ, EC_DETAIL_CMD));
			break;

		case CMD_ID_SHUTDOWN:
			request_list.push_back(new CECPacket(EC_OP_SHUTDOWN));
			break;

 		case CMD_ID_CONNECT:
			if ( !args.IsEmpty() ) {
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
				addr.m_ip[0] = ip[0];
				addr.m_ip[1] = ip[1];
				addr.m_ip[2] = ip[2];
				addr.m_ip[3] = ip[3];
				addr.m_port = port;
				request = new CECPacket(EC_OP_SERVER_CONNECT);
				request->AddTag(CECTag(EC_TAG_SERVER, addr));
				request_list.push_back(request);
			} else {
				request_list.push_back(new CECPacket(EC_OP_CONNECT));
			}
			break;

		case CMD_ID_CONNECT_ED2K:
			request_list.push_back(new CECPacket(EC_OP_SERVER_CONNECT));
			break;

 		case CMD_ID_CONNECT_KAD:
			request_list.push_back(new CECPacket(EC_OP_KAD_START));
			break;

 		case CMD_ID_DISCONNECT:
			request_list.push_back(new CECPacket(EC_OP_DISCONNECT));
			break;

		case CMD_ID_DISCONNECT_ED2K:
			request_list.push_back(new CECPacket(EC_OP_SERVER_DISCONNECT));
			break;

 		case CMD_ID_DISCONNECT_KAD:
			request_list.push_back(new CECPacket(EC_OP_KAD_STOP));
			break;

		case CMD_ID_RELOAD_SHARED:
			request_list.push_back(new CECPacket(EC_OP_SHAREDFILES_RELOAD));
			break;

		case CMD_ID_RELOAD_IPFILTER:
			request_list.push_back(new CECPacket(EC_OP_IPFILTER_RELOAD));
			break;

		case CMD_ID_SET_IPFILTER_ON:
			tmp_int = 1;
		case CMD_ID_SET_IPFILTER_OFF:
			{
				request = new CECPacket(EC_OP_SET_PREFERENCES);
				CECEmptyTag prefs(EC_TAG_PREFS_SECURITY);
				prefs.AddTag(CECTag(EC_TAG_IPFILTER_ENABLED, (uint8)tmp_int));
				request->AddTag(prefs);
				request_list.push_back(request);
			}
			CmdId = CMD_ID_GET_IPFILTER_STATE;
		case CMD_ID_GET_IPFILTER:
		case CMD_ID_GET_IPFILTER_STATE:
			request = new CECPacket(EC_OP_GET_PREFERENCES);
			request->AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)EC_PREFS_SECURITY));
			request_list.push_back(request);
			break;

		case CMD_ID_SET_IPFILTER_LEVEL:
			if (!args.IsEmpty()) // This 'if' must stay as long as we support the deprecated 'IPLevel' command.
			{
				unsigned long int level = 0;
				if (args.ToULong(&level) == true && level < 256) {
					request = new CECPacket(EC_OP_SET_PREFERENCES);
					CECEmptyTag prefs(EC_TAG_PREFS_SECURITY);
					prefs.AddTag(CECTag(EC_TAG_IPFILTER_LEVEL, (uint8)level));
					request->AddTag(prefs);
					request_list.push_back(request);
				} else {
					return CMD_ERR_INVALID_ARG;
				}
			}
			CmdId = CMD_ID_GET_IPFILTER_LEVEL;
		case CMD_ID_GET_IPFILTER_LEVEL:
			request = new CECPacket(EC_OP_GET_PREFERENCES);
			request->AddTag(CECTag(EC_TAG_SELECT_PREFS, (uint32)EC_PREFS_SECURITY));
			request_list.push_back(request);
			break;

		case CMD_ID_PAUSE:
		case CMD_ID_CANCEL:
		case CMD_ID_RESUME:
			if ( args.IsEmpty() ) {
				Show(_("This command requieres an argument. Valid arguments: 'all' or a number.\n"));
				return 0;
			} else if ( args.Left(3) == wxT("all") ) {
				CECPacket request_all(EC_OP_GET_DLOAD_QUEUE, EC_DETAIL_CMD);
				const CECPacket *reply_all = SendRecvMsg_v2(&request_all);
				if (reply_all) {
					switch(CmdId) {
						case CMD_ID_PAUSE:
							request = new CECPacket(EC_OP_PARTFILE_PAUSE); break;
						case CMD_ID_CANCEL:
							request = new CECPacket(EC_OP_PARTFILE_DELETE); break;
						case CMD_ID_RESUME:
							request = new CECPacket(EC_OP_PARTFILE_RESUME); break;
						default: wxASSERT(0);
					}
					
					for(int i = 0;i < reply_all->GetTagCount();i++) {
						const CECTag *tag = reply_all->GetTagByIndex(i);
						if (tag) {
							request->AddTag(CECTag(EC_TAG_PARTFILE, tag->GetMD4Data()));
						}
					}
					request_list.push_back(request);
					delete reply_all;
				}
			} else {
				CMD4Hash hash;
				if (hash.Decode(args.Trim(false).Trim(true))) {
					if (!hash.IsEmpty()) {
						switch(CmdId) {
							case CMD_ID_PAUSE:
								request = new CECPacket(EC_OP_PARTFILE_PAUSE); break;
							case CMD_ID_CANCEL:
								request = new CECPacket(EC_OP_PARTFILE_DELETE); break;
							case CMD_ID_RESUME:
								request = new CECPacket(EC_OP_PARTFILE_RESUME); break;
							default: wxASSERT(0);
						}
						request->AddTag(CECTag(EC_TAG_PARTFILE, hash));
						request_list.push_back(request);
					} else {
						Show(_("Not a valid number\n"));
						return 0;
					}
				} else {
						Show(_("Not a valid hash (length should be exactly 32 chars)\n"));
						return 0;					
				}
			}
			break;

		case CMD_ID_SHOW_UL:
			request_list.push_back(new CECPacket(EC_OP_GET_ULOAD_QUEUE));
			break;

		case CMD_ID_SHOW_DL:
			request_list.push_back(new CECPacket(EC_OP_GET_DLOAD_QUEUE));
			break;

		case CMD_ID_SHOW_LOG:
			request_list.push_back(new CECPacket(EC_OP_GET_LOG));
			break;

		case CMD_ID_SHOW_SERVERS:
			request_list.push_back(new CECPacket(EC_OP_GET_SERVER_LIST, EC_DETAIL_CMD));
			break;

		case CMD_ID_RESET_LOG:
			request_list.push_back(new CECPacket(EC_OP_RESET_LOG));
			break;

		case CMD_ID_ADDLINK:
			//aMule doesn't like AICH links without |/| in front of h=
			if (args.Find(wxT("|h=")) > -1 && args.Find(wxT("|/|h=")) == -1) {
				args.Replace(wxT("|h="),wxT("|/|h="));
			}
			request = new CECPacket(EC_OP_ED2K_LINK);
			request->AddTag(CECTag(EC_TAG_STRING, args));
			request_list.push_back(request);
			break;

		case CMD_ID_SET_BWLIMIT_UP:
			tmp_int = EC_TAG_CONN_MAX_UL - EC_TAG_CONN_MAX_DL;
		case CMD_ID_SET_BWLIMIT_DOWN:
			tmp_int += EC_TAG_CONN_MAX_DL;
			{
				unsigned long int limit;
				if (args.ToULong(&limit)) {
					request = new CECPacket(EC_OP_SET_PREFERENCES);
					CECEmptyTag prefs(EC_TAG_PREFS_CONNECTIONS);
					prefs.AddTag(CECTag(tmp_int, (uint16)limit));
					request->AddTag(prefs);
					request_list.push_back(request);
				} else {
					return CMD_ERR_INVALID_ARG;
				}
			}
		case CMD_ID_GET_BWLIMITS:
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
						return CMD_ERR_INVALID_ARG;
					}
				} else {
					delete request;
					return CMD_ERR_INVALID_ARG;
				}
			}
			request_list.push_back(request);
			break;

		default:
			return CMD_ERR_PROCESS_CMD;
	}

	m_last_cmd_id = CmdId;

	if ( ! request_list.empty() ) {
		std::list<CECPacket *>::iterator it = request_list.begin();
		while ( it != request_list.end() ) {
			CECPacket *curr = *it++;
			const CECPacket *reply = SendRecvMsg_v2(curr);
			delete curr;
			if ( reply ) {
				Process_Answer_v2(reply);
				delete reply;
			}
		}
		request_list.resize(0);
	}

	if (CmdId == CMD_ID_SHUTDOWN)
		return CMD_ID_QUIT;
	else
		return CMD_OK;
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
void CamulecmdApp::Process_Answer_v2(const CECPacket *response)
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
				const CECTag *tag = response->GetTagByIndex(0);
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
					if (m_last_cmd_id == CMD_ID_GET_IPFILTER ||
					    m_last_cmd_id == CMD_ID_GET_IPFILTER_STATE) {
						s << wxString::Format(_("IPFilter is %s.\n"),
								      (tab->GetTagByName(EC_TAG_IPFILTER_ENABLED) == NULL) ? _("OFF") : _("ON"));
					}
					if (m_last_cmd_id == CMD_ID_GET_IPFILTER ||
					    m_last_cmd_id == CMD_ID_GET_IPFILTER_LEVEL) {
						s << wxString::Format(_("Current IPFilter Level is %d.\n"),
								      ipfilterLevel->GetInt8Data());
					}
				}
				tab = response->GetTagByNameSafe(EC_TAG_PREFS_CONNECTIONS);
				const CECTag *connMaxUL = tab->GetTagByName(EC_TAG_CONN_MAX_UL);
				const CECTag *connMaxDL = tab->GetTagByName(EC_TAG_CONN_MAX_DL);
				if (connMaxUL && connMaxDL) {
					s << wxString::Format(_("Bandwidth Limits: Up: %u kB/s, Down: %u kB/s.\n"),
						(uint16)connMaxUL->GetInt(),
						(uint16)connMaxDL->GetInt());
				}
			}
			break;
		case EC_OP_STRINGS:
			for (int i = 0; i < response->GetTagCount(); ++i) {
				const CECTag *tag = response->GetTagByIndex(i);
				if (tag) {
					s << tag->GetStringData() << wxT("\n");
				} else {
				}
			}
			break;
		case EC_OP_STATS: {
			CEC_ConnState_Tag *connState = (CEC_ConnState_Tag*)response->GetTagByName(EC_TAG_CONNSTATE);
			if (connState) {
				s << _("ED2K") << wxT(": ");
				if (connState->IsConnectedED2K()) {
					CECTag *server = connState->GetTagByName(EC_TAG_SERVER);
					CECTag *serverName = server ? server->GetTagByName(EC_TAG_SERVER_NAME) : NULL;
					if (server && serverName) {
						s << CFormat(_("Connected to %s %s %s")) %
						 serverName->GetStringData() %
						 server->GetIPv4Data().StringIP() %
						 (connState->HasLowID() ? _("with LowID") : _("with HighID"));
					}
				} else if (connState->IsConnectingED2K()) {
					s << _("Now connecting");
				} else {
					s << _("Not connected");
				}
				s << wxT('\n') << _("Kad") << wxT(": ");
				if (connState->IsKadRunning()) {
					if (connState->IsConnectedKademlia()) {
						s << _("Connected") << wxT(" (");
						if (connState->IsKadFirewalled()) {
							s << _("firewalled");
						} else {
							s << _("ok");
						}
						s << wxT(')');
					} else {
						s << _("Not connected");
					}
				} else {
					s << _("Not running");
				}
				s << wxT('\n');
			}
			const CECTag *tmpTag;
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
				const CECTag *tag = response->GetTagByIndex(i);
				const CECTag *clientName = tag ? tag->GetTagByName(EC_TAG_CLIENT_NAME) : NULL;
				const CECTag *partfileName = tag ? tag->GetTagByName(EC_TAG_PARTFILE_NAME) : NULL;
				const CECTag *partfileSizeXfer = tag ? tag->GetTagByName(EC_TAG_PARTFILE_SIZE_XFER) : NULL;
				const CECTag *partfileSpeed = tag ? tag->GetTagByName(EC_TAG_CLIENT_UP_SPEED) : NULL;
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
		case EC_OP_LOG:
			for (int i = 0; i < response->GetTagCount(); ++i) {
				const CECTag *tag = response->GetTagByIndex(i);
				if (tag) {
					s << tag->GetStringData() << wxT("\n");
				} else {
				}
			}
			break;
		case EC_OP_SERVER_LIST:
			for(int i = 0; i < response->GetTagCount(); i ++) {
				const CECTag *tag = response->GetTagByIndex(i);
				const CECTag *serverName = tag ? tag->GetTagByName(EC_TAG_SERVER_NAME) : NULL;
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

void CamulecmdApp::OnInitCommandSet()
{
	CCommandTree *tmp;
	CCommandTree *tmp2;

	CaMuleExternalConnector::OnInitCommandSet();

	m_commands.AddCommand(wxT("Status"), CMD_ID_STATUS, wxTRANSLATE("Show short status information."),
			      wxTRANSLATE("Show connection status, current up/download speeds, etc.\n"), CMD_PARAM_NEVER);

	m_commands.AddCommand(wxT("Statistics"), CMD_ID_STATTREE, wxTRANSLATE("Show full statistics tree."),
			      wxTRANSLATE("Optionally, a number in the range 0-255 can be passed as an argument to this\n"
					  "command, which tells how many entries of the client version subtrees should be\n"
					  "shown. Passing 0 or omitting it means 'unlimited'.\n"
					  "\n"
					  "Example: 'statistics 5' will show only the top 5 versions for each client type.\n"));

	m_commands.AddCommand(wxT("Shutdown"), CMD_ID_SHUTDOWN, wxTRANSLATE("Shutdown aMule."),
			      wxTRANSLATE("Shutdown the remote running core (amule/amuled).\n"
					  "This will also shut down the text client, since it is unusable without a\n"
					  "running core.\n"), CMD_PARAM_NEVER);

	tmp = m_commands.AddCommand(wxT("Reload"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Reloads the given object."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("Shared"), CMD_ID_RELOAD_SHARED, wxTRANSLATE("Reloads shared files list."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("IPFilter"), CMD_ID_RELOAD_IPFILTER, wxTRANSLATE("Reloads IP Filter table from file."), wxEmptyString, CMD_PARAM_NEVER);

	tmp = m_commands.AddCommand(wxT("Connect"), CMD_ID_CONNECT, wxTRANSLATE("Connect to the network."),
				    wxTRANSLATE("This will connect to all networks that are enabled in Preferences.\n"
						"You may also optionally specify a server address in IP:Port form, to connect to\n"
						"that server only. The IP must be a dotted decimal IPv4 address,\n"
						"or a resolvable DNS name."), CMD_PARAM_OPTIONAL);
	tmp->AddCommand(wxT("ED2K"), CMD_ID_CONNECT_ED2K, wxTRANSLATE("Connect to ED2K only."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("Kad"), CMD_ID_CONNECT_KAD, wxTRANSLATE("Connect to Kad only."), wxEmptyString, CMD_PARAM_NEVER);

	tmp = m_commands.AddCommand(wxT("Disconnect"), CMD_ID_DISCONNECT, wxTRANSLATE("Disconnect from the network."),
				    wxTRANSLATE("This will disconnect from all networks that are currently connected.\n"), CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("ED2K"), CMD_ID_DISCONNECT_ED2K, wxTRANSLATE("Disconnect from ED2K only."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("Kad"), CMD_ID_DISCONNECT_KAD, wxTRANSLATE("Disconnect from Kad only."), wxEmptyString, CMD_PARAM_NEVER);

 	m_commands.AddCommand(wxT("Add"), CMD_ID_ADDLINK, wxTRANSLATE("Adds an ed2k link to core."),
			      wxTRANSLATE("The ed2k link to be added can be:\n"
					  "*) a file link (ed2k://|file|...), it will be added to the download queue,\n"
					  "*) a server link (ed2k://|server|...), it will be added to the server list,\n"
					  "*) or a serverlist link, in which case all servers in the list will be added to the\n"
					  "   server list.\n"), CMD_PARAM_ALWAYS);

	tmp = m_commands.AddCommand(wxT("Set"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Set a preference value."),
				    wxEmptyString, CMD_PARAM_NEVER);

	tmp2 = tmp->AddCommand(wxT("IPFilter"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Set IPFilter preferences."),
			       wxEmptyString, CMD_PARAM_NEVER);
	tmp2->AddCommand(wxT("On"), CMD_ID_SET_IPFILTER_ON, wxTRANSLATE("Turn IP filtering on."), wxEmptyString, CMD_PARAM_NEVER);
	tmp2->AddCommand(wxT("Off"), CMD_ID_SET_IPFILTER_OFF, wxTRANSLATE("Turn IP filtering off."), wxEmptyString, CMD_PARAM_NEVER);
	tmp2->AddCommand(wxT("Level"), CMD_ID_SET_IPFILTER_LEVEL, wxTRANSLATE("Select IP filtering level."),
			 wxTRANSLATE("Valid filtering levels are in the range 0-255, and it's default (initial)\n"
				     "value is 127.\n"), CMD_PARAM_ALWAYS);

	tmp2 = tmp->AddCommand(wxT("BwLimit"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Set bandwidth limits."),
			       wxTRANSLATE("The value given to these commands has to be in kilobytes/sec.\n"), CMD_PARAM_NEVER);
	tmp2->AddCommand(wxT("Up"), CMD_ID_SET_BWLIMIT_UP, wxTRANSLATE("Set upload bandwidth limit."),
			 wxT("The given value must be in kilobytes/sec.\n"), CMD_PARAM_ALWAYS);
	tmp2->AddCommand(wxT("Down"), CMD_ID_SET_BWLIMIT_DOWN, wxTRANSLATE("Set download bandwidth limit."),
			 wxT("The given value must be in kilobytes/sec.\n"), CMD_PARAM_ALWAYS);

	tmp = m_commands.AddCommand(wxT("Get"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Get and display a preference value."),
				    wxEmptyString, CMD_PARAM_NEVER);

	tmp2 = tmp->AddCommand(wxT("IPFilter"), CMD_ID_GET_IPFILTER, wxTRANSLATE("Get IPFilter preferences."),
			       wxEmptyString, CMD_PARAM_NEVER);
	tmp2->AddCommand(wxT("State"), CMD_ID_GET_IPFILTER_STATE, wxTRANSLATE("Get IPFilter state."), wxEmptyString, CMD_PARAM_NEVER);
	tmp2->AddCommand(wxT("Level"), CMD_ID_GET_IPFILTER_LEVEL, wxTRANSLATE("Get IPFilter level."), wxEmptyString, CMD_PARAM_NEVER);

	tmp->AddCommand(wxT("BwLimits"), CMD_ID_GET_BWLIMITS, wxTRANSLATE("Get bandwidth limits."), wxEmptyString, CMD_PARAM_NEVER);


	//
	// TODO: These commands below need implementation and/or rewrite!
	//

  	m_commands.AddCommand(wxT("Pause"), CMD_ID_PAUSE, wxTRANSLATE("Pause download."),
 			      wxEmptyString, CMD_PARAM_ALWAYS);

  	m_commands.AddCommand(wxT("Resume"), CMD_ID_RESUME, wxTRANSLATE("Resume download."),
 			      wxEmptyString, CMD_PARAM_ALWAYS);

   	m_commands.AddCommand(wxT("Cancel"), CMD_ID_CANCEL, wxTRANSLATE("Cancel download."),
  			      wxEmptyString, CMD_PARAM_ALWAYS);

	tmp = m_commands.AddCommand(wxT("Show"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Show queues/lists."),
				    wxTRANSLATE("Shows upload/download queue, server list or shared files list.\n"), CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("UL"), CMD_ID_SHOW_UL, wxTRANSLATE("Show upload queue."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("DL"), CMD_ID_SHOW_DL, wxTRANSLATE("Show download queue."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("Log"), CMD_ID_SHOW_LOG, wxTRANSLATE("Show log."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("Servers"), CMD_ID_SHOW_SERVERS, wxTRANSLATE("Show servers list."), wxEmptyString, CMD_PARAM_NEVER);
// 	tmp->AddCommand(wxT("Shared"), CMD_ID_SHOW_SHARED, wxTRANSLATE("Show shared files list."), wxEmptyString, CMD_PARAM_NEVER);

	m_commands.AddCommand(wxT("Reset"), CMD_ID_RESET_LOG, wxTRANSLATE("Reset log."), wxEmptyString, CMD_PARAM_NEVER);

	//
	// Deprecated commands, kept for backwards compatibility only.
	//

	m_commands.AddCommand(wxT("Stats"), CMD_ID_STATUS | CMD_DEPRECATED, wxTRANSLATE("Deprecated command, now 'Status'."),
			      wxTRANSLATE("This is a deprecated command, and may be removed in the future.\n"
					  "Use 'Status' instead.\n"), CMD_PARAM_NEVER);

	m_commands.AddCommand(wxT("SetIPFilter"), CMD_ID_SET_IPFILTER | CMD_DEPRECATED, wxTRANSLATE("Deprecated command, now 'Set IPFilter'."),
			      wxTRANSLATE("This is a deprecated command, and may be removed in the future.\n"
					  "Use 'Set IPFilter' instead.\n"), CMD_PARAM_OPTIONAL);

	m_commands.AddCommand(wxT("GetIPLevel"), CMD_ID_GET_IPFILTER_LEVEL | CMD_DEPRECATED, wxTRANSLATE("Deprecated command, now 'Get IPFilter Level'."),
			      wxTRANSLATE("This is a deprecated command, and may be removed in the future.\n"
					  "Use 'Get IPFilter Level' instead.\n"), CMD_PARAM_NEVER);

	m_commands.AddCommand(wxT("SetIPLevel"), CMD_ID_SET_IPFILTER_LEVEL | CMD_DEPRECATED, wxTRANSLATE("Deprecated command, now 'Set IPFilter Level'."),
			      wxTRANSLATE("This is a deprecated command, and may be removed in the future.\n"
					  "Use 'Set IPFilter Level' instead.\n"), CMD_PARAM_ALWAYS);

	m_commands.AddCommand(wxT("IPLevel"), CMD_ID_SET_IPFILTER_LEVEL | CMD_DEPRECATED, wxTRANSLATE("Deprecated command, now 'Get/Set IPFilter Level'."),
			      wxTRANSLATE("This is a deprecated command, and may be removed in the future.\n"
					  "Use 'Get/Set IPFilter Level' instead.\n"), CMD_PARAM_OPTIONAL);

	m_commands.AddCommand(wxT("Servers"), CMD_ID_SHOW_SERVERS | CMD_DEPRECATED, wxTRANSLATE("Deprecated command, now 'Show Servers'."),
			      wxTRANSLATE("This is a deprecated command, and may be removed in the future.\n"
					  "Use 'Show Servers' instead.\n"), CMD_PARAM_NEVER);

	m_commands.AddCommand(wxT("GetBWLimits"), CMD_ID_GET_BWLIMITS | CMD_DEPRECATED, wxTRANSLATE("Deprecated command, now 'Get BwLimits'."),
			      wxTRANSLATE("This is a deprecated command, and may be removed in the future.\n"
					  "Use 'Get BwLimits' instead.\n"), CMD_PARAM_NEVER);

	m_commands.AddCommand(wxT("SetUpBWLimit"), CMD_ID_SET_BWLIMIT_UP | CMD_DEPRECATED, wxTRANSLATE("Deprecated command, now 'Set BwLimit Up'."),
			      wxTRANSLATE("This is a deprecated command, and may be removed in the future.\n"
					  "Use 'Set BwLimit Up' instead.\n"), CMD_PARAM_ALWAYS);

	m_commands.AddCommand(wxT("SetDownBWLimit"), CMD_ID_SET_BWLIMIT_DOWN | CMD_DEPRECATED, wxTRANSLATE("Deprecated command, now 'Set BwLimit Down'."),
			      wxTRANSLATE("This is a deprecated command, and may be removed in the future.\n"
					  "Use 'Set BwLimit Down' instead.\n"), CMD_PARAM_ALWAYS);
}

int CamulecmdApp::OnRun()
{
	ConnectAndRun(wxT("aMulecmd"), wxT(VERSION));
	return 0;
}
