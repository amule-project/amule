/*
 * Copyright (C) 2004 aMule Team (http://www.amule.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"	// Needed for VERSION
#endif

//-------------------------------------------------------------------

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ExternalConnector.h"
#endif
#include "ExternalConnector.h"

//-------------------------------------------------------------------

#if !wxUSE_GUI
	#include <unistd.h>	// For getpass() and pause()
	// For readline
	#ifdef HAVE_LIBREADLINE
		#if defined(HAVE_READLINE_READLINE_H)
			#include <readline/readline.h>
		#elif defined(HAVE_READLINE_H)
			#include <readline.h>
		#else /* !defined(HAVE_READLINE_H) */
			extern char *readline ();
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
			extern void add_history ();
			extern int write_history ();
			extern int read_history ();
		#endif /* defined(HAVE_READLINE_HISTORY_H) */
	#else
		/* no history */
	#endif /* HAVE_READLINE_HISTORY */
#endif

//-------------------------------------------------------------------

#if wxUSE_GUI
	#include <wx/textdlg.h>	// For GetTextFromUser, GetPasswordFromUser
#endif

#include <wx/filefn.h>
#if wxCHECK_VERSION(2,4,2)
	#include <wx/config.h>	// For wxFileConfig in wx-2.4.2
#endif
#include <wx/fileconf.h>	// For wxFileConfig
#include <wx/intl.h>		// For _()
#include <wx/tokenzr.h>		// For wxStringTokenizer

//-------------------------------------------------------------------

#include "ECPacket.h"		// Needed for CECPacket, CECTag
#include "ECcodes.h"		// Needed for OPcodes and TAGnames

//-------------------------------------------------------------------

#include "MD5Sum.h"
#include "otherfunctions.h"

//-------------------------------------------------------------------
//
// Static data initialization -- memorize this, you'll need some day!
// 
//-------------------------------------------------------------------
const wxCmdLineEntryDesc CaMuleExternalConnector::cmdLineDesc[8] = 
{
	{ wxCMD_LINE_SWITCH, wxT("h"), wxT("help"),
		wxT("show this help"),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_HELP },
	{ wxCMD_LINE_OPTION, wxT("rh"), wxT("remote-host"),
		wxT("host where aMule is running (default localhost)"),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
	{ wxCMD_LINE_OPTION, wxT("p"), wxT("port"),
		wxT("aMule's port for External Connection"),
		wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL },
	{ wxCMD_LINE_OPTION, wxT("pw"), wxT("password"), 
		wxT("Password."),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
	{ wxCMD_LINE_SWITCH, wxT("f"), wxT("file-config"), 
		wxT("Read configuration (password/port) from file."),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
	{ wxCMD_LINE_SWITCH, wxT("q"), wxT("quiet"), 
		wxT("Do not print any output to stdout."),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
	{ wxCMD_LINE_SWITCH, wxT("v"), wxT("verbose"), 
		wxT("Be verbose - show also debug messages."),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
	{ wxCMD_LINE_NONE, wxEmptyString, wxEmptyString,
		wxEmptyString,
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL }
};

//-------------------------------------------------------------------

CaMuleExternalConnector::CaMuleExternalConnector()
{
	m_ECClient = NULL;
	m_isConnected = false;
	m_HasCommandLinePassword = false;
	m_HasConfigFromFile = false;
	m_KeepQuiet = false;
	m_InputLine = NULL;
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
		printf("%s", unicode2char(s));
#endif
	}
}

void CaMuleExternalConnector::Dump(const wxString &s)
{
	FILE *fp = fopen("x.txt", "a");
	fprintf(fp, "%s", unicode2char(s));
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

	if (cmd == wxEmptyString) {
		return 0;
	}
	
	m_cmdargs = wxEmptyString;
	while ( tokens.HasMoreTokens() ) {
		m_cmdargs += tokens.GetNextToken().MakeLower() + wxT(" ");
	}
	// Remove last space.
	m_cmdargs.Trim();

	register int i = 0;
	bool found = false;
	while ( !found && (commands[i].cmd != wxEmptyString) ) {
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
		const wxCharBuffer buf = unicode2charbuf(
			wxGetTextFromUser(prompt, _T("Enter Command")));
		const char *text = (const char *)buf;
#else
#if defined(HAVE_LIBREADLINE)
		if (m_InputLine) {
			free(m_InputLine);
			m_InputLine = (char *)NULL;
		}
		const wxCharBuffer buf = unicode2charbuf(prompt + wxT("$ "));
		const char *p = (const char *)buf;
		m_InputLine = readline(p);
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

wxString CaMuleExternalConnector::SendRecvMsg(const wxChar *msg)
{
	CECPacket request(EC_OP_COMPAT);
	request.AddTag(CECTag(EC_TAG_STRING, wxString(msg)));
	if (m_ECClient->WritePacket(&request)) {
		CECPacket *reply = m_ECClient->ReadPacket();
		if (reply != NULL) {
			// An EC_OP_COMPAT request implies the EC_OP_COMPAT reply.
			wxString s = reply->GetTagByIndex(0)->GetStringData();
			delete reply;
			return s;
		} else {
			delete reply;
			return wxEmptyString;
		}
	} else {
		return wxEmptyString;
	}
}

void CaMuleExternalConnector::ConnectAndRun(const wxString &ProgName, const wxString& ProgVersion, CmdId *UNUSED_IN_GUI(commands))
{
	wxString pass_plain;
	wxString pass_hash;

	// HostName, Port and Password
	if ( m_HasConfigFromFile ) {
		// Do nothing, just don't do the rest ;)
		// m_sHostName and m_sPort have already been set in OnCmdLineParsed()
	} else if ( m_HasCommandLinePassword ) {
		pass_plain = m_CommandLinePassword;
	} else {
#if wxUSE_GUI
		m_sHostName = wxGetTextFromUser(
			_T("Enter hostname or ip of the box running aMule"),
			_T("Enter Hostname"), wxT("localhost"));
		m_sPort = wxGetTextFromUser(
			_T("Enter port for aMule's External Connection"),
			_T("Enter Port"), wxT("4712"));
		pass_plain = ::wxGetPasswordFromUser(
			_T("Enter password for mule connection (OK if no pass defined)"), 
			_T("Enter Password"));
#else  // wxUse_GUI
		pass_plain = char2unicode(
			getpass("Enter password for mule connection (return if no pass defined): "));
#endif // wxUse_GUI
	}
	
	// Create the packet
	CECPacket packet(EC_OP_AUTH_REQ);
	packet.AddTag(CECTag(EC_TAG_CLIENT_NAME, ProgName));
	packet.AddTag(CECTag(EC_TAG_CLIENT_VERSION, ProgVersion));
	packet.AddTag(CECTag(EC_TAG_PROTOCOL_VERSION, (uint16)0x01f0));

	if ( m_HasConfigFromFile ) {
		wxFileConfig eMuleIni(
			wxT("eMule"),
			wxT("eMule-project"),
			wxT(".eMule")
		);
		m_sPort   = eMuleIni.Read(wxT("/ExternalConnect/ECPort"));
		pass_hash = eMuleIni.Read(wxT("/ExternalConnect/ECPassword"));
	} else if ( !pass_plain.IsEmpty() ) {
		pass_hash = MD5Sum(pass_plain).GetHash();
	}

	if (!pass_hash.IsEmpty()) {
		packet.AddTag(CECTag(EC_TAG_PASSWD_HASH, pass_hash));
	}

	// Clear passwords
	m_CommandLinePassword	= wxT("01234567890123456789");
	pass_plain		= wxT("01234567890123456789");
	pass_hash		= wxT("01234567890123456789");

	// Create the socket
	Show(_("\nCreating client...\n"));
	m_ECClient = new ECSocket();

	Show(_("Now, doing connection....\n"));
	wxIPV4address addr;
	addr.Hostname(m_sHostName);
	addr.Service(m_sPort);

	Show(	_("Using host '") + addr.Hostname() + 
		_("' port:") + wxString::Format(wxT("%d"), addr.Service()) + 
		wxT("\n") );
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
				Show(wxString(_("ExternalConn: Access denied because: ")) + wxGetTranslation(reason->GetStringData()) + wxT("\n"));
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
				Show(_("Succeeded! Connection established to aMule ") + reply->GetTagByName(EC_TAG_SERVER_VERSION)->GetStringData() + wxT("\n"));
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
					pause();
				}
			} else {
				TextShell(ProgName, commands);
			}
			Show(_("\nOk, exiting ") + ProgName + wxT("...\n"));
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
}

void CaMuleExternalConnector::OnInitCmdLine(wxCmdLineParser& parser)
{
	parser.SetDesc(cmdLineDesc);
}

bool CaMuleExternalConnector::OnCmdLineParsed(wxCmdLineParser& parser)
{

	bool result = true;

	// Call base class version to process standard command line options
	//result = wxAppConsole::OnCmdLineParsed(amuleweb_parser);

	if ( !parser.Found(wxT("rh"), &m_sHostName) ) {
		m_sHostName = wxT("localhost");
	}

	long port;
	if (!parser.Found(wxT("p"), &port)) {
		//get the default port
		m_sPort = wxT("4712"); 
	} else {
		m_sPort = wxString::Format(wxT("%li"), port);
	}

	m_HasCommandLinePassword = parser.Found(wxT("password"), &m_CommandLinePassword);
	m_HasConfigFromFile = parser.Found(wxT("file-config"));
	m_KeepQuiet = parser.Found(wxT("quiet"));
	m_Verbose = parser.Found(wxT("verbose"));

	return result;
}

