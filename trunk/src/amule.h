// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef AMULE_H
#define AMULE_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/app.h>			// Needed for wxApp
#include <wx/intl.h>		// Needed for wxLocale
#include <wx/timer.h>		// Needed for wxTimerEvent

#include "CTypedPtrList.h"
#include "types.h"			// Needed for int32, uint16 and uint64

#include <deque>

// must be moved to a file
enum GUI_Event_ID {
	INVALID_EVENT = 0,
	// queue list
	QLIST_CTRL_ADD_CLIENT,
	QLIST_CTRL_RM_CLIENT,
	QLIST_CTRL_REFRESH_CLIENT,
	// shared files
	SHAREDFILES_UPDATEITEM,
	// download control
	DOWNLOAD_CTRL_UPDATEITEM,
	DOWNLOAD_CTRL_ADD_FILE,
	DOWNLOAD_CTRL_ADD_SOURCE,
	DOWNLOAD_CTRL_RM_FILE,
	DOWNLOAD_CTRL_RM_SOURCE,
	DOWNLOAD_CTRL_CHANGE_CAT,
	DOWNLOAD_CTRL_HIDE_FILE,
	DOWNLOAD_CTRL_HIDE_SOURCE,
	DOWNLOAD_CTRL_INIT_SORT,
	DOWNLOAD_CTRL_SHOW_FILE,
	DOWNLOAD_CTRL_SHOW_FILES_COUNT,
	DOWNLOAD_CTRL_THAW,
	DOWNLOAD_CTRL_FREEZE,
	// upload control
	UPLOAD_CTRL_ADD_CLIENT,
	UPLOAD_CTRL_REFRESH_CLIENT,
	UPLOAD_CTRL_RM_CLIENT,
	// server
	SERVER_ADD,
	SERVER_RM,
	SERVER_RM_DEAD,
	SERVER_RM_ALL,
	SERVER_HIGHLIGHT,
	SERVER_REFRESH,
	SERVER_FREEZE,
	SERVER_THAW,
	// search window
	SEARCH_CANCEL,
	// notification
	SHOW_NOTIFIER,
	SHOW_CONN_STATE,
	SHOW_QUEUE_COUNT,
	SHOW_UPDATE_CAT_TABS,
	// logging
	ADDLOGLINE,
	ADDDEBUGLOGLINE
};

class GUIEvent {
	public:
	GUIEvent(GUI_Event_ID new_id) {
		ID 					= new_id;	
		byte_value 		= 0;
		long_value 		= 0;
		longlong_value 	= 0;
		string_value 	= wxEmptyString;
		ptr_value			= NULL;
                ptr_aux_value                   = NULL;
	}
	
	GUIEvent(GUI_Event_ID new_id, byte value8, wxString value_s) {
		ID 					= new_id;	
		byte_value 		= value8;
		long_value 		= 0;
		longlong_value 	= 0;
		string_value 	= value_s;
		ptr_value			= NULL;
                ptr_aux_value                   = NULL;
	}

        GUIEvent(GUI_Event_ID new_id, void *new_ptr) {
                ID              = new_id;       
                byte_value      = 0;
                long_value      = 0;
                longlong_value  = 0;
                string_value    = wxEmptyString;
                ptr_value       = new_ptr;
                ptr_aux_value   = NULL;
        }

        GUIEvent(GUI_Event_ID new_id, void *new_ptr,  byte value8) {
                ID              = new_id;       
                byte_value      = value8;
                long_value      = 0;
                longlong_value  = 0;
                string_value    = wxEmptyString;
                ptr_value       = new_ptr;
                ptr_aux_value   = NULL;
        }

        GUIEvent(GUI_Event_ID new_id, void *new_ptr, void *new_aux_ptr,  byte value8) {
                ID              = new_id;       
                byte_value      = value8;
                long_value      = 0;
                longlong_value  = 0;
                string_value    = wxEmptyString;
                ptr_value       = new_ptr;
                ptr_aux_value   = new_aux_ptr;
        }
	
	GUI_Event_ID ID;
	byte			byte_value;
	uint32		long_value;
	uint64		longlong_value;
	wxString		string_value;

	// this should NEVER be needed
	void*			ptr_value; 
	void*			ptr_aux_value; 
};

//
// macros for creation of notification events
//
#define Notify_0_ValEvent(id) theApp.NotifyEvent(GUIEvent(id))
#define Notify_1_ValEvent(id, val) theApp.NotifyEvent(GUIEvent(id, val))
#define Notify_2_ValEvent(id, val) theApp.NotifyEvent(GUIEvent(id, val0, val1))
#define Notify_SharedFilesUpdateItem(ptr)           Notify_1_ValEvent(SHAREDFILES_UPDATEITEM, ptr)

// download ctrl
#define Notify_DownloadCtrlUpdateItem(ptr)          Notify_1_ValEvent(DOWNLOAD_CTRL_UPDATEITEM, ptr)
#define Notify_DownloadCtrlAddFile(ptr)             Notify_1_ValEvent(DOWNLOAD_CTRL_ADD_FILE, ptr)
#define Notify_DownloadCtrlAddSource(ptr0, ptr1)    Notify_2_ValEvent(DOWNLOAD_CTRL_ADD_SOURCE, ptr0, ptr1)
#define Notify_DownloadCtrlRemoveFile(ptr0, ptr1)   Notify_2_ValEvent(DOWNLOAD_CTRL_RM_FILE, ptr0, ptr1)
#define Notify_DownloadCtrlRemoveSource(ptr)        Notify_1_ValEvent(DOWNLOAD_CTRL_RM_SOURCE, ptr)
#define Notify_DownloadCtrlChangeCat(val)           Notify_1_ValEvent(DOWNLOAD_CTRL_CHANGE_CAT, val)
#define Notify_DownloadCtrlHideFile(ptr)            Notify_1_ValEvent(DOWNLOAD_CTRL_HIDE_FILE, ptr)
#define Notify_DownloadCtrlHideSource(ptr)          Notify_1_ValEvent(DOWNLOAD_CTRL_HIDE_SOURCE, ptr)
#define Notify_DownloadCtrlInitSort()               Notify_0_ValEvent(DOWNLOAD_CTRL_INIT_SORT)
#define Notify_DownloadCtrlShowFile(ptr)            Notify_1_ValEvent(DOWNLOAD_CTRL_SHOW_FILE, ptr)
#define Notify_DownloadCtrlShowFilesCount()         Notify_0_ValEvent(DOWNLOAD_CTRL_SHOW_FILES_COUNT)
#define Notify_DownloadCtrlThaw()                   Notify_0_ValEvent(DOWNLOAD_CTRL_THAW)
#define Notify_DownloadCtrlFreeze()                 Notify_0_ValEvent(DOWNLOAD_CTRL_FREEZE)

// upload ctrl
#define Notify_UploadCtrlAddClient(ptr)             Notify_1_ValEvent(UPLOAD_CTRL_ADD_CLIENT, ptr)
#define Notify_UploadCtrlRefreshClient(ptr)         Notify_1_ValEvent(UPLOAD_CTRL_REFRESH_CLIENT, ptr)
#define Notify_UploadCtrlRemoveClient(ptr)          Notify_1_ValEvent(UPLOAD_CTRL_RM_CLIENT, ptr)

// server
#define Notify_ServerAdd(ptr)                       Notify_1_ValEvent(SERVER_ADD, ptr)
#define Notify_ServerRemove(ptr)                    Notify_1_ValEvent(SERVER_RM, ptr)
#define Notify_ServerRemoveDead()                   Notify_0_ValEvent(SERVER_RM_DEAD)
#define Notify_ServerRemoveAll()                    Notify_0_ValEvent(SERVER_RM_ALL)
#define Notify_ServerHighlight(ptr, val)            Notify_2_ValEvent(SERVER_HIGHLIGHT, ptr, val)
#define Notify_ServerRefresh(ptr)                   Notify_1_ValEvent(SERVER_REFRESH, ptr)
#define Notify_ServerFreeze()                       Notify_0_ValEvent(SERVER_FREEZE)
#define Notify_ServerThaw()                         Notify_0_ValEvent(SERVER_THAW)

// queue list
#define Notify_QlistAddClient(ptr)                  Notify_1_ValEvent(QLIST_CTRL_ADD_CLIENT, ptr)
#define Notify_QlistRemoveClient(ptr)               Notify_1_ValEvent(QLIST_CTRL_RM_CLIENT, ptr)
#define Notify_QlistThaw()                          Notify_0_ValEvent()

// search
#define Notify_SearchCancel(ptr)                    Notify_1_ValEvent(SEARCH_CANCEL, ptr)

// misc
#define Notify_ShowNotifier(ptr)                    Notify_1_ValEvent(SHOW_NOTIFIER, ptr)
#define Notify_ShowConnState(val)                   Notify_1_ValEvent(SHOW_CONN_STATE, val)
#define Notify_ShowUpdateCatTabTitles()             Notify_0_ValEvent(SHOW_UPDATE_CAT_TABS)


#define AddLogLineM(x,y); theApp.NotifyEvent(GUIEvent(ADDLOGLINE,x,y));
#define AddDebugLogLineM(x,y); theApp.NotifyEvent(GUIEvent(ADDDEBUGLOGLINE,x,y));

// __VA_ARGS__ is not ansi standard 
void AddDebugLogLineF(bool addtostatus, const wxChar *line, ...);
void AddLogLineF(bool addtostatus, const wxChar *line, ...);

class CAbstractFile;
class ExternalConn;
class CamuleDlg;
class CPreferences;
class CDownloadQueue;
class CUploadQueue;
class CServerConnect;
class CSharedFileList;
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
	
typedef struct {
	wxString line;
	bool		addtostatus;
} QueuedLogLine;


class CamuleApp : public wxApp
{
public:
					CamuleApp() {};
	virtual			~CamuleApp() {};

	virtual bool	OnInit();
	int				OnExit();
	void			OnFatalException();

	// Barry - To find out if app is running or shutting/shut down
	bool IsRunning() const { return (m_app_state == APP_STATE_RUNNING); }

	// ed2k URL functions
	wxString		StripInvalidFilenameChars(const wxString& strText, bool bKeepSpaces = true);
	wxString		CreateED2kLink( CAbstractFile* f );
	wxString		CreateHTMLED2kLink( CAbstractFile* f );
	wxString		CreateED2kSourceLink( CAbstractFile* f );
	wxString		CreateED2kHostnameSourceLink( CAbstractFile* f );
	wxString		GenFakeCheckUrl(CAbstractFile *file);
	wxString        GenFakeCheckUrl2(CAbstractFile *file);
	
	// websearch function
	wxString        GenWebSearchUrl( const wxString &filename );
	
	void QueueLogLine(bool addtostatusbar, const wxChar* line, ...);
	void FlushQueuedLogLines();
		
	// Misc functions
	bool			CopyTextToClipboard( wxString strText );
	void			OnlineSig(bool zero = false); 
	void			Localize_mule();
	void			Trigger_New_version(wxString old_version, wxString new_version);
	void			LaunchUrl(const wxString &url);
	

	// Kry - External connections
	wxServer*		localserver;	
	
	// shakraw - new EC code using wxSocketBase
	ExternalConn*	ECServerHandler;

	// Kry - avoid chmod on win32
	bool use_chmod;
	bool IsReady;
	

	// Statistic functions. I plan on moving these to a class of their own -- Xaignar
	void			UpdateReceivedBytes(int32 bytesToAdd);
	uint64			GetUptimeMsecs();
	uint32			GetUptimeSecs();
	uint32			GetTransferSecs();
	uint32			GetServerSecs();
	void			UpdateSentBytes(int32 bytesToAdd);

	// Statistic variables. I plan on moving these to a class of their own -- Xaignar
	uint64			Start_time;
	double			sTransferDelay;
	uint64			stat_sessionReceivedBytes;
	uint64			stat_sessionSentBytes;
	uint32			stat_reconnects;
	uint64			stat_transferStarttime;
	uint64			stat_serverConnectTime;
	uint32			stat_filteredclients;


	// Other parts of the interface and such
	CamuleDlg*			amuledlg;
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
	CIPFilter*			ipfilter;

	void ShutDown();
	
	void NotifyEvent(GUIEvent event);

#ifdef __DEBUG__
	void AddSocketDeleteDebug(uint32 socket_pointer, uint32 creation_time);
#endif
	void SetOSFiles(const wxString new_path); 
	
	wxString ConfigDir;
	
protected:
	// Socket handlers
	void ListenSocketHandler(wxSocketEvent& event);
	void ClientReqSocketHandler(wxSocketEvent& event);
	void UDPSocketHandler(wxSocketEvent& event);
	void ServerSocketHandler(wxSocketEvent& event);
	void ClientUDPSocketHandler(wxSocketEvent& event);


	void OnDnsDone(wxCommandEvent& evt);
	void OnSourcesDnsDone(wxCommandEvent& evt);

	void OnUDPTimer(wxTimerEvent& evt);
	void OnTCPTimer(wxTimerEvent& evt);

	void OnCoreTimer(wxTimerEvent& evt);

	void OnFinishedHashing(wxCommandEvent& evt);
	void OnFinishedCompletion(wxCommandEvent& evt);
	void OnHashingShutdown(wxCommandEvent&);

	void 			SetTimeOnTransfer();
	
	wxTimer* core_timer;
		
	wxCriticalSection m_LogQueueLock;
	std::deque<QueuedLogLine>	QueuedAddLogLines;
#ifdef __DEBUG__
	std::deque<socket_deletion_log_item>	SocketDeletionList;
#endif
	wxLocale		m_locale;

	APPState			m_app_state;	

	wxString emulesig_path;
	wxString amulesig_path;

	DECLARE_EVENT_TABLE()
};

DECLARE_APP(CamuleApp)

#endif // AMULE_H
