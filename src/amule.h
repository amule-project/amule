//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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


#ifndef AMULE_H
#define AMULE_H


#include <wx/app.h>		// Needed for wxApp
#include <wx/intl.h>		// Needed for wxLocale


#include "Types.h"		// Needed for int32, uint16 and uint64
#ifndef __WXMSW__
	#include <map>
	#include <signal.h>
	#ifndef __WXMAC__
		#include <wx/unix/execute.h>
	#endif
#endif // __WXMSW__


class CAbstractFile;
class CKnownFile;
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
class CFriendList;
class CClientUDPSocket;
class CIPFilter;
class UploadBandwidthThrottler;
#ifdef ENABLE_UPNP
class CUPnPControlPoint;
class CUPnPPortMapping;
#endif
class CStatistics;
class wxSocketEvent;
class wxCommandEvent;
class wxFFileOutputStream;
class CUpDownClient;
class CTimer;
class CTimerEvent;
class wxSingleInstanceChecker;
class CHashingEvent;
class CMuleInternalEvent;
class CCompletionEvent;
class CAllocFinishedEvent;
class wxExecuteData;
class CLoggingEvent;


namespace MuleNotify {
	class CMuleGUIEvent;
}


using MuleNotify::CMuleGUIEvent;


#ifdef AMULE_DAEMON
#define AMULE_APP_BASE wxAppConsole
#else
#define AMULE_APP_BASE wxApp
#endif

#define CONNECTED_ED2K (1<<0)
#define CONNECTED_KAD_NOT (1<<1)
#define CONNECTED_KAD_OK (1<<2)
#define CONNECTED_KAD_FIREWALLED (1<<3)


class CamuleApp : public AMULE_APP_BASE
{
private:
	enum APPState {
		APP_STATE_RUNNING = 0,
		APP_STATE_SHUTTINGDOWN,
		APP_STATE_STARTING
	};

public:
	CamuleApp();
	virtual	 ~CamuleApp();

	virtual bool	OnInit();
	int		OnExit();
	void		OnFatalException();
	bool		ReinitializeNetwork(wxString *msg);

	// derived classes may override those
	virtual int InitGui(bool geometry_enable, wxString &geometry_string);

	// Socket handlers
	void ListenSocketHandler(wxSocketEvent& event);
	void ServerSocketHandler(wxSocketEvent& event);
	void UDPSocketHandler(wxSocketEvent& event);

	virtual void ShowAlert(wxString msg, wxString title, int flags) = 0;

	// Barry - To find out if app is running or shutting/shut down
	const bool IsRunning() const { return (m_app_state == APP_STATE_RUNNING); }
	const bool IsOnShutDown() const { return (m_app_state == APP_STATE_SHUTTINGDOWN); }

	// Check ED2K and Kademlia state
	bool IsFirewalled();
	// Check Kad state
	bool IsFirewalledKad();
	// Check if we should callback this client
	bool DoCallback( CUpDownClient *client );

	// Connection to ED2K
	bool IsConnectedED2K();
	// Connection to Kad
	bool IsConnectedKad();
	// Are we connected to at least one network?
	bool IsConnected();

	// What about Kad? Is it running?
	bool IsKadRunning();

	// URL functions
	wxString	CreateMagnetLink(const CAbstractFile *f);
	wxString	CreateED2kLink(const CAbstractFile* f, bool add_source = false, bool use_hostname = false, bool addcryptoptions = false);	
	wxString	CreateED2kAICHLink(const CKnownFile* f);

	// Misc functions
	void		OnlineSig(bool zero = false);
	void		Localize_mule();
	void		Trigger_New_version(wxString newMule);

	// shakraw - new EC code using wxSocketBase
	ExternalConn*	ECServerHandler;

	// return current (valid) public IP or 0 if unknown
	// If ignorelocal is true, don't use m_localip
	uint32	GetPublicIP(bool ignorelocal = false) const; 
	void		SetPublicIP(const uint32 dwIP);
	
	uint32	GetED2KID() const;
	uint32	GetID() const;	

	// Other parts of the interface and such
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
	CFriendList*		friendlist;
	CClientUDPSocket*	clientudp;
	CStatistics*		m_statistics;
	CIPFilter*		ipfilter;
	UploadBandwidthThrottler* uploadBandwidthThrottler;
#ifdef ENABLE_UPNP
	CUPnPControlPoint*	m_upnp;
	std::vector<CUPnPPortMapping> m_upnpMappings;
#endif
	wxLocale m_locale;

	void ShutDown();

	wxString GetLog(bool reset = false);
	wxString GetServerLog(bool reset = false);
	wxString GetDebugLog(bool reset = false);

	bool AddServer(CServer *srv, bool fromUser = false);
	void AddServerMessageLine(wxString &msg);
#ifdef __DEBUG__
	void AddSocketDeleteDebug(uint32 socket_pointer, uint32 creation_time);
#endif
	void SetOSFiles(const wxString new_path);

	wxString ConfigDir;

	void AddLogLine(const wxString &msg);

	const wxString& GetOSType() const { return OSType; }

	void ShowUserCount();

	void ShowConnectionState();

	void StartKad();
	void StopKad();

	/** Bootstraps kad from the specified IP (must be in hostorder). */
	void BootstrapKad(uint32 ip, uint16 port);
	/** Updates the nodes.dat file from the specified url. */
	void UpdateNotesDat(const wxString& str);


	void DisconnectED2K();
	
	bool CryptoAvailable() const;
	
	//! TODO: Move to CLogger
	wxFFileOutputStream* applog;
protected:
	// Used to detect a previous running instance of aMule
	wxSingleInstanceChecker*	m_singleInstance;
	
#ifdef __WXDEBUG__
	/**
	 * Handles asserts in a thread-safe manner.
	 */
	virtual void OnAssertFailure(const wxChar* file, int line,
		const wxChar* func, const wxChar* cond, const wxChar* msg);
#endif

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
		bool		show;
	};

	void OnUDPDnsDone(CMuleInternalEvent& evt);
	void OnSourceDnsDone(CMuleInternalEvent& evt);
	void OnServerDnsDone(CMuleInternalEvent& evt);

	void OnTCPTimer(CTimerEvent& evt);
	void OnCoreTimer(CTimerEvent& evt);

	void OnFinishedHashing(CHashingEvent& evt);
	void OnFinishedAICHHashing(CHashingEvent& evt);
	void OnFinishedCompletion(CCompletionEvent& evt);
	void OnFinishedAllocation(CAllocFinishedEvent& evt);
	void OnFinishedHTTPDownload(CMuleInternalEvent& evt);
	void OnHashingShutdown(CMuleInternalEvent&);
	void OnNotifyEvent(CMuleGUIEvent& evt);

	void SetTimeOnTransfer();

	std::list<QueuedLogLine> m_logLines;

	APPState m_app_state;

	wxString m_emulesig_path;
	wxString m_amulesig_path;

	char *strFullMuleVersion;
	char *strOSDescription;
	wxString OSType;

	uint32 m_dwPublicIP;

	long webserver_pid;

	bool enable_stdout_log;
	bool enable_daemon_fork;
	wxString server_msg;

	CTimer* core_timer;

private:
	virtual void OnUnhandledException();

	void CheckNewVersion(uint32 result);

	uint32 m_localip;
};


#ifndef AMULE_DAEMON


class CamuleGuiBase {
public:
	CamuleGuiBase();
	virtual	 ~CamuleGuiBase();

	wxString	m_FrameTitle;
	CamuleDlg*	amuledlg;
	int m_FileDetailDialogActive;

	bool CopyTextToClipboard( wxString strText );

	virtual int InitGui(bool geometry_enable, wxString &geometry_string);
	virtual void ShowAlert(wxString msg, wxString title, int flags);
};


#ifndef CLIENT_GUI


class CamuleGuiApp : public CamuleApp, public CamuleGuiBase
{

    virtual int InitGui(bool geometry_enable, wxString &geometry_string);

	int OnExit();
	bool OnInit();
	
public:

	virtual void ShowAlert(wxString msg, wxString title, int flags);

	void ShutDown(wxCloseEvent &evt);
	void OnLoggingEvent(CLoggingEvent& evt);

	wxString GetLog(bool reset = false);
	wxString GetServerLog(bool reset = false);
	void AddServerMessageLine(wxString &msg);
	DECLARE_EVENT_TABLE()
};


DECLARE_APP(CamuleGuiApp)
#ifdef AMULE_CPP
	CamuleGuiApp *theApp;
#else
	extern CamuleGuiApp *theApp;
#endif


#else /* !CLIENT_GUI */


#include "amule-remote-gui.h"


#endif // CLIENT_GUI


#define CALL_APP_DATA_LOCK


#else /* ! AMULE_DAEMON */


#include <wx/apptrait.h>
#include <wx/socket.h>


class CSocketSet;


class CAmuledGSocketFuncTable : public GSocketGUIFunctionsTable
{
private:
	CSocketSet *m_in_set, *m_out_set;

	wxMutex m_lock;
public:
	CAmuledGSocketFuncTable();

	void AddSocket(GSocket *socket, GSocketEvent event);
	void RemoveSocket(GSocket *socket, GSocketEvent event);
	void RunSelect();

	virtual bool OnInit();
	virtual void OnExit();
	virtual bool CanUseEventLoop();
	virtual bool Init_Socket(GSocket *socket);
	virtual void Destroy_Socket(GSocket *socket);
	virtual void Install_Callback(GSocket *socket, GSocketEvent event);
	virtual void Uninstall_Callback(GSocket *socket, GSocketEvent event);
	virtual void Enable_Events(GSocket *socket);
	virtual void Disable_Events(GSocket *socket);
};


typedef std::map<int, wxEndProcessData *> EndProcessDataMap;


class CDaemonAppTraits : public wxConsoleAppTraits
{
private:
	CAmuledGSocketFuncTable *m_table;
	wxMutex m_lock;
	std::list<wxObject *> m_sched_delete;
#ifndef __WXMSW__
	struct sigaction m_oldSignalChildAction;
	struct sigaction m_newSignalChildAction;
#endif

public:
	CDaemonAppTraits(CAmuledGSocketFuncTable *table);
	virtual GSocketGUIFunctionsTable* GetSocketGUIFunctionsTable();
	virtual void ScheduleForDestroy(wxObject *object);
	virtual void RemoveFromPendingDelete(wxObject *object);

	void DeletePending();

#ifndef __WXMSW__
	virtual int WaitForChild(wxExecuteData& execData);
#endif
#ifdef __WXMAC__
	virtual wxStandardPathsBase& GetStandardPaths();
#endif
};


#ifndef __WXMSW__
	void OnSignalChildHandler(int signal, siginfo_t *siginfo, void *ucontext);
	pid_t AmuleWaitPid(pid_t pid, int *status, int options, wxString *msg);
#endif // __WXMSW__


class CamuleDaemonApp : public CamuleApp
{
private:
	bool m_Exit;
	CAmuledGSocketFuncTable *m_table;
#ifndef __WXMSW__
	struct sigaction m_oldSignalChildAction;
	struct sigaction m_newSignalChildAction;
#endif // __WXMSW__

	bool OnInit();
	int OnRun();
	int OnExit();

	virtual int InitGui(bool geometry_enable, wxString &geometry_string);
	
public:
	CamuleDaemonApp();
	
	void ExitMainLoop() { m_Exit = true; }
	
	bool CopyTextToClipboard(wxString strText);
	
	virtual void ShowAlert(wxString msg, wxString title, int flags);
	
	void OnLoggingEvent(CLoggingEvent& evt);
	
	DECLARE_EVENT_TABLE()
	
	wxAppTraits *CreateTraits();
};

DECLARE_APP(CamuleDaemonApp)
#ifdef AMULE_CPP
	CamuleDaemonApp *theApp;
#else
	extern CamuleDaemonApp *theApp;
#endif

#endif /* ! AMULE_DAEMON */

#endif // AMULE_H
// File_checked_for_headers