//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project (http://www.amule.org)
// Copyright (C) 2002 Merkur (merkur-@users.sourceforge.net / http://www.emule-project.net)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 

#ifndef AMULE_H
#define AMULE_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/app.h>			// Needed for wxApp
#include <wx/intl.h>		// Needed for wxLocale

#include "types.h"			// Needed for int32, uint16 and uint64
#include "GuiEvents.h"

#include <deque>

// If wx version is less than 2.5.2, we need this defined. This new flag 
// is needed to ensure the old behaviour of sizers.
#if !wxCHECK_VERSION(2,5,2)
	#define wxFIXED_MINSIZE 0
#endif

class CAbstractFile;
class ExternalConn;
class CamuleDlg;
class CPreferences;
class CDownloadQueue;
class CUploadQueue;
class CServerConnect;
class CSharedFileList;
class CServer;
class CFriend;
class CMD4Hash;
class CServerList;
class CListenSocket;
class CClientList;
class CKnownFileList;
class CSearchList;
class CClientCreditsList;
class CClientUDPSocket;
class CIPFilter;
class wxServer;
class wxString;
class wxSocketEvent;
class wxTimer;
class wxTimerEvent;
class wxCommandEvent;

#ifdef __DEBUG__
	typedef struct {
		uint32 socket_n;
		uint32 creation_time;
		wxString backtrace;
	} socket_deletion_log_item;
#endif

#define theApp wxGetApp()

enum APPState {
	APP_STATE_RUNNING = 0,
	APP_STATE_SHUTINGDOWN,
	APP_STATE_DONE,
	APP_STATE_STARTING
};	


#include <wx/event.h>

DECLARE_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_FINISHED, wxEVT_USER_FIRST+FILE_HASHING_FINISHED)
DECLARE_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_SHUTDOWN, wxEVT_USER_FIRST+FILE_HASHING_SHUTDOWN)
DECLARE_EVENT_TYPE(wxEVT_CORE_FINISHED_FILE_COMPLETION, wxEVT_USER_FIRST+FILE_COMPLETION_FINISHED)

DECLARE_EVENT_TYPE(wxEVT_CORE_SOURCE_DNS_DONE, wxEVT_USER_FIRST+SOURCE_DNS_DONE)
DECLARE_EVENT_TYPE(wxEVT_CORE_DNS_DONE, wxEVT_USER_FIRST+DNS_DONE)

DECLARE_EVENT_TYPE(wxEVT_AMULE_TIMER, wxEVT_USER_FIRST+EVENT_TIMER)

class wxMuleInternalEvent : public wxEvent {
	void *m_ptr;
	long m_value;
	int  m_commandInt;
	public:
	wxMuleInternalEvent(int id, int event_id) : wxEvent(event_id, id)
	{
	}
	wxMuleInternalEvent(int id) : wxEvent(-1, id)
	{
	}
	wxMuleInternalEvent(int id, void *ptr, long value) : wxEvent(-1, id)
	{
		m_ptr = ptr;
		m_value = value;
	}
	wxEvent *Clone(void) const
	{
		return new wxMuleInternalEvent(*this);
	}
	void SetExtraLong(long value)
	{
		m_value = value;
	}
	long GetExtraLong()
	{
		return m_value;
	}
	void SetInt(int i)
	{
		m_commandInt = i;
	}
	long GetInt() const
	{
		return m_commandInt; 
	}

	void SetClientData(void *ptr)
	{
		m_ptr = ptr;
	}
	void *GetClientData()
	{
		return m_ptr;
	}
};


#ifdef AMULE_DAEMON
#define AMULE_APP_BASE wxAppConsole
#else
#define AMULE_APP_BASE wxApp
#endif

class CamuleApp : public AMULE_APP_BASE
{
public:
			CamuleApp() {};
	virtual		~CamuleApp() {};
#ifdef AMULE_DAEMON
	virtual int	OnRun();
#endif

	virtual bool	OnInit();
	int		OnExit();
	void		OnFatalException();

	// Barry - To find out if app is running or shutting/shut down
	bool IsRunning() const { return (m_app_state == APP_STATE_RUNNING); }

	// ed2k URL functions
	wxString	StripInvalidFilenameChars(const wxString& strText, bool bKeepSpaces = true);
	wxString	CreateED2kLink(const CAbstractFile* f);
	wxString	CreateHTMLED2kLink(const CAbstractFile* f);
	wxString	CreateED2kSourceLink(const CAbstractFile* f);
	wxString	CreateED2kHostnameSourceLink(const CAbstractFile* f);
	wxString	GenFakeCheckUrl(const CAbstractFile *f);
	wxString        GenFakeCheckUrl2(const CAbstractFile *f);
	
	void RunAICHThread();
	
	void		QueueLogLine(bool addtostatusbar, const wxString& line);
	void		FlushQueuedLogLines();
		
	// Misc functions
	bool		CopyTextToClipboard( wxString strText );
	void		OnlineSig(bool zero = false); 
	void		Localize_mule();
	void		Trigger_New_version(wxString new_version);
	
	// Used to detect a previous running instance of aMule
	wxServer*	localserver;
	
	// shakraw - new EC code using wxSocketBase
	ExternalConn*	ECServerHandler;

	// Kry - avoid chmod on win32
	bool		use_chmod;
	bool		IsReady;
	
	uint32	GetPublicIP() const;	// return current (valid) public IP or 0 if unknown
	void		SetPublicIP(const uint32 dwIP);

	// Statistic functions. I plan on moving these to a class of their own -- Xaignar
	void		UpdateReceivedBytes(int32 bytesToAdd);
	uint64		GetUptimeMsecs();
	uint32		GetUptimeSecs();
	uint32		GetTransferSecs();
	uint32		GetServerSecs();
	void		UpdateSentBytes(int32 bytesToAdd);

	// Statistic variables. I plan on moving these to a class of their own -- Xaignar
	uint64		Start_time;
	double		sTransferDelay;
	uint64		stat_sessionReceivedBytes;
	uint64		stat_sessionSentBytes;
	uint32		stat_reconnects;
	uint64		stat_transferStarttime;
	uint64		stat_serverConnectTime;
	uint32		stat_filteredclients;


	// Other parts of the interface and such
	CamuleDlg*		amuledlg;
	CPreferences*		glob_prefs;
	CDownloadQueue*		downloadqueue;
	CUploadQueue*		uploadqueue;
	CServerConnect*		serverconnect;
	CSharedFileList*	sharedfiles;
	CServerList*		serverlist;
	CListenSocket*		listensocket;
	CClientList*		clientlist;
	CKnownFileList*		knownfiles;
	CSearchList*		searchlist;
	CClientCreditsList*	clientcredits;
	CClientUDPSocket*	clientudp;
	CIPFilter*		ipfilter;

	// Frame title
	wxString	m_FrameTitle;
	
	void ShutDown();
	
	void NotifyEvent(GUIEvent event);

	wxString GetLog(bool reset = false);
	wxString GetServerLog(bool reset = false);
	wxString GetDebugLog(bool reset = false);
	
	bool AddServer(CServer *srv);
	CFriend *FindFriend(CMD4Hash *hash, uint32 ip, uint16 port);
	void AddServerMessageLine(wxString &msg);
#ifdef __DEBUG__
	void AddSocketDeleteDebug(uint32 socket_pointer, uint32 creation_time);
#endif
	void SetOSFiles(const wxString new_path); 
	
	wxString ConfigDir;
	
	wxMutex data_mutex;
	wxMutex hashing_mut;

protected:
	/**
	 * This class is used to contain log messages that are to be displayed
	 * on the GUI, when it is currently impossible to do so. This is in order 
	 * to allows us to queue messages till after the dialog has been created.
	 */
	struct QueuedLogLine
	{
		//! The text line to be displayed
		wxString 	line;
		//! True if the line should be shown on the status bar, false otherwise.
		bool		addtostatus;
	};


	// Socket handlers
	void ListenSocketHandler(wxSocketEvent& event);
	void UDPSocketHandler(wxSocketEvent& event);
	void ServerSocketHandler(wxSocketEvent& event);
	void ClientUDPSocketHandler(wxSocketEvent& event);


	void OnDnsDone(wxEvent& evt);
	void OnSourcesDnsDone(wxEvent& evt);

	void OnUDPTimer(AMULE_TIMER_EVENT_CLASS& evt);
	void OnTCPTimer(AMULE_TIMER_EVENT_CLASS& evt);

	void OnCoreTimer(AMULE_TIMER_EVENT_CLASS& evt);

	void OnFinishedHashing(wxEvent& evt);
	void OnFinishedCompletion(wxEvent& evt);
	void OnHashingShutdown(wxEvent&);
	
	void OnNotifyEvent(wxEvent& evt);
	
	void SetTimeOnTransfer();
	
	AMULE_TIMER_CLASS* core_timer;
		
	wxCriticalSection m_LogQueueLock;
	std::list<QueuedLogLine> QueuedAddLogLines;
#ifdef __DEBUG__
	std::deque<socket_deletion_log_item>	SocketDeletionList;
#endif
	wxLocale m_locale;

	APPState m_app_state;	

	wxString emulesig_path;
	wxString amulesig_path;
	
	uint32 m_dwPublicIP;

	DECLARE_EVENT_TABLE()
};

DECLARE_APP(CamuleApp)

#ifdef AMULE_DAEMON
#define CALL_APP_DATA_LOCK wxMutexLocker locker(theApp.data_mutex)
//#define CALL_APP_DATA_LOCK

#else
#define CALL_APP_DATA_LOCK
#endif

#endif // AMULE_H
