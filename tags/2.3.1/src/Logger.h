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

#ifndef LOGGER_H
#define LOGGER_H

#include <wx/log.h>
#include <wx/event.h>
#include <iosfwd>


enum DebugType 
{
	//! Standard warning, not debug
	logStandard = -1,
	//! General warnings/errors.
	logGeneral = 0,
	//! Warnings/Errors for the main hashing thread.
	logHasher,
	//! Warnings/Errors for client-objects.
	logClient,
	//! Warnings/Errors for the local client protocol.
	logLocalClient,
	//! Warnings/Errors for the remote client protocol.
	logRemoteClient,
	//! Warnings/Errors when parsing packets.
	logPacketErrors,
	//! Warnings/Errors for the CFile class.
	logCFile,
	//! Warnings/Errors related to reading/writing files.
	logFileIO,
	//! Warnings/Errors when using the zLib library.
	logZLib,
	//! Warnings/Errors for the AICH-syncronization thread.
	logAICHThread,
	//! Warnings/Errors for transfering AICH hash-sets.
	logAICHTransfer,
	//! Warnings/Errors when recovering with AICH.
	logAICHRecovery,
	//! Warnings/Errors for the CListenSocket class.
	logListenSocket,
	//! Warnings/Errors for Client-Credits.
	logCredits,
	//! Warnings/Errors for the client UDP socket.
	logClientUDP,
	//! Warnings/Errors for the download-queue.
	logDownloadQueue,
	//! Warnings/Errors for the IP-Filter.
	logIPFilter,
	//! Warnings/Errors for known-files.
	logKnownFiles,
	//! Warnings/Errors for part-files.
	logPartFile,
	//! Warnings/Errors for SHA-hashset creation.
	logSHAHashSet,
	//! Warnings/Errors for servers, server connections.
	logServer,
	//! Warnings/Errors for proxy.
	logProxy,
	//! Warnings/Errors related to searching.
	logSearch,
	//! Warnings/Errors related to the server UDP socket.
	logServerUDP,
	//! Warning/Errors related to Kademlia UDP comunication on client
	logClientKadUDP,
	//! Warning/Errors related to Kademlia Search
	logKadSearch,
	//! Warning/Errors related to Kademlia Routing
	logKadRouting,
	//! Warning/Errors related to Kademlia Indexing
	logKadIndex,
	//! Warning/Errors related to Kademlia Main Thread
	logKadMain,
	//! Warning/Errors related to Kademlia Preferences
	logKadPrefs,
	//! Warnings/Errors related to partfile importer
	logPfConvert,
	//! Warnings/Errors related to the basic UDP socket-class.
	logMuleUDP,
	//! Warnings/Errors related to the thread-scheduler.
	logThreads,
	//! Warnings/Errors related to the Universal Plug and Play subsystem.
	logUPnP,
	//! Warnings/Errors related to the UDP Firewall Tester
	logKadUdpFwTester,
	//! Warnings/Errors related to Kad packet tracking.
	logKadPacketTracking,
	//! Warnings/Errors related to Kad entry tracking.
	logKadEntryTracking,
	//! Full log of external connection packets
	logEC,
	//! Warnings/Errors related to HTTP traffic
	logHTTP
	// IMPORTANT NOTE: when you add values to this enum, update the g_debugcats
	// array in Logger.cpp!
};



/**
 * Container-class for the debugging categories.
 */
class CDebugCategory
{
public:
	/**
	 * Constructor.
	 * 
	 * @param type The actual debug-category type.
	 * @param name The user-readable name.
	 */
	CDebugCategory( DebugType type, const wxString& name )
		: m_name(name), m_type(type), m_enabled(false)
	{}


	/**
	 * Returns true if the category is enabled.
	 */
	bool IsEnabled() const		{ return m_enabled; }

	/**
	 * Enables/Disables the category.
	 */
	void SetEnabled( bool enabled )	{ m_enabled = enabled; }


	/**
	 * Returns the user-readable name.
	 */
	const wxString& GetName() const	{ return m_name; }

	/**
	 * Returns the actual type.
	 */
	DebugType GetType() const	{ return m_type; }

private:
	//! The user-readable name.
	wxString	m_name;
	//! The actual type.
	DebugType	m_type;
	//! Whenever or not the category is enabled.
	bool		m_enabled;
};


/**
 * Functions for logging operations.
 */
class CLogger: public wxEvtHandler
{
public:
	/**
	 * Returns true if debug-messages should be generated for a specific category.
	 */
#ifdef __DEBUG__
	bool IsEnabled( DebugType ) const;
#else
	bool IsEnabled( DebugType ) const 	{ return false; }
#endif
	
	/**
	 * Enables or disables debug-messages for a specific category.
	 */
	void SetEnabled( DebugType type, bool enabled );

	/**
	 * Returns true if logging to stdout is enabled
	 */
	bool IsEnabledStdoutLog() const		{ return m_StdoutLog; }
	
	/**
	 * Enables or disables logging to stdout.
	 */
	void SetEnabledStdoutLog(bool enabled)	{ m_StdoutLog = enabled; }

	
	/**
	 * Logs the specified line of text, prefixed with the name of the DebugType.
	 * (except for logStandard)
	 *
	 * @param file
	 * @param line
	 * @param critical If true, then the message will be made visible directly to the user.
	 * @param type The debug-category, the name of which will be prepended to the line.
	 * @param str The actual line of text.
	 *
	 * This function is thread-safe. If it is called by the main thread, the 
	 * event will be sent directly to the application, otherwise it will be
	 * queued in the event-loop.
	 */
	void AddLogLine(
		const wxString &file,
		int line,
		bool critical,
		DebugType type,
		const wxString &str,
		bool toStdout = false,
		bool toGUI = true);

	// for UPnP
	void AddLogLine(
		const wxString &file,
		int line,
		bool critical,
		DebugType type,
		const std::ostringstream &msg);

	void AddLogLine(
		const wxString &file,
		int line,
		bool critical,
		const std::ostringstream &msg);


	/**
	 * Returns a category specified by index.
	 */
	const CDebugCategory&	GetDebugCategory( int index );

	/**
	 * Returns the number of debug-categories.
	 */
	unsigned int GetDebugCategoryCount();

	/**
	 * Open Logfile, true on success
	 */
	bool OpenLogfile(const wxString & name);

	/**
	 * Close Logfile
	 */
	void CloseLogfile();

	/**
	 * Get name of Logfile
	 */
	const wxString & GetLogfileName() const { 
		return m_LogfileName; 
	}

	/**
	 * Event handler
	 */
	void OnLoggingEvent(class CLoggingEvent& evt);

	/**
	 * Construct
	 */
	CLogger() {
		applog = NULL;
		m_StdoutLog = false;
		m_count = 0;
	}

private:
	class wxFFileOutputStream* applog; 	// the logfile
	wxString m_LogfileName;
	wxString m_ApplogBuf;
	bool m_StdoutLog;
	int  m_count;			// output line counter

	/**
	 * Write all waiting log info to the logfile
	 */
	void FlushApplog();

	/**
	 * Really output a single line
	 */
	void DoLine(const wxString & line, bool toStdout, bool toGUI);

	DECLARE_EVENT_TABLE()
};

extern CLogger theLogger;

/**
 * This class forwards log-lines from wxWidgets to CLogger.
 */
class CLoggerTarget : public wxLog
{
public:
	CLoggerTarget();
	
	/**
	 * @see wxLog::DoLogString
	 */
#if wxCHECK_VERSION(2, 9, 0)
	void DoLogText(const wxString &msg);
#else
	void DoLogString(const wxChar *msg, time_t);
#endif
};


DECLARE_LOCAL_EVENT_TYPE(MULE_EVT_LOGLINE, -1)


/** This event is sent when a log-line is queued. */
class CLoggingEvent : public wxEvent
{
public:
	CLoggingEvent(bool critical, bool toStdout, bool toGUI, const wxString& msg)
		: wxEvent(-1, MULE_EVT_LOGLINE)
		, m_critical(critical)
		, m_stdout(toStdout)
		, m_GUI(toGUI)
		// Deep copy, to avoid thread-unsafe reference counting. */
		, m_msg(msg.c_str(), msg.Length())
	{
	}
	
	const wxString& Message() const {
		return m_msg;
	}

	bool IsCritical() const {
		return m_critical;
	}

	bool ToStdout() const {
		return m_stdout;
	}

	bool ToGUI() const {
		return m_GUI;
	}

	wxEvent* Clone() const {
		return new CLoggingEvent(m_critical, m_stdout, m_GUI, m_msg);
	}
	
private:
	bool		m_critical;
	bool		m_stdout;
	bool		m_GUI;
	wxString	m_msg;
};


typedef void (wxEvtHandler::*MuleLogEventFunction)(CLoggingEvent&);

//! Event-handler for completed hashings of new shared files and partfiles.
#define EVT_MULE_LOGGING(func) \
	DECLARE_EVENT_TABLE_ENTRY(MULE_EVT_LOGLINE, -1, -1, \
	(wxObjectEventFunction) (wxEventFunction) \
	wxStaticCastEvent(MuleLogEventFunction, &func), (wxObject*) NULL),


// access the logfile for EC
class CLoggerAccess
{
private:
	class wxFFileInputStream * m_logfile;
	class wxCharBuffer * m_buffer;
	size_t m_bufferlen;
	size_t m_pos;

	bool m_ready;
public:
	//
	// construct/destruct
	//
	CLoggerAccess();
	~CLoggerAccess();
	//
	// Reset (used when logfile is cleared)
	//
	void Reset();
	//
	// get a String (if there is one)
	//
	bool GetString(wxString & s);
	//
	// is a String available ?
	//
	bool HasString();
};


/**
 * These macros should be used when logging. The 
 * AddLogLineM macro will simply call one of the
 * two CLogger::AddLogLine functions depending on
 * parameters, but AddDebugLogLine* will only log
 * a message if the message is either critical or
 * the specified debug-type is enabled in the 
 * preferences.
 * AddLogLineMS will also always print to stdout.
 */
#ifdef MULEUNIT
	#define AddDebugLogLineN(...) do {} while (false)
	#define AddLogLineN(...) do {} while (false)
	#define AddLogLineNS(...) do {} while (false)
	#define AddDebugLogLineC(...) do {} while (false)
	#define AddLogLineC(...) do {} while (false)
	#define AddLogLineCS(...) do {} while (false)
#else
// Macro for UPnP. This is not a debug macro, but wants its category printed nevertheless (sigh).
	#define AddLogLineU(critical, type, string) theLogger.AddLogLine(__TFILE__, __LINE__, critical, type, string)
// Macros for 'N'on critical logging
	#ifdef __DEBUG__
		#define AddDebugLogLineN(type, string) theLogger.AddLogLine(__TFILE__, __LINE__, false, type, string)
	#else
		#define AddDebugLogLineN(type, string)	do {} while (false)
	#endif
	#define AddLogLineN(string) theLogger.AddLogLine(__TFILE__, __LINE__, false, logStandard, string)
	#define AddLogLineNS(string) theLogger.AddLogLine(__TFILE__, __LINE__, false, logStandard, string, true)
// Macros for 'C'ritical logging
	#define AddDebugLogLineC(type, string) theLogger.AddLogLine(__TFILE__, __LINE__, true, type, string)
	#define AddLogLineC(string) theLogger.AddLogLine(__TFILE__, __LINE__, true, logStandard, string)
	#define AddLogLineCS(string) theLogger.AddLogLine(__TFILE__, __LINE__, true, logStandard, string, true)
// Macros for logging to logfile only
	#define AddDebugLogLineF(type, string) theLogger.AddLogLine(__TFILE__, __LINE__, false, type, string, false, false)
	#define AddLogLineF(string) theLogger.AddLogLine(__TFILE__, __LINE__, false, logStandard, string, false, false)
#endif

#endif
// File_checked_for_headers
