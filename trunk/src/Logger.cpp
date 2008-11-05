//
// This file is part of the aMule Project.
//
// Copyright (c) 2005-2008 aMule Team ( admin@amule.org / http://www.amule.org )
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
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/sstream.h>
#include <wx/filename.h>
#include <sstream>


DEFINE_LOCAL_EVENT_TYPE(MULE_EVT_LOGLINE)


CDebugCategory::CDebugCategory( DebugType type, const wxString& name )
	: m_name( name ),
	  m_type( type )
{
	m_enabled = false;
}


bool CDebugCategory::IsEnabled() const
{
	return m_enabled;
}


void CDebugCategory::SetEnabled( bool enabled )
{
	m_enabled = enabled;
}


const wxString& CDebugCategory::GetName() const
{
	return m_name;
}


DebugType CDebugCategory::GetType() const
{
	return m_type;
}

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
	CDebugCategory( logKadEntryTracking,	wxT("Kademlia Entry Tracking") )
};


const int categoryCount = sizeof( g_debugcats ) / sizeof( g_debugcats[0] );



bool CLogger::IsEnabled( DebugType type )
{
#ifdef __DEBUG__
	int index = (int)type;
	
	if ( index >= 0 && index < categoryCount ) {
		const CDebugCategory& cat = g_debugcats[ index ];
		wxASSERT( type == cat.GetType() );

		return ( cat.IsEnabled() && thePrefs::GetVerbose() );
	} 

	wxASSERT( false );
#endif
	return false;
}


void CLogger::SetEnabled( DebugType type, bool enabled ) 
{
	int index = (int)type;
	
	if ( index >= 0 && index < categoryCount ) {
		CDebugCategory& cat = g_debugcats[ index ];
		wxASSERT( type == cat.GetType() );

		cat.SetEnabled( enabled );
	} else {
		wxASSERT( false );
	}
}


bool CLogger::IsEnabledStdoutLog()
{
	return m_StdoutLog;
}


void CLogger::SetEnabledStdoutLog(bool enabled)
{
	m_StdoutLog = enabled;
}


void CLogger::AddLogLine(
	const wxString &file,
	int line,
	bool critical,
	DebugType type,
	const wxString &str,
	bool toStdout)
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
			wxASSERT( false );
		}
	}

#ifdef __DEBUG__
	msg = file.AfterLast(wxFileName::GetPathSeparator()).AfterLast(wxT('/')) << wxT("(") << line << wxT("): ") + msg;
#endif

	CLoggingEvent Event(critical, toStdout, msg);

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
	AddLogLine(file, line, critical, type, char2unicode(msg.str().c_str()));
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
	} else {
		CloseLogfile();
	}
	return ret; 
}


void CLogger::CloseLogfile()
{
	delete applog;
	applog = NULL;
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

	// critical lines get a ! prepended, others a blank
	wxString prefix = evt.IsCritical() ? wxT("!") : wxT(" ");

	if ( bufferline.IsEmpty() ) {
		// If it's empty we just write a blank line with no timestamp.
		DoLine(wxT(" \n"), evt.ToStdout());
	} else {
		// Split multi-line messages into individual lines
		wxStringTokenizer tokens( bufferline, wxT("\n") );		
		while ( tokens.HasMoreTokens() ) {
			wxString fullline = prefix + stamp + tokens.GetNextToken() + wxT("\n");
			DoLine(fullline, evt.ToStdout());
		}
	}
}


void CLogger::DoLine(const wxString & line, bool toStdout)
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
	theApp->AddGuiLogLine(line);
#endif
}


void CLogger::FlushApplog()
{
	if (applog) { // Wait with output until logfile is actually opened
		wxStringInputStream stream(m_ApplogBuf);
		(*applog) << stream;
		applog->Sync();
		m_ApplogBuf = wxEmptyString;
	}
}

CLogger theLogger;

BEGIN_EVENT_TABLE(CLogger, wxEvtHandler)
	EVT_MULE_LOGGING(CLogger::OnLoggingEvent)
END_EVENT_TABLE()


CLoggerTarget::CLoggerTarget()
{
}


void CLoggerTarget::DoLogString(const wxChar* msg, time_t)
{
	wxCHECK_RET(msg, wxT("Log message is NULL in DoLogString!"));
	
	wxString str(msg);
	
	// This is much simpler than manually handling all wx log-types.
	bool critical = str.StartsWith(_("ERROR: ")) || str.StartsWith(_("WARNING: "));

	AddLogLineM(critical, str);
}

// File_checked_for_headers
