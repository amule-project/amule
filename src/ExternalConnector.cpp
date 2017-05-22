//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "ExternalConnector.h"

#ifdef HAVE_CONFIG_H
	#include "config.h"	// Needed for VERSION and readline detection
#else
	#include <common/ClientVersion.h>
#endif

#include <common/Format.h>		// Needed for CFormat
#include <wx/tokenzr.h>		// For wxStringTokenizer

// For readline
#ifdef HAVE_LIBREADLINE
	#if defined(HAVE_READLINE_READLINE_H)
		#include <readline/readline.h>  // Do_not_auto_remove
	#elif defined(HAVE_READLINE_H)
		#include <readline.h> // Do_not_auto_remove
	#else /* !defined(HAVE_READLINE_H) */
		extern "C" char *readline (const char*);
	#endif /* !defined(HAVE_READLINE_H) */
#else /* !defined(HAVE_READLINE_READLINE_H) */
	/* no readline */
#endif /* HAVE_LIBREADLINE */

// For history
#ifdef HAVE_READLINE_HISTORY
	#if defined(HAVE_READLINE_HISTORY_H)
		#include <readline/history.h> // Do_not_auto_remove
	#elif defined(HAVE_HISTORY_H)
		#include <history.h> // Do_not_auto_remove
	#else /* !defined(HAVE_HISTORY_H) */
		extern "C" void add_history (const char*);
	#endif /* defined(HAVE_READLINE_HISTORY_H) */
#else
	/* no history */
#endif /* HAVE_READLINE_HISTORY */


#include <ec/cpp/ECFileConfig.h>	// Needed for CECFileConfig
#include <common/MD5Sum.h>
#include "OtherFunctions.h"		// Needed for GetPassword()
#include "MuleVersion.h"		// Needed for GetMuleVersion()

#ifdef _MSC_VER  // silly warnings about deprecated functions
#pragma warning(disable:4996)
#endif

//-------------------------------------------------------------------

CCommandTree::~CCommandTree()
{
	DeleteContents(m_subcommands);
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
				m_app.Show(CFormat(_("\nAll commands are case insensitive.\nType '%s <command>' to get detailed info on <command>.\n")) % wxT("help"));
			}
		}
	}
	m_app.Show(wxT("\n"));
}

//-------------------------------------------------------------------

CaMuleExternalConnector::CaMuleExternalConnector()
	: m_configFile(NULL),
	  m_port(-1),
	  m_ZLIB(false),
	  m_KeepQuiet(false),
	  m_Verbose(false),
	  m_interactive(false),
	  m_commands(*this),
	  m_appname(NULL),
	  m_ECClient(NULL),
	  m_InputLine(NULL),
	  m_NeedsConfigSave(false),
	  m_locale(NULL),
	  m_strFullVersion(NULL),
	  m_strOSDescription(NULL)
{
	SetAppName(wxT("aMule"));	// Do not change!
}

CaMuleExternalConnector::~CaMuleExternalConnector()
{
	delete m_configFile;
	delete m_locale;
	free(m_strFullVersion);
	free(m_strOSDescription);
}

void CaMuleExternalConnector::OnInitCommandSet()
{
	m_commands.AddCommand(wxT("Quit"), CMD_ID_QUIT, wxTRANSLATE("Exits from the application."), wxEmptyString);
	m_commands.AddCommand(wxT("Exit"), CMD_ID_QUIT, wxTRANSLATE("Exits from the application."), wxEmptyString);
	m_commands.AddCommand(wxT("Help"), CMD_ID_HELP, wxTRANSLATE("Show help."),
			      /* TRANSLATORS:
				 Do not translate the word 'help', it is a command to the program! */
			      wxTRANSLATE("To get help on a command, type 'help <command>'.\nTo get the full command list type 'help'.\n"));
}

void CaMuleExternalConnector::Show(const wxString &s)
{
	if( !m_KeepQuiet ) {
		printf("%s", (const char *)unicode2char(s));
#ifdef __WINDOWS__
		fflush(stdout);
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
#ifdef HAVE_LIBREADLINE
		char *text = readline(unicode2char(prompt + wxT("$ ")));
		if (text && *text &&
		    (m_InputLine == 0 || strcmp(text,m_InputLine) != 0)) {
		  add_history (text);
		}
		if (m_InputLine)
		  free(m_InputLine);
		m_InputLine = text;
#else
		Show(prompt + wxT("$ "));
		const char *text = fgets(buffer, buffer_size, stdin);	// == buffer if ok, NULL if eof
#endif /* HAVE_LIBREADLINE */
		if ( text ) {
			size_t len = strlen(text);
			if (len > buffer_size - 1) {
				len = buffer_size - 1;
			}
			if (buffer != text) {
				strncpy(buffer, text, len);
			}
			buffer[len] = 0;
		} else {
			strncpy(buffer, "quit", buffer_size);
		}
}

void CaMuleExternalConnector::TextShell(const wxString &prompt)
{
	char buffer[2048];
	wxString buf;

	bool The_End = false;
	do {
		GetCommand(prompt, buffer, sizeof buffer);
		buf = char2unicode(buffer);
		The_End = Parse_Command(buf);
	} while ((!The_End) && (m_ECClient->IsSocketConnected()));
}

void CaMuleExternalConnector::ConnectAndRun(const wxString &ProgName, const wxString& ProgVersion)
{
	if (m_NeedsConfigSave) {
		SaveConfigFile();
		return;
	}

	#ifdef SVNDATE
		Show(CFormat(_("This is %s %s %s\n")) % wxString::FromAscii(m_appname) % wxT(VERSION) % wxT(SVNDATE));
	#else
		Show(CFormat(_("This is %s %s\n")) % wxString::FromAscii(m_appname) % wxT(VERSION));
	#endif

	// HostName, Port and Password
	if ( m_password.IsEmpty() ) {
		m_password = GetPassword(true);
		// MD5 hash for an empty string, according to rfc1321.
		if (m_password.Encode() == wxT("D41D8CD98F00B204E9800998ECF8427E")) {
			m_password.Clear();
		}
	}

	if (!m_password.IsEmpty()) {

		// Create the socket
		Show(_("\nCreating client...\n"));
		m_ECClient = new CRemoteConnect(NULL);
		m_ECClient->SetCapabilities(m_ZLIB, true, false);	// ZLIB, UTF8 numbers, notification

		// ConnectToCore is blocking since m_ECClient was initialized with NULL
		if (!m_ECClient->ConnectToCore(m_host, m_port, wxT("foobar"), m_password.Encode(), ProgName, ProgVersion)) {
			// no connection => close gracefully
			if (!m_ECClient->GetServerReply().IsEmpty()) {
					Show(CFormat(wxT("%s\n")) % m_ECClient->GetServerReply());
			}
			Show(CFormat(_("Connection Failed. Unable to connect to %s:%d\n")) % m_host % m_port);
		} else {
			// Authenticate ourselves
			// ConnectToCore() already authenticated for us.
			//m_ECClient->ConnectionEstablished();
			Show(m_ECClient->GetServerReply()+wxT("\n"));
			if (m_ECClient->IsSocketConnected()) {
				if (m_interactive) {
					ShowGreet();
				}
				Pre_Shell();
				TextShell(ProgName);
				Post_Shell();
				if (m_interactive) {
					Show(CFormat(_("\nOk, exiting %s...\n")) % ProgName);
				}
			}
		}
		m_ECClient->DestroySocket();
	} else {
		Show(_("Cannot connect with an empty password.\nYou must specify a password either in config file\nor on command-line, or enter one when asked.\n\nExiting...\n"));
	}
}

void CaMuleExternalConnector::OnInitCmdLine(wxCmdLineParser& parser, const char* appname)
{
	m_appname = appname;

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
		printf("%s %s\n", m_appname, (const char *)unicode2char(GetMuleVersion()));
		return false;
	}

	if (!parser.Found(wxT("config-file"), &m_configFileName)) {
		m_configFileName = wxT("remote.conf");
	}
	m_configDir = GetConfigDir(m_configFileName);
	m_configFileName = m_configDir + m_configFileName;

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
	if (parser.Found(wxT("port"), &port)) {
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
	m_port = cfg.Read(wxT("/ExternalConnect/ECPort"), 4712l);
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
		m_port = m_configFile->Read(wxT("/EC/Port"), 4712l);
		m_configFile->ReadHash(wxT("/EC/Password"), &m_password);
		m_ZLIB = m_configFile->Read(wxT("/EC/ZLIB"), 1l) != 0;
	}
}

void CaMuleExternalConnector::SaveConfigFile()
{
	if (!wxFileName::DirExists(m_configDir)) {
		wxFileName::Mkdir(m_configDir);
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
#ifndef __WINDOWS__
	#if wxUSE_ON_FATAL_EXCEPTION
		// catch fatal exceptions
		wxHandleFatalExceptions(true);
	#endif
#endif

	// If we didn't know that OnInit is called only once when creating the
	// object, it could cause a memory leak. The two pointers below should
	// be free()'d before assigning the new value.
	// cppcheck-suppress publicAllocationError
	m_strFullVersion = strdup((const char *)unicode2char(GetMuleVersion()));
	m_strOSDescription = strdup((const char *)unicode2char(wxGetOsDescription()));

	// Handle uncaught exceptions
	InstallMuleExceptionHandler();

	bool retval = wxApp::OnInit();
	OnInitCommandSet();
	InitCustomLanguages();
	SetLocale(m_language);
	return retval;
}

wxString CaMuleExternalConnector::SetLocale(const wxString& language)
{
	if (!language.IsEmpty()) {
		m_language = language;
		if (m_locale) {
			delete m_locale;
		}
		m_locale = new wxLocale;
		InitLocale(*m_locale, StrLang2wx(language));
	}

	return m_locale == NULL ? wxString() : m_locale->GetCanonicalName();
}

#if !wxUSE_GUI && defined(__WXMAC__) && !wxCHECK_VERSION(2, 9, 0)

#include <wx/apptrait.h> // Do_not_auto_remove
#include <wx/stdpaths.h> // Do_not_auto_remove

class CaMuleExternalConnectorTraits : public wxConsoleAppTraits
{
public:
	virtual wxStandardPathsBase& GetStandardPaths()
	{
		return s_stdPaths;
	}

private:
	static wxStandardPathsCF s_stdPaths;
};

wxStandardPathsCF CaMuleExternalConnectorTraits::s_stdPaths;

wxAppTraits* CaMuleExternalConnector::CreateTraits()
{
	return new CaMuleExternalConnectorTraits;
}

#endif

#if wxUSE_ON_FATAL_EXCEPTION
// Gracefully handle fatal exceptions and print backtrace if possible
void CaMuleExternalConnector::OnFatalException()
{
	/* Print the backtrace */
	fprintf(stderr, "\n--------------------------------------------------------------------------------\n");
	fprintf(stderr, "A fatal error has occurred and %s has crashed.\n", m_appname);
	fprintf(stderr, "Please assist us in fixing this problem by posting the backtrace below in our\n");
	fprintf(stderr, "'aMule Crashes' forum and include as much information as possible regarding the\n");
	fprintf(stderr, "circumstances of this crash. The forum is located here:\n");
	fprintf(stderr, "    http://forum.amule.org/index.php?board=67.0\n");
	fprintf(stderr, "If possible, please try to generate a real backtrace of this crash:\n");
	fprintf(stderr, "    http://wiki.amule.org/wiki/Backtraces\n\n");
	fprintf(stderr, "----------------------------=| BACKTRACE FOLLOWS: |=----------------------------\n");
	fprintf(stderr, "Current version is: %s %s\n", m_appname, m_strFullVersion);
	fprintf(stderr, "Running on: %s\n\n", m_strOSDescription);

	print_backtrace(1); // 1 == skip this function.

	fprintf(stderr, "\n--------------------------------------------------------------------------------\n");
}
#endif

#ifdef __WXDEBUG__
void CaMuleExternalConnector::OnAssertFailure(const wxChar *file, int line, const wxChar *func, const wxChar *cond, const wxChar *msg)
{
#if !defined wxUSE_STACKWALKER || !wxUSE_STACKWALKER
	wxString errmsg = CFormat( wxT("%s:%s:%d: Assertion '%s' failed. %s") ) % file % func % line % cond % ( msg ? msg : wxT("") );

	fprintf(stderr, "Assertion failed: %s\n", (const char*)unicode2char(errmsg));

	// Skip the function-calls directly related to the assert call.
	fprintf(stderr, "\nBacktrace follows:\n");
	print_backtrace(3);
	fprintf(stderr, "\n");
#else
	wxApp::OnAssertFailure(file, line, func, cond, msg);
#endif
}
#endif
// File_checked_for_headers
