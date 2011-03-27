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
	CDebugCategory( logGeneral,		wxT("General") ),
	CDebugCategory( logHasher,		wxT("Hasher") ),
	CDebugCategory( logClient,		wxT("ED2k Client") ),
	CDebugCategory( logLocalClient,		wxT("Local Client Protocol") ),
	CDebugCategory( logRemoteClient,	wxT("Remote Client Protocol") ),
	CDebugCategory( logPacketErrors,	wxT("Packet Parsing Errors") ),
	CDebugCategory( logCFile,		wxT("CFile") ),
	CDebugCategory( logFileIO,		wxT("FileIO") ),
	CDebugCategory( logZLib,		wxT("ZLib") ),
	CDebugCategory( logAICHThread,		wxT("AICH-Hasher") ),
	CDebugCategory( logAICHTransfer,	wxT("AICH-Transfer") ),
	CDebugCategory( logAICHRecovery,	wxT("AICH-Recovery") ),
	CDebugCategory( logListenSocket,	wxT("ListenSocket") ),
	CDebugCategory( logCredits,		wxT("Credits") ),
	CDebugCategory( logClientUDP,		wxT("ClientUDPSocket") ),
	CDebugCategory( logDownloadQueue,	wxT("DownloadQueue") ),
	CDebugCategory( logIPFilter,		wxT("IPFilter") ),
	CDebugCategory( logKnownFiles,		wxT("KnownFileList") ),
	CDebugCategory( logPartFile,		wxT("PartFiles") ),
	CDebugCategory( logSHAHashSet,		wxT("SHAHashSet") ),
	CDebugCategory( logServer,		wxT("Servers") ),
	CDebugCategory( logProxy,		wxT("Proxy") ),
	CDebugCategory( logSearch,		wxT("Searching") ),
	CDebugCategory( logServerUDP,		wxT("ServerUDP") ),
	CDebugCategory( logClientKadUDP,	wxT("Client Kademlia UDP") ),
	CDebugCategory( logKadSearch,		wxT("Kademlia Search") ),
	CDebugCategory( logKadRouting,		wxT("Kademlia Routing") ),
	CDebugCategory( logKadIndex,		wxT("Kademlia Indexing") ),
	CDebugCategory( logKadMain,		wxT("Kademlia Main Thread") ),
	CDebugCategory( logKadPrefs,		wxT("Kademlia Preferences") ),
	CDebugCategory( logPfConvert,		wxT("PartFileConvert") ),
	CDebugCategory( logMuleUDP,		wxT("MuleUDPSocket" ) ),
	CDebugCategory( logThreads,		wxT("ThreadScheduler" ) ),
	CDebugCategory( logUPnP,		wxT("Universal Plug and Play" ) ),
	CDebugCategory( logKadUdpFwTester,	wxT("Kademlia UDP Firewall Tester") ),
	CDebugCategory( logKadPacketTracking,	wxT("Kademlia Packet Tracking") ),
	CDebugCategory( logKadEntryTracking,	wxT("Kademlia Entry Tracking") ),
	CDebugCategory( logEC,			wxT("External Connect") ),
	CDebugCategory( logHTTP,		wxT("HTTP") )
};


const int categoryCount = sizeof( g_debugcats ) / sizeof( g_debugcats[0] );



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
		int index = (int)type;
		
		if ( index >= 0 && index < categoryCount ) {
			const CDebugCategory& cat = g_debugcats[ index ];
			wxASSERT(type == cat.GetType());

			msg = cat.GetName() + wxT(": ") + msg;
		} else {
			wxFAIL;
		}
	}

#ifdef __DEBUG__
	if (line) {
		msg = file.AfterLast(wxFileName::GetPathSeparator()).AfterLast(wxT('/')) << wxT("(") << line << wxT("): ") + msg;
	}
#endif

	CLoggingEvent Event(critical, toStdout, toGUI, msg);

	// Try to handle events immediatly when possible (to save to file).
	if (wxThread::IsMain()) {
		// main thread and log file available: process directly
		ProcessEvent(Event);			
	} else {
		// otherwise put to background
		AddPendingEvent(Event);
	}
}


void CLogger::AddLogLine(
	const wxString &file,
	int line,
	bool critical,
	DebugType type,
	const std::ostringstream &msg)
{
	int index = (int)type;
	
	if ( index >= 0 && index < categoryCount ) {
		const CDebugCategory& cat = g_debugcats[ index ];
		wxASSERT(type == cat.GetType());

		AddLogLine(file, line, critical, logStandard, 
			cat.GetName() + wxT(": ") + char2unicode(msg.str().c_str()));
	}
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
	// Remove newspace at end
	wxString bufferline = evt.Message().Strip(wxString::trailing);

	// Create the timestamp
	wxString stamp = wxDateTime::Now().FormatISODate() + wxT(" ") + wxDateTime::Now().FormatISOTime()
#ifdef CLIENT_GUI
 					+ wxT(" (remote-GUI): ");
#else
 					+ wxT(": ");
#endif

	// critical lines get a ! prepended, ordinary lines a blank
	// logfile-only lines get a . to prevent transmission on EC
	wxString prefix = !evt.ToGUI() ? wxT(".") : (evt.IsCritical() ? wxT("!") : wxT(" "));

	if ( bufferline.IsEmpty() ) {
		// If it's empty we just write a blank line with no timestamp.
		DoLine(wxT(" \n"), evt.ToStdout(), evt.ToGUI());
	} else {
		// Split multi-line messages into individual lines
		wxStringTokenizer tokens( bufferline, wxT("\n") );		
		while ( tokens.HasMoreTokens() ) {
			wxString fullline = prefix + stamp + tokens.GetNextToken() + wxT("\n");
			DoLine(fullline, evt.ToStdout(), evt.ToGUI());
		}
	}
}


void CLogger::DoLine(const wxString & line, bool toStdout, bool GUI_ONLY(toGUI))
{
	++m_count;

	// write to logfile
	m_ApplogBuf += line;
	FlushApplog();

	// write to Stdout
	if (m_StdoutLog || toStdout) {
		printf("%s", (const char*)unicode2char(line));
	}
#ifndef AMULE_DAEMON
	// write to Listcontrol
	if (toGUI) {
		theApp->AddGuiLogLine(line);
	}
#endif
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

#if wxCHECK_VERSION(2, 9, 0)
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
#else
void CLoggerTarget::DoLogString(const wxChar* msg, time_t)
{
	// prevent infinite recursion
	static bool recursion = false;
	if (recursion) {
		return;
	}
	recursion = true;

	wxCHECK_RET(msg, wxT("Log message is NULL in DoLogString!"));
	
	wxString str(msg);
	
	// This is much simpler than manually handling all wx log-types.
	if (str.StartsWith(_("ERROR: ")) || str.StartsWith(_("WARNING: "))) {
		AddLogLineC(str);
	} else {
		AddLogLineN(str);
	}

	recursion = false;
}
#endif

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
bool ECLogIsEnabled()
{
	return theLogger.IsEnabled(logEC);
}

void DoECLogLine(const wxString &line)
{
	// without file/line
	theLogger.AddLogLine(wxEmptyString, 0, false, logStandard, line, false, false);
}

// File_checked_for_headers
