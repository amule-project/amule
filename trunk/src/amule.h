//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include <map>
#ifndef __WINDOWS__ 
	#include <signal.h>
//	#include <wx/unix/execute.h>
#endif // __WINDOWS__ 


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
class CCanceledFileList;
class CSearchList;
class CClientCreditsList;
class CFriendList;
class CClientUDPSocket;
class CIPFilter;
class UploadBandwidthThrottler;
class CAsioService;
#ifdef ENABLE_UPNP
class CUPnPControlPoint;
class CUPnPPortMapping;
#endif
class CStatistics;
class wxSocketEvent;
class wxCommandEvent;
class wxCloseEvent;
class wxFFileOutputStream;
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
#define CORE_TIMER_PERIOD 300
#else
#define AMULE_APP_BASE wxApp
#define CORE_TIMER_PERIOD 100
#endif

#define CONNECTED_ED2K (1<<0)
#define CONNECTED_KAD_NOT (1<<1)
#define CONNECTED_KAD_OK (1<<2)
#define CONNECTED_KAD_FIREWALLED (1<<3)


// Base class common to amule, aamuled and amulegui
class CamuleAppCommon
{
private:
	// Used to detect a previous running instance of aMule
	wxSingleInstanceChecker*	m_singleInstance;

	bool		CheckPassedLink(const wxString &in, wxString &out, int cat);
protected:
	wxString	FullMuleVersion;
	wxString	OSDescription;
	wxString	OSType;
	bool		enable_daemon_fork;
	bool		ec_config;
	bool		m_skipConnectionDialog;
	bool		m_geometryEnabled;
	wxString	m_geometryString;
	wxString	m_logFile;
	wxString	m_appName;
	wxString	m_PidFile;

	bool		InitCommon(int argc, wxChar ** argv);
	void		RefreshSingleInstanceChecker();
	bool		CheckMuleDirectory(const wxString& desc, const class CPath& directory, const wxString& alternative, class CPath& outDir);
public:
	wxString	ConfigDir;
	wxString	m_configFile;

	CamuleAppCommon();
	~CamuleAppCommon();
	void		AddLinksFromFile();
	// URL functions
	wxString	CreateMagnetLink(const CAbstractFile *f);
	wxString	CreateED2kLink(const CAbstractFile* f, bool add_source = false, bool use_hostname = false, bool add_cryptoptions = false, bool add_AICH = false);
	// Who am I ?
#ifdef AMULE_DAEMON
	bool		IsDaemon() const { return true; }
#else
	bool		IsDaemon() const { return false; }
#endif

#ifdef CLIENT_GUI
	bool		IsRemoteGui() const { return true; }
#else
	bool		IsRemoteGui() const { return false; }
#endif

	const wxString&	GetMuleAppName() const { return m_appName; }
	const wxString	GetFullMuleVersion() const;
};

class CamuleApp : public AMULE_APP_BASE, public CamuleAppCommon
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
#if wxUSE_ON_FATAL_EXCEPTION
	void		OnFatalException();
#endif
	bool		ReinitializeNetwork(wxString *msg);

	// derived classes may override those
	virtual int InitGui(bool geometry_enable, wxString &geometry_string);

	// Socket handlers
	void ListenSocketHandler(wxSocketEvent& event);
	void ServerSocketHandler(wxSocketEvent& event);
	void UDPSocketHandler(wxSocketEvent& event);

	virtual int ShowAlert(wxString msg, wxString title, int flags) = 0;

	// Barry - To find out if app is running or shutting/shut down
	bool IsRunning() const { return (m_app_state == APP_STATE_RUNNING); }
	bool IsOnShutDown() const { return (m_app_state == APP_STATE_SHUTTINGDOWN); }

	// Check ED2K and Kademlia state
	bool IsFirewalled() const;
	// Are we connected to at least one network?
	bool IsConnected() const;
	// Connection to ED2K
	bool IsConnectedED2K() const;

	// What about Kad? Is it running?
	bool IsKadRunning() const;
	// Connection to Kad
	bool IsConnectedKad() const;
	// Check Kad state (TCP)
	bool IsFirewalledKad() const;
	// Check Kad state (UDP)
	bool IsFirewalledKadUDP() const;
	// Check Kad state (LAN mode)
	bool IsKadRunningInLanMode() const;
	// Kad stats
	uint32	GetKadUsers() const;
	uint32	GetKadFiles() const;
	uint32	GetKadIndexedSources() const;
	uint32	GetKadIndexedKeywords() const;
	uint32	GetKadIndexedNotes() const;
	uint32	GetKadIndexedLoad() const;
	// True IP of machine
	uint32	GetKadIPAdress() const;
	// Buddy status
	uint8	GetBuddyStatus() const;
	uint32	GetBuddyIP() const;
	uint32	GetBuddyPort() const;

	// Check if we should callback this client
	bool CanDoCallback(uint32 clientServerIP, uint16 clientServerPort);

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
	CCanceledFileList*	canceledfiles;
	CSearchList*		searchlist;
	CClientCreditsList*	clientcredits;
	CFriendList*		friendlist;
	CClientUDPSocket*	clientudp;
	CStatistics*		m_statistics;
	CIPFilter*		ipfilter;
	UploadBandwidthThrottler* uploadBandwidthThrottler;
	CAsioService*		m_AsioService;
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
	void SetOSFiles(const wxString& new_path);

	const wxString& GetOSType() const { return OSType; }

	void ShowUserCount();

	void ShowConnectionState(bool forceUpdate = false);

	void StartKad();
	void StopKad();

	/** Bootstraps kad from the specified IP (must be in hostorder). */
	void BootstrapKad(uint32 ip, uint16 port);
	/** Updates the nodes.dat file from the specified url. */
	void UpdateNotesDat(const wxString& str);


	void DisconnectED2K();

	bool CryptoAvailable() const;

protected:

#ifdef __WXDEBUG__
	/**
	 * Handles asserts in a thread-safe manner.
	 */
	virtual void OnAssertFailure(const wxChar* file, int line,
		const wxChar* func, const wxChar* cond, const wxChar* msg);
#endif

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

	APPState m_app_state;

	wxString m_emulesig_path;
	wxString m_amulesig_path;

	uint32 m_dwPublicIP;

	long webserver_pid;

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
	int		m_FileDetailDialogActive;

	bool CopyTextToClipboard( wxString strText );
	void ResetTitle();

	virtual int InitGui(bool geometry_enable, wxString &geometry_string);
	virtual int ShowAlert(wxString msg, wxString title, int flags);

	void AddGuiLogLine(const wxString& line);
protected:
	/**
	 * This list is used to contain log messages that are to be displayed
	 * on the GUI, when it is currently impossible to do so. This is in order
	 * to allows us to queue messages till after the dialog has been created.
	 */
	std::list<wxString> m_logLines;
};


#ifndef CLIENT_GUI


class CamuleGuiApp : public CamuleApp, public CamuleGuiBase
{

    virtual int InitGui(bool geometry_enable, wxString &geometry_string);

	int OnExit();
	bool OnInit();

public:

	virtual int ShowAlert(wxString msg, wxString title, int flags);

	void ShutDown(wxCloseEvent &evt);

	wxString GetLog(bool reset = false);
	wxString GetServerLog(bool reset = false);
	void AddServerMessageLine(wxString &msg);
	DECLARE_EVENT_TABLE()
};


DECLARE_APP(CamuleGuiApp)
extern CamuleGuiApp *theApp;


#else /* !CLIENT_GUI */


#include "amule-remote-gui.h"


#endif // CLIENT_GUI


#define CALL_APP_DATA_LOCK


#else /* ! AMULE_DAEMON */

// wxWidgets 2.8 requires special code for event handling and sockets.
// 2.9 doesn't, so standard event loop and sockets can be used
//
// Windows: aMuled compiles with 2.8 (without the special code),
// but works only with 2.9

#if !wxCHECK_VERSION(2, 9, 0)
	// wx 2.8 needs a hand-made event loop in any case
	#define AMULED28_EVENTLOOP

	// wx 2.8 also needs extra socket code, unless we have ASIO sockets
	// 
	#ifdef HAVE_CONFIG_H
	#	include "config.h"		// defines ASIO_SOCKETS
	#endif

	#ifndef ASIO_SOCKETS
		// MSW: can't run amuled with 2.8 without ASIO sockets, just get it compiled
		#ifndef __WINDOWS__ 
			#define AMULED28_SOCKETS
		#endif
	#endif
#endif

#ifdef AMULED28_SOCKETS
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


#endif // AMULED28_SOCKETS

// AppTrait functionality is required for 2.8 wx sockets
// Otherwise it's used to prevent zombie child processes,
// which stops working with wx 2.9.5.
// So disable it there (no idea if this has a noticeable impact).

#if !wxCHECK_VERSION(2, 9, 5) && !defined(__WINDOWS__ )
#define AMULED_APPTRAITS
#endif

#ifdef AMULED_APPTRAITS

typedef std::map<int, class wxEndProcessData *> EndProcessDataMap;

#include <wx/apptrait.h>

class CDaemonAppTraits : public wxConsoleAppTraits
{
private:
	struct sigaction m_oldSignalChildAction;
	struct sigaction m_newSignalChildAction;

#ifdef AMULED28_SOCKETS
	CAmuledGSocketFuncTable *m_table;
	wxMutex m_lock;
	std::list<wxObject *> m_sched_delete;
public:
	CDaemonAppTraits(CAmuledGSocketFuncTable *table);
	virtual GSocketGUIFunctionsTable* GetSocketGUIFunctionsTable();
	virtual void ScheduleForDestroy(wxObject *object);
	virtual void RemoveFromPendingDelete(wxObject *object);

	void DeletePending();
#else	// AMULED28_SOCKETS
public:
	CDaemonAppTraits();
#endif	// !AMULED28_SOCKETS

	virtual int WaitForChild(wxExecuteData& execData);

#if defined(__WXMAC__) && !wxCHECK_VERSION(2, 9, 0)
	virtual wxStandardPathsBase& GetStandardPaths();
#endif
};

void OnSignalChildHandler(int signal, siginfo_t *siginfo, void *ucontext);
pid_t AmuleWaitPid(pid_t pid, int *status, int options, wxString *msg);

#endif // AMULED_APPTRAITS


class CamuleDaemonApp : public CamuleApp
{
private:
#ifdef AMULED28_EVENTLOOP
	bool m_Exit;
#endif
#ifdef AMULED28_SOCKETS
	CAmuledGSocketFuncTable *m_table;
#endif
	bool OnInit();
	int OnRun();
	int OnExit();

	virtual int InitGui(bool geometry_enable, wxString &geometry_string);

#ifdef AMULED_APPTRAITS
	struct sigaction m_oldSignalChildAction;
	struct sigaction m_newSignalChildAction;
public:
	wxAppTraits *CreateTraits();
#endif // AMULED_APPTRAITS

public:

#ifdef AMULED28_EVENTLOOP
	CamuleDaemonApp();

	void ExitMainLoop() { m_Exit = true; }
#endif

	bool CopyTextToClipboard(wxString strText);

	virtual int ShowAlert(wxString msg, wxString title, int flags);

	DECLARE_EVENT_TABLE()
};

DECLARE_APP(CamuleDaemonApp)
extern CamuleDaemonApp *theApp;

#endif /* ! AMULE_DAEMON */

#endif // AMULE_H
// File_checked_for_headers
