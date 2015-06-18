//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <common/ClientVersion.h>

#include "TextClient.h"

#ifndef __WINDOWS__ 
	#include <unistd.h> // Do_not_auto_remove
#endif


//-------------------------------------------------------------------


//-------------------------------------------------------------------

#include <ec/cpp/ECSpecialTags.h>

#include <wx/tokenzr.h>

#include <common/Format.h>		// Needed for CFormat
#include "OtherFunctions.h"
#include "KnownFile.h"			// Needed for Priority Levels
#include "DataToText.cpp"		// Needed for PriorityToStr

//-------------------------------------------------------------------

enum {
	CMD_ID_STATUS,
	CMD_ID_RESUME,
	CMD_ID_PAUSE,
	CMD_ID_PRIORITY_LOW,
	CMD_ID_PRIORITY_NORMAL,
	CMD_ID_PRIORITY_HIGH,
	CMD_ID_PRIORITY_AUTO,
	CMD_ID_CANCEL,
	CMD_ID_CONNECT,
	CMD_ID_CONNECT_ED2K,
	CMD_ID_CONNECT_KAD,
	CMD_ID_DISCONNECT,
	CMD_ID_DISCONNECT_ED2K,
	CMD_ID_DISCONNECT_KAD,
	CMD_ID_RELOAD_SHARED,
	CMD_ID_RELOAD_IPFILTER_LOCAL,
	CMD_ID_RELOAD_IPFILTER_NET,
	CMD_ID_SET_IPFILTER_ON,
	CMD_ID_SET_IPFILTER_OFF,
	CMD_ID_SET_IPFILTER_CLIENTS_ON,
	CMD_ID_SET_IPFILTER_CLIENTS_OFF,
	CMD_ID_SET_IPFILTER_SERVERS_ON,
	CMD_ID_SET_IPFILTER_SERVERS_OFF,
	CMD_ID_SET_IPFILTER_LEVEL,
	CMD_ID_GET_IPFILTER,
	CMD_ID_GET_IPFILTER_STATE,
	CMD_ID_GET_IPFILTER_STATE_CLIENTS,
	CMD_ID_GET_IPFILTER_STATE_SERVERS,
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
	CMD_ID_SEARCH,
	CMD_ID_SEARCH_GLOBAL,
	CMD_ID_SEARCH_LOCAL,
	CMD_ID_SEARCH_KAD,
	CMD_ID_SEARCH_RESULTS,
	CMD_ID_SEARCH_PROGRESS,
	CMD_ID_DOWNLOAD,
	// IDs for deprecated commands
	CMD_ID_SET_IPFILTER

};

// method to create a SearchFile
SearchFile::SearchFile(const CEC_SearchFile_Tag *tag)
{
	nHash = tag->FileHash();
	sHash = nHash.Encode();
	sFileName = tag->FileName();
	lFileSize = tag->SizeFull();
	lSourceCount = tag->SourceCount();
	bPresent = tag->AlreadyHave();
}

//-------------------------------------------------------------------
IMPLEMENT_APP (CamulecmdApp)
//-------------------------------------------------------------------

void CamulecmdApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	CaMuleExternalConnector::OnInitCmdLine(parser, "amulecmd");
	parser.AddOption(wxT("c"), wxT("command"),
		_("Execute <str> and exit."),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
}

bool CamulecmdApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	m_HasCmdOnCmdLine = parser.Found(wxT("command"), &m_CmdString);
	if (m_CmdString.Lower().StartsWith(wxT("help")))
	{
		OnInitCommandSet();
		printf("%s %s\n", m_appname, (const char *)unicode2char(GetMuleVersion()));
		Parse_Command(m_CmdString);
		exit(0);
	}
	m_interactive = !m_HasCmdOnCmdLine;
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
	EC_SEARCH_TYPE search_type = EC_SEARCH_KAD;

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
				int result = sscanf(unicode2char(args), "%3d.%3d.%3d.%3d:%5d", &ip[0], &ip[1], &ip[2], &ip[3], &port);
				if (result != 5) {
					// Try to resolve DNS -- good for dynamic IP servers
					wxString serverName(args.BeforeFirst(wxT(':')));
					long lPort;
					bool ok = args.AfterFirst(wxT(':')).ToLong(&lPort);
					port = (unsigned int)lPort;
					wxIPV4address a;
					a.Hostname(serverName);
					a.Service(port);
					result = sscanf(unicode2char(a.IPAddress()), "%3d.%3d.%3d.%3d", &ip[0], &ip[1], &ip[2], &ip[3]);
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

		case CMD_ID_RELOAD_IPFILTER_LOCAL:
			request_list.push_back(new CECPacket(EC_OP_IPFILTER_RELOAD));
			break;

		case CMD_ID_RELOAD_IPFILTER_NET:
			request = new CECPacket(EC_OP_IPFILTER_UPDATE);
			request->AddTag(EC_TAG_STRING, args);
			request_list.push_back(request);
			break;

		case CMD_ID_SET_IPFILTER_ON:
		case CMD_ID_SET_IPFILTER_CLIENTS_ON:
		case CMD_ID_SET_IPFILTER_SERVERS_ON:
			tmp_int = 1;
		case CMD_ID_SET_IPFILTER_OFF:
		case CMD_ID_SET_IPFILTER_CLIENTS_OFF:
		case CMD_ID_SET_IPFILTER_SERVERS_OFF:
			{
				if (CmdId == CMD_ID_SET_IPFILTER_CLIENTS_ON || CmdId == CMD_ID_SET_IPFILTER_CLIENTS_OFF) {
					CmdId = CMD_ID_GET_IPFILTER_STATE_CLIENTS;
				} else if (CmdId == CMD_ID_SET_IPFILTER_SERVERS_ON || CmdId == CMD_ID_SET_IPFILTER_SERVERS_OFF) {
					CmdId = CMD_ID_GET_IPFILTER_STATE_SERVERS;
				} else {
					CmdId = CMD_ID_GET_IPFILTER_STATE;
				}

				request = new CECPacket(EC_OP_SET_PREFERENCES);
				CECEmptyTag prefs(EC_TAG_PREFS_SECURITY);
				if (CmdId != CMD_ID_GET_IPFILTER_STATE_SERVERS) {
					prefs.AddTag(CECTag(EC_TAG_IPFILTER_CLIENTS, (uint8)tmp_int));
				}
				if (CmdId != CMD_ID_GET_IPFILTER_STATE_CLIENTS) {
					prefs.AddTag(CECTag(EC_TAG_IPFILTER_SERVERS, (uint8)tmp_int));
				}
				request->AddTag(prefs);
				request_list.push_back(request);
			}
		case CMD_ID_GET_IPFILTER:
		case CMD_ID_GET_IPFILTER_STATE:
		case CMD_ID_GET_IPFILTER_STATE_CLIENTS:
		case CMD_ID_GET_IPFILTER_STATE_SERVERS:
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
		{
			if ( args.IsEmpty() ) {
				Show(_("This command requires an argument. Valid arguments: 'all', filename, or a number.\n"));
				return 0;
			} else {
				wxStringTokenizer argsTokenizer(args);
				wxString token;
				CMD4Hash hash;

				// Grab the entire dl queue right away
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
						default: wxFAIL;
					}

					// We loop through all the arguments
					while(argsTokenizer.HasMoreTokens()) {
						token=argsTokenizer.GetNextToken();

						// If the user requested all, then we select all files and exit the loop
						// since there is little point to add anything more to "everything"
						if( token == wxT("all") ) {
							for (CECPacket::const_iterator it = reply_all->begin(); it != reply_all->end(); ++it) {
								const CEC_PartFile_Tag *tag = static_cast<const CEC_PartFile_Tag *>(&*it);
								request->AddTag(CECTag(EC_TAG_PARTFILE, tag->FileHash()));
							}
							break;
						} else if ( hash.Decode(token.Trim(false).Trim(true)) ) {
							if ( !hash.IsEmpty() ) {
								Show(_("Processing by hash: "+token+wxT("\n")));
								request->AddTag(CECTag(EC_TAG_PARTFILE, hash));
							}
						} else {
							 // Go through the dl queue and look at each filename
							for (CECPacket::const_iterator it = reply_all->begin(); it != reply_all->end(); ++it) {
								const CEC_PartFile_Tag *tag = static_cast<const CEC_PartFile_Tag *>(&*it);
								wxString partmetname = tag->PartMetName();

								// We check for filename, XXX.pat.met, XXX.part, XXX
								if( tag->FileName() == token ||
									partmetname == token ||
									partmetname.Truncate(partmetname.Len()-4) == token ||
									partmetname.Truncate(partmetname.Len()-5) == token) {
									Show(_("Processing by filename: "+token+wxT("\n")));
									request->AddTag(CECTag(EC_TAG_PARTFILE, tag->FileHash()));
								}
							}
						} // End of filename check else
					} // End of argument token loop

				request_list.push_back(request);

				delete reply_all;

				} // End of dl queue processing

			} // end of command processing
			break;
		}

		case CMD_ID_PRIORITY_LOW:
		case CMD_ID_PRIORITY_NORMAL:
		case CMD_ID_PRIORITY_HIGH:
		case CMD_ID_PRIORITY_AUTO:
			if ( args.IsEmpty() ) {
				Show(_("This command requires an argument. Valid arguments: a file hash.\n"));
				return 0;
			} else {
				CMD4Hash hash;
				if (hash.Decode(args.Trim(false).Trim(true))) {
					if (!hash.IsEmpty()) {
						request = new CECPacket(EC_OP_PARTFILE_PRIO_SET);
						CECTag hashtag(EC_TAG_PARTFILE, hash);
						switch(CmdId) {
							case CMD_ID_PRIORITY_LOW:
								hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, (uint8)PR_LOW));
								break;
							case CMD_ID_PRIORITY_NORMAL:
								hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, (uint8)PR_NORMAL));
								break;
							case CMD_ID_PRIORITY_HIGH:
								hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, (uint8)PR_HIGH));
								break;
							case CMD_ID_PRIORITY_AUTO:
								hashtag.AddTag(CECTag(EC_TAG_PARTFILE_PRIO, (uint8)PR_AUTO));
								break;
							default: wxFAIL;
						}
						request->AddTag(hashtag);
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

		case CMD_ID_SHOW_SHARED:
			request_list.push_back(new CECPacket(EC_OP_GET_SHARED_FILES));
			break;

		case CMD_ID_RESET_LOG:
			request_list.push_back(new CECPacket(EC_OP_RESET_LOG));
			break;

		case CMD_ID_ADDLINK:
			if (args.StartsWith(wxT("ed2k://"))) {
				//aMule doesn't like AICH links without |/| in front of h=
				if (args.Find(wxT("|h=")) > -1 && args.Find(wxT("|/|h=")) == -1) {
					args.Replace(wxT("|h="),wxT("|/|h="));
				}
				// repair links where | is replaced with %7C (Firefox)
				if (args.StartsWith(wxT("ed2k://%7C"))) {
					args.Replace(wxT("%7C"),wxT("|"));
				}
			}
			request = new CECPacket(EC_OP_ADD_LINK);
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
		case CMD_ID_SEARCH_GLOBAL:
			search_type = EC_SEARCH_GLOBAL;
		case CMD_ID_SEARCH_LOCAL:
			if (search_type != EC_SEARCH_GLOBAL){
				search_type = EC_SEARCH_LOCAL;
			}
		case CMD_ID_SEARCH_KAD:
			if (search_type != EC_SEARCH_GLOBAL && search_type != EC_SEARCH_LOCAL){
				search_type = EC_SEARCH_KAD;
			}
			if (!args.IsEmpty())
			{
				wxString search = args;
				wxString type;
				wxString extention;
				uint32 avail = 0;
				uint32 min_size = 0;
				uint32 max_size = 0;

				request = new CECPacket(EC_OP_SEARCH_START);
				request->AddTag(CEC_Search_Tag (search, search_type, type, extention, avail, min_size, max_size));
				request_list.push_back(request);
			}
			break;
		case CMD_ID_SEARCH:
			/* TRANSLATORS:
			   'help search' is a command to the program, do not translate it. */
			Show(_("No search type defined.\nType 'help search' to get more help.\n"));
			break;


		case CMD_ID_SEARCH_RESULTS:
			request_list.push_back(new CECPacket(EC_OP_SEARCH_RESULTS, EC_DETAIL_FULL));
			break;

		case CMD_ID_SEARCH_PROGRESS:
			request_list.push_back(new CECPacket(EC_OP_SEARCH_PROGRESS));
			break;

		case CMD_ID_DOWNLOAD:
			if (!args.IsEmpty())
			{
				unsigned long int id = 0;
				if (args.ToULong(&id) == true && id < m_Results_map.size()) {

					SearchFile* file = m_Results_map[id];
					Show(CFormat(_("Download File: %lu %s\n")) % id % file->sFileName);
					request = new CECPacket(EC_OP_DOWNLOAD_SEARCH_RESULT);
					// get with id the hash and category=0
					uint32 category = 0;
					CECTag hashtag(EC_TAG_PARTFILE, file->nHash);
					hashtag.AddTag(CECTag(EC_TAG_PARTFILE_CAT, category));
					request->AddTag(hashtag);
					request_list.push_back(request);
				} else {
					return CMD_ERR_INVALID_ARG;
				}
			}
			break;

		default:
			return CMD_ERR_PROCESS_CMD;
	}

	m_last_cmd_id = CmdId;

	if ( ! request_list.empty() ) {
		std::list<CECPacket *>::iterator it = request_list.begin();
		while ( it != request_list.end() ) {
			CECPacket *curr = *it++;
			if (curr->GetOpCode() == EC_OP_SHUTDOWN) {
				SendPacket(curr);
				delete curr;
				return CMD_ID_QUIT;
			}
			const CECPacket *reply = SendRecvMsg_v2(curr);
			delete curr;
			if ( reply ) {
				Process_Answer_v2(reply);
				delete reply;
			}
		}
		request_list.resize(0);
	}

	return CMD_OK;
}

 /*
 * Method to show the results in the console
 */
void CamulecmdApp::ShowResults(CResultMap results_map)
{
	unsigned int name_max = 80;
	unsigned int mb_max = 5;
	unsigned int nr_max = 5;
	unsigned long int id = 0;
	wxString output, name, sources, mb , kb;

	printf("Nr.    Filename:                                                                        Size(MB):  Sources: \n");
	printf("-----------------------------------------------------------------------------------------------------------\n");

	for( std::map<unsigned long int,SearchFile*>::iterator iter = results_map.begin(); iter != results_map.end(); ++iter ) {
		id = (*iter).first;
		SearchFile* file = (*iter).second;

		output.Printf(wxT("%i.      "), id);
		output = output.SubString(0, nr_max).Append(file->sFileName).Append(' ', name_max);
		mb.Printf(wxT("     %d"), file->lFileSize/1024/1024);
		kb.Printf(wxT(".%d"), file->lFileSize/1024%1024);
		output = output.SubString(0, nr_max + name_max + mb_max - mb.Length() ).Append(mb).Append(kb);
		printf("%s     %ld\n",(const char*)unicode2char(output), file->lSourceCount );
	}
}


// Formats a statistics (sub)tree to text
wxString StatTree2Text(const CEC_StatTree_Node_Tag *tree, int depth)
{
	if (!tree) {
		return wxEmptyString;
	}
	wxString result = wxString(wxChar(' '), depth) + tree->GetDisplayString() + wxT("\n");
	for (CECTag::const_iterator it = tree->begin(); it != tree->end(); ++it) {
		const CEC_StatTree_Node_Tag *tmp = static_cast<const CEC_StatTree_Node_Tag*>(&*it);
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
			{
				const CECTag *tag = response->GetFirstTagSafe();
				if (tag->IsString()) {
					s <<	CFormat(_("Request failed with the following error: %s")) % wxGetTranslation(tag->GetStringData());
				} else {
					s << msgFailedUnknown;
				}
			}
			break;
		case EC_OP_SET_PREFERENCES:
			{
				const CECTag *tab = response->GetTagByNameSafe(EC_TAG_PREFS_SECURITY);
				const CECTag *ipfilterLevel = tab->GetTagByName(EC_TAG_IPFILTER_LEVEL);
				if (ipfilterLevel) {
					if (m_last_cmd_id == CMD_ID_GET_IPFILTER ||
					    m_last_cmd_id == CMD_ID_GET_IPFILTER_STATE ||
					    m_last_cmd_id == CMD_ID_GET_IPFILTER_STATE_CLIENTS) {
						s += CFormat(_("IP filtering for clients is %s.\n"))
								% ((tab->GetTagByName(EC_TAG_IPFILTER_CLIENTS) == NULL) ? _("OFF") : _("ON"));
					}
					if (m_last_cmd_id == CMD_ID_GET_IPFILTER ||
					    m_last_cmd_id == CMD_ID_GET_IPFILTER_STATE ||
					    m_last_cmd_id == CMD_ID_GET_IPFILTER_STATE_SERVERS) {
						s += CFormat(_("IP filtering for servers is %s.\n"))
								% ((tab->GetTagByName(EC_TAG_IPFILTER_SERVERS) == NULL) ? _("OFF") : _("ON"));
					}
					if (m_last_cmd_id == CMD_ID_GET_IPFILTER ||
					    m_last_cmd_id == CMD_ID_GET_IPFILTER_LEVEL) {
						s << CFormat(_("Current IPFilter Level is %d.\n")) % ipfilterLevel->GetInt();
					}
				}
				tab = response->GetTagByNameSafe(EC_TAG_PREFS_CONNECTIONS);
				const CECTag *connMaxUL = tab->GetTagByName(EC_TAG_CONN_MAX_UL);
				const CECTag *connMaxDL = tab->GetTagByName(EC_TAG_CONN_MAX_DL);
				if (connMaxUL && connMaxDL) {
					s << CFormat(_("Bandwidth limits: Up: %u kB/s, Down: %u kB/s.\n"))
						% connMaxUL->GetInt() % connMaxDL->GetInt();
				}
			}
			break;
		case EC_OP_STRINGS:
			for (CECPacket::const_iterator it = response->begin(); it != response->end(); ++it) {
				const CECTag &tag = *it;
				s << tag.GetStringData() << wxT("\n");
			}
			break;
		case EC_OP_STATS: {
			const CEC_ConnState_Tag *connState = static_cast<const CEC_ConnState_Tag*>(response->GetTagByName(EC_TAG_CONNSTATE));
			if (connState) {
				s << _("eD2k") << wxT(": ");
				if (connState->IsConnectedED2K()) {
					const CECTag *server = connState->GetTagByName(EC_TAG_SERVER);
					const CECTag *serverName = server ? server->GetTagByName(EC_TAG_SERVER_NAME) : NULL;
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
				s <<	CFormat(_("\nDownload:\t%s")) % CastItoSpeed(tmpTag->GetInt());
			}
			if ((tmpTag = response->GetTagByName(EC_TAG_STATS_UL_SPEED)) != 0) {
				s <<	CFormat(_("\nUpload:\t%s")) % CastItoSpeed(tmpTag->GetInt());
			}
			if ((tmpTag = response->GetTagByName(EC_TAG_STATS_UL_QUEUE_LEN)) != 0) {
				s <<	CFormat(_("\nClients in queue:\t%d\n")) % tmpTag->GetInt();
			}
			if ((tmpTag = response->GetTagByName(EC_TAG_STATS_TOTAL_SRC_COUNT)) != 0) {
				s <<	CFormat(_("\nTotal sources:\t%d\n")) % tmpTag->GetInt();
			}
			break;
		}
		case EC_OP_DLOAD_QUEUE:
			for (CECPacket::const_iterator it = response->begin(); it != response->end(); ++it) {
				const CEC_PartFile_Tag *tag = static_cast<const CEC_PartFile_Tag *>(&*it);
				uint64 filesize, donesize;
				filesize = tag->SizeFull();
				donesize = tag->SizeDone();
				s <<	tag->FileHashString() << wxT(" ") <<
					tag->FileName() <<
					(CFormat(wxT("\n\t [%.1f%%] %4i/%4i "))
						% ((float)donesize / ((float)filesize)*100.0)
						% ((int)tag->SourceCount() - (int)tag->SourceNotCurrCount())
						% (int)tag->SourceCount()) <<
					((int)tag->SourceCountA4AF() ? wxString(CFormat(wxT("+%2.2i ")) % (int)tag->SourceCountA4AF()) : wxString(wxT("    "))) <<
					((int)tag->SourceXferCount() ? wxString(CFormat(wxT("(%2.2i) - ")) % (int)tag->SourceXferCount()) : wxString(wxT("     - "))) <<
					tag->GetFileStatusString();
				s << wxT(" - ") << tag->PartMetName();
				if (tag->DownPrio() < 10) {
					s << wxT(" - ") << PriorityToStr((int)tag->DownPrio(), 0);
				} else {
					s << wxT(" - ") << PriorityToStr((tag->DownPrio() - 10), 1);
				}
				if ( tag->SourceXferCount() > 0) {
					s << wxT(" - ") + CastItoSpeed(tag->Speed());
				}
				s << wxT("\n");
			}
			break;
		case EC_OP_ULOAD_QUEUE:
			for (CECPacket::const_iterator it = response->begin(); it != response->end(); ++it) {
				const CECTag *tag = & *it;
				const CECTag *clientName = tag->GetTagByName(EC_TAG_CLIENT_NAME);
				const CECTag *partfileName = tag->GetTagByName(EC_TAG_PARTFILE_NAME);
				const CECTag *partfileSizeXfer = tag->GetTagByName(EC_TAG_PARTFILE_SIZE_XFER);
				const CECTag *partfileSpeed = tag->GetTagByName(EC_TAG_CLIENT_UP_SPEED);
				if (clientName && partfileName && partfileSizeXfer && partfileSpeed) {
					s <<	wxT("\n") <<
						CFormat(wxT("%10u ")) % tag->GetInt() <<
						clientName->GetStringData() << wxT(" ") <<
						partfileName->GetStringData() << wxT(" ") <<
						CastItoXBytes(partfileSizeXfer->GetInt()) << wxT(" ") <<
						CastItoSpeed(partfileSpeed->GetInt());
				}
			}
			break;
		case EC_OP_LOG:
			for (CECPacket::const_iterator it = response->begin(); it != response->end(); ++it) {
				const CECTag &tag = *it;
				s << tag.GetStringData() << wxT("\n");
			}
			break;
		case EC_OP_SERVER_LIST:
			for (CECPacket::const_iterator it = response->begin(); it != response->end(); ++it) {
				const CECTag &tag = *it;
				const CECTag *serverName = tag.GetTagByName(EC_TAG_SERVER_NAME);
				if (serverName) {
					wxString ip = tag.GetIPv4Data().StringIP();
					ip.Append(' ', 24 - ip.Length());
					s << ip << serverName->GetStringData() << wxT("\n");
				}
			}
			break;

		case EC_OP_SHARED_FILES:
			for (CECPacket::const_iterator it = response->begin(); it != response->end(); ++it) {
				const CEC_SharedFile_Tag *tag = static_cast<const CEC_SharedFile_Tag *>(&*it);
				s << tag->FileHashString() << wxT(" ");
				wxString filePath = tag->FilePath();
				bool ispartfile = true;
				if (filePath.EndsWith(wxT(".part"))) {
					for (unsigned i = 0; i < filePath.Length() - 5; i++) {
						if (filePath[i] < wxT('0') || filePath[i] > wxT('9')) {
							ispartfile = false;
							break;
						}
					}
				} else {
					ispartfile = false;
				}
				if (ispartfile) {
					s << _("[PartFile]") << wxT(" ");
				} else {
					s << filePath
#ifdef __WINDOWS__
					  << wxT('\\');
#else
					  << wxT('/');
#endif
				}
				s << tag->FileName()
				  << wxT("\n\t") << PriorityToStr(tag->UpPrio() % 10, tag->UpPrio() >= 10) << wxT(" - ") << CFormat(wxT("%i(%i) / %i(%i) - %s (%s) - %.2f\n"))
					% tag->GetRequests() % tag->GetAllRequests()
					% tag->GetAccepts() % tag->GetAllAccepts()
					% CastItoXBytes(tag->GetXferred()) % CastItoXBytes(tag->GetAllXferred())
					% (static_cast<float>(tag->GetAllXferred()) / static_cast<float>(tag->SizeFull()));
			}
			break;

		case EC_OP_STATSTREE:
			s << StatTree2Text(static_cast<const CEC_StatTree_Node_Tag*>(response->GetTagByName(EC_TAG_STATTREE_NODE)), 0);
			break;

		case EC_OP_SEARCH_RESULTS:
		{
			int i = 0;
			m_Results_map.clear();
			s += CFormat(_("Number of search results: %i\n")) % response->GetTagCount();
			for (CECPacket::const_iterator it = response->begin(); it != response->end(); ++it) {
				const CEC_SearchFile_Tag *tag = static_cast<const CEC_SearchFile_Tag *>(&*it);
				//printf("Tag FileName: %s \n",(const char*)unicode2char(tag->FileName()));
				m_Results_map[i++] = new SearchFile(tag);
			}
			ShowResults(m_Results_map);
			break;
		}
		case EC_OP_SEARCH_PROGRESS:
		{
			const CECTag *tab = response->GetTagByNameSafe(EC_TAG_SEARCH_STATUS);
			uint32 progress = tab->GetInt();
			if (progress <= 100) {
				s += CFormat(_("Search progress: %u %% \n")) % progress;
			} else {
				s += _("Search progress not available");
			}
			break;
		}
		default:
			s += CFormat(_("Received an unknown reply from the server, OpCode = %#x.")) % response->GetOpCode();
	}
	Process_Answer(s);
}

void CamulecmdApp::OnInitCommandSet()
{
	CCommandTree *tmp;
	CCommandTree *tmp2;
	CCommandTree *tmp3;

	CaMuleExternalConnector::OnInitCommandSet();

	m_commands.AddCommand(wxT("Status"), CMD_ID_STATUS, wxTRANSLATE("Show short status information."),
			      wxTRANSLATE("Show connection status, current up/download speeds, etc.\n"), CMD_PARAM_NEVER);

	m_commands.AddCommand(wxT("Statistics"), CMD_ID_STATTREE, wxTRANSLATE("Show full statistics tree."),
			      wxTRANSLATE("Optionally, a number in the range 0-255 can be passed as an argument to this\ncommand, which tells how many entries of the client version subtrees should be\nshown. Passing 0 or omitting it means 'unlimited'.\n\nExample: 'statistics 5' will show only the top 5 versions for each client type.\n"));

	m_commands.AddCommand(wxT("Shutdown"), CMD_ID_SHUTDOWN, wxTRANSLATE("Shut down aMule."),
			      wxTRANSLATE("Shut down the remote running core (amule/amuled).\nThis will also shut down the text client, since it is unusable without a\nrunning core.\n"), CMD_PARAM_NEVER);

	tmp = m_commands.AddCommand(wxT("Reload"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Reload the given object."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("Shared"), CMD_ID_RELOAD_SHARED, wxTRANSLATE("Reload shared files list."), wxEmptyString, CMD_PARAM_NEVER);

	tmp2 = tmp->AddCommand(wxT("IPFilter"), CMD_ID_RELOAD_IPFILTER_LOCAL, wxTRANSLATE("Reload IP filtering table."), wxEmptyString, CMD_PARAM_OPTIONAL);
	tmp2->AddCommand(wxT("File"), CMD_ID_RELOAD_IPFILTER_LOCAL, wxTRANSLATE("Reload current IP filtering table."), wxEmptyString, CMD_PARAM_NEVER);
	tmp2->AddCommand(wxT("Net"), CMD_ID_RELOAD_IPFILTER_NET, wxTRANSLATE("Update IP filtering table from URL."),
					wxTRANSLATE("If URL is omitted the URL from the preferences is used."), CMD_PARAM_OPTIONAL);

	tmp = m_commands.AddCommand(wxT("Connect"), CMD_ID_CONNECT, wxTRANSLATE("Connect to the network."),
				    wxTRANSLATE("This will connect to all networks that are enabled in Preferences.\nYou may also optionally specify a server address in IP:Port form, to connect to\nthat server only. The IP must be a dotted decimal IPv4 address,\nor a resolvable DNS name."), CMD_PARAM_OPTIONAL);
	tmp->AddCommand(wxT("ED2K"), CMD_ID_CONNECT_ED2K, wxTRANSLATE("Connect to eD2k only."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("Kad"), CMD_ID_CONNECT_KAD, wxTRANSLATE("Connect to Kad only."), wxEmptyString, CMD_PARAM_NEVER);

	tmp = m_commands.AddCommand(wxT("Disconnect"), CMD_ID_DISCONNECT, wxTRANSLATE("Disconnect from the network."),
				    wxTRANSLATE("This will disconnect from all networks that are currently connected.\n"), CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("ED2K"), CMD_ID_DISCONNECT_ED2K, wxTRANSLATE("Disconnect from eD2k only."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("Kad"), CMD_ID_DISCONNECT_KAD, wxTRANSLATE("Disconnect from Kad only."), wxEmptyString, CMD_PARAM_NEVER);

	m_commands.AddCommand(wxT("Add"), CMD_ID_ADDLINK, wxTRANSLATE("Add an eD2k or magnet link to core."),
			      wxTRANSLATE("The eD2k link to be added can be:\n*) a file link (ed2k://|file|...), it will be added to the download queue,\n*) a server link (ed2k://|server|...), it will be added to the server list,\n*) or a serverlist link, in which case all servers in the list will be added to the\n   server list.\n\nThe magnet link must contain the eD2k hash and file length.\n"), CMD_PARAM_ALWAYS);

	tmp = m_commands.AddCommand(wxT("Set"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Set a preference value."),
				    wxEmptyString, CMD_PARAM_NEVER);

	tmp2 = tmp->AddCommand(wxT("IPFilter"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Set IP filtering preferences."), wxEmptyString, CMD_PARAM_NEVER);
	tmp2->AddCommand(wxT("On"), CMD_ID_SET_IPFILTER_ON, wxTRANSLATE("Turn IP filtering on for both clients and servers."), wxEmptyString, CMD_PARAM_NEVER);
	tmp2->AddCommand(wxT("Off"), CMD_ID_SET_IPFILTER_OFF, wxTRANSLATE("Turn IP filtering off for both clients and servers."), wxEmptyString, CMD_PARAM_NEVER);
	tmp3 = tmp2->AddCommand(wxT("Clients"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Enable/Disable IP filtering for clients."), wxEmptyString, CMD_PARAM_NEVER);
	tmp3->AddCommand(wxT("On"), CMD_ID_SET_IPFILTER_CLIENTS_ON, wxTRANSLATE("Turn IP filtering on for clients."), wxEmptyString, CMD_PARAM_NEVER);
	tmp3->AddCommand(wxT("Off"), CMD_ID_SET_IPFILTER_CLIENTS_OFF, wxTRANSLATE("Turn IP filtering off for clients."), wxEmptyString, CMD_PARAM_NEVER);
	tmp3 = tmp2->AddCommand(wxT("Servers"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Enable/Disable IP filtering for servers."), wxEmptyString, CMD_PARAM_NEVER);
	tmp3->AddCommand(wxT("On"), CMD_ID_SET_IPFILTER_SERVERS_ON, wxTRANSLATE("Turn IP filtering on for servers."), wxEmptyString, CMD_PARAM_NEVER);
	tmp3->AddCommand(wxT("Off"), CMD_ID_SET_IPFILTER_SERVERS_OFF, wxTRANSLATE("Turn IP filtering off for servers."), wxEmptyString, CMD_PARAM_NEVER);
	tmp2->AddCommand(wxT("Level"), CMD_ID_SET_IPFILTER_LEVEL, wxTRANSLATE("Select IP filtering level."),
			 wxTRANSLATE("Valid filtering levels are in the range 0-255, and it's default (initial)\nvalue is 127.\n"), CMD_PARAM_ALWAYS);

	tmp2 = tmp->AddCommand(wxT("BwLimit"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Set bandwidth limits."),
			       wxTRANSLATE("The value given to these commands has to be in kilobytes/sec.\n"), CMD_PARAM_NEVER);
	tmp2->AddCommand(wxT("Up"), CMD_ID_SET_BWLIMIT_UP, wxTRANSLATE("Set upload bandwidth limit."),
			 wxTRANSLATE("The given value must be in kilobytes/sec.\n"), CMD_PARAM_ALWAYS);
	tmp2->AddCommand(wxT("Down"), CMD_ID_SET_BWLIMIT_DOWN, wxTRANSLATE("Set download bandwidth limit."),
			 wxTRANSLATE("The given value must be in kilobytes/sec.\n"), CMD_PARAM_ALWAYS);

	tmp = m_commands.AddCommand(wxT("Get"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Get and display a preference value."),
				    wxEmptyString, CMD_PARAM_NEVER);

	tmp2 = tmp->AddCommand(wxT("IPFilter"), CMD_ID_GET_IPFILTER, wxTRANSLATE("Get IP filtering preferences."), wxEmptyString, CMD_PARAM_NEVER);
	tmp3 = tmp2->AddCommand(wxT("State"), CMD_ID_GET_IPFILTER_STATE, wxTRANSLATE("Get IP filtering state for both clients and servers."), wxEmptyString, CMD_PARAM_NEVER);
	tmp3->AddCommand(wxT("Clients"), CMD_ID_GET_IPFILTER_STATE_CLIENTS, wxTRANSLATE("Get IP filtering state for clients only."), wxEmptyString, CMD_PARAM_NEVER);
	tmp3->AddCommand(wxT("Servers"), CMD_ID_GET_IPFILTER_STATE_SERVERS, wxTRANSLATE("Get IP filtering state for servers only."), wxEmptyString, CMD_PARAM_NEVER);
	tmp2->AddCommand(wxT("Level"), CMD_ID_GET_IPFILTER_LEVEL, wxTRANSLATE("Get IP filtering level."), wxEmptyString, CMD_PARAM_NEVER);

	tmp->AddCommand(wxT("BwLimits"), CMD_ID_GET_BWLIMITS, wxTRANSLATE("Get bandwidth limits."), wxEmptyString, CMD_PARAM_NEVER);

	tmp = m_commands.AddCommand(wxT("Search"), CMD_ID_SEARCH, wxTRANSLATE("Execute a search."),
			      wxTRANSLATE("A search type has to be specified by giving the type:\n    GLOBAL\n    LOCAL\n    KAD\nExample: 'search kad file' will execute a kad search for \"file\".\n"), CMD_PARAM_ALWAYS);
	tmp->AddCommand(wxT("global"), CMD_ID_SEARCH_GLOBAL, wxTRANSLATE("Execute a global search."), wxEmptyString, CMD_PARAM_ALWAYS);
	tmp->AddCommand(wxT("local"), CMD_ID_SEARCH_LOCAL, wxTRANSLATE("Execute a local search"), wxEmptyString, CMD_PARAM_ALWAYS);
	tmp->AddCommand(wxT("kad"), CMD_ID_SEARCH_KAD, wxTRANSLATE("Execute a kad search"), wxEmptyString, CMD_PARAM_ALWAYS);

	m_commands.AddCommand(wxT("Results"), CMD_ID_SEARCH_RESULTS, wxTRANSLATE("Show the results of the last search."),
			      wxTRANSLATE("Return the results of the previous search.\n"), CMD_PARAM_NEVER);

	m_commands.AddCommand(wxT("Progress"), CMD_ID_SEARCH_PROGRESS, wxTRANSLATE("Show the progress of a search."),
			      wxTRANSLATE("Show the progress of a search.\n"), CMD_PARAM_NEVER);

	m_commands.AddCommand(wxT("Download"), CMD_ID_DOWNLOAD, wxTRANSLATE("Start downloading a file"),
			      wxTRANSLATE("The number of a file from the last search has to be given.\nExample: 'download 12' will start to download the file with the number 12 of the previous search.\n"), CMD_PARAM_ALWAYS);


	//
	// TODO: These commands below need implementation and/or rewrite!
	//

	m_commands.AddCommand(wxT("Pause"), CMD_ID_PAUSE, wxTRANSLATE("Pause download."),
			      wxEmptyString, CMD_PARAM_ALWAYS);

	m_commands.AddCommand(wxT("Resume"), CMD_ID_RESUME, wxTRANSLATE("Resume download."),
			      wxEmptyString, CMD_PARAM_ALWAYS);

	m_commands.AddCommand(wxT("Cancel"), CMD_ID_CANCEL, wxTRANSLATE("Cancel download."),
			      wxEmptyString, CMD_PARAM_ALWAYS);

	tmp = m_commands.AddCommand(wxT("Priority"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Set download priority."),
				    wxTRANSLATE("Set priority of a download to Low, Normal, High or Auto.\n"), CMD_PARAM_ALWAYS);
	tmp->AddCommand(wxT("Low"), CMD_ID_PRIORITY_LOW, wxTRANSLATE("Set priority to low."), wxEmptyString, CMD_PARAM_ALWAYS);
	tmp->AddCommand(wxT("Normal"), CMD_ID_PRIORITY_NORMAL, wxTRANSLATE("Set priority to normal."), wxEmptyString, CMD_PARAM_ALWAYS);
	tmp->AddCommand(wxT("High"), CMD_ID_PRIORITY_HIGH, wxTRANSLATE("Set priority to high."), wxEmptyString, CMD_PARAM_ALWAYS);
	tmp->AddCommand(wxT("Auto"), CMD_ID_PRIORITY_AUTO, wxTRANSLATE("Set priority to auto."), wxEmptyString, CMD_PARAM_ALWAYS);

	tmp = m_commands.AddCommand(wxT("Show"), CMD_ERR_INCOMPLETE, wxTRANSLATE("Show queues/lists."),
				    wxTRANSLATE("Show upload/download queue, server list or shared files list.\n"), CMD_PARAM_ALWAYS);
	tmp->AddCommand(wxT("UL"), CMD_ID_SHOW_UL, wxTRANSLATE("Show upload queue."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("DL"), CMD_ID_SHOW_DL, wxTRANSLATE("Show download queue."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("Log"), CMD_ID_SHOW_LOG, wxTRANSLATE("Show log."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("Servers"), CMD_ID_SHOW_SERVERS, wxTRANSLATE("Show servers list."), wxEmptyString, CMD_PARAM_NEVER);
	tmp->AddCommand(wxT("Shared"), CMD_ID_SHOW_SHARED, wxTRANSLATE("Show shared files list."), wxEmptyString, CMD_PARAM_NEVER);

	m_commands.AddCommand(wxT("Reset"), CMD_ID_RESET_LOG, wxTRANSLATE("Reset log."), wxEmptyString, CMD_PARAM_NEVER);

	//
	// Deprecated commands, kept for backwards compatibility only.
	//

#define DEPRECATED(OLDCMD, ID, NEWCMD, PARAM) \
	m_commands.AddCommand(wxT(OLDCMD), CMD_ID_##ID | CMD_DEPRECATED, CFormat(wxTRANSLATE("Deprecated command, use '%s' instead.")) % wxT(NEWCMD), \
			      CFormat(wxTRANSLATE("This is a deprecated command, and may be removed in the future.\nUse '%s' instead.\n")) % wxT(NEWCMD), CMD_PARAM_##PARAM)

	DEPRECATED("Stats", STATUS, "Status", NEVER);
	DEPRECATED("SetIPFilter", SET_IPFILTER, "Set IPFilter", OPTIONAL);
	DEPRECATED("GetIPLevel", GET_IPFILTER_LEVEL, "Get IPFilter Level", NEVER);
	DEPRECATED("SetIPLevel", SET_IPFILTER_LEVEL, "Set IPFilter Level", ALWAYS);
	DEPRECATED("IPLevel", SET_IPFILTER_LEVEL, "Get/Set IPFilter Level", OPTIONAL);
	DEPRECATED("Servers", SHOW_SERVERS, "Show Servers", NEVER);
	DEPRECATED("GetBWLimits", GET_BWLIMITS, "Get BwLimits", NEVER);
	DEPRECATED("SetUpBWLimit", SET_BWLIMIT_UP, "Set BwLimit Up", ALWAYS);
	DEPRECATED("SetDownBWLimit", SET_BWLIMIT_DOWN, "Set BwLimit Down", ALWAYS);
}

int CamulecmdApp::OnRun()
{
	ConnectAndRun(wxT("aMulecmd"), wxT(VERSION));
	return 0;
}

// Dummy functions for EC logging
bool ECLogIsEnabled()
{
	return false;
}

void DoECLogLine(const wxString &)
{
}


namespace MuleNotify
{
	void HandleNotification(const class CMuleNotiferBase&) {}
	void HandleNotificationAlways(const class CMuleNotiferBase&) {}
	void UDPSocketSend(class CMuleUDPSocket *) {}
	void UDPSocketReceive(class CMuleUDPSocket *) {}
}

// File_checked_for_headers
