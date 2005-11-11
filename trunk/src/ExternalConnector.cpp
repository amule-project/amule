//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
	#include "config.h"	// Needed for VERSION and readline detection
#endif

#include "Format.h"		// Needed for CFormat

#include <cstdio>		// Needed for fprintf(stderr, ...)
#include <wx/filefn.h>
#include <wx/intl.h>		// For _()
#include <wx/tokenzr.h>		// For wxStringTokenizer

#if wxUSE_GUI
	#include <wx/textdlg.h>	// For GetTextFromUser, GetPasswordFromUser
#endif

#if !wxUSE_GUI
	#include <unistd.h>	// For getpass() and pause()
	// For readline
	#ifdef HAVE_LIBREADLINE
		#if defined(HAVE_READLINE_READLINE_H)
			#include <readline/readline.h>
		#elif defined(HAVE_READLINE_H)
			#include <readline.h>
		#else /* !defined(HAVE_READLINE_H) */
			extern "C" char *readline (const char*);
		#endif /* !defined(HAVE_READLINE_H) */
	#else /* !defined(HAVE_READLINE_READLINE_H) */
		/* no readline */
	#endif /* HAVE_LIBREADLINE */
 	// For history
	#ifdef HAVE_READLINE_HISTORY
		#if defined(HAVE_READLINE_HISTORY_H)
			#include <readline/history.h>
		#elif defined(HAVE_HISTORY_H)
			#include <history.h>
		#else /* !defined(HAVE_HISTORY_H) */
			extern "C" void add_history (const char*);
		#endif /* defined(HAVE_READLINE_HISTORY_H) */
	#else
		/* no history */
	#endif /* HAVE_READLINE_HISTORY */
#endif

#include "ECFileConfig.h"	// Needed for CECFileConfig
#include "ECPacket.h"		// Needed for CECPacket, CECTag
#include "ECcodes.h"		// Needed for OPcodes and TAGnames
#include "ECVersion.h"		// Needed for EC_VERSION_ID
#include "ExternalConnector.h"
#include "MD5Sum.h"
#include "OtherFunctions.h"
#include "StringFunctions.h"

//-------------------------------------------------------------------

CCommandTree::~CCommandTree()
{
	for (CmdPos_t it = m_subcommands.begin(); it != m_subcommands.end(); ++it) {
		delete *it;
	}
	m_subcommands.clear();
}


CCommandTree* CCommandTree::AddCommand(CCommandTree* command)
{
	command->m_parent = this;
	const wxString& cmd = command->m_command;
	CmdPos_t it;
	for (it = m_subcommands.begin(); it != m_subcommands.end(); ++it) {
		if ((*it)->m_command > cmd) {
			break;
		}
	}
	m_subcommands.insert(it, command);
	return command;
}


int CCommandTree::FindCommandId(const wxString& command, wxString& args, wxString& cmdstr) const
{
	wxString cmd = command.BeforeFirst(wxT(' ')).Lower();
	for (CmdPosConst_t it = m_subcommands.begin(); it != m_subcommands.end(); ++it) {
		if ((*it)->m_command.Lower() == cmd) {
			args = command.AfterFirst(wxT(' ')).Trim(false);
			return (*it)->FindCommandId(args, args, cmdstr);
		}
	}
	cmdstr = GetFullCommand().Lower();
	if (m_params == CMD_PARAM_ALWAYS && args.IsEmpty()) {
		return CMD_ERR_MUST_HAVE_PARAM;
	} else if (m_params == CMD_PARAM_NEVER && !args.IsEmpty()) {
		return CMD_ERR_NO_PARAM;
	} else {
		if ((m_cmd_id >= 0) && (m_cmd_id & CMD_DEPRECATED)) {
			m_app.Show(wxT('\n') + m_verbose + wxT('\n'));
			return m_cmd_id & ~CMD_DEPRECATED;
		} else {
			return m_cmd_id;
		}
	}
}


wxString CCommandTree::GetFullCommand() const
{
	wxString cmd = m_command;

	const CCommandTree *parent = m_parent;
	while (parent && parent->m_parent) {
		cmd = parent->m_command + wxT(" ") + cmd;
		parent = parent->m_parent;
	}

	return cmd;
}


void CCommandTree::PrintHelpFor(const wxString& command) const
{
	wxString cmd = command.BeforeFirst(wxT(' ')).Lower();
	if (!cmd.IsEmpty()) {
		for (CmdPosConst_t it = m_subcommands.begin(); it != m_subcommands.end(); ++it) {
			if ((*it)->m_command.Lower() == cmd) {
				(*it)->PrintHelpFor(command.AfterFirst(wxT(' ')).Trim(false));
				return;
			}
		}
		if (m_parent) {
			m_app.Show(CFormat(_("Unknown extension '%s' for the '%s' command.\n")) % command % GetFullCommand());
		} else {
			m_app.Show(CFormat(_("Unknown command '%s'.\n")) % command);
		}
	} else {
		wxString fullcmd = GetFullCommand();
		if (!fullcmd.IsEmpty()) {
			m_app.Show(fullcmd.Upper() + wxT(": ") + wxGetTranslation(m_short) + wxT("\n"));
			if (!m_verbose.IsEmpty()) {
				m_app.Show(wxT("\n"));
				m_app.Show(wxGetTranslation(m_verbose));
			}
		}
		if (m_params == CMD_PARAM_NEVER) {
			m_app.Show(_("\nThis command cannot have an argument.\n"));
		} else if (m_params == CMD_PARAM_ALWAYS) {
			m_app.Show(_("\nThis command must have an argument.\n"));
		}
		if (m_cmd_id == CMD_ERR_INCOMPLETE) {
			m_app.Show(_("\nThis command is incomplete, you must use one of the extensions below.\n"));
		}
		if (!m_subcommands.empty()) {
			CmdPosConst_t it;
			int maxlen = 0;
			if (m_parent) {
				m_app.Show(_("\nAvailable extensions:\n"));
			} else {
				m_app.Show(_("Available commands:\n"));
			}
			for (it = m_subcommands.begin(); it != m_subcommands.end(); ++it) {
				if (!((*it)->m_cmd_id >= 0 && (*it)->m_cmd_id & CMD_DEPRECATED) || m_parent) {
					int len = (*it)->m_command.Length();
					if (len > maxlen) {
						maxlen = len;
					}
				}
			}
			maxlen += 4;
			for (it = m_subcommands.begin(); it != m_subcommands.end(); ++it) {
				if (!((*it)->m_cmd_id >= 0 && (*it)->m_cmd_id & CMD_DEPRECATED) || m_parent) {
					m_app.Show((*it)->m_command + wxString(wxT(' '), maxlen - (*it)->m_command.Length()) + wxGetTranslation((*it)->m_short) + wxT("\n"));
				}
			}
			if (!m_parent) {
				m_app.Show(_("\nAll commands are case insensitive.\n"
					   "Type 'help <command>' to get detailed info on <command>.\n"));
			}
		}
	}
	m_app.Show(wxT("\n"));
}

//-------------------------------------------------------------------

CaMuleExternalConnector::CaMuleExternalConnector()
	: m_commands(*this)
{
	m_ECClient = NULL;
	m_KeepQuiet = false;
	m_InputLine = NULL;
	m_port = -1;
	m_NeedsConfigSave = false;
	m_Verbose = false;
	m_configFile = NULL;
	SetAppName(wxT("aMule"));
}

CaMuleExternalConnector::~CaMuleExternalConnector()
{
	delete m_configFile;
}

void CaMuleExternalConnector::OnInitCommandSet()
{
	m_commands.AddCommand(wxT("Quit"), CMD_ID_QUIT, wxTRANSLATE("Exits from the application."), wxEmptyString);
	m_commands.AddCommand(wxT("Exit"), CMD_ID_QUIT, wxTRANSLATE("Exits from the application."), wxEmptyString);
	m_commands.AddCommand(wxT("Help"), CMD_ID_HELP, wxTRANSLATE("Show help."),
			      wxTRANSLATE("To get help on a command, type 'help <command>'.\n"
					  "To get the full command list type 'help'.\n"));
}

void CaMuleExternalConnector::Show(const wxString &s)
{
	if( !m_KeepQuiet ) {
#if wxUSE_GUI
		if ( wxThread::IsMain() ) {
			LocalShow(s);
		} else {
			// Print it later
			wxMutexLocker lock(m_mutex_printlist);
			m_printlist.push_back(s);
			//wxWakeUpIdle();
		}
#else
		printf("%s", (const char *)unicode2char(s));
#endif
	}
}

void CaMuleExternalConnector::ShowGreet()
{
	wxString text = GetGreetingTitle();
	int len = text.Length();
	Show(wxT('\n') + wxString(wxT('-'), 22 + len) + wxT('\n'));
	Show(wxT('|') + wxString(wxT(' '), 10) + text + wxString(wxT(' '), 10) + wxT('|') + wxT('\n'));
	Show(wxString(wxT('-'), 22 + len) + wxT('\n'));
	// Do not merge the line below, or translators could translate "Help"
	Show(CFormat(_("\nUse '%s' for command list\n\n")) % wxT("Help"));
}

#if wxUSE_GUI
void CaMuleExternalConnector::MainThreadIdleNow()
{
	wxMutexLocker lock(m_mutex_printlist);
	for( StrList::iterator i = m_printlist.begin(); i != m_printlist.end(); ++i)
	{
		LocalShow(*i);
	}
	m_printlist.clear();
}
#endif

void CaMuleExternalConnector::Process_Answer(const wxString& answer)
{
	wxStringTokenizer tokens(answer, wxT("\n"));
	while ( tokens.HasMoreTokens() ) {
		Show(wxT(" > ") + tokens.GetNextToken() + wxT("\n"));
	}
}

bool CaMuleExternalConnector::Parse_Command(const wxString& buffer)
{
	wxString cmd;
	wxStringTokenizer tokens(buffer);
	while (tokens.HasMoreTokens()) {
		cmd += tokens.GetNextToken() + wxT(' ');
	}
	cmd.Trim(false);
	cmd.Trim(true);
	int cmd_ID = GetIDFromString(cmd);
	if ( cmd_ID >= 0 ) {
		cmd_ID = ProcessCommand(cmd_ID);
	}
	wxString error;
	switch (cmd_ID) {
		case CMD_ID_HELP:
			m_commands.PrintHelpFor(GetCmdArgs());
			break;
		case CMD_ERR_SYNTAX:
			error = _("Syntax error!");
			break;
		case CMD_ERR_PROCESS_CMD:
			Show(_("Error processing command - should never happen! Report bug, please\n"));
			break;
		case CMD_ERR_NO_PARAM:
			error = _("This command should not have any parameters.");
			break;
		case CMD_ERR_MUST_HAVE_PARAM:
			error = _("This command must have a parameter.");
			break;
		case CMD_ERR_INVALID_ARG:
			error = _("Invalid argument.");
			break;
		case CMD_ERR_INCOMPLETE:
			error = _("This is an incomplete command.");
			break;
	}
	if (!error.IsEmpty()) {
		Show(error + wxT('\n'));
		wxString helpStr(wxT("help"));
		if (!GetLastCmdStr().IsEmpty()) {
			helpStr << wxT(' ') << GetLastCmdStr();
		}
		Show(CFormat(_("Type '%s' to get more help.\n")) % helpStr);
	}
	return cmd_ID == CMD_ID_QUIT;
}

void CaMuleExternalConnector::GetCommand(const wxString &prompt, char* buffer, size_t buffer_size)
{
	if( !m_KeepQuiet ) {
#if wxUSE_GUI
		const wxCharBuffer buf = unicode2char(
			wxGetTextFromUser(prompt, wxT("Enter Command")));
		const char *text = (const char *)buf;
#else
#ifdef HAVE_LIBREADLINE
		if (m_InputLine) {
			free(m_InputLine);
			m_InputLine = (char *)NULL;
		}
		m_InputLine = readline(unicode2char(prompt + wxT("$ ")));
		if (m_InputLine && *m_InputLine) {
			add_history (m_InputLine);
		}
		const char *text = m_InputLine;
#else
		Show(prompt + wxT("$ "));
		fflush(stdin);
		fgets(buffer, buffer_size, stdin);
		const char *text = buffer;
#endif /* HAVE_LIBREADLINE */
#endif /* wxUSE_GUI */
		if ( text ) {
			size_t len = strlen(text);
			if (len > buffer_size - 2) {
				len = buffer_size - 2;
			}
			strncpy(buffer, text, len);
			buffer[len] = '\n';
			buffer[len + 1] = 0;
		} else {
			strncpy(buffer, "quit", buffer_size);
		}
	}
}

void CaMuleExternalConnector::TextShell(const wxString &prompt)
{
	char buffer[256];
	wxString buf;

	bool The_End = false;
	do {
		GetCommand(prompt, buffer, 256);
		buf = char2unicode(buffer);
		The_End = Parse_Command(buf);
	} while ((!The_End) && (m_ECClient->IsConnected()));
}

CECPacket *CaMuleExternalConnector::SendRecvMsg_v2(CECPacket *request)
{
	if (m_ECClient->WritePacket(request)) {
		CECPacket *reply = m_ECClient->ReadPacket();
		return reply;
	}
	return 0;
}

void CaMuleExternalConnector::ConnectAndRun(const wxString &ProgName, const wxString& ProgVersion)
{
	if (m_NeedsConfigSave) {
		SaveConfigFile();
		return;
	}

	wxString appName =
		// Find out Application Name
		#ifdef AMULECMDDLG
			wxT("amulecmd [DLG version]")
		#else
			#ifdef AMULEWEBDLG
				wxT("amuleweb [DLG version]")
			#else
				#ifdef WEBSERVERDIR
					wxT("amuleweb")
				#else
					wxT("amulecmd")
				#endif
			#endif
		#endif
	;

	#ifdef CVSDATE
		Show(CFormat(_("This is %s %s %s\n")) % appName % wxT(VERSION) % wxT(CVSDATE));
	#else
		Show(CFormat(_("This is %s %s\n")) % appName % wxT(VERSION));
	#endif

	// HostName, Port and Password
	if ( m_password.IsEmpty() ) {
		wxString pass_plain;
#if wxUSE_GUI
		m_host = wxGetTextFromUser(
			_("Enter hostname or ip of the box running aMule"),
			_("Enter Hostname"), wxT("localhost"));
		wxString sPort = wxGetTextFromUser(
			_("Enter port for aMule's External Connection"),
			_("Enter Port"), wxT("4712"));
		if (!sPort.ToLong(&m_port)) {
			// invalid input, use default
			m_port = 4712;
		}
		pass_plain = ::wxGetPasswordFromUser(
			_("Enter password for mule connection"),
			_("Enter Password"));
#else  // wxUse_GUI
		#ifndef __WXMSW__
			pass_plain = char2unicode(getpass("Enter password for mule connection: "));
		#else
			#warning This way, pass enter is not hidden on windows. Bad thing.
			char temp_str[512];
			fflush(stdin);
			printf("Enter password for mule connection: \n");
			fgets(temp_str, 512, stdin);
			temp_str[strlen(temp_str)-1] = '\0';
			pass_plain = char2unicode(temp_str);
		#endif
#endif // wxUse_GUI
		m_password.Decode(MD5Sum(pass_plain).GetHash());
		// MD5 hash for an empty string, according to rfc1321.
		if (m_password == CMD4Hash(wxT("D41D8CD98F00B204E9800998ECF8427E"))) {
			m_password.Clear();
		}

		// Clear plain-text password
		pass_plain		= wxT("01234567890123456789");
	}
	
	if (!m_password.IsEmpty()) {

		// Create the socket
		Show(_("\nCreating client...\n"));
		m_ECClient = new CRemoteConnect(NULL);

		m_ECClient->ConnectToCore(m_host, m_port, wxT("foobar"), m_password.Encode(),
								 ProgName, ProgVersion);
		
		m_ECClient->WaitOnConnect(10);

		if (!m_ECClient->IsConnected()) {
			// no connection => close gracefully
			Show(_("Connection Failed. Unable to connect to the specified host\n"));
		} else {
			// Authenticate ourselves
			m_ECClient->ConnectionEstablished();
			Show(m_ECClient->GetServerReply()+wxT("\n"));
			if (m_ECClient->IsConnected()) {
				ShowGreet();
				Pre_Shell();
#if wxUSE_GUI
				// Do nothing, the shell is in another place.
#else
				if (m_KeepQuiet) {
					while(true) {
						#ifndef __WXMSW__
						pause();
						#else
						wxSleep(10);
						#endif
					}
				} else {
					TextShell(ProgName);
				}
				Show(CFormat(_("\nOk, exiting %s...\n")) % ProgName);
#endif
				Post_Shell();
			}
		}
#if wxUSE_GUI
		// Destroy will be called elsewhere in gui.
#else
		m_ECClient->Destroy();
#endif
	} else {
		Show(_("Cannot connect with an empty password.\nYou must specify a password either in config file\nor on command-line, or enter one when asked.\n\nExiting...\n"));
	}
}

void CaMuleExternalConnector::OnInitCmdLine(wxCmdLineParser& parser)
{
	parser.AddSwitch(wxEmptyString, wxT("help"),
		_("Show this help text."),
		wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("h"), wxT("host"),
		_("Host where aMule is running. (default: localhost)"),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("p"), wxT("port"),
		_("aMule's port for External Connection. (default: 4712)"),
		wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("P"), wxT("password"),
		_("External Connection password."),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("f"), wxT("config-file"),
		_("Read configuration from file."),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddSwitch(wxT("q"), wxT("quiet"),
		_("Do not print any output to stdout."),
		wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddSwitch(wxT("v"), wxT("verbose"),
		_("Be verbose - show also debug messages."),
		wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxT("l"), wxT("locale"),
		_("Sets program locale (language)."),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddSwitch(wxT("w"), wxT("write-config"),
		_("Write command line options to config file."),
		wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddOption(wxEmptyString, wxT("create-config-from"),
		_("Creates config file based on aMule's config file."),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddSwitch(wxEmptyString, wxT("version"),
		_("Print program version."),
		wxCMD_LINE_PARAM_OPTIONAL);
}

bool CaMuleExternalConnector::OnCmdLineParsed(wxCmdLineParser& parser)
{
	if (parser.Found(wxT("version"))) {
		const char *appName =
			// Find out Application Name
			#ifdef AMULECMDDLG
				"amulecmd DLG"
			#else
				#ifdef AMULEWEBDLG
					"amuleweb DLG"
				#else
					#ifdef WEBSERVERDIR
						"amuleweb"
					#else
						"amulecmd"
					#endif
				#endif
			#endif
		;
		printf("%s %s\n", appName, (const char *)unicode2char(GetMuleVersion()));
		return false;
	}

	if (!parser.Found(wxT("config-file"), &m_configFileName)) {
		m_configFileName = GetConfigDir() + wxT("remote.conf");
	}

	wxString aMuleConfigFile;
	if (parser.Found(wxT("create-config-from"), &aMuleConfigFile)) {
		aMuleConfigFile = FinalizeFilename(aMuleConfigFile);
		if (!::wxFileExists(aMuleConfigFile)) {
			fprintf(stderr, "%s\n", (const char *)unicode2char(wxT("FATAL ERROR: File does not exist: ") + aMuleConfigFile));
			exit(1);
		}
		CECFileConfig aMuleConfig(aMuleConfigFile);
		LoadAmuleConfig(aMuleConfig);
		SaveConfigFile();
		m_configFile->Flush();
		exit(0);
	}

	LoadConfigFile();

	if ( !parser.Found(wxT("host"), &m_host) ) {
		if ( m_host.IsEmpty() ) {
			m_host = wxT("localhost");
		}
	}

	long port;
	if (!parser.Found(wxT("port"), &port)) {
		if (m_port == -1) {
			m_port = 4712;
		}
	} else {
		m_port = port;
	}

	wxString pass_plain;
	if (parser.Found(wxT("password"), &pass_plain)) {
		if (!pass_plain.IsEmpty()) {
			m_password.Decode(MD5Sum(pass_plain).GetHash());
		} else {
			m_password.Clear();
		}
	}

	if (parser.Found(wxT("write-config"))) {
		m_NeedsConfigSave = true;
	}

	parser.Found(wxT("locale"), &m_language);

	if (parser.Found(wxT("help"))) {
		parser.Usage();
		return false;
	}

	m_KeepQuiet = parser.Found(wxT("quiet"));
	m_Verbose = parser.Found(wxT("verbose"));

	return true;
}

void CaMuleExternalConnector::LoadAmuleConfig(CECFileConfig& cfg)
{
	m_host = wxT("localhost");
	m_port = cfg.Read(wxT("/ExternalConnect/ECPort"), -1l);
	cfg.ReadHash(wxT("/ExternalConnect/ECPassword"), &m_password);
	m_language = cfg.Read(wxT("/eMule/Language"), wxEmptyString);
}


void CaMuleExternalConnector::LoadConfigFile()
{
	if (!m_configFile) {
		m_configFile = new CECFileConfig(m_configFileName);
	}
	if (m_configFile) {
		m_language = m_configFile->Read(wxT("/Locale"), wxEmptyString);
		m_host = m_configFile->Read(wxT("/EC/Host"), wxEmptyString);
		m_port = m_configFile->Read(wxT("/EC/Port"), -1l);
		m_configFile->ReadHash(wxT("/EC/Password"), &m_password);
	}
}

void CaMuleExternalConnector::SaveConfigFile()
{
	if (!wxFileName::DirExists(GetConfigDir())) {
		wxFileName::Mkdir(GetConfigDir());
	}
	if (!m_configFile) {
		m_configFile = new CECFileConfig(m_configFileName);
	}
	if (m_configFile) {
		m_configFile->Write(wxT("/Locale"), m_language);
		m_configFile->Write(wxT("/EC/Host"), m_host);
		m_configFile->Write(wxT("/EC/Port"), m_port);
		m_configFile->WriteHash(wxT("/EC/Password"), m_password);
	}
}

bool CaMuleExternalConnector::OnInit()
{
	bool retval = wxApp::OnInit();
	OnInitCommandSet();
	InitCustomLanguages();
	InitLocale(m_locale, StrLang2wx(m_language));
	return retval;
}
