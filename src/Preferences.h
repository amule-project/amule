//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "Types.h"			// Needed for ints
#include "MD4Hash.h"			// Needed for CMD4Hash
#include "Color.h"			// Needed for COLORREF

#include <wx/string.h>			// Needed for wxString
#if wxCHECK_VERSION(2, 5, 2)
#	include <wx/arrstr.h>		// Needed for wxArrayString
#endif

#include <map>
#include <vector>

#include "Proxy.h"

class CPreferences;
class wxConfigBase;
class wxWindow;

#define DEFAULT_COL_SIZE 65535

enum EViewSharedFilesAccess{
	vsfaEverybody = 0,
	vsfaFriends = 1,
	vsfaNobody = 2
};

struct Category_Struct
{
	wxString	incomingpath;
	wxString	title;
	wxString	comment;
	uint32		color;
	uint8		prio;
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
	virtual bool HasChanged() 			{ return m_changed; }


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



const int cntStatColors = 16;


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

	static bool		Score()				{ return s_scorsystem; }
	static void		SetScoreSystem(bool val)	{ s_scorsystem = val; }
	static bool		Reconnect()			{ return s_reconnect; }
	static void		SetReconnect(bool val)		{ s_reconnect = val; }
	static bool		DeadServer()			{ return s_deadserver; }
	static void		SetDeadServer(bool val)		{ s_deadserver = val; }
	static const wxString&	GetUserNick()			{ return s_nick; }
	static void		SetUserNick(const wxString& nick) { s_nick = nick; }

	static const wxString&	GetAddress()			{ return s_Addr; }
	static uint16		GetPort()			{ return s_port; }
	static void		SetPort(uint16 val);
	static uint16		GetUDPPort()			{ return s_udpport; }
	static uint16		GetEffectiveUDPPort()	{ return s_UDPDisable ? 0 : s_udpport; }
	static void		SetUDPPort(uint16 val)		{ s_udpport = val; }
	static bool		IsUDPDisabled()			{ return s_UDPDisable; }
	static void		SetUDPDisable(bool val)		{ s_UDPDisable = val; }
	static const wxString&	GetIncomingDir()		{ return s_incomingdir; }
	static const wxString&	GetTempDir()			{ return s_tempdir; }
	static const CMD4Hash&	GetUserHash()			{ return s_userhash; }
	static uint16		GetMaxUpload()			{ return s_maxupload; }
	static uint16		GetSlotAllocation()		{ return s_slotallocation; }
	static bool		IsICHEnabled()			{ return s_ICH; }
	static void		SetICHEnabled(bool val)		{ s_ICH = val; }
	static bool		IsTrustingEveryHash()		{ return s_AICHTrustEveryHash; }
	static void		SetTrustingEveryHash(bool val)	{ s_AICHTrustEveryHash = val; }
	static bool		AutoServerlist()		{ return s_autoserverlist; }
	static void		SetAutoServerlist(bool val)	{ s_autoserverlist = val; }
	static bool		DoMinToTray()			{ return s_mintotray; }
	static void		SetMinToTray(bool val)		{ s_mintotray = val; }
	static bool		UseTrayIcon()			{ return s_trayiconenabled; }
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
	static void		Add2TotalDownloaded(uint64 in)	{ s_totalDownloadedBytes += in; }
	static void		Add2TotalUploaded(uint64 in)	{ s_totalUploadedBytes += in; }
	static uint64		GetTotalDownloaded()		{ return s_totalDownloadedBytes; }
	static uint64		GetTotalUploaded()		{ return s_totalUploadedBytes; }
	static bool		IsConfirmExitEnabled()		{ return s_confirmExit; }
	static bool		FilterLanIPs()			{ return s_filterLanIP; }
	static void		SetFilterLanIPs(bool val)	{ s_filterLanIP = val; }
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
	static uint32	GetServerKeepAliveTimeout()	{ return s_dwServerKeepAliveTimeoutMins*60000; }
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
	static bool		GetPreviewPrio()		{ return s_bpreviewprio; }
	static void		SetPreviewPrio(bool in)		{ s_bpreviewprio = in; }
	static bool		TransferFullChunks()		{ return s_btransferfullchunks; }
	static void		SetTransferFullChunks( bool m_bintransferfullchunks ) 
					{s_btransferfullchunks = m_bintransferfullchunks; }
	static bool		StartNextFile()			{ return s_bstartnextfile; }
	static bool		StartNextFileSame()		{ return s_bstartnextfilesame; }
	static void		SetStartNextFile(bool val)	{ s_bstartnextfile = val; }
	static void		SetStartNextFileSame(bool val)	{ s_bstartnextfilesame = val; }
	static bool		ShowOverhead()			{ return s_bshowoverhead; }
	static void		SetNewAutoUp(bool m_bInUAP) 	{ s_bUAP = m_bInUAP; }
	static bool		GetNewAutoUp() 			{ return s_bUAP; }
	static void		SetNewAutoDown(bool m_bInDAP) 	{ s_bDAP = m_bInDAP; }
	static bool		GetNewAutoDown() 		{ return s_bDAP; }

	static const wxString&	GetVideoPlayer()		{ return s_VideoPlayer; }

	static uint32		GetFileBufferSize() 		{ return s_iFileBufferSize*15000; }
	static void		SetFileBufferSize(uint32 val)	{ s_iFileBufferSize = val/15000; }
	static uint32		GetQueueSize() 			{ return s_iQueueSize*100; }
	static void		SetQueueSize(uint32 val)	{ s_iQueueSize = val/100; }

	// Barry
	static uint8		Get3DDepth() 			{ return s_depth3D;}
	static bool		AddNewFilesPaused()		{ return s_addnewfilespaused; }
	static void		SetAddNewFilesPaused(bool val)	{ s_addnewfilespaused = val; }

	static void		SetMaxConsPerFive(int in)	{ s_MaxConperFive=in; }

	static uint16		GetMaxConperFive()		{ return s_MaxConperFive; }
	static uint16		GetDefaultMaxConperFive();

	static bool		IsSafeServerConnectEnabled()	{ return s_safeServerConnect; }
	static void		SetSafeServerConnectEnabled(bool val) { s_safeServerConnect = val; }
	static bool		IsMoviePreviewBackup()		{ return s_moviePreviewBackup; }
	
	static bool		IsCheckDiskspaceEnabled()	{ return s_checkDiskspace; }
	static void		SetCheckDiskspaceEnabled(bool val)	{ s_checkDiskspace = val; }
	static uint32		GetMinFreeDiskSpace()		{ return s_uMinFreeDiskSpace; }
	static void		SetMinFreeDiskSpace(uint32 val)	{ s_uMinFreeDiskSpace = val; }

	static const wxString&	GetYourHostname() 		{ return s_yourHostname; }

	static void		SetMaxUpload(uint16 in);
	static void		SetMaxDownload(uint16 in);
	static void		SetSlotAllocation(uint16 in) 	{ s_slotallocation = in; };

	wxArrayString shareddir_list;
	wxArrayString adresses_list;

	static bool 	AutoConnectStaticOnly() 	{ return s_autoconnectstaticonly; }
	static void		SetAutoConnectStaticOnly(bool val) { s_autoconnectstaticonly = val; }
	static bool		IsManualHighPrio()		{ return s_bmanualhighprio; }
	static void		SetManualHighPrio(bool val)	{ s_bmanualhighprio = val; }
	void			LoadCats();
	static const wxString&	GetDateTimeFormat() 		{ return s_datetimeformat; }
	// Download Categories (Ornis)
	uint32			AddCat(Category_Struct* cat);
	void			RemoveCat(size_t index);
	uint32			GetCatCount();
	Category_Struct* GetCategory(size_t index);
	const wxString&	GetCatPath(uint8 index);
	uint32			GetCatColor(size_t index);
	Category_Struct *CreateCategory(wxString name, wxString path, wxString comment, uint32 color, uint8 prio);
	void			UpdateCategory(uint8 cat, wxString name, wxString path, wxString comment, uint32 color, uint8 prio);

	static uint32		GetAllcatType() 		{ return s_allcatType; }
	static void		SetAllcatType(uint32 in)	{ s_allcatType = in; }
	static bool		ShowAllNotCats() 		{ return s_showAllNotCats; }

	// WebServer
	static uint16 		GetWSPort() 			{ return s_nWebPort; }
	static void		SetWSPort(uint16 uPort) 	{ s_nWebPort=uPort; }
	static const wxString&	GetWSPass() 			{ return s_sWebPassword; }
	static void		SetWSPass(const wxString& pass)	{ s_sWebPassword = pass; }
	static bool		GetWSIsEnabled() 		{ return s_bWebEnabled; }
	static void		SetWSIsEnabled(bool bEnable) 	{ s_bWebEnabled=bEnable; }
	static bool		GetWebUseGzip() 		{ return s_bWebUseGzip; }
	static void		SetWebUseGzip(bool bUse) 	{ s_bWebUseGzip=bUse; }
	static uint32 		GetWebPageRefresh() 		{ return s_nWebPageRefresh; }
	static void		SetWebPageRefresh(uint32 nRefresh) { s_nWebPageRefresh=nRefresh; }
	static bool		GetWSIsLowUserEnabled() 	{ return s_bWebLowEnabled; }
	static void		SetWSIsLowUserEnabled(bool in) 	{ s_bWebLowEnabled=in; }
	static const wxString&	GetWSLowPass() 			{ return s_sWebLowPassword; }
	static void		SetWSLowPass(const wxString& pass)	{ s_sWebLowPassword = pass; }

	static void		SetMaxSourcesPerFile(uint16 in) { s_maxsourceperfile=in;}
	static void		SetMaxConnections(uint16 in) 	{ s_maxconnections =in;}
	
	static uint32 		GetDesktopMode() 		{ return s_desktopMode; }
	static void 		SetDesktopMode(uint32 mode) 	{ s_desktopMode=mode; }

	static bool		ShowCatTabInfos() 		{ return s_showCatTabInfos; }
	static void		ShowCatTabInfos(bool in) 	{ s_showCatTabInfos=in; }
	
	// Madcat - Sources Dropping Tweaks
	static bool		DropNoNeededSources() 		{ return s_NoNeededSources > 0; }
	static bool		SwapNoNeededSources() 		{ return s_NoNeededSources == 2; }
	static uint8		GetNoNeededSources()		{ return s_NoNeededSources; }
	static void		SetNoNeededSources(uint8 val)	{ s_NoNeededSources = val; }
	static bool		DropFullQueueSources() 		{ return s_DropFullQueueSources; }
	static void		SetDropFullQueueSources(bool val) { s_DropFullQueueSources = val; }
	static bool		DropHighQueueRankingSources() 	{ return s_DropHighQueueRankingSources; }
	static void		SetDropHighQueueRankingSources(bool val) { s_DropHighQueueRankingSources = val; }
	static uint32		HighQueueRanking() 		{ return s_HighQueueRanking; }
	static void		SetHighQueueRanking(uint32 val)	{ s_HighQueueRanking = val; }
	static uint32		GetAutoDropTimer() 		{ return s_AutoDropTimer; }
	static void		SetAutoDropTimer(uint32 val)	{ s_AutoDropTimer = val; }
	
	// Kry - External Connections
	static bool 		AcceptExternalConnections()	{ return s_AcceptExternalConnections; }
	static void			EnableExternalConnections( bool val ) { s_AcceptExternalConnections = val; }
	static const wxString&	GetECAddress()			{ return s_ECAddr; }
	static uint32 		ECPort()			{ return s_ECPort; }
	static void			SetECPort(uint32 val) { s_ECPort = val; }
	static const wxString&	ECPassword()			{ return s_ECPassword; }
	// Madcat - Fast ED2K Links Handler Toggling
	static bool 		GetFED2KLH()			{ return s_FastED2KLinksHandler; }

	// Kry - Ip filter 
	static bool		GetIPFilterOn()			{ return s_IPFilterOn; }
	static void		SetIPFilterOn(bool val)		{ s_IPFilterOn = val; }
	static uint8		GetIPFilterLevel()		{ return s_filterlevel;}
	static void		SetIPFilterLevel(uint8 level);
	static bool		IPFilterAutoLoad()		{ return s_IPFilterAutoLoad; }
	static void		SetIPFilterAutoLoad(bool val)	{ s_IPFilterAutoLoad = val; }
	static const wxString&	IPFilterURL()			{ return s_IPFilterURL; }
	static void		SetIPFilterURL(const wxString& url)	{ s_IPFilterURL = url; }

	// Kry - Source seeds On/Off
	static bool		GetSrcSeedsOn() 		{ return s_UseSrcSeeds; }
	static void		SetSrcSeedsOn(bool val)		{ s_UseSrcSeeds = val; }
	
	static bool		IsSecureIdentEnabled()		{ return s_SecIdent; }
	static void		SetSecureIdentEnabled(bool val)	{ s_SecIdent = val; }
	
	static bool		GetExtractMetaData()		{ return s_ExtractMetaData; }
	static void		SetExtractMetaData(bool val)	{ s_ExtractMetaData = val; }
	
	static bool		ShowProgBar()			{ return s_ProgBar; }
	static bool		ShowPercent()			{ return s_Percent; }	
	
	static bool		GetAllocFullPart()		{ return s_AllocFullPart; };
	static void		SetAllocFullPart(bool val)	{ s_AllocFullPart = val; }
	static bool		GetAllocFullChunk()		{ return s_AllocFullChunk; };
	static void		SetAllocFullChunk(bool val)	{ s_AllocFullChunk = val; }

	static wxString 	GetBrowser();
	
	static const wxString&	GetSkinFile()			{ return s_SkinFile; }
	
	static bool		UseSkin()			{ return s_UseSkinFile; }

	static bool		VerticalToolbar() { return s_ToolbarOrientation; }
		
	static const wxString&	GetOSDir()			{ return s_OSDirectory; }
	static uint16	GetOSUpdate()			{ return s_OSUpdate; }

	static uint8		GetToolTipDelay()		{ return s_iToolDelayTime; }

	static int		GetFilePermissions();
	static void		SetFilePermissions( int perms );
	static int		GetDirPermissions();
	static void		SetDirPermissions( int perms );

	static void		UnsetAutoServerStart();
	static void		CheckUlDlRatio();
	
	static void BuildItemList( const wxString& appdir );
	static void EraseItemList();
	
	static void LoadAllItems(wxConfigBase* cfg);
	static void SaveAllItems(wxConfigBase* cfg);

	static bool 		GetShowRatesOnTitle()		{ return s_ShowRatesOnTitle; }
	
	// Message Filters
	
	static bool		MustFilterMessages()		{ return s_MustFilterMessages; }
	static void		SetMustFilterMessages(bool val)	{ s_MustFilterMessages = val; }
	static bool		IsFilterAllMessages()		{ return s_FilterAllMessages; }
	static void		SetFilterAllMessages(bool val)	{ s_FilterAllMessages = val; }
	static bool		MsgOnlyFriends() 		{ return s_msgonlyfriends;}
	static void		SetMsgOnlyFriends(bool val)	{ s_msgonlyfriends = val; }
	static bool		MsgOnlySecure() 		{ return s_msgsecure;}
	static void		SetMsgOnlySecure(bool val)	{ s_msgsecure = val; }
	static bool		IsFilterByKeywords()		{ return s_FilterSomeMessages; }
	static void		SetFilterByKeywords(bool val)	{ s_FilterSomeMessages = val; }
	static const wxString&	GetMessageFilterString()	{ return s_MessageFilterString; }
	static void		SetMessageFilterString(const wxString& val) { s_MessageFilterString = val; }
	static wxString 	MessageFilter() { 
		if (s_FilterAllMessages) { 
			return wxT("*");
		} else {
			if (s_FilterSomeMessages) {
				return s_MessageFilterString;
			} else {
				return wxEmptyString;
			}
		}
	}
	// I cant have it return a reference, I'll need a pointer later.
	static const CProxyData *GetProxyData()			{ return &s_ProxyData; }
	
	// Hidden files
	
	static bool ShareHiddenFiles() { return s_ShareHiddenFiles; }
	
	static bool AutoSortDownload() { return s_AutoSortDownload; } 
	
	// Version check
	
	static bool CheckNewVersion() { return s_NewVersionCheck; }

	// Networks
	static bool GetNetworkKademlia()		{ return s_ConnectToKad; }
	static void SetNetworkKademlia(bool val)	{ s_ConnectToKad = val; }
	static bool GetNetworkED2K()			{ return s_ConnectToED2K; }
	static void SetNetworkED2K(bool val)		{ s_ConnectToED2K = val; }

	// Statistics
	static unsigned		GetMaxClientVersions()		{ return s_maxClientVersions; }

	// Command on completion
	static bool	CommandOnCompletion()				{ return s_ExecOnCompletion; }
	static const wxString& GetCommandOnCompletion()	{ return s_ExecOnCompletionCommand; }
	
protected:
	void	CreateUserHash();
	void	SetStandartValues();
	static	int32 GetRecommendedMaxConnections();

	//! Temporary storage for statistic-colors.
	static COLORREF	s_colors[cntStatColors];
	//! Reference for checking if the colors has changed.
	static COLORREF	s_colors_ref[cntStatColors];
	 
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
////////////// USER
	static wxString	s_nick;
	
	static CMD4Hash s_userhash;

////////////// CONNECTION
	static uint16	s_maxupload;
	static uint16	s_maxdownload;
	static uint16	s_slotallocation;
	static wxString s_Addr;
	static uint16	s_port;
	static uint16	s_udpport;
	static bool	s_UDPDisable;
	static uint16	s_maxconnections;
	static bool	s_reconnect;
	static bool	s_autoconnect;
	static bool	s_autoconnectstaticonly;

////////////// PROXY
	static CProxyData s_ProxyData;

////////////// SERVERS
	static bool	s_autoserverlist;
	static bool	s_deadserver;

////////////// FILES
	static wxString	s_incomingdir;
	static wxString	s_tempdir;
	static bool	s_ICH;
	static bool	s_AICHTrustEveryHash;
	static int	s_perms_files;
	static int	s_perms_dirs;

////////////// GUI
	static uint8	s_depth3D;
	
	static bool	s_scorsystem;
	static bool	s_mintotray;
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
	static bool	s_onlineSig;

	static uint64  	s_totalDownloadedBytes;
	static uint64	s_totalUploadedBytes;
	static wxString	s_languageID;
	static bool	s_transferDoubleclick;
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
	static uint32 	s_uMinFreeDiskSpace;
	static wxString	s_yourHostname;
	static bool	s_bVerbose;
	static bool	s_bmanualhighprio;
	static bool	s_btransferfullchunks;
	static bool	s_bstartnextfile;
	static bool	s_bstartnextfilesame;	
	static bool	s_bshowoverhead;
	static bool	s_bDAP;
	static bool	s_bUAP;
	static bool	s_bDisableKnownClientList;
	static bool	s_bDisableQueueList;

	static bool	s_ShowRatesOnTitle;

	static wxString	s_VideoPlayer;
	static bool	s_moviePreviewBackup;
	static bool	s_indicateratings;
	static bool	s_showAllNotCats;
	
	static bool	s_msgonlyfriends;
	static bool	s_msgsecure;

	static uint8	s_iFileBufferSize;
	static uint8	s_iQueueSize;

	static uint16	s_maxmsgsessions;
	static wxString	s_datetimeformat;
	
	static bool	s_ToolbarOrientation;

	// Web Server [kuchin]
	static wxString	s_sWebPassword;
	static wxString	s_sWebLowPassword;
	static uint16	s_nWebPort;
	static bool	s_bWebEnabled;
	static bool	s_bWebUseGzip;
	static uint32	s_nWebPageRefresh;
	static bool	s_bWebLowEnabled;

	static bool	s_showCatTabInfos;
	static uint32	s_allcatType;
	
	static uint32 	s_desktopMode;
	
	// Madcat - Sources Dropping Tweaks
	static uint8	s_NoNeededSources; // 0: Keep, 1: Drop, 2:Swap
	static bool	s_DropFullQueueSources;
	static bool	s_DropHighQueueRankingSources;
	static uint32	s_HighQueueRanking;
	static uint32	s_AutoDropTimer;
	
	// Kry - external connections
	static bool 	s_AcceptExternalConnections;
	static wxString s_ECAddr;
	static uint32	s_ECPort;
	static wxString	s_ECPassword;
	
	// Kry - IPFilter 
	static bool	s_IPFilterOn;
	static uint8	s_filterlevel;
	static bool	s_IPFilterAutoLoad;
	static wxString s_IPFilterURL;
	
	// Kry - Source seeds on/off
	static bool	s_UseSrcSeeds;
	
	static bool	s_ProgBar;
	static bool	s_Percent;	
	
	static bool	s_SecIdent;
	
	static bool	s_ExtractMetaData;
	
	static bool	s_AllocFullPart;
	static bool	s_AllocFullChunk;
	
	static uint16	s_Browser;
	static wxString	s_CustomBrowser;
	static bool	s_BrowserTab;     // Jacobo221 - Open in tabs if possible
	
	static wxString	s_OSDirectory;
	static uint16	s_OSUpdate;
	
	static wxString	s_SkinFile;
	static bool	s_UseSkinFile;
	
	static bool	s_FastED2KLinksHandler;	// Madcat - Toggle Fast ED2K Links Handler
	
	// Message Filtering
	static bool 		s_MustFilterMessages;
	static wxString 	s_MessageFilterString;
	static bool		s_FilterAllMessages;
	static bool		s_FilterSomeMessages;
	
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

	// Exec command on completion
	static bool s_ExecOnCompletion;
	static wxString s_ExecOnCompletionCommand;
};


#endif // PREFERENCES_H
