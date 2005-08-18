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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ExternalConnector.h"
#pragma implementation "ECFileConfig.h"
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

CaMuleExternalConnector::CaMuleExternalConnector()
{
	m_ECClient = NULL;
	m_isConnected = false;
	m_KeepQuiet = false;
	m_InputLine = NULL;
	m_port = -1;
	m_NeedsConfigSave = false;
	m_Verbose = false;
	m_configFile = NULL;
}

CaMuleExternalConnector::~CaMuleExternalConnector()
{
	delete m_configFile;
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

void CaMuleExternalConnector::Dump(const wxString &s)
{
	FILE *fp = fopen("x.txt", "a");
	fprintf(fp, "%s", (const char *)unicode2char(s));
	fclose(fp);
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

int CaMuleExternalConnector::GetIDFromString(wxString &buffer, CmdId commands[])
{
	wxStringTokenizer tokens(buffer);
	wxString cmd = tokens.GetNextToken().MakeLower();

	if ( cmd.IsEmpty() ) {
		return 0;
	}
	
	m_cmdargs.Clear();
	while ( tokens.HasMoreTokens() ) {
		m_cmdargs += tokens.GetNextToken().MakeLower() + wxT(" ");
	}
	// Remove last space.
	m_cmdargs.Trim();

	register int i = 0;
	bool found = false;
	while ( !found && ( commands[i].cmd.IsEmpty() == false ) ) {
		found = commands[i].cmd == cmd;
		if (!found) {
			i++;
		}
	}
	
	return found ? commands[i].id : CMD_ID_SYNTAX_ERROR;
}

void CaMuleExternalConnector::Process_Answer(const wxString& answer)
{
	wxStringTokenizer tokens(answer, wxT("\n"));
	wxString t;
	while ( tokens.HasMoreTokens() ) {
		Show(wxT(" > ") + tokens.GetNextToken() + wxT("\n"));
	}
}

bool CaMuleExternalConnector::Parse_Command(wxString &buffer, CmdId commands[])
{
	int cmd_ID = GetIDFromString(buffer, commands);
	bool quit = cmd_ID == CMD_ID_QUIT;
	if ( (cmd_ID > 0) && !quit ) {
		if ( ProcessCommand(cmd_ID) < 0 ) {
			Show(_("Error processing command - should never happen! Report bug, please\n"));
		}
	} else if ( cmd_ID == CMD_ID_SYNTAX_ERROR ) {
		Show(_("Syntax error!\n"));
	}
	
	return quit;
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

void CaMuleExternalConnector::TextShell(const wxString &prompt, CmdId commands[])
{
	char buffer[256];
	wxString buf;

	bool The_End = false;
	do {
		GetCommand(prompt, buffer, 256);
		buf = char2unicode(buffer);
		The_End = Parse_Command(buf, commands);
	} while ((!The_End) && (m_isConnected));
}

CECPacket *CaMuleExternalConnector::SendRecvMsg_v2(CECPacket *request)
{
	if (m_ECClient->WritePacket(request)) {
		CECPacket *reply = m_ECClient->ReadPacket();
		return reply;
	}
	return 0;
}

void CaMuleExternalConnector::ConnectAndRun(const wxString &ProgName, const wxString& ProgVersion, CmdId *UNUSED_IN_GUI(commands))
{
	wxString pass_plain;

	if (m_NeedsConfigSave) {
		SaveConfigFile();
		return;
	}

	// HostName, Port and Password
	if ( m_password.IsEmpty() ) {
#if wxUSE_GUI
		m_host = wxGetTextFromUser(
			wxT("Enter hostname or ip of the box running aMule"),
			wxT("Enter Hostname"), wxT("localhost"));
		wxString sPort = wxGetTextFromUser(
			wxT("Enter port for aMule's External Connection"),
			wxT("Enter Port"), wxT("4712"));
		if (!sPort.ToLong(&m_port)) {
			// invalid input, use default
			m_port = 4712;
		}
		pass_plain = ::wxGetPasswordFromUser(
			wxT("Enter password for mule connection"),
			wxT("Enter Password"));
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
	}
	
	if (!m_password.IsEmpty()) {

		// Create the packet
		CECPacket packet(EC_OP_AUTH_REQ);
		packet.AddTag(CECTag(EC_TAG_CLIENT_NAME, ProgName));
		packet.AddTag(CECTag(EC_TAG_CLIENT_VERSION, ProgVersion));
		packet.AddTag(CECTag(EC_TAG_PROTOCOL_VERSION, (uint16)EC_CURRENT_PROTOCOL_VERSION));
		packet.AddTag(CECTag(EC_TAG_PASSWD_HASH, m_password));

#ifdef EC_VERSION_ID
		packet.AddTag(CECTag(EC_TAG_VERSION_ID, CMD4Hash(wxT(EC_VERSION_ID))));
#endif

		// Clear passwords
		pass_plain		= wxT("01234567890123456789");

		// Create the socket
		Show(_("\nCreating client...\n"));
		m_ECClient = new ECSocket();

		Show(_("Now, doing connection....\n"));
		wxIPV4address addr;
		addr.Hostname(m_host);
		addr.Service(m_port);

		Show( CFormat(_("Using host '%s' port: %d\n")) % addr.Hostname() % addr.Service());
		Show(_("Trying to connect (timeout = 10 sec)...\n"));
	  	m_ECClient->Connect(addr, false);
		m_ECClient->WaitOnConnect(10);

		if (!m_ECClient->IsConnected()) {
			// no connection => close gracefully
			Show(_("Connection Failed. Unable to connect to the specified host\n"));
		} else {
			// Authenticate ourselves
			bool success = m_ECClient->WritePacket(&packet);
			CECPacket *reply = NULL;
			if (success) {
			    reply = m_ECClient->ReadPacket();
			    success = (reply != NULL);
			}
			if (success) {
			    if (reply->GetOpCode() == EC_OP_AUTH_FAIL) {
				if (reply->GetTagCount() > 0) {
				    CECTag *reason = reply->GetTagByName(EC_TAG_STRING);
				    if (reason != NULL) {
					Show(CFormat(_("ExternalConn: Access denied because: %s\n")) % wxGetTranslation(reason->GetStringData()));
				    } else {
					Show(_("ExternalConn: Access denied.\n"));
				    }
				} else {
				    Show(_("ExternalConn: Access denied.\n"));
				}
			    } else if (reply->GetOpCode() != EC_OP_AUTH_OK) {
				Show(_("ExternalConn: Bad reply from server. Connection closed.\n"));
			    } else {
				m_isConnected = true;
				if (reply->GetTagByName(EC_TAG_SERVER_VERSION)) {
					Show(CFormat(_("Succeeded! Connection established to aMule %s\n")) % reply->GetTagByName(EC_TAG_SERVER_VERSION)->GetStringData());
				} else {
					Show(_("Succeeded! Connection established.\n"));
				}
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
					TextShell(ProgName, commands);
				}
				Show(CFormat(_("\nOk, exiting %s...\n")) % ProgName);
#endif
				Post_Shell();
			    }
			} else {
			    Show(_("A socket error occured during authentication. Exiting.\n"));
			}

			delete reply;
#if wxUSE_GUI
			// Destroy will be called elsewhere in gui.
#else
			m_ECClient->Destroy();
#endif
		}
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
	InitCustomLanguages();
	InitLocale(m_locale, StrLang2wx(m_language));

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
