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


// must be moved to a file
enum GUI_Event_ID {
	INVALID_EVENT = 0,
	SHAREDFILES_UPDATEITEM,
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
	};
	
	GUIEvent(GUI_Event_ID new_id, byte value8, wxString value_s) {
		ID 					= new_id;	
		byte_value 		= value8;
		long_value 		= 0;
		longlong_value 	= 0;
		string_value 	= value_s;
		ptr_value			= NULL;
	};
	
	GUI_Event_ID ID;
	byte			byte_value;
	uint32		long_value;
	uint64		longlong_value;
	wxString		string_value;
	// this should NEVER be needed
	void*			ptr_value; 
};


#define AddLogLineM(x,y); theApp.NotifyEvent(GUIEvent(ADDLOGLINE,x,y));
		
#define AddDebugLogLineM(x,y); theApp.NotifyEvent(GUIEvent(ADDDEBUGLOGLINE,x,y));

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
	const bool IsRunning() { return (m_app_state == APP_STATE_RUNNING); }

	// ed2k URL functions
	wxString		StripInvalidFilenameChars(const wxString& strText, bool bKeepSpaces = true);
	wxString		CreateED2kLink( CAbstractFile* f );
	wxString		CreateHTMLED2kLink( CAbstractFile* f );
	wxString		CreateED2kSourceLink( CAbstractFile* f );
	wxString		CreateED2kHostnameSourceLink( CAbstractFile* f );
	wxString		GenFakeCheckUrl(CAbstractFile *file);
	wxString        GenFakeCheckUrl2(CAbstractFile *file);
	
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
	CList<QueuedLogLine>	QueuedAddLogLines;
#ifdef __DEBUG__
	CList<socket_deletion_log_item>	SocketDeletionList;
#endif
	wxLocale		m_locale;

	APPState			m_app_state;	

	wxString emulesig_path;
	wxString amulesig_path;

	DECLARE_EVENT_TABLE()
};

DECLARE_APP(CamuleApp)

#endif // AMULE_H
