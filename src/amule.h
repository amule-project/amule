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


#include <wx/app.h>			// Needed for wxApp
#include <wx/intl.h>			// Needed for wxLocale


#include "Types.h"			// Needed for int32, uint16 and uint64
#include <map>
#ifndef __WINDOWS__
	#include <signal.h>
#endif // __WINDOWS__

#include "config.h"			// Needed for ASIO_SOCKETS

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
#ifdef ASIO_SOCKETS
class CAsioService;
#else
class wxSocketEvent;
#endif
#ifdef ENABLE_UPNP
class CUPnPControlPoint;
class CUPnPPortMapping;
#endif
class CStatistics;
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


namespace Kademlia {
	class CUInt128;
}


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


void OnShutdownSignal( int /* sig */ );


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

#ifndef ASIO_SOCKETS
	// Socket handlers
	void ListenSocketHandler(wxSocketEvent& event);
	void UDPSocketHandler(wxSocketEvent& event);
#endif

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
	// Kad ID
	const Kademlia::CUInt128& GetKadID() const;

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
#ifdef ASIO_SOCKETS
	CAsioService*		m_AsioService;
#endif
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


class CamuleDaemonApp : public CamuleApp
{
private:
	bool OnInit();
	int OnRun();
	int OnExit();

	virtual int InitGui(bool geometry_enable, wxString &geometry_string);
	// The GTK wxApps sets its file name conversion properly
	// in wxApp::Initialize(), while wxAppConsole::Initialize()
	// does not, leaving wxConvFile being set to wxConvLibc. File
	// name conversion should be set otherwise amuled will abort to
	// handle non-ASCII file names which monolithic amule can handle.
	// This function are overridden to perform this.
	virtual bool Initialize(int& argc_, wxChar **argv_);

public:

	bool CopyTextToClipboard(wxString strText);

	virtual int ShowAlert(wxString msg, wxString title, int flags);

	DECLARE_EVENT_TABLE()
};

DECLARE_APP(CamuleDaemonApp)
extern CamuleDaemonApp *theApp;

#endif /* ! AMULE_DAEMON */

#endif // AMULE_H
// File_checked_for_headers
