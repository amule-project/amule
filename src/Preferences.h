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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "MD4Hash.h"			// Needed for CMD4Hash

#include <wx/arrstr.h>			// Needed for wxArrayString

#include <map>

#include "Proxy.h"
#include "OtherStructs.h"

#include <common/ClientVersion.h>	// Needed for __SVN__

class CPreferences;
class wxConfigBase;
class wxWindow;

enum EViewSharedFilesAccess {
	vsfaEverybody = 0,
	vsfaFriends = 1,
	vsfaNobody = 2
};

enum AllCategoryFilter {
	acfAll = 0,
	acfAllOthers,
	acfIncomplete,
	acfCompleted,
	acfWaiting,
	acfDownloading,
	acfErroneous,
	acfPaused,
	acfStopped,
	acfVideo,
	acfAudio,
	acfArchive,
	acfCDImages,
	acfPictures,
	acfText,
	acfActive
};

/**
 * Base-class for automatically loading and saving of preferences.
 *
 * The purpose of this class is to perform two tasks:
 * 1) To load and save a variable using wxConfig
 * 2) If nescecarry, to syncronize it with a widget
 *
 * This pure-virtual class servers as the base of all the Cfg types
 * defined below, and exposes the entire interface.
 *
 * Please note that for reasons of simplicity these classes dont provide
 * direct access to the variables they maintain, as there is no real need
 * for this.
 *
 * To create a sub-class you need only provide the Load/Save functionality,
 * as it is given that not all variables have a widget assosiated.
 */
class Cfg_Base
{
public:
	/**
	 * Constructor.
	 *
	 * @param keyname This is the keyname under which the variable is to be saved.
	 */
	Cfg_Base( const wxString& keyname )
	 : m_key( keyname ),
	   m_changed( false )
	{}

	/**
	 * Destructor.
	 */
	virtual ~Cfg_Base() {}

	/**
	 * This function loads the assosiated variable from the provided config object.
	 */
	virtual void LoadFromFile(wxConfigBase* cfg) = 0;
	/**
	 * This function saves the assosiated variable to the provided config object.
	 */
	virtual void SaveToFile(wxConfigBase* cfg) = 0;

	/**
	 * Syncs the variable with the contents of the widget.
	 *
	 * @return True of success, false otherwise.
	 */
	virtual bool TransferFromWindow()	{ return false; }
	/**
	 * Syncs the widget with the contents of the variable.
	 *
	 * @return True of success, false otherwise.
	 */
	virtual bool TransferToWindow()		{ return false; }

	/**
	 * Connects a widget with the specified ID to the Cfg object.
	 *
	 * @param id The ID of the widget.
	 * @param parent A pointer to the widgets parent, to speed up searches.
	 * @return True on success, false otherwise.
	 *
	 * This function only makes sense for Cfg-classes that have the capability
	 * to interact with a widget, see Cfg_Tmpl::ConnectToWidget().
	 */
	virtual	bool ConnectToWidget( int WXUNUSED(id), wxWindow* WXUNUSED(parent) = NULL )	{ return false; }

	/**
	 * Gets the key assosiated with Cfg object.
	 *
	 * @return The config-key of this object.
	 */
	virtual const wxString& GetKey()	{ return m_key; }


	/**
	 * Specifies if the variable has changed since the TransferFromWindow() call.
	 *
	 * @return True if the variable has changed, false otherwise.
	 */
	virtual bool HasChanged()			{ return m_changed; }


protected:
	/**
	 * Sets the changed status.
	 *
	 * @param changed The new status.
	 */
	virtual void SetChanged( bool changed )
	{
		m_changed = changed;
	};

private:

	//! The Config-key under which to save the variable
	wxString	m_key;

	//! The changed-status of the variable
	bool		m_changed;
};


class Cfg_Lang_Base {
public:
	virtual void UpdateChoice(int pos);
};

const int cntStatColors = 15;


//! This typedef is a shortcut similar to the theApp shortcut, but uses :: instead of .
//! It only allows access to static preference functions, however this is to be desired anyway.
typedef CPreferences thePrefs;


class CPreferences
{
public:
	friend class PrefsUnifiedDlg;

	CPreferences();
	~CPreferences();

	void			Save();
	void			SaveCats();
	void			ReloadSharedFolders();

	static const wxString&	GetConfigDir()			{ return s_configDir; }
	static void		SetConfigDir(const wxString& dir) { s_configDir = dir; }

	static bool		Score()				{ return s_scorsystem; }
	static void		SetScoreSystem(bool val)	{ s_scorsystem = val; }
	static bool		Reconnect()			{ return s_reconnect; }
	static void		SetReconnect(bool val)		{ s_reconnect = val; }
	static bool		DeadServer()			{ return s_deadserver; }
	static void		SetDeadServer(bool val)		{ s_deadserver = val; }
	static const wxString&	GetUserNick()			{ return s_nick; }
	static void		SetUserNick(const wxString& nick) { s_nick = nick; }
	static Cfg_Lang_Base * GetCfgLang()		{ return s_cfgLang; }

	static const wxString&	GetAddress()			{ return s_Addr; }
	static uint16		GetPort()			{ return s_port; }
	static void		SetPort(uint16 val);
	static uint16		GetUDPPort()			{ return s_udpport; }
	static uint16		GetEffectiveUDPPort()	{ return s_UDPEnable ? s_udpport : 0; }
	static void		SetUDPPort(uint16 val)		{ s_udpport = val; }
	static bool		IsUDPDisabled()			{ return !s_UDPEnable; }
	static void		SetUDPDisable(bool val)		{ s_UDPEnable = !val; }
	static const CPath&	GetIncomingDir()		{ return s_incomingdir; }
	static void		SetIncomingDir(const CPath& dir){ s_incomingdir = dir; }
	static const CPath&	GetTempDir()			{ return s_tempdir; }
	static void		SetTempDir(const CPath& dir)	{ s_tempdir = dir; }
	static const CMD4Hash&	GetUserHash()			{ return s_userhash; }
	static void		SetUserHash(const CMD4Hash& h)	{ s_userhash = h; }
	static uint16		GetMaxUpload()			{ return s_maxupload; }
	static uint16		GetSlotAllocation()		{ return s_slotallocation; }
	static bool		IsICHEnabled()			{ return s_ICH; }
	static void		SetICHEnabled(bool val)		{ s_ICH = val; }
	static bool		IsTrustingEveryHash()		{ return s_AICHTrustEveryHash; }
	static void		SetTrustingEveryHash(bool val)	{ s_AICHTrustEveryHash = val; }
	static bool		AutoServerlist()		{ return s_autoserverlist; }
	static void		SetAutoServerlist(bool val)	{ s_autoserverlist = val; }
	static bool		DoMinToTray()			{ return s_mintotray; }
	static bool		ShowNotifications()		{ return s_notify; }
	static void		SetMinToTray(bool val)		{ s_mintotray = val; }
	static bool		UseTrayIcon()			{ return s_trayiconenabled; }
	static void		SetUseTrayIcon(bool val)	{ s_trayiconenabled = val; }
	static bool		HideOnClose()			{ return s_hideonclose; }
	static void		SetHideOnClose(bool val)	{ s_hideonclose = val; }
	static bool		DoAutoConnect()			{ return s_autoconnect; }
	static void		SetAutoConnect(bool inautoconnect)
						{s_autoconnect = inautoconnect; }
	static bool		AddServersFromServer()		{ return s_addserversfromserver; }
	static void		SetAddServersFromServer(bool val) { s_addserversfromserver = val; }
	static bool		AddServersFromClient()		{ return s_addserversfromclient; }
	static void		SetAddServersFromClient(bool val) { s_addserversfromclient = val; }
	static uint16		GetTrafficOMeterInterval()	{ return s_trafficOMeterInterval; }
	static void		SetTrafficOMeterInterval(uint16 in)
						{ s_trafficOMeterInterval = in; }
	static uint16		GetStatsInterval()		{ return s_statsInterval;}
	static void		SetStatsInterval(uint16 in)	{ s_statsInterval = in; }
	static bool		IsConfirmExitEnabled()		{ return s_confirmExit; }
	static bool		FilterLanIPs()			{ return s_filterLanIP; }
	static void		SetFilterLanIPs(bool val)	{ s_filterLanIP = val; }
	static bool		ParanoidFilter()			{ return s_paranoidfilter; }
	static void		SetParanoidFilter(bool val)	{ s_paranoidfilter = val; }
	static bool		IsOnlineSignatureEnabled()	{ return s_onlineSig; }
	static void		SetOnlineSignatureEnabled(bool val) { s_onlineSig = val; }
	static uint32		GetMaxGraphUploadRate()		{ return s_maxGraphUploadRate; }
	static uint32		GetMaxGraphDownloadRate()	{ return s_maxGraphDownloadRate; }
	static void		SetMaxGraphUploadRate(uint32 in){ s_maxGraphUploadRate=in; }
	static void		SetMaxGraphDownloadRate(uint32 in)
						{ s_maxGraphDownloadRate = in; }

	static uint16		GetMaxDownload()		{ return s_maxdownload; }
	static uint16		GetMaxConnections()		{ return s_maxconnections; }
	static uint16		GetMaxSourcePerFile()		{ return s_maxsourceperfile; }
	static uint16		GetMaxSourcePerFileSoft() {
					uint16 temp = (uint16)(s_maxsourceperfile*0.9);
					if( temp > 1000 ) return 1000;
                                        return temp; }
	static uint16		GetMaxSourcePerFileUDP() {
					uint16 temp = (uint16)(s_maxsourceperfile*0.75);
					if( temp > 100 ) return 100;
                                        return temp; }
	static uint16		GetDeadserverRetries()		{ return s_deadserverretries; }
	static void		SetDeadserverRetries(uint16 val) { s_deadserverretries = val; }
	static uint32		GetServerKeepAliveTimeout()	{ return s_dwServerKeepAliveTimeoutMins*60000; }
	static void		SetServerKeepAliveTimeout(uint32 val)	{ s_dwServerKeepAliveTimeoutMins = val/60000; }

	static const wxString&	GetLanguageID()			{ return s_languageID; }
	static void		SetLanguageID(const wxString& new_id)	{ s_languageID = new_id; }
	static uint8		CanSeeShares()			{ return s_iSeeShares; }
	static void		SetCanSeeShares(uint8 val)	{ s_iSeeShares = val; }

	static uint8		GetStatsMax()			{ return s_statsMax; }
	static bool		UseFlatBar()			{ return (s_depth3D==0); }
	static uint8		GetStatsAverageMinutes()	{ return s_statsAverageMinutes; }
	static void		SetStatsAverageMinutes(uint8 in){ s_statsAverageMinutes = in; }

	static bool		GetStartMinimized()		{ return s_startMinimized; }
	static void		SetStartMinimized(bool instartMinimized)
					{ s_startMinimized = instartMinimized;}
	static bool		GetSmartIdCheck()		{ return s_smartidcheck; }
	static void		SetSmartIdCheck( bool in_smartidcheck )
						{ s_smartidcheck = in_smartidcheck; }
	static uint8		GetSmartIdState()		{ return s_smartidstate; }
	static void		SetSmartIdState( uint8 in_smartidstate )
						{ s_smartidstate = in_smartidstate; }
	static bool		GetVerbose()			{ return s_bVerbose; }
	static void		SetVerbose(bool val)		{ s_bVerbose = val; }
	static bool		GetVerboseLogfile()			{ return s_bVerboseLogfile; }
	static void		SetVerboseLogfile(bool val)		{ s_bVerboseLogfile = val; }
	static bool		GetPreviewPrio()		{ return s_bpreviewprio; }
	static void		SetPreviewPrio(bool in)		{ s_bpreviewprio = in; }
	static bool		StartNextFile()			{ return s_bstartnextfile; }
	static bool		StartNextFileSame()		{ return s_bstartnextfilesame; }
	static bool		StartNextFileAlpha()		{ return s_bstartnextfilealpha; }
	static void		SetStartNextFile(bool val)	{ s_bstartnextfile = val; }
	static void		SetStartNextFileSame(bool val)	{ s_bstartnextfilesame = val; }
	static void		SetStartNextFileAlpha(bool val)	{ s_bstartnextfilealpha = val; }
	static bool		ShowOverhead()			{ return s_bshowoverhead; }
	static void		SetNewAutoUp(bool m_bInUAP)	{ s_bUAP = m_bInUAP; }
	static bool		GetNewAutoUp()			{ return s_bUAP; }
	static void		SetNewAutoDown(bool m_bInDAP)	{ s_bDAP = m_bInDAP; }
	static bool		GetNewAutoDown()		{ return s_bDAP; }

	static const wxString&	GetVideoPlayer()		{ return s_VideoPlayer; }

	static uint32		GetFileBufferSize()		{ return s_iFileBufferSize*15000; }
	static void		SetFileBufferSize(uint32 val)	{ s_iFileBufferSize = val/15000; }
	static uint32		GetQueueSize()			{ return s_iQueueSize*100; }
	static void		SetQueueSize(uint32 val)	{ s_iQueueSize = val/100; }

	static uint8		Get3DDepth()			{ return s_depth3D;}
	static bool		AddNewFilesPaused()		{ return s_addnewfilespaused; }
	static void		SetAddNewFilesPaused(bool val)	{ s_addnewfilespaused = val; }

	static void		SetMaxConsPerFive(int in)	{ s_MaxConperFive=in; }

	static uint16		GetMaxConperFive()		{ return s_MaxConperFive; }
	static uint16		GetDefaultMaxConperFive();

	static bool		IsSafeServerConnectEnabled()	{ return s_safeServerConnect; }
	static void		SetSafeServerConnectEnabled(bool val) { s_safeServerConnect = val; }

	static bool		IsCheckDiskspaceEnabled()			{ return s_checkDiskspace; }
	static void		SetCheckDiskspaceEnabled(bool val)	{ s_checkDiskspace = val; }
	static uint32	GetMinFreeDiskSpaceMB()				{ return s_uMinFreeDiskSpace; }
	static uint64	GetMinFreeDiskSpace()				{ return s_uMinFreeDiskSpace * 1048576ull; }
	static void		SetMinFreeDiskSpaceMB(uint32 val)	{ s_uMinFreeDiskSpace = val; }

	static const wxString&	GetYourHostname()		{ return s_yourHostname; }
	static void		SetYourHostname(const wxString& s)	{ s_yourHostname = s; }

	static void		SetMaxUpload(uint16 in);
	static void		SetMaxDownload(uint16 in);
	static void		SetSlotAllocation(uint16 in)	{ s_slotallocation = (in >= 1) ? in : 1; };

	typedef std::vector<CPath> PathList;
	PathList shareddir_list;

	wxArrayString adresses_list;

	static bool		AutoConnectStaticOnly()		{ return s_autoconnectstaticonly; }
	static void		SetAutoConnectStaticOnly(bool val) { s_autoconnectstaticonly = val; }
	static bool		GetUPnPEnabled()		{ return s_UPnPEnabled; }
	static void		SetUPnPEnabled(bool val)	{ s_UPnPEnabled = val; }
	static bool		GetUPnPECEnabled()		{ return s_UPnPECEnabled; }
	static void		SetUPnPECEnabled(bool val)	{ s_UPnPECEnabled = val; }
	static bool		GetUPnPWebServerEnabled()	{ return s_UPnPWebServerEnabled; }
	static void		SetUPnPWebServerEnabled(bool val){ s_UPnPWebServerEnabled = val; }
	static uint16		GetUPnPTCPPort()		{ return s_UPnPTCPPort; }
	static void		SetUPnPTCPPort(uint16 val)	{ s_UPnPTCPPort = val; }
	static bool		IsManualHighPrio()		{ return s_bmanualhighprio; }
	static void		SetManualHighPrio(bool val)	{ s_bmanualhighprio = val; }
	void			LoadCats();
	static const wxString&	GetDateTimeFormat()		{ return s_datetimeformat; }
	// Download Categories
	uint32			AddCat(Category_Struct* cat);
	void			RemoveCat(size_t index);
	uint32			GetCatCount();
	Category_Struct* GetCategory(size_t index);
	const CPath&	GetCatPath(uint8 index);
	uint32			GetCatColor(size_t index);
	bool			CreateCategory(Category_Struct *& category, const wxString& name, const CPath& path,
						const wxString& comment, uint32 color, uint8 prio);
	bool			UpdateCategory(uint8 cat, const wxString& name, const CPath& path,
						const wxString& comment, uint32 color, uint8 prio);

	static AllCategoryFilter	GetAllcatFilter()		{ return s_allcatFilter; }
	static void		SetAllcatFilter(AllCategoryFilter in)	{ s_allcatFilter = in; }

	static bool		ShowAllNotCats()		{ return s_showAllNotCats; }

	// WebServer
	static uint16		GetWSPort()			{ return s_nWebPort; }
	static void		SetWSPort(uint16 uPort)		{ s_nWebPort=uPort; }
	static uint16		GetWebUPnPTCPPort()		{ return s_nWebUPnPTCPPort; }
	static void		SetWebUPnPTCPPort(uint16 val)	{ s_nWebUPnPTCPPort = val; }
	static const wxString&	GetWSPass()			{ return s_sWebPassword; }
	static void		SetWSPass(const wxString& pass)	{ s_sWebPassword = pass; }
	static const wxString&	GetWSPath()			{ return s_sWebPath; }
	static void		SetWSPath(const wxString& path)	{ s_sWebPath = path; }
	static bool		GetWSIsEnabled()		{ return s_bWebEnabled; }
	static void		SetWSIsEnabled(bool bEnable)	{ s_bWebEnabled=bEnable; }
	static bool		GetWebUseGzip()			{ return s_bWebUseGzip; }
	static void		SetWebUseGzip(bool bUse)	{ s_bWebUseGzip=bUse; }
	static uint32		GetWebPageRefresh()		{ return s_nWebPageRefresh; }
	static void		SetWebPageRefresh(uint32 nRefresh) { s_nWebPageRefresh=nRefresh; }
	static bool		GetWSIsLowUserEnabled()		{ return s_bWebLowEnabled; }
	static void		SetWSIsLowUserEnabled(bool in)	{ s_bWebLowEnabled=in; }
	static const wxString&	GetWSLowPass()			{ return s_sWebLowPassword; }
	static void		SetWSLowPass(const wxString& pass)	{ s_sWebLowPassword = pass; }
	static const wxString&	GetWebTemplate()		{ return s_WebTemplate; }
	static void		SetWebTemplate(const wxString& val) { s_WebTemplate = val; }

	static void		SetMaxSourcesPerFile(uint16 in) { s_maxsourceperfile=in;}
	static void		SetMaxConnections(uint16 in)	{ s_maxconnections =in;}

	static bool		ShowCatTabInfos()		{ return s_showCatTabInfos; }
	static void		ShowCatTabInfos(bool in)	{ s_showCatTabInfos=in; }

	// External Connections
	static bool		AcceptExternalConnections()	{ return s_AcceptExternalConnections; }
	static void			EnableExternalConnections( bool val ) { s_AcceptExternalConnections = val; }
	static const wxString&	GetECAddress()			{ return s_ECAddr; }
	static uint32		ECPort()			{ return s_ECPort; }
	static void			SetECPort(uint32 val) { s_ECPort = val; }
	static const wxString&	ECPassword()			{ return s_ECPassword; }
	static void		SetECPass(const wxString& pass)	{ s_ECPassword = pass; }
	static bool		IsTransmitOnlyUploadingClients() { return s_TransmitOnlyUploadingClients; }

	// Fast ED2K Links Handler Toggling
	static bool		GetFED2KLH()			{ return s_FastED2KLinksHandler; }

	// Ip filter
	static bool		IsFilteringClients()		{ return s_IPFilterClients; }
	static void		SetFilteringClients(bool val);
	static bool		IsFilteringServers()		{ return s_IPFilterServers; }
	static void		SetFilteringServers(bool val);
	static uint8		GetIPFilterLevel()		{ return s_filterlevel;}
	static void		SetIPFilterLevel(uint8 level);
	static bool		IPFilterAutoLoad()		{ return s_IPFilterAutoLoad; }
	static void		SetIPFilterAutoLoad(bool val)	{ s_IPFilterAutoLoad = val; }
	static const wxString&	IPFilterURL()			{ return s_IPFilterURL; }
	static void		SetIPFilterURL(const wxString& url)	{ s_IPFilterURL = url; }
	static bool		UseIPFilterSystem()		{ return s_IPFilterSys; }
	static void		SetIPFilterSystem(bool val)	{ s_IPFilterSys = val; }

	// Source seeds On/Off
	static bool		GetSrcSeedsOn()			{ return s_UseSrcSeeds; }
	static void		SetSrcSeedsOn(bool val)		{ s_UseSrcSeeds = val; }

	static bool		IsSecureIdentEnabled()			{ return s_SecIdent; }
	static void		SetSecureIdentEnabled(bool val)	{ s_SecIdent = val; }

	static bool		GetExtractMetaData()			{ return s_ExtractMetaData; }
	static void		SetExtractMetaData(bool val)	{ s_ExtractMetaData = val; }

	static bool		ShowProgBar()			{ return s_ProgBar; }
	static bool		ShowPercent()			{ return s_Percent; }

	static bool		GetAllocFullFile()		{ return s_allocFullFile; };
	static void		SetAllocFullFile(bool val)	{ s_allocFullFile = val; }

	static bool		CreateFilesSparse()		{ return s_createFilesSparse; }
	// Beware! This function reverts the value it gets, that's why the name is also different!
	// In EC we send/receive the reverted value, that's the reason for a reverse setter.
	static void		CreateFilesNormal(bool val)	{ s_createFilesSparse = !val; }

	static wxString		GetBrowser();

	static const wxString&	GetSkin()			{ return s_Skin; }

	static bool		VerticalToolbar()		{ return s_ToolbarOrientation; }

	static const CPath&	GetOSDir()			{ return s_OSDirectory; }
	static uint16		GetOSUpdate()			{ return s_OSUpdate; }

	static uint8		GetToolTipDelay()		{ return s_iToolDelayTime; }

	static void		UnsetAutoServerStart();
	static void		CheckUlDlRatio();

	static void BuildItemList( const wxString& appdir );
	static void EraseItemList();

	static void LoadAllItems(wxConfigBase* cfg);
	static void SaveAllItems(wxConfigBase* cfg);

#ifndef __SVN__
	static bool		ShowVersionOnTitle()		{ return s_showVersionOnTitle; }
#else
	static bool		ShowVersionOnTitle()		{ return true; }
#endif
	static uint8_t		GetShowRatesOnTitle()		{ return s_showRatesOnTitle; }
	static void		SetShowRatesOnTitle(uint8_t val) { s_showRatesOnTitle = val; }

	// Message Filters

	static bool		MustFilterMessages()		{ return s_MustFilterMessages; }
	static void		SetMustFilterMessages(bool val)	{ s_MustFilterMessages = val; }
	static bool		IsFilterAllMessages()		{ return s_FilterAllMessages; }
	static void		SetFilterAllMessages(bool val)	{ s_FilterAllMessages = val; }
	static bool		MsgOnlyFriends()		{ return s_msgonlyfriends;}
	static void		SetMsgOnlyFriends(bool val)	{ s_msgonlyfriends = val; }
	static bool		MsgOnlySecure()			{ return s_msgsecure;}
	static void		SetMsgOnlySecure(bool val)	{ s_msgsecure = val; }
	static bool		IsFilterByKeywords()		{ return s_FilterSomeMessages; }
	static void		SetFilterByKeywords(bool val)	{ s_FilterSomeMessages = val; }
	static const wxString&	GetMessageFilterString()	{ return s_MessageFilterString; }
	static void		SetMessageFilterString(const wxString& val) { s_MessageFilterString = val; }
	static bool		IsMessageFiltered(const wxString& message);
	static bool		ShowMessagesInLog()		{ return s_ShowMessagesInLog; }
	static bool		IsAdvancedSpamfilterEnabled()	{ return s_IsAdvancedSpamfilterEnabled;}
	static bool		IsChatCaptchaEnabled()	{ return IsAdvancedSpamfilterEnabled() && s_IsChatCaptchaEnabled; }

	static bool		FilterComments()		{ return s_FilterComments; }
	static void		SetFilterComments(bool val)	{ s_FilterComments = val; }
	static const wxString&	GetCommentFilterString()	{ return s_CommentFilterString; }
	static void		SetCommentFilterString(const wxString& val) { s_CommentFilterString = val; }
	static bool		IsCommentFiltered(const wxString& comment);

	// Can't have it return a reference, will need a pointer later.
	static const CProxyData *GetProxyData()			{ return &s_ProxyData; }

	// Hidden files

	static bool ShareHiddenFiles() { return s_ShareHiddenFiles; }
	static void SetShareHiddenFiles(bool val) { s_ShareHiddenFiles = val; }

	static bool AutoSortDownload()		{ return s_AutoSortDownload; }
	static bool AutoSortDownload(bool val)	{ bool tmp = s_AutoSortDownload; s_AutoSortDownload = val; return tmp; }

	// Version check

	static bool GetCheckNewVersion() { return s_NewVersionCheck; }
	static void SetCheckNewVersion(bool val) { s_NewVersionCheck = val; }

	// Networks
	static bool GetNetworkKademlia()		{ return s_ConnectToKad; }
	static void SetNetworkKademlia(bool val)	{ s_ConnectToKad = val; }
	static bool GetNetworkED2K()			{ return s_ConnectToED2K; }
	static void SetNetworkED2K(bool val)		{ s_ConnectToED2K = val; }

	// Statistics
	static unsigned		GetMaxClientVersions()		{ return s_maxClientVersions; }

	// Dropping slow sources
	static bool GetDropSlowSources()					{ return s_DropSlowSources; }

	// server.met and nodes.dat urls
	static const wxString& GetKadNodesUrl() { return s_KadURL; }
	static void SetKadNodesUrl(const wxString& url) { s_KadURL = url; }

	static const wxString& GetEd2kServersUrl() { return s_Ed2kURL; }
	static void SetEd2kServersUrl(const wxString& url) { s_Ed2kURL = url; }

	// Crypt
	static bool		IsClientCryptLayerSupported()		{return s_IsClientCryptLayerSupported;}
	static bool		IsClientCryptLayerRequested()		{return IsClientCryptLayerSupported() && s_bCryptLayerRequested;}
	static bool		IsClientCryptLayerRequired()		{return IsClientCryptLayerRequested() && s_IsClientCryptLayerRequired;}
	static bool		IsClientCryptLayerRequiredStrict()	{return false;} // not even incoming test connections will be answered
	static bool		IsServerCryptLayerUDPEnabled()		{return IsClientCryptLayerSupported();}
	static bool		IsServerCryptLayerTCPRequested()	{return IsClientCryptLayerRequested();}
	static bool		IsServerCryptLayerTCPRequired()		{return IsClientCryptLayerRequired();}
	static uint32	GetKadUDPKey()						{return s_dwKadUDPKey;}
	static uint8	GetCryptTCPPaddingLength()			{return s_byCryptTCPPaddingLength;}

	static void		SetClientCryptLayerSupported(bool v)	{s_IsClientCryptLayerSupported = v;}
	static void		SetClientCryptLayerRequested(bool v)	{s_bCryptLayerRequested = v; }
	static void		SetClientCryptLayerRequired(bool v)		{s_IsClientCryptLayerRequired = v;}

	// GeoIP
	static bool				IsGeoIPEnabled()		{return s_GeoIPEnabled;}
	static void				SetGeoIPEnabled(bool v)	{s_GeoIPEnabled = v;}
	static const wxString&	GetGeoIPUpdateUrl()		{return s_GeoIPUpdateUrl;}

	// Stats server
	static const wxString&	GetStatsServerName()		{return s_StatsServerName;}
	static const wxString&	GetStatsServerURL()		{return s_StatsServerURL;}

	// HTTP download
	static wxString	GetLastHTTPDownloadURL(uint8 t);
	static void		SetLastHTTPDownloadURL(uint8 t, const wxString& val);

	// Sleep
	static bool		GetPreventSleepWhileDownloading() { return s_preventSleepWhileDownloading; }
	static void		SetPreventSleepWhileDownloading(bool status) { s_preventSleepWhileDownloading = status; }
protected:
	static	int32 GetRecommendedMaxConnections();

	//! Temporary storage for statistic-colors.
	static unsigned long	s_colors[cntStatColors];
	//! Reference for checking if the colors has changed.
	static unsigned long	s_colors_ref[cntStatColors];

	typedef std::vector<Cfg_Base*>			CFGList;
	typedef std::map<int, Cfg_Base*>		CFGMap;
	typedef std::vector<Category_Struct*>	CatList;


	static CFGMap	s_CfgList;
	static CFGList	s_MiscList;
	CatList			m_CatList;

private:
	void LoadPreferences();
	void SavePreferences();

protected:
	static wxString	s_configDir;

////////////// USER
	static wxString	s_nick;

	static CMD4Hash s_userhash;

	static Cfg_Lang_Base * s_cfgLang;

////////////// CONNECTION
	static uint16	s_maxupload;
	static uint16	s_maxdownload;
	static uint16	s_slotallocation;
	static wxString s_Addr;
	static uint16	s_port;
	static uint16	s_udpport;
	static bool	s_UDPEnable;
	static uint16	s_maxconnections;
	static bool	s_reconnect;
	static bool	s_autoconnect;
	static bool	s_autoconnectstaticonly;
	static bool	s_UPnPEnabled;
	static bool	s_UPnPECEnabled;
	static bool	s_UPnPWebServerEnabled;
	static uint16	s_UPnPTCPPort;

////////////// PROXY
	static CProxyData s_ProxyData;

////////////// SERVERS
	static bool	s_autoserverlist;
	static bool	s_deadserver;

////////////// FILES
	static CPath	s_incomingdir;
	static CPath	s_tempdir;
	static bool	s_ICH;
	static bool	s_AICHTrustEveryHash;

////////////// GUI
	static uint8	s_depth3D;

	static bool	s_scorsystem;
	static bool	s_hideonclose;
	static bool	s_mintotray;
	static bool	s_notify;
	static bool	s_trayiconenabled;
	static bool	s_addnewfilespaused;
	static bool	s_addserversfromserver;
	static bool	s_addserversfromclient;
	static uint16	s_maxsourceperfile;
	static uint16	s_trafficOMeterInterval;
	static uint16	s_statsInterval;
	static uint32	s_maxGraphDownloadRate;
	static uint32	s_maxGraphUploadRate;
	static bool	s_confirmExit;


	static bool	s_filterLanIP;
	static bool	s_paranoidfilter;
	static bool	s_onlineSig;

	static wxString	s_languageID;
	static uint8	s_iSeeShares;		// 0=everybody 1=friends only 2=noone
	static uint8	s_iToolDelayTime;	// tooltip delay time in seconds
	static uint8	s_splitterbarPosition;
	static uint16	s_deadserverretries;
	static uint32	s_dwServerKeepAliveTimeoutMins;

	static uint8	s_statsMax;
	static uint8	s_statsAverageMinutes;

	static bool	s_bpreviewprio;
	static bool	s_smartidcheck;
	static uint8	s_smartidstate;
	static bool	s_safeServerConnect;
	static bool	s_startMinimized;
	static uint16	s_MaxConperFive;
	static bool	s_checkDiskspace;
	static uint32	s_uMinFreeDiskSpace;
	static wxString	s_yourHostname;
	static bool	s_bVerbose;
	static bool s_bVerboseLogfile;
	static bool	s_bmanualhighprio;
	static bool	s_bstartnextfile;
	static bool	s_bstartnextfilesame;
	static bool	s_bstartnextfilealpha;
	static bool	s_bshowoverhead;
	static bool	s_bDAP;
	static bool	s_bUAP;

#ifndef __SVN__
	static bool	s_showVersionOnTitle;
#endif
	static uint8_t	s_showRatesOnTitle;	// 0=no, 1=after app name, 2=before app name

	static wxString	s_VideoPlayer;
	static bool	s_showAllNotCats;

	static bool	s_msgonlyfriends;
	static bool	s_msgsecure;

	static uint8	s_iFileBufferSize;
	static uint8	s_iQueueSize;

	static wxString	s_datetimeformat;

	static bool	s_ToolbarOrientation;

	// Web Server [kuchin]
	static wxString	s_sWebPassword;
	static wxString	s_sWebPath;
	static wxString	s_sWebLowPassword;
	static uint16	s_nWebPort;
	static uint16	s_nWebUPnPTCPPort;
	static bool	s_bWebEnabled;
	static bool	s_bWebUseGzip;
	static uint32	s_nWebPageRefresh;
	static bool	s_bWebLowEnabled;
	static wxString s_WebTemplate;

	static bool	s_showCatTabInfos;
	static AllCategoryFilter s_allcatFilter;

	// Kry - external connections
	static bool	s_AcceptExternalConnections;
	static wxString s_ECAddr;
	static uint32	s_ECPort;
	static wxString	s_ECPassword;
	static bool		s_TransmitOnlyUploadingClients;

	// Kry - IPFilter
	static bool	s_IPFilterClients;
	static bool	s_IPFilterServers;
	static uint8	s_filterlevel;
	static bool	s_IPFilterAutoLoad;
	static wxString s_IPFilterURL;
	static bool	s_IPFilterSys;

	// Kry - Source seeds on/off
	static bool	s_UseSrcSeeds;

	static bool	s_ProgBar;
	static bool	s_Percent;

	static bool s_SecIdent;

	static bool	s_ExtractMetaData;

	static bool	s_allocFullFile;
	static bool	s_createFilesSparse;

	static wxString	s_CustomBrowser;
	static bool	s_BrowserTab;     // Jacobo221 - Open in tabs if possible

	static CPath	s_OSDirectory;
	static uint16	s_OSUpdate;

	static wxString	s_Skin;

	static bool	s_FastED2KLinksHandler;	// Madcat - Toggle Fast ED2K Links Handler

	// Message Filtering
	static bool	s_MustFilterMessages;
	static wxString	s_MessageFilterString;
	static bool	s_FilterAllMessages;
	static bool	s_FilterSomeMessages;
	static bool	s_ShowMessagesInLog;
	static bool	s_IsAdvancedSpamfilterEnabled;
	static bool	s_IsChatCaptchaEnabled;

	static bool	s_FilterComments;
	static wxString	s_CommentFilterString;


	// Hidden files sharing
	static bool	s_ShareHiddenFiles;

	static bool s_AutoSortDownload;

	// Version check
	static bool s_NewVersionCheck;

	// Kad
	static bool s_ConnectToKad;
	static bool s_ConnectToED2K;

	// Statistics
	static	unsigned	s_maxClientVersions;	// 0 = unlimited

	// Drop slow sources if needed
	static bool s_DropSlowSources;

	static wxString s_Ed2kURL;
	static wxString s_KadURL;

	// Crypt
	static bool s_IsClientCryptLayerSupported;
	static bool s_IsClientCryptLayerRequired;
	static bool s_bCryptLayerRequested;
	static uint32	s_dwKadUDPKey;
	static uint8 s_byCryptTCPPaddingLength;

	// GeoIP
	static bool s_GeoIPEnabled;
	static wxString s_GeoIPUpdateUrl;

	// Sleep vetoing
	static bool s_preventSleepWhileDownloading;

	// Stats server
	static wxString s_StatsServerName;
	static wxString s_StatsServerURL;
};


#endif // PREFERENCES_H
// File_checked_for_headers
