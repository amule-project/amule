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

#include <ctime>		// Needed for time(2)

#include "types.h"		// Needed for int8, int16, int32, int64, uint8, uint16, uint32 and uint64

#include "MD5Sum.h"		// Needed for MD5Sum
#include "CMD4Hash.h"
#include "otherfunctions.h"
#include "PrefsUnifiedDlg.h"			// Needed for UNIFIED_PREF_HANDLING
#include <wx/dynarray.h>

class Rse;

#ifndef MAX_PATH
#include <climits>		// This is not really garanteed to define _POSIX_PATH_MAX or anything.
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/string.h>		// Needed for wxArrayString
#ifdef MAXPATHLEN
#define MAX_PATH MAXPATHLEN
#elif defined(MAX_PATH_LEN)
#define MAX_PATH MAX_PATH_LEN
#elif defined(MAXPATH)
#define MAX_PATH MAXPATH
#elif defined(_MAXPATH)
#define MAX_PATH _MAXPATH
#elif defined(PATH_MAX)
#define MAX_PATH PATH_MAX
#elif defined(_POSIX_PATH_MAX)
#define MAX_PATH _POSIX_PATH_MAX
#else
#define MAX_PATH 512
#endif
#endif

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

// deadlake PROXYSUPPORT
struct ProxySettings{
	uint16 type;
	uint16 port;
	char name[50];
	char user[50];
	char password[50];
	bool EnablePassword;
	bool UseProxy;
};

#pragma pack(1)
struct Category_Struct{
	wxString	incomingpath;
	wxString	title;
	wxString	comment;
	DWORD	color;
	uint8	prio;
};

#pragma pack(1)
#undef Bool	// Yeah right.

#define cntStatColors 13

struct Preferences_Struct{
	typedef int16 Bool;	// an ugly way of appearing to be bool but being writeable to file as integer

	char	nick[255];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	slotallocation;
	uint16	port;
	uint16	udpport;
	bool		UDPDisable;
	uint16	maxconnections;
	bool	reconnect;
	bool	deadserver;
	bool	scorsystem;
	char	incomingdir[MAX_PATH];
	char	tempdir[MAX_PATH];
	bool	ICH;
	bool	autoserverlist;
	bool	updatenotify;
	bool	mintotray;
	bool	autoconnect;
	bool	autoconnectstaticonly; // Barry
	bool	autotakeed2klinks;     // Barry
	bool	addnewfilespaused;     // Barry
	int8	depth3D;			   // Barry
	bool	addserversfromserver;
	bool	addserversfromclient;
	int16	maxsourceperfile;
	int16	trafficOMeterInterval;
	int16	statsInterval;
	WINDOWPLACEMENT EmuleWindowPlacement;
	int	maxGraphDownloadRate;
	int	maxGraphUploadRate;
	bool	beepOnError;
	bool	confirmExit;
	int16	downloadColumnWidths[13];
	Bool	downloadColumnHidden[13];
	int16	downloadColumnOrder[13];
	int16	uploadColumnWidths[8];
	Bool	uploadColumnHidden[8];
	int16	uploadColumnOrder[8];
	int16	queueColumnWidths[10];
	Bool	queueColumnHidden[10];
	int16	queueColumnOrder[10];
	int16	searchColumnWidths[5];
	Bool	searchColumnHidden[5];
	int16	searchColumnOrder[5];
	int16	sharedColumnWidths[12];
	Bool	sharedColumnHidden[12];
	int16	sharedColumnOrder[12];
	int16	serverColumnWidths[12];
	Bool	serverColumnHidden[12];
	int16 	serverColumnOrder[12];
	int16	clientListColumnWidths[8];
	Bool	clientListColumnHidden[8];
	int16 	clientListColumnOrder[8];

	bool	splashscreen;
	bool	filterBadIP;
	bool	onlineSig;

	uint64  totalDownloadedBytes;
	uint64	totalUploadedBytes;
	uint16	languageID;
	bool	transferDoubleclick;
	int8	m_iSeeShares;		// 0=everybody 1=friends only 2=noone
	int8	m_iToolDelayTime;	// tooltip delay time in seconds
	bool	bringtoforeground;
	int8	splitterbarPosition;
	uint16	deadserverretries;
	uint32	m_dwServerKeepAliveTimeoutMins;

	uint8   statsMax;
	int8	statsAverageMinutes;

	bool    useDownloadNotifier;
	bool    useChatNotifier;
	bool    useLogNotifier;	
	bool    useSoundInNotifier;
	bool	sendEmailNotifier;
	bool    notifierPopsEveryChatMsg;
	bool	notifierImportantError;
	bool	notifierNewVersion;
	char    notifierSoundFilePath[510];

	char	m_sircserver[50];
	char	m_sircnick[30];
	char	m_sircchannamefilter[50];
	bool	m_bircaddtimestamp;
	bool	m_bircusechanfilter;
	uint16	m_iircchanneluserfilter;
	char	m_sircperformstring[255];
	bool	m_bircuseperform;
	bool	m_birclistonconnect;
	bool	m_bircacceptlinks;
	bool	m_bircignoreinfomessage;
	bool	m_bircignoreemuleprotoinfomessage;

	bool	m_bpreviewprio;
	bool	smartidcheck;
	uint8	smartidstate;
	bool	safeServerConnect;
	bool	startMinimized;
	uint16	MaxConperFive;
	bool	checkDiskspace;
	uint32 m_uMinFreeDiskSpace;
	char	yourHostname[127];
	bool	m_bVerbose;
	bool	m_bupdatequeuelist;
	bool	m_bmanualhighprio;
	bool	m_btransferfullchunks;
	bool	m_bstartnextfile;
	bool	m_bshowoverhead;
	bool	m_bDAP;
	bool	m_bUAP;
	bool	m_bDisableKnownClientList;
	bool	m_bDisableQueueList;

	int8	versioncheckdays;

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	int32		tableSortItemDownload;
	int32		tableSortItemUpload;
	int32		tableSortItemQueue;
	int32		tableSortItemSearch;
	int32		tableSortItemShared;
	int32		tableSortItemServer;
	int32		tableSortItemClientList;
	bool	tableSortAscendingDownload;
	bool	tableSortAscendingUpload;
	bool	tableSortAscendingQueue;
	bool	tableSortAscendingSearch;
	bool	tableSortAscendingShared;
	bool	tableSortAscendingServer;
	bool	tableSortAscendingClientList;

	bool	showRatesInTitle;

	char	TxtEditor[256];
	char	VideoPlayer[256];
	bool	moviePreviewBackup;
	bool	indicateratings;
	bool	showAllNotCats;
	bool	watchclipboard;
	bool	filterserverbyip;
	bool	m_bFirstStart;
	bool	m_bCreditSystem;

	bool	log2disk;
	bool	debug2disk;
	int32		iMaxLogMessages;
	bool	scheduler;
	bool	dontcompressavi;
	bool	msgonlyfriends;
	bool	msgsecure;

	uint8	filterlevel;
	uint8	m_iFileBufferSize;
	uint8	m_iQueueSize;

	uint16	maxmsgsessions;
	uint32	versioncheckLastAutomatic;
	char messageFilter[512];
	char commentFilter[512];
	char notifierConfiguration[510];
	char datetimeformat[32];
	char m_szLRUServermetURL[512];

	// Web Server [kuchin]
	char		m_sWebPassword[256];
	char		m_sWebLowPassword[256];
	uint16		m_nWebPort;
	bool		m_bWebEnabled;
	bool		m_bWebUseGzip;
	int32			m_nWebPageRefresh;
	bool		m_bWebLowEnabled;
	char		m_sWebResDir[MAX_PATH];

	char		m_sTemplateFile[MAX_PATH];
	ProxySettings proxy; // deadlake PROXYSUPPORT
	bool		m_bIsASCWOP;

	bool		showCatTabInfos;
	bool		resumeSameCat;
	bool		dontRecreateGraphs;
	int32		allcatType;
	
	int32 desktopMode;
	
	// Madcat - Sources Dropping Tweaks
	uint8	NoNeededSources; // 0: Keep, 1: Drop, 2:Swap
	bool	DropFullQueueSources;
	bool	DropHighQueueRankingSources;
	int32	HighQueueRanking;
	int32	AutoDropTimer;
	
	// Kry - external connections
	bool 	AcceptExternalConnections;
	bool 	ECUseTCPPort;
	int32	ECPort;
	char	ECPassword[256];
	
	// Kry - IPFilter On/Off
	bool		IPFilterOn;
	
	// Kry - Source seeds on/off
	bool		UseSrcSeeds;
	
	// Kry - Safe Max Connections
	bool		UseSafeMaxConn;
	
	bool		VerbosePacketError;

	bool		ProgBar;
	bool		Percent;	
	
	bool		SecIdent;
	
	bool		ExtractMetaData;
	
	bool		AllocFullPart;
	bool		AllocFullChunk;
	
	uint16	Browser;
	char		CustomBrowser[256];
	bool            BrowserTab;     // Jacobo221 - Open in tabs if possible
	
	char		OSDirectory[512];
	
	char		SkinFile[1024];
	
	bool		UseSkinFile;
	
	bool		FastED2KLinksHandler;	// Madcat - Toggle Fast ED2K Links Handler
	bool		bDlgTabsOnTop;			// Creteil: dlg aesthetics
};
#pragma pack()

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

	int8	Score() const {return prefs->scorsystem;}
	bool	Reconnect() const {return prefs->reconnect;}
	int8	DeadServer() const {return prefs->deadserver;}
	const char*	GetUserNick() const {return prefs->nick;}
	void	SetUploadlimit(uint16 in) { prefs->maxupload=in;}
	void	SetDownloadlimit(uint16 in) { prefs->maxdownload=in;}

	uint16	GetPort() const {return prefs->port;}
	uint16	GetUDPPort() const {return prefs->udpport;}
	const char*	GetIncomingDir() const {return prefs->incomingdir;}
	const char*	GetTempDir() const {return prefs->tempdir;}
	const CMD4Hash&	GetUserHash() const {return m_userhash;}
	uint16	GetMaxUpload() const {return	prefs->maxupload;}
	uint16	GetSlotAllocation() const {return	prefs->slotallocation;}
	bool	IsICHEnabled() const {return prefs->ICH;}
	bool	AutoServerlist() const {return prefs->autoserverlist;}
	bool	UpdateNotify() const {return prefs->updatenotify;}
	bool	DoMinToTray() const {return prefs->mintotray;}
	bool	DoAutoConnect() const {return prefs->autoconnect;}
	void	SetAutoConnect( bool inautoconnect) {prefs->autoconnect = inautoconnect;}
	bool	AddServersFromServer() const {return prefs->addserversfromserver;}
	bool	AddServersFromClient() const {return prefs->addserversfromclient;}
	uint16	GetTrafficOMeterInterval() const { return prefs->trafficOMeterInterval;}
	void	SetTrafficOMeterInterval(int16 in) { prefs->trafficOMeterInterval=in;}
	uint16	GetStatsInterval() const { return prefs->statsInterval;}
	void	SetStatsInterval(int16 in) { prefs->statsInterval=in;}
	void	Add2TotalDownloaded(uint64 in) {prefs->totalDownloadedBytes+=in;}
	void	Add2TotalUploaded(uint64 in) {prefs->totalUploadedBytes+=in;}
	uint64  GetTotalDownloaded() const {return prefs->totalDownloadedBytes;}
	uint64	GetTotalUploaded() const {return prefs->totalUploadedBytes;}
	bool	IsConfirmExitEnabled() const {return prefs->confirmExit;}
	bool	UseSplashScreen() const {return prefs->splashscreen;}
	bool	FilterBadIPs() const {return prefs->filterBadIP;}
	bool	IsOnlineSignatureEnabled() const {return prefs->onlineSig;}
	int32	GetMaxGraphUploadRate() const {return prefs->maxGraphUploadRate;}
	int32	GetMaxGraphDownloadRate() const {return prefs->maxGraphDownloadRate;}
	void	SetMaxGraphUploadRate(int32 in) {prefs->maxGraphUploadRate=in;}
	void	SetMaxGraphDownloadRate(int32 in) {prefs->maxGraphDownloadRate=in;}

	uint16	GetMaxDownload() const { return prefs->maxdownload; }
	uint16	GetMaxConnections() const {return	prefs->maxconnections;}
	uint16	GetMaxSourcePerFile() const {return prefs->maxsourceperfile;}
	uint16	GetMaxSourcePerFileSoft() const {uint16 temp = (uint16)(prefs->maxsourceperfile*0.9);
						if( temp > 1000 ) return 1000;
                                            	return temp;}
	uint16	GetMaxSourcePerFileUDP() const {uint16 temp = (uint16)(prefs->maxsourceperfile*0.75);
						if( temp > 100 ) return 100;
                                            	return temp;}
	uint16	GetDeadserverRetries() const {return prefs->deadserverretries;}
	DWORD	GetServerKeepAliveTimeout() const {return prefs->m_dwServerKeepAliveTimeoutMins*60000;}
	
	int32     GetColumnWidth (Table t, int index) const;
	bool    GetColumnHidden(Table t, int index) const;
	int32     GetColumnOrder (Table t, int index) const;
	void	SetColumnWidth (Table t, int index, int32 width);
	void	SetColumnHidden(Table t, int index, bool bHidden);
	void	SetColumnOrder (Table t, INT *piOrder);

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	int32	GetColumnSortItem 	(Table t) const;
	bool	GetColumnSortAscending 	(Table t) const;
	void	SetColumnSortItem 	(Table t, int32 sortItem);
	void	SetColumnSortAscending 	(Table t, bool sortAscending);

	WORD	GetLanguageID() const {return prefs->languageID;}
	void	SetLanguageID(WORD new_id) {prefs->languageID = new_id;}	
	int8	CanSeeShares(void) const {return prefs->m_iSeeShares;}

	int8	GetStatsMax() const {return prefs->statsMax;}
	int8	UseFlatBar() const {return (prefs->depth3D==0);}
	int8	GetStatsAverageMinutes() const {return prefs->statsAverageMinutes;}
	void	SetStatsAverageMinutes(int8 in) {prefs->statsAverageMinutes=in;}

	bool	GetNotifierPopOnImportantError() const {return prefs->notifierImportantError;}

	bool	GetStartMinimized() const {return prefs->startMinimized;}
	void	SetStartMinimized( bool instartMinimized) {prefs->startMinimized = instartMinimized;}
	bool	GetSmartIdCheck()	 const {return prefs->smartidcheck;}
	void	SetSmartIdCheck( bool in_smartidcheck ) {prefs->smartidcheck = in_smartidcheck;}
	uint8	GetSmartIdState() const {return prefs->smartidstate;}
	void	SetSmartIdState( uint8 in_smartidstate ) { prefs->smartidstate = in_smartidstate;}
	bool	GetVerbose() const {return prefs->m_bVerbose;}
	bool	GetPreviewPrio() const {return prefs->m_bpreviewprio;}
	void	SetPreviewPrio(bool in) {prefs->m_bpreviewprio=in;}
	bool	GetUpdateQueueList() const {return prefs->m_bupdatequeuelist;}
	void	SetUpdateQueueList(bool new_state) {prefs->m_bupdatequeuelist = new_state;}
	bool	TransferFullChunks() const {return prefs->m_btransferfullchunks;}
	void	SetTransferFullChunks( bool m_bintransferfullchunks ) {prefs->m_btransferfullchunks = m_bintransferfullchunks;}
	bool	StartNextFile() const {return prefs->m_bstartnextfile;}
	bool	ShowOverhead() const {return prefs->m_bshowoverhead;}
	void	SetNewAutoUp(bool m_bInUAP) {prefs->m_bUAP = m_bInUAP;}
	bool	GetNewAutoUp() const {return prefs->m_bUAP;}
	void	SetNewAutoDown(bool m_bInDAP) {prefs->m_bDAP = m_bInDAP;}
	bool	GetNewAutoDown() const {return prefs->m_bDAP;}
	bool	UseCreditSystem() const {return prefs->m_bCreditSystem;}
	void	SetCreditSystem(bool m_bInCreditSystem) {prefs->m_bCreditSystem = m_bInCreditSystem;}

	wxString	GetVideoPlayer() const {if (strlen(prefs->VideoPlayer)==0) return wxT(""); else return wxString(char2unicode(prefs->VideoPlayer));}

	uint32	GetFileBufferSize() const {return prefs->m_iFileBufferSize*15000;}
	uint32	GetQueueSize() const {return prefs->m_iQueueSize*100;}

	// Barry
	uint16	Get3DDepth() const { return prefs->depth3D;}
	bool	AddNewFilesPaused() const {return prefs->addnewfilespaused;}

	void	SetMaxConsPerFive(int in) {prefs->MaxConperFive=in;}

	uint16	GetMaxConperFive() const {return prefs->MaxConperFive;}
	uint16	GetDefaultMaxConperFive();

	bool	IsSafeServerConnectEnabled() const {return prefs->safeServerConnect;}
	bool	IsMoviePreviewBackup() const {return prefs->moviePreviewBackup;}
	
	bool	IsCheckDiskspaceEnabled() const {return prefs->checkDiskspace != 0;}
	int32	GetMinFreeDiskSpace() const {return prefs->m_uMinFreeDiskSpace;}

	char*	GetYourHostname() const {return prefs->yourHostname;}

	// quick-speed changer [xrmb]
	void	SetMaxUpload(uint16 in)  {prefs->maxupload =in;};
	void	SetMaxDownload(uint16 in) {prefs->maxdownload=in; };
	void	SetSlotAllocation(uint16 in) {prefs->slotallocation=in; };

	wxArrayString shareddir_list;
	wxArrayString adresses_list;

	void 	SetLanguage();
	bool 	AutoConnectStaticOnly() const {return prefs->autoconnectstaticonly;}	
	int32	GetIPFilterLevel() const { return prefs->filterlevel;}
	void	LoadCats();
	wxString	GetDateTimeFormat() const { return wxString(char2unicode(prefs->datetimeformat));}
	// Download Categories (Ornis)
	int32	AddCat(Category_Struct* cat) { catMap.Add(cat); return catMap.GetCount()-1;}
	void	RemoveCat(size_t index);
	uint32	GetCatCount() const { return catMap.GetCount();}
	Category_Struct* GetCategory(size_t index) const { if (index>=0 && index<catMap.GetCount()) return catMap[index]; else return NULL;}

	const wxString&	GetCatPath(uint8 index) const { return catMap[index]->incomingpath;}

	DWORD	GetCatColor(size_t index) const { if (index>=0 && index<catMap.GetCount()) return catMap[index]->color; else return 0;}

	int32	GetAllcatType() const { return prefs->allcatType;}
	void	SetAllcatType(int32 in) const { prefs->allcatType=in; }
	bool	ShowAllNotCats() const { return prefs->showAllNotCats;}

	// WebServer
	uint16	GetWSPort() const { return prefs->m_nWebPort; }
	void	SetWSPort(uint16 uPort) { prefs->m_nWebPort=uPort; }
	wxString	GetWSPass() const { return wxString(char2unicode(prefs->m_sWebPassword)); }
	bool	GetWSIsEnabled() const { return prefs->m_bWebEnabled; }
	void	SetWSIsEnabled(bool bEnable) { prefs->m_bWebEnabled=bEnable; }
	bool	GetWebUseGzip() const { return prefs->m_bWebUseGzip; }
	void	SetWebUseGzip(bool bUse) { prefs->m_bWebUseGzip=bUse; }
	int32	GetWebPageRefresh() const { return prefs->m_nWebPageRefresh; }
	void	SetWebPageRefresh(int nRefresh) { prefs->m_nWebPageRefresh=nRefresh; }
	bool	GetWSIsLowUserEnabled() const { return prefs->m_bWebLowEnabled; }
	void	SetWSIsLowUserEnabled(bool in) { prefs->m_bWebLowEnabled=in; }
	wxString	GetWSLowPass() const { return wxString(char2unicode(prefs->m_sWebLowPassword)); }

	void	SetMaxSourcesPerFile(uint16 in) { prefs->maxsourceperfile=in;}
	void	SetMaxConnections(uint16 in) { prefs->maxconnections =in;}
	
	bool	MsgOnlyFriends() const { return prefs->msgonlyfriends;}
	bool	MsgOnlySecure() const { return prefs->msgsecure;}


	int32 	GetDesktopMode() const {return prefs->desktopMode;}
	void 	SetDesktopMode(int mode) {prefs->desktopMode=mode;}

	bool	ShowCatTabInfos() const { return prefs->showCatTabInfos;}
	void	ShowCatTabInfos(bool in) { prefs->showCatTabInfos=in;}
	
	// Madcat - Sources Dropping Tweaks
	bool	DropNoNeededSources() const { return prefs->NoNeededSources > 0; }
	bool    SwapNoNeededSources() const { return prefs->NoNeededSources == 2; }
	bool	DropFullQueueSources() const { return prefs->DropFullQueueSources; }
	bool	DropHighQueueRankingSources() const { return prefs->DropHighQueueRankingSources; }
	int32	HighQueueRanking() const { return prefs->HighQueueRanking; }
	int32	GetAutoDropTimer() const { return prefs->AutoDropTimer; }
	
	// Kry - External Connections
	bool 	AcceptExternalConnections() const { return prefs->AcceptExternalConnections; }
	bool 	ECUseTCPPort() const { return prefs->ECUseTCPPort; }
	int32	ECPort() const { return prefs->ECPort; }
	wxString	ECPassword() const { return wxString(char2unicode(prefs->ECPassword)); }
	// Madcat - Fast ED2K Links Handler Toggling
	bool	GetFED2KLH() const { return prefs->FastED2KLinksHandler; }
	bool	BDlgTabsOnTop() const { 
		#if defined(__WXMAC__) || defined(__WXMSW__) 
			return false;
		#else
			return prefs->bDlgTabsOnTop; 
		#endif
	}
	
	// Kry - Ip filter On/Off
	bool GetIPFilterOn() const { return prefs->IPFilterOn; }

	// Kry - Source seeds On/Off
	bool GetSrcSeedsOn() const { return prefs->UseSrcSeeds; }
	
	// Kry - Safe Max Connections
	bool GetSafeMaxConn() const { return prefs->UseSafeMaxConn; }
	
	bool GetVerbosePacketError() const { return prefs->VerbosePacketError; }
	
	bool IsSecureIdentEnabled() const { return prefs->SecIdent; }
	
	bool GetExtractMetaData() const { return prefs->ExtractMetaData; }
	
	bool ShowProgBar() const { return prefs->ProgBar; }
	bool ShowPercent() const { return prefs->Percent; }	
	
	bool	GetAllocFullPart() const { return prefs->AllocFullPart; };
	bool	GetAllocFullChunk() const { return prefs->AllocFullChunk; };

	wxString GetBrowser() const;
	
	wxString GetSkinFile() const { return char2unicode(prefs->SkinFile); }
	
	bool	UseSkin() const { return prefs->UseSkinFile; }
	
	char* GetOSDir() const {return prefs->OSDirectory;};
	
protected:
	void	CreateUserHash();
	void	SetStandartValues();
	static int32 GetRecommendedMaxConnections(); 

private:
	Preferences_Struct* prefs;
	Preferences_Ext_Struct* prefsExt;

	CMD4Hash m_userhash;
	WORD m_wWinVer;

	void LoadPreferences();
	void SavePreferences();

	ArrayOfCategory_Struct catMap;

	// deadlake PROXYSUPPORT
	bool m_UseProxyListenPort;
	uint16	ListenPort;

};

#endif // PREFERENCES_H
