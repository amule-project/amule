//
// This file is part of the aMule Project.
//
// Copyright (C) 2005-2006aMule Team ( admin@amule.org / http://www.amule.org )
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


enum DebugType 
{
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
	//! Warnings/Errors related to the Universal Plug and Play subsistem.
	logUPnP,
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
	CDebugCategory( DebugType type, const wxString& name );

	
	/**
	 * Returns true if the category is enabled.
	 */
	bool IsEnabled() const;

	/**
	 * Enables/Disables the category.
	 */
	void SetEnabled( bool );
	

	/**
	 * Returns the user-readable name.
	 */
	const wxString& GetName() const;
	
	/**
	 * Returns the actual type.
	 */
	DebugType GetType() const;

private:
	//! The user-readable name.
	wxString	m_name;
	//! The actual type.
	DebugType	m_type;
	//! Whenever or not the category is enabled.
	bool		m_enabled;
};


/**
 * Namespace containing functions for logging operations.
 */
namespace CLogger
{
	/**
	 * Returns true if debug-messages should be generated for a specific category.
	 */
	bool 	IsEnabled( DebugType );
	
	/**
	 * Enables or disables debug-messages for a specific category.
	 */
	void		SetEnabled( DebugType type, bool enabled );

	
	/**
	 * Logs the specified line of text.
	 *
	 * @param critical If true, then the message will be made visible directly to the user.
	 * @param str The actual line of text.
	 *
	 * This function is thread-safe. If it is called by the main thread, the 
	 * event will be sent directly to the application, otherwise it will be
	 * queued in the event-loop.
	 */
	void		AddLogLine( bool critical, const wxString& str );

	/**
	 * Logs the specified line of text, prefixed with the name of the DebugType.
	 *
	 * @param critical If true, then the message will be made visible directly to the user.
	 * @param type The debug-category, the name of which will be prepended to the line.
	 * @param str The actual line of text.
	 *
	 * This function is thread-safe. If it is called by the main thread, the 
	 * event will be sent directly to the application, otherwise it will be
	 * queued in the event-loop.
	 */
	void		AddLogLine( bool critical, DebugType type, const wxString& str );


	/**
	 * Ensures that any pending entries are sent to the app.
	 *
	 * TODO: Ensure that entries are appended to the
	 *       logfile even when queued to avoid risk of
	 *       data loss.
	 */
	void		FlushPendingEntries();
	
	/**
	 * Returns a category specified by index.
	 */
	const CDebugCategory&	GetDebugCategory( int index );

	/**
	 * Returns the number of debug-categories.
	 */
	unsigned int 			GetDebugCategoryCount();
}


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
	void DoLogString(const wxChar *msg, time_t);
};


DECLARE_LOCAL_EVENT_TYPE(MULE_EVT_LOGLINE, -1)


/** This event is sent when a log-line is queued. */
class CLoggingEvent : public wxEvent
{
public:
	CLoggingEvent(bool critical, const wxString& msg)
		: wxEvent(-1, MULE_EVT_LOGLINE)
		, m_critical(critical)
		, m_msg(msg)
	{
	}
	
	const wxString& Message() const {
		return m_msg;
	}

	bool IsCritical() const {
		return m_critical;
	}

	wxEvent* Clone() const {
		return new CLoggingEvent(m_critical, m_msg);
	}
	
private:
	bool		m_critical;
	wxString	m_msg;
};


typedef void (wxEvtHandler::*MuleLogEventFunction)(CLoggingEvent&);

//! Event-handler for completed hashings of new shared files and partfiles.
#define EVT_MULE_LOGGING(func) \
	DECLARE_EVENT_TABLE_ENTRY(MULE_EVT_LOGLINE, -1, -1, \
	(wxObjectEventFunction) (wxEventFunction) \
	wxStaticCastEvent(MuleLogEventFunction, &func), (wxObject*) NULL),


/**
 * These macros should be used when logging. The 
 * AddLogLineM macro will simply call one of the
 * two CLogger::AddLogLine functions depending on
 * paramteres, but AddDebugLogLineM will only log
 * a message if the message is either critical or
 * the specified debug-type is enabled in the 
 * preferences.
 */
#if defined(MULEUNIT)
	#define AddDebugLogLineM(critical, type, string) do {} while (false)
	#define AddLogLineM(...) do {} while (false)
#else
	#ifdef __DEBUG__
		#define AddDebugLogLineM(critical, type, string) \
		do { \
			if (critical || CLogger::IsEnabled(type)) { \
				CLogger::AddLogLine(critical, type, string); \
			} \
		} while (false)
	#else
		#define AddDebugLogLineM(critical, type, string) \
		do { \
			if (critical) { \
				CLogger::AddLogLine(critical, type, string); \
			} \
		} while (false)
	#endif

	#define AddLogLineM(...) CLogger::AddLogLine(__VA_ARGS__)
#endif

#endif
// File_checked_for_headers
