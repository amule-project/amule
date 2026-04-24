//
// This file is part of the aMule Project.
//
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "Logger.h"
#include "amule.h"
#include "Preferences.h"
#include <common/Macros.h>
#include <common/MacrosProgramSpecific.h>
#include <sstream>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/sstream.h>
#include <wx/filename.h>


DEFINE_LOCAL_EVENT_TYPE(MULE_EVT_LOGLINE)


CDebugCategory g_debugcats[] = {
	CDebugCategory( logGeneral,		"General" ),
	CDebugCategory( logHasher,		"Hasher" ),
	CDebugCategory( logClient,		"ED2k Client" ),
	CDebugCategory( logLocalClient,		"Local Client Protocol" ),
	CDebugCategory( logRemoteClient,	"Remote Client Protocol" ),
	CDebugCategory( logPacketErrors,	"Packet Parsing Errors" ),
	CDebugCategory( logCFile,		"CFile" ),
	CDebugCategory( logFileIO,		"FileIO" ),
	CDebugCategory( logZLib,		"ZLib" ),
	CDebugCategory( logAICHThread,		"AICH-Hasher" ),
	CDebugCategory( logAICHTransfer,	"AICH-Transfer" ),
	CDebugCategory( logAICHRecovery,	"AICH-Recovery" ),
	CDebugCategory( logListenSocket,	"ListenSocket" ),
	CDebugCategory( logCredits,		"Credits" ),
	CDebugCategory( logClientUDP,		"ClientUDPSocket" ),
	CDebugCategory( logDownloadQueue,	"DownloadQueue" ),
	CDebugCategory( logIPFilter,		"IPFilter" ),
	CDebugCategory( logKnownFiles,		"KnownFileList" ),
	CDebugCategory( logPartFile,		"PartFiles" ),
	CDebugCategory( logSHAHashSet,		"SHAHashSet" ),
	CDebugCategory( logServer,		"Servers" ),
	CDebugCategory( logProxy,		"Proxy" ),
	CDebugCategory( logSearch,		"Searching" ),
	CDebugCategory( logServerUDP,		"ServerUDP" ),
	CDebugCategory( logClientKadUDP,	"Client Kademlia UDP" ),
	CDebugCategory( logKadSearch,		"Kademlia Search" ),
	CDebugCategory( logKadRouting,		"Kademlia Routing" ),
	CDebugCategory( logKadIndex,		"Kademlia Indexing" ),
	CDebugCategory( logKadMain,		"Kademlia Main Thread" ),
	CDebugCategory( logKadPrefs,		"Kademlia Preferences" ),
	CDebugCategory( logPfConvert,		"PartFileConvert" ),
	CDebugCategory( logMuleUDP,		"MuleUDPSocket" ),
	CDebugCategory( logThreads,		"ThreadScheduler" ),
	CDebugCategory( logUPnP,		"Universal Plug and Play" ),
	CDebugCategory( logKadUdpFwTester,	"Kademlia UDP Firewall Tester" ),
	CDebugCategory( logKadPacketTracking,	"Kademlia Packet Tracking" ),
	CDebugCategory( logKadEntryTracking,	"Kademlia Entry Tracking" ),
	CDebugCategory( logEC,			"External Connect" ),
	CDebugCategory( logHTTP,		"HTTP" ),
	CDebugCategory( logAsio,		"Asio Sockets" )
};


const int categoryCount = itemsof(g_debugcats);



#ifdef __DEBUG__
bool CLogger::IsEnabled( DebugType type ) const
{
	int index = (int)type;

	if ( index >= 0 && index < categoryCount ) {
		const CDebugCategory& cat = g_debugcats[ index ];
		wxASSERT( type == cat.GetType() );

		return ( cat.IsEnabled() && thePrefs::GetVerbose() );
	}

	wxFAIL;
	return false;
}
#endif


void CLogger::SetEnabled( DebugType type, bool enabled )
{
	int index = (int)type;

	if ( index >= 0 && index < categoryCount ) {
		CDebugCategory& cat = g_debugcats[ index ];
		wxASSERT( type == cat.GetType() );

		cat.SetEnabled( enabled );
	} else {
		wxFAIL;
	}
}


void CLogger::AddLogLine(
	const wxString& DEBUG_ONLY(file),
	int DEBUG_ONLY(line),
	bool critical,
	DebugType type,
	const wxString &str,
	bool toStdout,
	bool toGUI)
{
	wxString msg(str);
// handle Debug messages
	if (type != logStandard) {
		if (!critical && !IsEnabled(type)) {
			return;
		}
		if (!critical && thePrefs::GetVerboseLogfile()) {
			// print non critical debug messages only to the logfile
			toGUI = false;
		}
		int index = (int)type;

		if ( index >= 0 && index < categoryCount ) {
			const CDebugCategory& cat = g_debugcats[ index ];
			wxASSERT(type == cat.GetType());

			msg = cat.GetName() + ": " + msg;
		} else {
			wxFAIL;
		}
	}

#ifdef __DEBUG__
	if (line) {
		msg = file.AfterLast(wxFileName::GetPathSeparator()).AfterLast('/') << "(" << line << "): " + msg;
	}
#endif

	if (toGUI && !wxThread::IsMain()) {
		// put to background
		CLoggingEvent Event(critical, toStdout, toGUI, msg);
		AddPendingEvent(Event);
	} else {
		// Try to handle events immediately when possible (to save to file).
		DoLines(msg, critical, toStdout, toGUI);
	}
}


void CLogger::AddLogLine(
	const wxString &file,
	int line,
	bool critical,
	DebugType type,
	const std::ostringstream &msg)
{
	AddLogLine(file, line, critical, type, wxString(char2unicode(msg.str().c_str())));
}


const CDebugCategory& CLogger::GetDebugCategory( int index )
{
	wxASSERT( index >= 0 && index < categoryCount );

	return g_debugcats[ index ];
}


unsigned int CLogger::GetDebugCategoryCount()
{
	return categoryCount;
}


bool CLogger::OpenLogfile(const wxString & name)
{
	applog = new wxFFileOutputStream(name);
	bool ret = applog->Ok();
	if (ret) {
		FlushApplog();
		m_LogfileName = name;
	} else {
		CloseLogfile();
	}
	return ret;
}


void CLogger::CloseLogfile()
{
	delete applog;
	applog = NULL;
	m_LogfileName.Clear();
}


void CLogger::OnLoggingEvent(class CLoggingEvent& evt)
{
	DoLines(evt.Message(), evt.IsCritical(), evt.ToStdout(), evt.ToGUI());
}


void CLogger::DoLines(const wxString & lines, bool critical, bool toStdout, bool toGUI)
{
	// Remove newspace at end
	wxString bufferline = lines.Strip(wxString::trailing);

	// Create the timestamp
	wxString stamp = wxDateTime::Now().FormatISODate() + " " + wxDateTime::Now().FormatISOTime()
#ifdef CLIENT_GUI
					+ " (remote-GUI): ";
#else
					+ ": ";
#endif

	// critical lines get a ! prepended, ordinary lines a blank
	// logfile-only lines get a . to prevent transmission on EC
	wxString prefix = !toGUI ? "." : (critical ? "!" : " ");

	if ( bufferline.IsEmpty() ) {
		// If it's empty we just write a blank line with no timestamp.
		DoLine(" \n", toStdout, toGUI);
	} else {
		// Split multi-line messages into individual lines
		wxStringTokenizer tokens( bufferline, "\n" );
		while ( tokens.HasMoreTokens() ) {
			wxString fullline = prefix + stamp + tokens.GetNextToken() + "\n";
			DoLine(fullline, toStdout, toGUI);
		}
	}
}


void CLogger::DoLine(const wxString & line, bool toStdout, bool GUI_ONLY(toGUI))
{
	{
		wxMutexLocker lock(m_lineLock);
		++m_count;

		// write to logfile
		m_ApplogBuf += line;
		FlushApplog();

		// write to Stdout
		if (m_StdoutLog || toStdout) {
			printf("%s", (const char*)unicode2char(line));
		}
	}
#ifndef AMULE_DAEMON
	// write to Listcontrol
	if (toGUI) {
		theApp->AddGuiLogLine(line);
	}
#endif
}


void CLogger::EmergencyLog(const wxString &message, bool closeLog)
{
	fprintf(stderr, "%s", (const char*)unicode2char(message));
	m_ApplogBuf += message;
	FlushApplog();
	if (closeLog && applog) {
		applog->Close();
		applog = NULL;
	}
}


void CLogger::FlushApplog()
{
	if (applog) { // Wait with output until logfile is actually opened
		wxStringInputStream stream(m_ApplogBuf);
		(*applog) << stream;
		applog->Sync();
		m_ApplogBuf.Clear();
	}
}

CLogger theLogger;

BEGIN_EVENT_TABLE(CLogger, wxEvtHandler)
	EVT_MULE_LOGGING(CLogger::OnLoggingEvent)
END_EVENT_TABLE()


CLoggerTarget::CLoggerTarget()
{
}

void CLoggerTarget::DoLogText(const wxString &msg)
{
	// prevent infinite recursion
	static bool recursion = false;
	if (recursion) {
		return;
	}
	recursion = true;

	// This is much simpler than manually handling all wx log-types.
	if (msg.StartsWith(_("ERROR: ")) || msg.StartsWith(_("WARNING: "))) {
		AddLogLineC(msg);
	} else {
		AddLogLineN(msg);
	}

	recursion = false;
}

CLoggerAccess::CLoggerAccess()
{
	m_bufferlen = 4096;
	m_buffer = new wxCharBuffer(m_bufferlen);
	m_logfile = NULL;
	Reset();
}


void CLoggerAccess::Reset()
{
	delete m_logfile;
	m_logfile = new wxFFileInputStream(theLogger.GetLogfileName());
	m_pos = 0;
	m_ready = false;
}


CLoggerAccess::~CLoggerAccess()
{
	delete m_buffer;
	delete m_logfile;
}


//
// read a line of text from the logfile if available
// (can't believe there's no library function for this >:( )
//
bool CLoggerAccess::HasString()
{
	while (!m_ready) {
		int c = m_logfile->GetC();
		if (c == wxEOF) {
			break;
		}
		// check for buffer overrun
		if (m_pos == m_bufferlen) {
			m_bufferlen += 1024;
			m_buffer->extend(m_bufferlen);
		}
		m_buffer->data()[m_pos++] = c;
		if (c == '\n') {
			if (m_buffer->data()[0] == '.') {
				// Log-only line, skip
				m_pos = 0;
			} else {
				m_ready = true;
			}
		}
	}
	return m_ready;
}


bool CLoggerAccess::GetString(wxString & s)
{
	if (!HasString()) {
		return false;
	}
	s = wxString(m_buffer->data(), wxConvUTF8, m_pos);
	m_pos = 0;
	m_ready = false;
	return true;
}

// Functions for EC logging

#ifdef __DEBUG__
#include "ec/cpp/ECLog.h"

bool ECLogIsEnabled()
{
	return theLogger.IsEnabled(logEC);
}

void DoECLogLine(const wxString &line)
{
	// without file/line
	theLogger.AddLogLine("", 0, false, logStandard, line, false, false);
}

#endif /* __DEBUG__ */
// File_checked_for_headers
