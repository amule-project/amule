//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <ctime>		// Needed for time(2)

#include "types.h"		// Needed for int8, int16, int32, int64, uint8, uint16, uint32 and uint64

#include "CString.h"		// Needed for CStringList
#include "MD5Sum.h"		// Needed for MD5Sum
#include "PrefsUnifiedDlg.h"			// Needed for UNIFIED_PREF_HANDLING
#include <wx/dynarray.h>

class CString;
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
	char	incomingpath[MAX_PATH];
	char	title[64];
	char	comment[255];
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
	uint16	maxconnections;
	int8	reconnect;
	int8	deadserver;
	int8	scorsystem;
	char	incomingdir[MAX_PATH];
	char	tempdir[MAX_PATH];
	int8	ICH;
	int8	autoserverlist;
	int8	updatenotify;
	int8	mintotray;
	int8	autoconnect;
	int8	autoconnectstaticonly; // Barry
	int8	autotakeed2klinks;     // Barry
	int8	addnewfilespaused;     // Barry
	int8	depth3D;			   // Barry
	int8	addserversfromserver;
	int8	addserversfromclient;
	int16	maxsourceperfile;
	int16	trafficOMeterInterval;
	int16	statsInterval;
	unsigned char	userhash[16];
	WINDOWPLACEMENT EmuleWindowPlacement;
	int	maxGraphDownloadRate;
	int	maxGraphUploadRate;
	uint8	beepOnError;
	uint8	confirmExit;
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
#ifndef UNIFIED_PREF_HANDLING
	DWORD	statcolors[cntStatColors];
#endif

	uint8	splashscreen;
	uint8	filterBadIP;
	uint8	onlineSig;

	uint64  totalDownloadedBytes;
	uint64	totalUploadedBytes;
	uint16	languageID;
	int8	transferDoubleclick;
	int8	m_iSeeShares;		// 0=everybody 1=friends only 2=noone
	int8	m_iToolDelayTime;	// tooltip delay time in seconds
	int8	bringtoforeground;
	int8	splitterbarPosition;
	uint16	deadserverretries;
	uint32	m_dwServerKeepAliveTimeoutMins;

	uint8   statsMax;
	int8	statsAverageMinutes;

	int8    useDownloadNotifier;
	int8    useChatNotifier;
	int8    useLogNotifier;	
	int8    useSoundInNotifier;
	int8	sendEmailNotifier;
	int8    notifierPopsEveryChatMsg;
	int8	notifierImportantError;
	int8	notifierNewVersion;
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
	bool	DropNoNeededSources;
	bool    SwapNoNeededSources;
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

	bool		ProgBar;
	bool		Percent;	
	
	bool		SecIdent;
	
	bool		ExtractMetaData;
	
	bool	FastED2KLinksHandler;	// Madcat - Toggle Fast ED2K Links Handler
	bool	bDlgTabsOnTop;			// Creteil: dlg aesthetics
};
#pragma pack()

#pragma pack(1)
struct Preferences_Import19c_Struct{
	int8	version;
	char	nick[50];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	maxconnections;
	int8	reconnect;
	int8	deadserver;
	int8	scorsystem;
	char	incomingdir[510];
	char	tempdir[510];
	int8	ICH;
	int8	autoserverlist;
	int8	updatenotify;
	int8	mintotray;
	unsigned char	userhash[16];
	int8	autoconnect;
	int8	addserversfromserver;
	int8	addserversfromclient;
};
#pragma pack()

#pragma pack(1)
struct Preferences_Import20a_Struct{
	int8	version;
	char	nick[50];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	maxconnections;
	int8	reconnect;
	int8	deadserver;
	uint16	deadserverretries;
	int8	scorsystem;
	char	incomingdir[510];
	char	tempdir[510];
	int8	ICH;
	int8	autoserverlist;
	int8	updatenotify;
	int8	mintotray;
	unsigned char	userhash[16];
	int8	autoconnect;
	int8	addserversfromserver;
	int8	addserversfromclient;
	int16	maxsourceperfile;
	int16	trafficOMeterInterval;
	int32   totalDownloaded;
	int32	totalUploaded;
	int32		maxGraphDownloadRate;
	int32		maxGraphUploadRate;
	uint8	beepOnError;
	uint8	confirmExit;
	WINDOWPLACEMENT EmuleWindowPlacement;
	int32 transferColumnWidths[9];
	int32 serverColumnWidths[8];
	uint8	splashscreen;
	uint8	filerBadIP;
};
#pragma pack()

#pragma pack(1)
struct Preferences_Import20b_Struct{
	int8	version;
	char	nick[50];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	maxconnections;
	int8	reconnect;
	int8	deadserver;
	int8	scorsystem;
	char	incomingdir[510];
	char	tempdir[510];
	int8	ICH;
	int8	autoserverlist;
	int8	updatenotify;
	int8	mintotray;
	unsigned char	userhash[16];
	int8	autoconnect;
	int8	addserversfromserver;
	int8	addserversfromclient;
	int16	maxsourceperfile;
	int16	trafficOMeterInterval;
	int32   totalDownloaded;	// outdated
	int32	totalUploaded;		// outdated
	int32	maxGraphDownloadRate;
	int32	maxGraphUploadRate;
	uint8	beepOnError;
	uint8	confirmExit;
	WINDOWPLACEMENT EmuleWindowPlacement;
	int32 	transferColumnWidths[9];
	int32 	serverColumnWidths[8];
	uint8	splashscreen;
	uint8	filerBadIP;
	int64   totalDownloadedBytes;
	int64	totalUploadedBytes;
};
#pragma pack()

WX_DECLARE_OBJARRAY(Category_Struct*, ArrayOfCategory_Struct);

class CPreferences{
public:
	enum Table { tableDownload, tableUpload, tableQueue, tableSearch,
		tableShared, tableServer, tableClientList, tableNone };

	friend class CPreferencesWnd;
	friend class CPPgGeneral;
	friend class CPPgConnection;
	friend class CPPgServer;
	friend class CPPgDirectories;
	friend class CPPgFiles;
	friend class CPPgNotify;
	friend class CPPgIRC;
	friend class Wizard;
	friend class CPPgTweaks;
	friend class CPPgDisplay;
	friend class CPPgSecurity;
	friend class CPPgScheduler;
	friend class CPPgSourcesDropping;
	friend class CPPgGuiTweaks;
	friend class PrefsUnifiedDlg;

	CPreferences();
	~CPreferences();

	char*	GetAppDir()			{return appdir;}
	bool	Save();
	void	SaveCats();

	int8	Score()				{return prefs->scorsystem;}
	bool	Reconnect()			{return prefs->reconnect;}
	int8	DeadServer()			{return prefs->deadserver;}
	char*	GetUserNick()			{return prefs->nick;}
	void	SetUserNick(CString in)		{sprintf(prefs->nick,"%s",in.GetData());}
	void	SetUploadlimit(uint16 in)	{ prefs->maxupload=in;}
	void	SetDownloadlimit(uint16 in)	{ prefs->maxdownload=in;}

	uint16	GetPort()			{return prefs->port;}
	uint16	GetUDPPort()			{return prefs->udpport;}
	char*	GetIncomingDir()		{return prefs->incomingdir;}
	char*	GetTempDir()			{return prefs->tempdir;}
	char*	GetUserHash()			{return userhash;}
	uint16	GetMaxUpload()			{return	prefs->maxupload;}
	uint16	GetSlotAllocation()		{return	prefs->slotallocation;}
	bool	IsICHEnabled()			{return prefs->ICH;}
	bool	AutoServerlist()		{return prefs->autoserverlist;}
	bool	UpdateNotify()			{return prefs->updatenotify;}
	bool	DoMinToTray()			{return prefs->mintotray;}
	bool	DoAutoConnect() 		{return prefs->autoconnect;}
	void	SetAutoConnect( bool inautoconnect)	{prefs->autoconnect = inautoconnect;}
	bool	AddServersFromServer()		{return prefs->addserversfromserver;}
	bool	AddServersFromClient()		{return prefs->addserversfromclient;}
	char*	GetLRUServermetURL()		{return prefs->m_szLRUServermetURL;}
	void	SetLRUServermetURL(const char* pszURL) {snprintf(prefs->m_szLRUServermetURL,sizeof prefs->m_szLRUServermetURL,"%s",pszURL);}
	int8*	GetMinTrayPTR() 		{return &prefs->mintotray;}
	uint16	GetTrafficOMeterInterval() 	{ return prefs->trafficOMeterInterval;}
	void	SetTrafficOMeterInterval(int16 in) { prefs->trafficOMeterInterval=in;}
	uint16	GetStatsInterval() 		{ return prefs->statsInterval;}
	void	SetStatsInterval(int16 in) 	{ prefs->statsInterval=in;}
	void	Add2TotalDownloaded(uint64 in) 	{prefs->totalDownloadedBytes+=in;}
	void	Add2TotalUploaded(uint64 in) 	{prefs->totalUploadedBytes+=in;}
	uint64  GetTotalDownloaded()		{return prefs->totalDownloadedBytes;}
	uint64	GetTotalUploaded()		{return prefs->totalUploadedBytes;}
	bool	IsErrorBeepEnabled()		{return prefs->beepOnError;}
	bool	IsConfirmExitEnabled()		{return prefs->confirmExit;}
	bool	UseSplashScreen()		{return prefs->splashscreen;}
	bool	FilterBadIPs()			{return prefs->filterBadIP;}
	bool	IsOnlineSignatureEnabled()  	{return prefs->onlineSig;}
	int32	GetMaxGraphUploadRate()		{return prefs->maxGraphUploadRate;}
	int32	GetMaxGraphDownloadRate()	{return prefs->maxGraphDownloadRate;}
	void	SetMaxGraphUploadRate(int32 in)	{prefs->maxGraphUploadRate=in;}
	void	SetMaxGraphDownloadRate(int32 in)	{prefs->maxGraphDownloadRate=in;}

	uint16	GetMaxDownload();
	uint16	GetMaxConnections()		{return	prefs->maxconnections;}
	uint16	GetMaxSourcePerFile()		{return prefs->maxsourceperfile;}
	uint16	GetMaxSourcePerFileSoft()	{uint16 temp = (uint16)(prefs->maxsourceperfile*0.9);
						if( temp > 1000 ) return 1000;
                                            	return temp;}
	uint16	GetMaxSourcePerFileUDP()	{uint16 temp = (uint16)(prefs->maxsourceperfile*0.75);
						if( temp > 100 ) return 100;
                                            	return temp;}
	uint16	GetDeadserverRetries()		{return prefs->deadserverretries;}
	DWORD	GetServerKeepAliveTimeout()	{return prefs->m_dwServerKeepAliveTimeoutMins*60000;}
	
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

	WORD	GetLanguageID()			{return prefs->languageID;}
	void	SetLanguageID(WORD new_id)			{prefs->languageID = new_id;}	
	int8	IsDoubleClickEnabled()		{return prefs->transferDoubleclick;}
	int8	CanSeeShares(void)		{return prefs->m_iSeeShares;}
	int8	GetToolTipDelay(void)		{return prefs->m_iToolDelayTime;}
	int8	IsBringToFront()		{return prefs->bringtoforeground;}

	int8    GetSplitterbarPosition()	{return prefs->splitterbarPosition;}
	void	SetSplitterbarPosition(int8 pos) {prefs->splitterbarPosition=pos;}
	int8	GetStatsMax()			{return prefs->statsMax;}
	int8	UseFlatBar()			{return (prefs->depth3D==0);}
	int8	GetStatsAverageMinutes()	{return prefs->statsAverageMinutes;}
	void	SetStatsAverageMinutes(int8 in)	{prefs->statsAverageMinutes=in;}

	bool    GetUseDownloadNotifier()	{return prefs->useDownloadNotifier;}
	bool    GetUseChatNotifier()	 	{return prefs->useChatNotifier;}
	bool    GetUseLogNotifier()		{return prefs->useLogNotifier;}
	bool    GetUseSoundInNotifier()  	{return prefs->useSoundInNotifier;}
	bool	GetSendEmailNotifier()		{return prefs->sendEmailNotifier;}
	bool    GetNotifierPopsEveryChatMsg() 	{return prefs->notifierPopsEveryChatMsg;}
	bool	GetNotifierPopOnImportantError(){return prefs->notifierImportantError;}
	bool	GetNotifierPopOnNewVersion()	{return prefs->notifierNewVersion;}
	char*   GetNotifierWavSoundPath() 	{return prefs->notifierSoundFilePath;}

	CString	GetIRCNick()				{return (CString)prefs->m_sircnick;}
	void	SetIRCNick( char in_nick[] )		{ strcpy(prefs->m_sircnick,in_nick);}
	CString	GetIRCServer()				{return (CString)prefs->m_sircserver;}
	void	SetIRCServer( char in_serv[] )		{ strcpy(prefs->m_sircserver,in_serv);}
	bool	GetIRCAddTimestamp()			{return prefs->m_bircaddtimestamp;}
	void	SetIRCAddTimestamp( bool flag )		{prefs->m_bircaddtimestamp = flag;}
	CString	GetIRCChanNameFilter()			{return (CString)prefs->m_sircchannamefilter;}
	bool	GetIRCUseChanFilter()			{return prefs->m_bircusechanfilter;}
	uint16	GetIRCChannelUserFilter()		{return	prefs->m_iircchanneluserfilter;}
	void	SetIRCChanNameFilter( char in_name[] )	{ strcpy(prefs->m_sircchannamefilter,in_name);}
	void	SetIRCUseChanFilter( bool flag )	{prefs->m_bircusechanfilter = flag;}
	void	SetIRCChanUserFilter( uint16 in_user)	{prefs->m_iircchanneluserfilter = in_user;}
	CString	GetIrcPerformString()			{return (CString)prefs->m_sircperformstring;}
	bool	GetIrcUsePerform()			{return prefs->m_bircuseperform;}
	bool	GetIRCListOnConnect()			{return prefs->m_birclistonconnect;}
	void	SetIRCListonConnect( bool flag )	{prefs->m_birclistonconnect = flag;}
	void	SetIRCPerformString( char in_perf[] )	{ strcpy(prefs->m_sircperformstring, in_perf);}
	void	SetIrcUsePerform( bool flag )		{prefs->m_bircuseperform = flag;}
	bool	GetIrcAcceptLinks()			{return prefs->m_bircacceptlinks;}
	void	SetIrcAcceptLInks( bool flag )		{prefs->m_bircacceptlinks = flag;}
	bool	GetIrcIgnoreInfoMessage()		{return prefs->m_bircignoreinfomessage;}
	void	SetIrcIgnoreInfoMessage( bool flag )	{prefs->m_bircignoreinfomessage = flag;}
	bool	GetIrcIgnoreEmuleProtoInfoMessage()	{return prefs->m_bircignoreemuleprotoinfomessage;}
	void	SetIrcIgnoreEmuleProtoInfoMessage( bool flag )	{prefs->m_bircignoreemuleprotoinfomessage = flag;}
	WORD	GetWindowsVersion();
	bool	GetStartMinimized()			{return prefs->startMinimized;}
	void	SetStartMinimized( bool instartMinimized){prefs->startMinimized = instartMinimized;}
	bool	GetSmartIdCheck()			{return prefs->smartidcheck;}
	void	SetSmartIdCheck( bool in_smartidcheck )	{prefs->smartidcheck = in_smartidcheck;}
	uint8	GetSmartIdState()			{return prefs->smartidstate;}
	void	SetSmartIdState( uint8 in_smartidstate ){prefs->smartidstate = in_smartidstate;}
	bool	GetVerbose()				{return prefs->m_bVerbose;}
	bool	GetPreviewPrio()			{return prefs->m_bpreviewprio;}
	void	SetPreviewPrio(bool in)			{prefs->m_bpreviewprio=in;}
	bool	GetUpdateQueueList()			{return prefs->m_bupdatequeuelist;}
	void	SetUpdateQueueList(bool new_state)	{prefs->m_bupdatequeuelist = new_state;}
	bool	GetManualHighPrio()			{return prefs->m_bmanualhighprio;}
	bool	TransferFullChunks()			{return prefs->m_btransferfullchunks;}
	void	SetTransferFullChunks( bool m_bintransferfullchunks )	{prefs->m_btransferfullchunks = m_bintransferfullchunks;}
	bool	StartNextFile()				{return prefs->m_bstartnextfile;}
	bool	ShowOverhead()				{return prefs->m_bshowoverhead;}
	void	SetNewAutoUp(bool m_bInUAP)		{prefs->m_bUAP = m_bInUAP;}
	bool	GetNewAutoUp()				{return prefs->m_bUAP;}
	void	SetNewAutoDown(bool m_bInDAP)		{prefs->m_bDAP = m_bInDAP;}
	bool	GetNewAutoDown()			{return prefs->m_bDAP;}
	bool	IsKnownClientListDisabled()		{return prefs->m_bDisableKnownClientList;}
	bool	IsQueueListDisabled()			{return prefs->m_bDisableQueueList;}
	bool	IsFirstStart()				{return prefs->m_bFirstStart;}
	bool	UseCreditSystem()			{return prefs->m_bCreditSystem;}
	void	SetCreditSystem(bool m_bInCreditSystem)	{prefs->m_bCreditSystem = m_bInCreditSystem;}

	char*	GetTxtEditor()				{return prefs->TxtEditor;}
	CString	GetVideoPlayer()			{if (strlen(prefs->VideoPlayer)==0) return ""; else return CString(prefs->VideoPlayer);}

	uint32	GetFileBufferSize()			{return prefs->m_iFileBufferSize*15000;}
	uint32	GetQueueSize()				{return prefs->m_iQueueSize*100;}

	// Barry
	uint16	Get3DDepth()				 { return prefs->depth3D;}
	bool	AutoTakeED2KLinks()			 {return prefs->autotakeed2klinks;}
	bool	AddNewFilesPaused() 			{return prefs->addnewfilespaused;}

	void	SetMaxConsPerFive(int in) 		{prefs->MaxConperFive=in;}

	uint16	GetMaxConperFive()			{return prefs->MaxConperFive;}
	uint16	GetDefaultMaxConperFive();

	bool	IsSafeServerConnectEnabled()		{return prefs->safeServerConnect;}
	void	SetSafeServerConnectEnabled(bool in)	{prefs->safeServerConnect=in;}
	bool	IsMoviePreviewBackup()			{return prefs->moviePreviewBackup;}
	
	bool	IsCheckDiskspaceEnabled()		{return prefs->checkDiskspace != 0;}
	int32	GetMinFreeDiskSpace()		{return prefs->m_uMinFreeDiskSpace;}

	char*	GetYourHostname()			{return prefs->yourHostname;}
	void	SetYourHostname(CString in)		{sprintf(prefs->yourHostname,"%s",in.GetData());}

	// quick-speed changer [xrmb]
	void	SetMaxUpload(uint16 in) 		{prefs->maxupload =in;};
	void	SetMaxDownload(uint16 in) 		{prefs->maxdownload=in; };
	void	SetSlotAllocation(uint16 in)		{prefs->slotallocation=in; };

	WINDOWPLACEMENT GetEmuleWindowPlacement() 	{return prefs->EmuleWindowPlacement; }
	void SetWindowLayout(WINDOWPLACEMENT in)	{prefs->EmuleWindowPlacement=in; }

	wxArrayString shareddir_list;
	CStringList adresses_list;

	void 	SetLanguage();
	int8 	AutoConnectStaticOnly()		{return prefs->autoconnectstaticonly;}	
	int8 	GetUpdateDays()			{return prefs->versioncheckdays;}
	uint32 	GetLastVC()			{return prefs->versioncheckLastAutomatic;}
	void   	UpdateLastVC()			{prefs->versioncheckLastAutomatic=time(NULL);}
	int32	GetIPFilterLevel()		{ return prefs->filterlevel;}
	CString GetMessageFilter()		{ return CString(prefs->messageFilter);}
	CString GetCommentFilter()		{ return CString(prefs->commentFilter);}
	bool	ShowRatesOnTitle()		{ return prefs->showRatesInTitle;}
	char*   GetNotifierConfiguration()   	{return prefs->notifierConfiguration;}; //<<-- enkeyDEV(kei-kun) -skinnable notifier-
	void    SetNotifierConfiguration(CString configFullPath) {sprintf(prefs->notifierConfiguration,"%s",configFullPath.GetData()); } //<<-- enkeyDEV(kei-kun) -skinnable notifier-
	void	LoadCats();
	CString	GetDateTimeFormat()		{ return CString(prefs->datetimeformat);}
	// Download Categories (Ornis)
	int32	AddCat(Category_Struct* cat) 	{ catMap.Add(cat); return catMap.GetCount()-1;}
	void	RemoveCat(size_t index);
	uint32	GetCatCount()			{ return catMap.GetCount();}
	Category_Struct* GetCategory(size_t index) { if (index>=0 && index<catMap.GetCount()) return catMap[index]; else return NULL;}
	char*	GetCatPath(uint8 index)		{ return catMap[index]->incomingpath;}
	DWORD	GetCatColor(size_t index)		{ if (index>=0 && index<catMap.GetCount()) return catMap[index]->color; else return 0;}

	bool	ShowRatingIndicator()		{ return prefs->indicateratings;}
	int32	GetAllcatType()			{ return prefs->allcatType;}
	void	SetAllcatType(int32 in)		{ prefs->allcatType=in; }
	bool	ShowAllNotCats()		{ return prefs->showAllNotCats;}
	bool	WatchClipboard4ED2KLinks()	{ return prefs->watchclipboard;}
	void	InvertShowAllNotCats()		{ prefs->showAllNotCats=!prefs->showAllNotCats; }
	bool	FilterServerByIP()		{ return prefs->filterserverbyip;}

	bool	Log2Disk()			{ return prefs->log2disk;}
	bool	Debug2Disk()			{ return prefs->debug2disk;}
	int32		GetMaxLogMessages() 	{ return prefs->iMaxLogMessages;}

	// WebServer
	uint16	GetWSPort()			{ return prefs->m_nWebPort; }
	void	SetWSPort(uint16 uPort)		{ prefs->m_nWebPort=uPort; }
	CString	GetWSPass()			{ return CString(prefs->m_sWebPassword); }
	void	SetWSPass(CString strNewPass)	{ sprintf(prefs->m_sWebPassword,"%s",MD5Sum(strNewPass).GetHash().GetData()); }
	bool	GetWSIsEnabled()		{ return prefs->m_bWebEnabled; }
	void	SetWSIsEnabled(bool bEnable)	{ prefs->m_bWebEnabled=bEnable; }
	bool	GetWebUseGzip()			{ return prefs->m_bWebUseGzip; }
	void	SetWebUseGzip(bool bUse)	{ prefs->m_bWebUseGzip=bUse; }
	int32	GetWebPageRefresh()		{ return prefs->m_nWebPageRefresh; }
	void	SetWebPageRefresh(int nRefresh)	{ prefs->m_nWebPageRefresh=nRefresh; }
	bool	GetWSIsLowUserEnabled()		{ return prefs->m_bWebLowEnabled; }
	void	SetWSIsLowUserEnabled(bool in)	{ prefs->m_bWebLowEnabled=in; }
	CString	GetWSLowPass()			{ return CString(prefs->m_sWebLowPassword); }
	void	SetWSLowPass(CString strNewPass){ sprintf(prefs->m_sWebLowPassword,"%s",MD5Sum(strNewPass).GetHash().GetData()); }

	void	SetMaxSourcesPerFile(uint16 in)	{ prefs->maxsourceperfile=in;}
	void	SetMaxConnections(uint16 in)	{ prefs->maxconnections =in;}
	bool	IsSchedulerEnabled()		{ return prefs->scheduler;}
	bool	GetDontCompressAvi()		{ return prefs->dontcompressavi;}
	
	bool	MsgOnlyFriends()		{ return prefs->msgonlyfriends;}
	bool	MsgOnlySecure()			{ return prefs->msgsecure;}
	uint16	GetMsgSessionsMax()		{ return prefs->maxmsgsessions;}

	CString	GetTemplate()			{ return CString(prefs->m_sTemplateFile);}
	void	SetTemplate(CString in)		{ sprintf(prefs->m_sTemplateFile,"%s",in.GetData());}

	int32 	GetDesktopMode() 		{return prefs->desktopMode;}
	void 	SetDesktopMode(int mode)	{prefs->desktopMode=mode;}

	// deadlake PROXYSUPPORT
	ProxySettings GetProxy()		{return prefs->proxy;}
	void 	SetProxySettings(ProxySettings proxysettings) {prefs->proxy	= proxysettings;}
	uint16	GetListenPort()			{if (m_UseProxyListenPort) return ListenPort; else return prefs->port;}
	void	SetListenPort(uint16 uPort)	{ListenPort = uPort; m_UseProxyListenPort = true;}
	void	ResetListenPort()		{ListenPort = 0; m_UseProxyListenPort = false;}
	void	SetUseProxy(bool in)		{ prefs->proxy.UseProxy=in;}

	bool	IsProxyASCWOP()			{ return prefs->m_bIsASCWOP;}
	void	SetProxyASCWOP(bool in)		{ prefs->m_bIsASCWOP=in;}

	bool	ShowCatTabInfos()		{ return prefs->showCatTabInfos;}
	void	ShowCatTabInfos(bool in)	{ prefs->showCatTabInfos=in;}
	bool	GetResumeSameCat()		{ return prefs->resumeSameCat;}
	bool	IsGraphRecreateDisabled()	{ return prefs->dontRecreateGraphs;}
	
	// Madcat - Sources Dropping Tweaks
	bool	DropNoNeededSources()		{ return prefs->DropNoNeededSources; }
	bool    SwapNoNeededSources()   	{ return prefs->SwapNoNeededSources; }
	bool	DropFullQueueSources()		{ return prefs->DropFullQueueSources; }
	bool	DropHighQueueRankingSources()	{ return prefs->DropHighQueueRankingSources; }
	int32	HighQueueRanking()		{ return prefs->HighQueueRanking; }
	int32	GetAutoDropTimer()		{ return prefs->AutoDropTimer; }
	
	// Kry - External Connections
	bool 	AcceptExternalConnections() { return prefs->AcceptExternalConnections; }
	bool 	ECUseTCPPort() { return prefs->ECUseTCPPort; }
	int32	ECPort() { return prefs->ECPort; }
	CString	ECPassword() { return CString(prefs->ECPassword); }
	// Madcat - Fast ED2K Links Handler Toggling
	bool	GetFED2KLH() { return prefs->FastED2KLinksHandler; }
	bool	BDlgTabsOnTop()	{ return prefs->bDlgTabsOnTop; }
	
	// Kry - Ip filter On/Off
	bool GetIPFilterOn() { return prefs->IPFilterOn; }

	// Kry - Source seeds On/Off
	bool GetSrcSeedsOn() { return prefs->UseSrcSeeds; }
	
	bool IsSecureIdentEnabled() { return prefs->SecIdent; }
	
	bool GetExtractMetaData() { return prefs->ExtractMetaData; }
	
	bool ShowProgBar() { return prefs->ProgBar; }
	bool ShowPercent() { return prefs->Percent; }	
	
		
#ifndef UNIFIED_PREF_HANDLING
	void	SetStatsColor(int index,DWORD value) 	{prefs->statcolors[index]=value;}
	DWORD	GetStatsColor(int index) 		{return prefs->statcolors[index];}
	void	ResetStatsColor(int index);
#endif

protected:
	void	CreateUserHash();
	void	SetStandartValues();
	static int32 GetRecommendedMaxConnections();

private:
	char* appdir;
	Preferences_Struct* prefs;
	Preferences_Ext_Struct* prefsExt;

	Preferences_Import19c_Struct* prefsImport19c;
	Preferences_Import20a_Struct* prefsImport20a;
	Preferences_Import20b_Struct* prefsImport20b;
	
	char userhash[16];
	WORD m_wWinVer;

	void LoadPreferences();
	void SavePreferences();

	ArrayOfCategory_Struct catMap;

	// deadlake PROXYSUPPORT
	bool m_UseProxyListenPort;
	uint16	ListenPort;

};

#endif // PREFERENCES_H
