// This file is part of aMule Project
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


#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "types.h"		// Needed for int8, int16, int32, int64, uint8, uint16, uint32 and uint64

#include "CMD4Hash.h"
#include "PrefsUnifiedDlg.h"			// Needed for UNIFIED_PREF_HANDLING
#include <wx/dynarray.h>

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/string.h>		// Needed for wxArrayString

enum EViewSharedFilesAccess{
	vsfaEverybody = 0,
	vsfaFriends = 1,
	vsfaNobody = 2
};

#define DEFAULT_COL_SIZE 65535

// DO NOT EDIT VALUES like making a uint16 to uint32, or insert any value. ONLY append new vars
#pragma pack(1)
struct Preferences_Ext_Struct{
	int8	version;
	unsigned char	userhash[16];
	WINDOWPLACEMENT EmuleWindowPlacement;
};
#pragma pack()

#pragma pack(1)
struct Category_Struct{
	wxString	incomingpath;
	wxString	title;
	wxString	comment;
	DWORD	color;
	uint8	prio;
};
#pragma pack()

#undef Bool	// Yeah right.

#define cntStatColors 13

WX_DECLARE_OBJARRAY(Category_Struct*, ArrayOfCategory_Struct);

class CPreferences{
public:
	enum Table { tableDownload, tableUpload, tableQueue, tableSearch,
		tableShared, tableServer, tableClientList, tableNone };

	friend class PrefsUnifiedDlg;

	CPreferences();
	~CPreferences();

	bool	Save();
	void	SaveCats();

	static int8	Score() {return s_scorsystem;}
	static bool	Reconnect() {return s_reconnect;}
	static int8	DeadServer() {return s_deadserver;}
	static const wxString&	GetUserNick() {return s_nick;}
	static void	SetUploadlimit(uint16 in) { s_maxupload=in;}
	static void	SetDownloadlimit(uint16 in) { s_maxdownload=in;}

	static uint16	GetPort() {return s_port;}
	static uint16	GetUDPPort() {return s_udpport;}
	static const wxString&	GetIncomingDir() {return s_incomingdir;}
	static const wxString&	GetTempDir() {return s_tempdir;}
	const CMD4Hash&	GetUserHash() {return m_userhash;}
	static uint16	GetMaxUpload() {return	s_maxupload;}
	static uint16	GetSlotAllocation() {return	s_slotallocation;}
	static bool	IsICHEnabled() {return s_ICH;}
	static bool	AutoServerlist() {return s_autoserverlist;}
	static bool	UpdateNotify() {return s_updatenotify;}
	static bool	DoMinToTray() {return s_mintotray;}
	static bool	DoAutoConnect() {return s_autoconnect;}
	static void	SetAutoConnect( bool inautoconnect) {s_autoconnect = inautoconnect;}
	static bool	AddServersFromServer() {return s_addserversfromserver;}
	static bool	AddServersFromClient() {return s_addserversfromclient;}
	static uint16	GetTrafficOMeterInterval() { return s_trafficOMeterInterval;}
	static void	SetTrafficOMeterInterval(int16 in) { s_trafficOMeterInterval=in;}
	static uint16	GetStatsInterval() { return s_statsInterval;}
	static void	SetStatsInterval(int16 in) { s_statsInterval=in;}
	static void	Add2TotalDownloaded(uint64 in) {s_totalDownloadedBytes+=in;}
	static void	Add2TotalUploaded(uint64 in) {s_totalUploadedBytes+=in;}
	static uint64  GetTotalDownloaded() {return s_totalDownloadedBytes;}
	static uint64	GetTotalUploaded() {return s_totalUploadedBytes;}
	static bool	IsConfirmExitEnabled() {return s_confirmExit;}
	static bool	UseSplashScreen() {return s_splashscreen;}
	static bool	FilterBadIPs() {return s_filterBadIP;}
	static bool	IsOnlineSignatureEnabled()  {return s_onlineSig;}
	static int32	GetMaxGraphUploadRate()  {return s_maxGraphUploadRate;}
	static int32	GetMaxGraphDownloadRate()  {return s_maxGraphDownloadRate;}
	static void	SetMaxGraphUploadRate(int32 in) {s_maxGraphUploadRate=in;}
	static void	SetMaxGraphDownloadRate(int32 in) {s_maxGraphDownloadRate=in;}

	static uint16	GetMaxDownload()  { return s_maxdownload; }
	static uint16	GetMaxConnections() {return	s_maxconnections;}
	static uint16	GetMaxSourcePerFile() {return s_maxsourceperfile;}
	static uint16	GetMaxSourcePerFileSoft() {uint16 temp = (uint16)(s_maxsourceperfile*0.9);
						if( temp > 1000 ) return 1000;
                                            	return temp;}
	static uint16	GetMaxSourcePerFileUDP() {uint16 temp = (uint16)(s_maxsourceperfile*0.75);
						if( temp > 100 ) return 100;
                                            	return temp;}
	static uint16	GetDeadserverRetries() {return s_deadserverretries;}
	static DWORD	GetServerKeepAliveTimeout()  {return s_dwServerKeepAliveTimeoutMins*60000;}
	
	static int32     GetColumnWidth (Table t, int index);
	static bool    GetColumnHidden(Table t, int index);
	static int32     GetColumnOrder (Table t, int index);
	static void	SetColumnWidth (Table t, int index, int32 width);
	static void	SetColumnHidden(Table t, int index, bool bHidden);
	static void	SetColumnOrder (Table t, INT *piOrder);

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	static int32	GetColumnSortItem 	(Table t);
	static bool	GetColumnSortAscending 	(Table t);
	static void	SetColumnSortItem 	(Table t, int32 sortItem);
	static void	SetColumnSortAscending 	(Table t, bool sortAscending);

	static WORD	GetLanguageID() {return s_languageID;}
	static void	SetLanguageID(WORD new_id) {s_languageID = new_id;}	
	static int8	CanSeeShares(void) {return s_iSeeShares;}

	static int8	GetStatsMax()  {return s_statsMax;}
	static int8	UseFlatBar()  {return (s_depth3D==0);}
	static int8	GetStatsAverageMinutes()  {return s_statsAverageMinutes;}
	static void	SetStatsAverageMinutes(int8 in) {s_statsAverageMinutes=in;}

	static bool	GetNotifierPopOnImportantError() {return s_notifierImportantError;}

	static bool	GetStartMinimized() {return s_startMinimized;}
	static void	SetStartMinimized( bool instartMinimized) {s_startMinimized = instartMinimized;}
	static bool	GetSmartIdCheck()	 {return s_smartidcheck;}
	static void	SetSmartIdCheck( bool in_smartidcheck ) {s_smartidcheck = in_smartidcheck;}
	static uint8	GetSmartIdState() {return s_smartidstate;}
	static void	SetSmartIdState( uint8 in_smartidstate ) { s_smartidstate = in_smartidstate;}
	static bool	GetVerbose() {return s_bVerbose;}
	static bool	GetPreviewPrio() {return s_bpreviewprio;}
	static void	SetPreviewPrio(bool in) {s_bpreviewprio=in;}
	static bool	GetUpdateQueueList()  {return s_bupdatequeuelist;}
	static void	SetUpdateQueueList(bool new_state) {s_bupdatequeuelist = new_state;}
	static bool	TransferFullChunks()  {return s_btransferfullchunks;}
	static void	SetTransferFullChunks( bool m_bintransferfullchunks ) {s_btransferfullchunks = m_bintransferfullchunks;}
	static bool	StartNextFile() {return s_bstartnextfile;}
	static bool	ShowOverhead() {return s_bshowoverhead;}
	static void	SetNewAutoUp(bool m_bInUAP) {s_bUAP = m_bInUAP;}
	static bool	GetNewAutoUp() {return s_bUAP;}
	static void	SetNewAutoDown(bool m_bInDAP) {s_bDAP = m_bInDAP;}
	static bool	GetNewAutoDown() {return s_bDAP;}
	static bool	UseCreditSystem() {return s_bCreditSystem;}
	static void	SetCreditSystem(bool m_bInCreditSystem) {s_bCreditSystem = m_bInCreditSystem;}

	static const wxString&	GetVideoPlayer() { return s_VideoPlayer;}

	static uint32	GetFileBufferSize() {return s_iFileBufferSize*15000;}
	static uint32	GetQueueSize() {return s_iQueueSize*100;}

	// Barry
	static uint16	Get3DDepth() { return s_depth3D;}
	static bool	AddNewFilesPaused() {return s_addnewfilespaused;}

	static void	SetMaxConsPerFive(int in) {s_MaxConperFive=in;}

	static uint16	GetMaxConperFive() {return s_MaxConperFive;}
	static uint16	GetDefaultMaxConperFive();

	static bool	IsSafeServerConnectEnabled() {return s_safeServerConnect;}
	static bool	IsMoviePreviewBackup() {return s_moviePreviewBackup;}
	
	static bool	IsCheckDiskspaceEnabled() {return s_checkDiskspace != 0;}
	static int32	GetMinFreeDiskSpace() {return s_uMinFreeDiskSpace;}

	static const wxString&	GetYourHostname() {return s_yourHostname;}

	// quick-speed changer [xrmb]
	static void	SetMaxUpload(uint16 in)  {s_maxupload =in;};
	static void	SetMaxDownload(uint16 in) {s_maxdownload=in; };
	static void	SetSlotAllocation(uint16 in) {s_slotallocation=in; };

	wxArrayString shareddir_list;
	wxArrayString adresses_list;

	static void 	SetLanguage();
	static bool 	AutoConnectStaticOnly() {return s_autoconnectstaticonly;}	
	static int32	GetIPFilterLevel() { return s_filterlevel;}
	void	LoadCats();
	static const wxString&	GetDateTimeFormat() { return s_datetimeformat;}
	// Download Categories (Ornis)
	int32	AddCat(Category_Struct* cat) { catMap.Add(cat); return catMap.GetCount()-1;}
	void	RemoveCat(size_t index);
	uint32	GetCatCount() { return catMap.GetCount();}
	Category_Struct* GetCategory(size_t index) { if (index<catMap.GetCount()) return catMap[index]; else return NULL;}

	const wxString&	GetCatPath(uint8 index) { return catMap[index]->incomingpath;}

	DWORD	GetCatColor(size_t index) { if (index<catMap.GetCount()) return catMap[index]->color; else return 0;}

	static int32	GetAllcatType() { return s_allcatType;}
	static void	SetAllcatType(int32 in) { s_allcatType=in; }
	static bool	ShowAllNotCats() { return s_showAllNotCats;}

	// WebServer
	static uint16	GetWSPort() { return s_nWebPort; }
	static void	SetWSPort(uint16 uPort) { s_nWebPort=uPort; }
	static const wxString&	GetWSPass() { return s_sWebPassword; }
	static bool	GetWSIsEnabled() { return s_bWebEnabled; }
	static void	SetWSIsEnabled(bool bEnable) { s_bWebEnabled=bEnable; }
	static bool	GetWebUseGzip() { return s_bWebUseGzip; }
	static void	SetWebUseGzip(bool bUse) { s_bWebUseGzip=bUse; }
	static int32	GetWebPageRefresh() { return s_nWebPageRefresh; }
	static void	SetWebPageRefresh(int nRefresh) { s_nWebPageRefresh=nRefresh; }
	static bool	GetWSIsLowUserEnabled() { return s_bWebLowEnabled; }
	static void	SetWSIsLowUserEnabled(bool in) { s_bWebLowEnabled=in; }
	static const wxString&	GetWSLowPass() { return s_sWebLowPassword; }

	static void	SetMaxSourcesPerFile(uint16 in) { s_maxsourceperfile=in;}
	static void	SetMaxConnections(uint16 in) { s_maxconnections =in;}
	
	static bool	MsgOnlyFriends() { return s_msgonlyfriends;}
	static bool	MsgOnlySecure() { return s_msgsecure;}


	static int32 	GetDesktopMode() {return s_desktopMode;}
	static void 	SetDesktopMode(int mode) {s_desktopMode=mode;}

	static bool	ShowCatTabInfos() { return s_showCatTabInfos;}
	static void	ShowCatTabInfos(bool in) { s_showCatTabInfos=in;}
	
	// Madcat - Sources Dropping Tweaks
	static bool	DropNoNeededSources() { return s_NoNeededSources > 0; }
	static bool    SwapNoNeededSources() { return s_NoNeededSources == 2; }
	static bool	DropFullQueueSources() { return s_DropFullQueueSources; }
	static bool	DropHighQueueRankingSources() { return s_DropHighQueueRankingSources; }
	static int32	HighQueueRanking() { return s_HighQueueRanking; }
	static int32	GetAutoDropTimer() { return s_AutoDropTimer; }
	
	// Kry - External Connections
	static bool 	AcceptExternalConnections() { return s_AcceptExternalConnections; }
	static bool 	ECUseTCPPort() { return s_ECUseTCPPort; }
	static int32	ECPort() { return s_ECPort; }
	static const wxString&	ECPassword() { return s_ECPassword; }
	// Madcat - Fast ED2K Links Handler Toggling
	static bool	GetFED2KLH() { return s_FastED2KLinksHandler; }

	static bool BDlgTabsOnTop() { return true; }
	
	// Kry - Ip filter On/Off
	static bool GetIPFilterOn() { return s_IPFilterOn; }
	static void SetIPFilterOn(bool val) { s_IPFilterOn = val; }

	// Kry - Source seeds On/Off
	static bool GetSrcSeedsOn() { return s_UseSrcSeeds; }
	
	// Kry - Safe Max Connections
	static bool GetSafeMaxConn() { return s_UseSafeMaxConn; }
	
	static bool GetVerbosePacketError() { return s_VerbosePacketError; }
	
	static bool IsSecureIdentEnabled() { return s_SecIdent; }
	
	static bool GetExtractMetaData() { return s_ExtractMetaData; }
	
	static bool ShowProgBar() { return s_ProgBar; }
	static bool ShowPercent() { return s_Percent; }	
	
	static bool	GetAllocFullPart() { return s_AllocFullPart; };
	static bool	GetAllocFullChunk() { return s_AllocFullChunk; };

	static wxString GetBrowser();
	
	static const wxString& GetSkinFile() { return s_SkinFile; }
	
	static bool	UseSkin() { return s_UseSkinFile; }
	
	static const wxString& GetOSDir() {return s_OSDirectory;};
	
protected:
	void	CreateUserHash();
	void	SetStandartValues();
	static int32 GetRecommendedMaxConnections(); 

private:
	Preferences_Ext_Struct* prefsExt;

	CMD4Hash m_userhash;
	WORD m_wWinVer;

	void LoadPreferences();
	void SavePreferences();

	ArrayOfCategory_Struct catMap;

	// deadlake PROXYSUPPORT
	bool m_UseProxyListenPort;
	uint16	ListenPort;

protected:
	// Preference vars:
	typedef int16 Bool;	// an ugly way of appearing to be bool but being writeable to file as integer

////////////// USER
	static wxString	s_nick;


////////////// CONNECTION
	static uint16	s_maxupload;
	static uint16	s_maxdownload;
	static uint16	s_slotallocation;
	static uint16	s_port;
	static uint16	s_udpport;
	static bool		s_UDPDisable;
	static uint16	s_maxconnections;
	static bool		s_reconnect;
	static bool		s_autoconnect;
	static bool		s_autoconnectstaticonly;


////////////// SERVERS
	static bool		s_autoserverlist;
	static bool		s_deadserver;


////////////// FILES
	static wxString	s_incomingdir;
	static wxString	s_tempdir;
	static bool		s_ICH;
	
////////////// MISC
	static int8		s_versioncheckdays;



////////////// GUI
	static int16	s_downloadColumnWidths[13];
	static Bool		s_downloadColumnHidden[13];
	static int16	s_downloadColumnOrder[13];
	static int16	s_uploadColumnWidths[11];
	static Bool		s_uploadColumnHidden[11];
	static int16	s_uploadColumnOrder[11];
	static int16	s_queueColumnWidths[11];
	static Bool		s_queueColumnHidden[11];
	static int16	s_queueColumnOrder[11];
	static int16	s_searchColumnWidths[5];
	static Bool		s_searchColumnHidden[5];
	static int16	s_searchColumnOrder[5];
	static int16	s_sharedColumnWidths[12];
	static Bool		s_sharedColumnHidden[12];
	static int16	s_sharedColumnOrder[12];
	static int16	s_serverColumnWidths[12];
	static Bool		s_serverColumnHidden[12];
	static int16 	s_serverColumnOrder[12];
	static int16	s_clientListColumnWidths[8];
	static Bool		s_clientListColumnHidden[8];
	static int16 	s_clientListColumnOrder[8];
	static int8		s_depth3D;
	
	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	static int32	s_tableSortItemDownload;
	static int32	s_tableSortItemUpload;
	static int32	s_tableSortItemQueue;
	static int32	s_tableSortItemSearch;
	static int32	s_tableSortItemShared;
	static int32	s_tableSortItemServer;
	static int32	s_tableSortItemClientList;
	static bool		s_tableSortAscendingDownload;
	static bool		s_tableSortAscendingUpload;
	static bool		s_tableSortAscendingQueue;
	static bool		s_tableSortAscendingSearch;
	static bool		s_tableSortAscendingShared;
	static bool		s_tableSortAscendingServer;
	static bool		s_tableSortAscendingClientList;






	static bool		s_scorsystem;
	static bool		s_updatenotify;
	static bool		s_mintotray;
	static bool		s_autotakeed2klinks;
	static bool		s_addnewfilespaused;
	static bool		s_addserversfromserver;
	static bool		s_addserversfromclient;
	static int16	s_maxsourceperfile;
	static int16	s_trafficOMeterInterval;
	static int16	s_statsInterval;
	static int		s_maxGraphDownloadRate;
	static int		s_maxGraphUploadRate;
	static bool		s_beepOnError;
	static bool		s_confirmExit;


	static bool		s_splashscreen;
	static bool		s_filterBadIP;
	static bool		s_onlineSig;

	static uint64  	s_totalDownloadedBytes;
	static uint64	s_totalUploadedBytes;
	static uint16	s_languageID;
	static bool		s_transferDoubleclick;
	static int8		s_iSeeShares;		// 0=everybody 1=friends only 2=noone
	static int8		s_iToolDelayTime;	// tooltip delay time in seconds
	static bool		s_bringtoforeground;
	static int8		s_splitterbarPosition;
	static uint16	s_deadserverretries;
	static uint32	s_dwServerKeepAliveTimeoutMins;

	static uint8	s_statsMax;
	static int8		s_statsAverageMinutes;

	static bool		s_useDownloadNotifier;
	static bool		s_useChatNotifier;
	static bool		s_useLogNotifier;	
	static bool		s_useSoundInNotifier;
	static bool		s_sendEmailNotifier;
	static bool		s_notifierPopsEveryChatMsg;
	static bool		s_notifierImportantError;
	static bool		s_notifierNewVersion;
	static wxString	s_notifierSoundFilePath;

	static bool		s_bpreviewprio;
	static bool		s_smartidcheck;
	static uint8	s_smartidstate;
	static bool		s_safeServerConnect;
	static bool		s_startMinimized;
	static uint16	s_MaxConperFive;
	static bool		s_checkDiskspace;
	static uint32 	s_uMinFreeDiskSpace;
	static wxString	s_yourHostname;
	static bool		s_bVerbose;
	static bool		s_bupdatequeuelist;
	static bool		s_bmanualhighprio;
	static bool		s_btransferfullchunks;
	static bool		s_bstartnextfile;
	static bool		s_bshowoverhead;
	static bool		s_bDAP;
	static bool		s_bUAP;
	static bool		s_bDisableKnownClientList;
	static bool		s_bDisableQueueList;




	static bool		s_showRatesInTitle;

	static wxString	s_VideoPlayer;
	static bool		s_moviePreviewBackup;
	static bool		s_indicateratings;
	static bool		s_showAllNotCats;
	static bool		s_watchclipboard;
	static bool		s_filterserverbyip;
	static bool		s_bFirstStart;
	static bool		s_bCreditSystem;

	static bool		s_msgonlyfriends;
	static bool		s_msgsecure;

	static uint8	s_filterlevel;
	static uint8	s_iFileBufferSize;
	static uint8	s_iQueueSize;

	static uint16	s_maxmsgsessions;
	static uint32	s_versioncheckLastAutomatic;
	static wxString	s_datetimeformat;

	// Web Server [kuchin]
	static wxString	s_sWebPassword;
	static wxString	s_sWebLowPassword;
	static uint16	s_nWebPort;
	static bool		s_bWebEnabled;
	static bool		s_bWebUseGzip;
	static int32	s_nWebPageRefresh;
	static bool		s_bWebLowEnabled;
	static wxString	s_sWebResDir;

	static wxString	s_sTemplateFile;
	static bool		s_bIsASCWOP;

	static bool		s_showCatTabInfos;
	static bool		s_resumeSameCat;
	static bool		s_dontRecreateGraphs;
	static int32	s_allcatType;
	
	static int32 	s_desktopMode;
	
	// Madcat - Sources Dropping Tweaks
	static uint8	s_NoNeededSources; // 0: Keep, 1: Drop, 2:Swap
	static bool		s_DropFullQueueSources;
	static bool		s_DropHighQueueRankingSources;
	static int32	s_HighQueueRanking;
	static int32	s_AutoDropTimer;
	
	// Kry - external connections
	static bool 	s_AcceptExternalConnections;
	static bool 	s_ECUseTCPPort;
	static int32	s_ECPort;
	static wxString	s_ECPassword;
	
	// Kry - IPFilter On/Off
	static bool		s_IPFilterOn;
	
	// Kry - Source seeds on/off
	static bool		s_UseSrcSeeds;
	
	// Kry - Safe Max Connections
	static bool		s_UseSafeMaxConn;
	
	static bool		s_VerbosePacketError;

	static bool		s_ProgBar;
	static bool		s_Percent;	
	
	static bool		s_SecIdent;
	
	static bool		s_ExtractMetaData;
	
	static bool		s_AllocFullPart;
	static bool		s_AllocFullChunk;
	
	static uint16	s_Browser;
	static wxString	s_CustomBrowser;
	static bool		s_BrowserTab;     // Jacobo221 - Open in tabs if possible
	
	static wxString	s_OSDirectory;
	
	static wxString	s_SkinFile;
	
	static bool		s_UseSkinFile;
	
	static bool		s_FastED2KLinksHandler;	// Madcat - Toggle Fast ED2K Links Handler
	static bool		s_bDlgTabsOnTop;			// Creteil: dlg aesthetics


};

#endif // PREFERENCES_H
