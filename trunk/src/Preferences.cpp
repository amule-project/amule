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


#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for PACKAGE_NAME and PACKAGE_STRING
#endif

#include <cstdio>
#include <cstdlib>
#include <clocale>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include "otherfunctions.h"	// Needed for atoll
#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
#endif
#include "PrefsUnifiedDlg.h"			// Needed for Rse
#include "opcodes.h"		// Needed for PREFFILE_VERSION
#include "color.h"			// Needed for RGB

#include <wx/config.h>
#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!
#include "amule.h"

#include "Preferences.h"


wxString	CPreferences::s_nick;
uint16		CPreferences::s_maxupload;
uint16		CPreferences::s_maxdownload;
uint16		CPreferences::s_slotallocation;
uint16		CPreferences::s_port;
uint16		CPreferences::s_udpport;
bool		CPreferences::s_UDPDisable;
uint16		CPreferences::s_maxconnections;
bool		CPreferences::s_reconnect;
bool		CPreferences::s_autoconnect;
bool		CPreferences::s_autoconnectstaticonly;
bool		CPreferences::s_autoserverlist;
bool		CPreferences::s_deadserver;
wxString	CPreferences::s_incomingdir;
wxString	CPreferences::s_tempdir;
bool	CPreferences::s_ICH;
int8	CPreferences::s_versioncheckdays;
int16	CPreferences::s_downloadColumnWidths[13];
CPreferences::Bool	CPreferences::s_downloadColumnHidden[13];
int16	CPreferences::s_downloadColumnOrder[13];
int16	CPreferences::s_uploadColumnWidths[8];
CPreferences::Bool	CPreferences::s_uploadColumnHidden[8];
int16	CPreferences::s_uploadColumnOrder[8];
int16	CPreferences::s_queueColumnWidths[10];
CPreferences::Bool	CPreferences::s_queueColumnHidden[10];
int16	CPreferences::s_queueColumnOrder[10];
int16	CPreferences::s_searchColumnWidths[5];
CPreferences::Bool	CPreferences::s_searchColumnHidden[5];
int16	CPreferences::s_searchColumnOrder[5];
int16	CPreferences::s_sharedColumnWidths[12];
CPreferences::Bool	CPreferences::s_sharedColumnHidden[12];
int16	CPreferences::s_sharedColumnOrder[12];
int16	CPreferences::s_serverColumnWidths[12];
CPreferences::Bool	CPreferences::s_serverColumnHidden[12];
int16 	CPreferences::s_serverColumnOrder[12];
int16	CPreferences::s_clientListColumnWidths[8];
CPreferences::Bool	CPreferences::s_clientListColumnHidden[8];
int16 	CPreferences::s_clientListColumnOrder[8];
int8	CPreferences::s_depth3D;
int32	CPreferences::s_tableSortItemDownload;
int32	CPreferences::s_tableSortItemUpload;
int32	CPreferences::s_tableSortItemQueue;
int32	CPreferences::s_tableSortItemSearch;
int32	CPreferences::s_tableSortItemShared;
int32	CPreferences::s_tableSortItemServer;
int32	CPreferences::s_tableSortItemClientList;
bool	CPreferences::s_tableSortAscendingDownload;
bool	CPreferences::s_tableSortAscendingUpload;
bool	CPreferences::s_tableSortAscendingQueue;
bool	CPreferences::s_tableSortAscendingSearch;
bool	CPreferences::s_tableSortAscendingShared;
bool	CPreferences::s_tableSortAscendingServer;
bool	CPreferences::s_tableSortAscendingClientList;
bool	CPreferences::s_scorsystem;
bool	CPreferences::s_updatenotify;
bool	CPreferences::s_mintotray;
bool	CPreferences::s_autotakeed2klinks;
bool	CPreferences::s_addnewfilespaused;
bool	CPreferences::s_addserversfromserver;
bool	CPreferences::s_addserversfromclient;
int16	CPreferences::s_maxsourceperfile;
int16	CPreferences::s_trafficOMeterInterval;
int16	CPreferences::s_statsInterval;
int		CPreferences::s_maxGraphDownloadRate;
int		CPreferences::s_maxGraphUploadRate;
bool	CPreferences::s_beepOnError;
bool	CPreferences::s_confirmExit;
bool	CPreferences::s_splashscreen;
bool	CPreferences::s_filterBadIP;
bool	CPreferences::s_onlineSig;
uint64  CPreferences::s_totalDownloadedBytes;
uint64	CPreferences::s_totalUploadedBytes;
uint16	CPreferences::s_languageID;
bool	CPreferences::s_transferDoubleclick;
int8	CPreferences::s_iSeeShares;
int8	CPreferences::s_iToolDelayTime;
bool	CPreferences::s_bringtoforeground;
int8	CPreferences::s_splitterbarPosition;
uint16	CPreferences::s_deadserverretries;
uint32	CPreferences::s_dwServerKeepAliveTimeoutMins;
uint8	CPreferences::s_statsMax;
int8	CPreferences::s_statsAverageMinutes;
bool    CPreferences::s_useDownloadNotifier;
bool	CPreferences::s_useChatNotifier;
bool	CPreferences::s_useLogNotifier;	
bool	CPreferences::s_useSoundInNotifier;
bool	CPreferences::s_sendEmailNotifier;
bool	CPreferences::s_notifierPopsEveryChatMsg;
bool	CPreferences::s_notifierImportantError;
bool	CPreferences::s_notifierNewVersion;
wxString	CPreferences::s_notifierSoundFilePath;
bool	CPreferences::s_bpreviewprio;
bool	CPreferences::s_smartidcheck;
uint8	CPreferences::s_smartidstate;
bool	CPreferences::s_safeServerConnect;
bool	CPreferences::s_startMinimized;
uint16	CPreferences::s_MaxConperFive;
bool	CPreferences::s_checkDiskspace;
uint32	CPreferences::s_uMinFreeDiskSpace;
wxString	CPreferences::s_yourHostname;
bool	CPreferences::s_bVerbose;
bool	CPreferences::s_bupdatequeuelist;
bool	CPreferences::s_bmanualhighprio;
bool	CPreferences::s_btransferfullchunks;
bool	CPreferences::s_bstartnextfile;
bool	CPreferences::s_bshowoverhead;
bool	CPreferences::s_bDAP;
bool	CPreferences::s_bUAP;
bool	CPreferences::s_bDisableKnownClientList;
bool	CPreferences::s_bDisableQueueList;
bool	CPreferences::s_showRatesInTitle;
wxString	CPreferences::s_VideoPlayer;
bool	CPreferences::s_moviePreviewBackup;
bool	CPreferences::s_indicateratings;
bool	CPreferences::s_showAllNotCats;
bool	CPreferences::s_watchclipboard;
bool	CPreferences::s_filterserverbyip;
bool	CPreferences::s_bFirstStart;
bool	CPreferences::s_bCreditSystem;
bool	CPreferences::s_msgonlyfriends;
bool	CPreferences::s_msgsecure;
uint8	CPreferences::s_filterlevel;
uint8	CPreferences::s_iFileBufferSize;
uint8	CPreferences::s_iQueueSize;
uint16	CPreferences::s_maxmsgsessions;
uint32	CPreferences::s_versioncheckLastAutomatic;
wxString 	CPreferences::s_datetimeformat;
wxString	CPreferences::s_sWebPassword;
wxString	CPreferences::s_sWebLowPassword;
uint16	CPreferences::s_nWebPort;
bool	CPreferences::s_bWebEnabled;
bool	CPreferences::s_bWebUseGzip;
int32	CPreferences::s_nWebPageRefresh;
bool	CPreferences::s_bWebLowEnabled;
wxString	CPreferences::s_sWebResDir;
wxString	CPreferences::s_sTemplateFile;
bool	CPreferences::s_bIsASCWOP;
bool	CPreferences::s_showCatTabInfos;
bool	CPreferences::s_resumeSameCat;
bool	CPreferences::s_dontRecreateGraphs;
int32	CPreferences::s_allcatType;
int32	CPreferences::s_desktopMode;
uint8	CPreferences::s_NoNeededSources;
bool	CPreferences::s_DropFullQueueSources;
bool	CPreferences::s_DropHighQueueRankingSources;
int32	CPreferences::s_HighQueueRanking;
int32	CPreferences::s_AutoDropTimer;
bool 	CPreferences::s_AcceptExternalConnections;
bool 	CPreferences::s_ECUseTCPPort;
int32	CPreferences::s_ECPort;
wxString	CPreferences::s_ECPassword;
bool	CPreferences::s_IPFilterOn;
bool	CPreferences::s_UseSrcSeeds;
bool	CPreferences::s_UseSafeMaxConn;
bool	CPreferences::s_VerbosePacketError;
bool	CPreferences::s_ProgBar;
bool	CPreferences::s_Percent;	
bool	CPreferences::s_SecIdent;
bool	CPreferences::s_ExtractMetaData;
bool	CPreferences::s_AllocFullPart;
bool	CPreferences::s_AllocFullChunk;
uint16	CPreferences::s_Browser;
wxString	CPreferences::s_CustomBrowser;
bool	CPreferences::s_BrowserTab;
wxString	CPreferences::s_OSDirectory;
wxString	CPreferences::s_SkinFile;
bool	CPreferences::s_UseSkinFile;
bool	CPreferences::s_FastED2KLinksHandler;
bool	CPreferences::s_bDlgTabsOnTop;




WX_DEFINE_OBJARRAY(ArrayOfCategory_Struct);

/// new implementation
CPreferences::CPreferences()
{
	srand((uint32)time(0)); // we need random numbers sometimes

	prefsExt=new Preferences_Ext_Struct;
	memset(prefsExt,0,sizeof(Preferences_Ext_Struct));

	CreateUserHash();

	// load preferences.dat or set standart values
	wxString fullpath(theApp.ConfigDir + wxT("preferences.dat"));

	FILE* preffile = fopen(unicode2char(fullpath),"rb");

	if (!preffile) {
		SetStandartValues();
	} else {
		fread(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		if (ferror(preffile)) {
			SetStandartValues();
		}
		md4cpy(m_userhash,prefsExt->userhash);
		fclose(preffile);
		s_smartidstate=0;
	}

	PrefsUnifiedDlg::BuildItemList(this, theApp.ConfigDir);

	LoadPreferences();

	// shared directories
	wxTextFile sdirfile(theApp.ConfigDir + wxT("shareddir.dat"));
	if( sdirfile.Exists() && sdirfile.Open() ) {
		if (sdirfile.GetLineCount()) {
			for (wxString str = sdirfile.GetFirstLine(); !sdirfile.Eof(); str = sdirfile.GetNextLine() ) {
    				shareddir_list.Add(str);
			}
		}
		sdirfile.Close();
	}

	// serverlist adresses
	wxTextFile slistfile(theApp.ConfigDir + wxT("addresses.dat"));
	if ( slistfile.Exists() && slistfile.Open()) {
		if (slistfile.GetLineCount()) {
			for (wxString str = slistfile.GetFirstLine(); !slistfile.Eof(); str = slistfile.GetNextLine() ) {
    				adresses_list.Add(str);
			}
		}
		slistfile.Close();
	}

	m_userhash[5] = 14;
	m_userhash[14] = 111;

	if (!::wxDirExists(GetIncomingDir())) {
		::wxMkdir(GetIncomingDir(),0750);
	}

	if (!::wxDirExists(GetTempDir())) {
		::wxMkdir(GetTempDir(),0750);
	}

	if (m_userhash.IsEmpty()) {
		CreateUserHash();
	}
	
	printf("Userhash loaded: %s\n", unicode2char(m_userhash.Encode()));
}

void CPreferences::SetStandartValues()
{
	CreateUserHash();
	WINDOWPLACEMENT defaultWPM;
	defaultWPM.length = sizeof(WINDOWPLACEMENT);
	defaultWPM.rcNormalPosition.left=10;
	defaultWPM.rcNormalPosition.top=10;
	defaultWPM.rcNormalPosition.right=700;
	defaultWPM.rcNormalPosition.bottom=500;
	defaultWPM.showCmd=0;
	s_versioncheckLastAutomatic=0;
	Save();
}

bool CPreferences::Save()
{
	wxString fullpath(theApp.ConfigDir + wxT("preferences.dat"));

	bool error = false;

	FILE* preffile = fopen(unicode2char(fullpath),"wb");
	prefsExt->version = PREFFILE_VERSION;

	if (preffile) {
		prefsExt->version=PREFFILE_VERSION;
		printf("Saving userhash: %s\n", unicode2char(m_userhash.Encode()));
		md4cpy(prefsExt->userhash,m_userhash.GetHash());
		error = fwrite(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		fclose(preffile);
	} else {
		error = true;
	}

	SavePreferences();

	wxString shareddir(theApp.ConfigDir + wxT("shareddir.dat"));

	wxRemoveFile(shareddir);

	wxTextFile sdirfile(shareddir);

	if(sdirfile.Create()) {
		for(unsigned int ii = 0; ii < shareddir_list.GetCount(); ++ii) {
			sdirfile.AddLine(shareddir_list[ii]);
		}
		sdirfile.Write(),
		sdirfile.Close();
	} else {
		error = true;
	}

	return error;
}

void CPreferences::CreateUserHash()
{
	for (int i = 0;i != 8; i++) {
		uint16	random = rand();
		memcpy(&m_userhash[i*2],&random,2);
	}
	// mark as emule client. that will be need in later version
	m_userhash[5] = 14;
	m_userhash[14] = 111;
}

int32 CPreferences::GetColumnWidth(Table t, int index)
{
	switch(t) {
	case tableDownload:
		return s_downloadColumnWidths[index];
	case tableUpload:
		return s_uploadColumnWidths[index];
	case tableQueue:
		return s_queueColumnWidths[index];
	case tableSearch:
		return s_searchColumnWidths[index];
	case tableShared:
		return s_sharedColumnWidths[index];
	case tableServer:
		return s_serverColumnWidths[index];
	case tableClientList:
		return s_clientListColumnWidths[index];
	case tableNone:
	default:
		return 0;
	}
}

void CPreferences::SetColumnWidth(Table t, int index, int32 width)
{
	switch(t) {
	case tableDownload:
		s_downloadColumnWidths[index] = width;
		break;
	case tableUpload:
		s_uploadColumnWidths[index] = width;
		break;
	case tableQueue:
		s_queueColumnWidths[index] = width;
		break;
	case tableSearch:
		s_searchColumnWidths[index] = width;
		break;
	case tableShared:
		s_sharedColumnWidths[index] = width;
		break;
	case tableServer:
		s_serverColumnWidths[index] = width;
		break;
	case tableClientList:
		s_clientListColumnWidths[index] = width;
		break;
	case tableNone:
	default:
		break;
	}
}

bool CPreferences::GetColumnHidden(Table t, int index)
{
	switch(t) {
	case tableDownload:
		return s_downloadColumnHidden[index];
	case tableUpload:
		return s_uploadColumnHidden[index];
	case tableQueue:
		return s_queueColumnHidden[index];
	case tableSearch:
		return s_searchColumnHidden[index];
	case tableShared:
		return s_sharedColumnHidden[index];
	case tableServer:
		return s_serverColumnHidden[index];
	case tableClientList:
		return s_clientListColumnHidden[index];
	case tableNone:
	default:
		return FALSE;
	}
}

void CPreferences::SetColumnHidden(Table t, int index, bool bHidden)
{
	switch(t) {
	case tableDownload:
		s_downloadColumnHidden[index] = bHidden;
		break;
	case tableUpload:
		s_uploadColumnHidden[index] = bHidden;
		break;
	case tableQueue:
		s_queueColumnHidden[index] = bHidden;
		break;
	case tableSearch:
		s_searchColumnHidden[index] = bHidden;
		break;
	case tableShared:
		s_sharedColumnHidden[index] = bHidden;
		break;
	case tableServer:
		s_serverColumnHidden[index] = bHidden;
		break;
	case tableClientList:
		s_clientListColumnHidden[index] = bHidden;
		break;
	case tableNone:
	default:
		break;
	}
}

int32 CPreferences::GetColumnOrder(Table t, int index)
{
	switch(t) {
	case tableDownload:
		return s_downloadColumnOrder[index];
	case tableUpload:
		return s_uploadColumnOrder[index];
	case tableQueue:
		return s_queueColumnOrder[index];
	case tableSearch:
		return s_searchColumnOrder[index];
	case tableShared:
		return s_sharedColumnOrder[index];
	case tableServer:
		return s_serverColumnOrder[index];
	case tableClientList:
		return s_clientListColumnOrder[index];
	case tableNone:
	default:
		return 0;
	}
}

void CPreferences::SetColumnOrder(Table t, INT *piOrder)
{
	switch(t) {
	case tableDownload:
		memcpy(s_downloadColumnOrder, piOrder, sizeof(s_downloadColumnOrder));
		break;
	case tableUpload:
		memcpy(s_uploadColumnOrder, piOrder, sizeof(s_uploadColumnOrder));
		break;
	case tableQueue:
		memcpy(s_queueColumnOrder, piOrder, sizeof(s_queueColumnOrder));
		break;
	case tableSearch:
		memcpy(s_searchColumnOrder, piOrder, sizeof(s_searchColumnOrder));
		break;
	case tableShared:
		memcpy(s_sharedColumnOrder, piOrder, sizeof(s_sharedColumnOrder));
		break;
	case tableServer:
		memcpy(s_serverColumnOrder, piOrder, sizeof(s_serverColumnOrder));
		break;
	case tableClientList:
		memcpy(s_clientListColumnOrder, piOrder, sizeof(s_clientListColumnOrder));
		break;
	case tableNone:
	default:
		break;
	}
}

CPreferences::~CPreferences()
{

	Category_Struct* delcat;
	while (!catMap.IsEmpty()) {
		delcat=catMap[0];
		catMap.RemoveAt(0);
		delete delcat;
	}

	catMap.Clear();

	delete prefsExt;
}

int32 CPreferences::GetRecommendedMaxConnections()
{
	int iRealMax = ::GetMaxConnections();
	if(iRealMax == -1 || iRealMax > 520) {
		return 500;
	}
	if(iRealMax < 20) {
		return iRealMax;
	}
	if(iRealMax <= 256) {
		return iRealMax - 10;
	}
	return iRealMax - 20;
}

void CPreferences::SavePreferences()
{
	wxConfigBase* cfg = wxConfig::Get();

	cfg->Write( wxT("/eMule/AppVersion"), wxT(PACKAGE_STRING) );

	PrefsUnifiedDlg::SaveAllItems((*cfg));

	// Ensure that the changes are saved to disk.
	cfg->Flush();
}

void CPreferences::SaveCats()
{
	if ( GetCatCount() ) {
		wxConfigBase* cfg = wxConfig::Get();

		// The first category is the default one and should not be counte

		cfg->Write( wxT("/General/Count"), (long)(catMap.GetCount() - 1) );

		for ( size_t i = 1; i < catMap.GetCount(); i++ ) {
			cfg->SetPath( wxString::Format(wxT("/Cat#%i"), i) );

			cfg->Write( wxT("Title"), catMap[i]->title );
			cfg->Write( wxT("Incoming"), catMap[i]->incomingpath );
			cfg->Write( wxT("Comment"), catMap[i]->comment );
			cfg->Write( wxT("Color"), wxString::Format(wxT("%u"), catMap[i]->color) );
			cfg->Write( wxT("Priority"), catMap[i]->prio );
		}
	}
}

void CPreferences::LoadPreferences()
{
	wxConfigBase* cfg = wxConfig::Get();

	PrefsUnifiedDlg::LoadAllItems( (*cfg)  );

	LoadCats();
}

void CPreferences::LoadCats() {
	// default cat ... Meow! =(^.^)=
	Category_Struct* newcat=new Category_Struct;
	newcat->title = wxEmptyString;
	newcat->incomingpath = wxEmptyString;
	newcat->comment = wxEmptyString;
	newcat->prio=0;
	newcat->color=0;
	AddCat( newcat );

	wxConfigBase* cfg = wxConfig::Get();

	long max = cfg->Read( wxT("/General/Count"), 0l );

	for ( int i = 1; i <= max ; i++ ) {
		cfg->SetPath( wxString::Format(wxT("/Cat#%i"), i) );

		Category_Struct* newcat = new Category_Struct;


		newcat->title = cfg->Read( wxT("Title"), wxT("") );
		newcat->incomingpath = cfg->Read( wxT("Incoming"), wxT("") );

		newcat->incomingpath = MakeFoldername(newcat->incomingpath);
		newcat->comment = cfg->Read( wxT("Comment"), wxT("") );

		newcat->prio = cfg->Read( wxT("Priority"), 0l );

		wxString color = cfg->Read( wxT("Color"), wxT("0") );
		newcat->color = StrToULong(color);

		AddCat(newcat);
		if (!wxFileName::DirExists(newcat->incomingpath)) {
			wxFileName::Mkdir(newcat->incomingpath);
		}
	}
}

uint16 CPreferences::GetDefaultMaxConperFive()
{
	return MAXCONPER5SEC;
}


// Barry - Provide a mechanism for all tables to store/retrieve sort order
int32 CPreferences::GetColumnSortItem(Table t)
{
	switch(t) {
		case tableDownload:
			return s_tableSortItemDownload;
		case tableUpload:
			return s_tableSortItemUpload;
		case tableQueue:
			return s_tableSortItemQueue;
		case tableSearch:
			return s_tableSortItemSearch;
		case tableShared:
			return s_tableSortItemShared;
		case tableServer:
			return s_tableSortItemServer;
		case tableClientList:
			return s_tableSortItemClientList;
		case tableNone:
		default:
			return 0;
	}
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
bool CPreferences::GetColumnSortAscending(Table t)
{
	switch(t) {
		case tableDownload:
			return s_tableSortAscendingDownload;
		case tableUpload:
			return s_tableSortAscendingUpload;
		case tableQueue:
			return s_tableSortAscendingQueue;
		case tableSearch:
			return s_tableSortAscendingSearch;
		case tableShared:
			return s_tableSortAscendingShared;
		case tableServer:
			return s_tableSortAscendingServer;
		case tableClientList:
			return s_tableSortAscendingClientList;
		case tableNone:
		default:
			return true;
	}
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
void CPreferences::SetColumnSortItem(Table t, int32 sortItem)
{
	switch(t) {
		case tableDownload:
			s_tableSortItemDownload = sortItem;
			break;
		case tableUpload:
			s_tableSortItemUpload = sortItem;
			break;
		case tableQueue:
			s_tableSortItemQueue = sortItem;
			break;
		case tableSearch:
			s_tableSortItemSearch = sortItem;
			break;
		case tableShared:
			s_tableSortItemShared = sortItem;
			break;
		case tableServer:
			s_tableSortItemServer = sortItem;
			break;
		case tableClientList:
			s_tableSortItemClientList = sortItem;
			break;
		case tableNone:
		default:
			break;
	}
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
void CPreferences::SetColumnSortAscending(Table t, bool sortAscending)
{
	switch(t) {
		case tableDownload:
			s_tableSortAscendingDownload = sortAscending;
			break;
		case tableUpload:
			s_tableSortAscendingUpload = sortAscending;
			break;
		case tableQueue:
			s_tableSortAscendingQueue = sortAscending;
			break;
		case tableSearch:
			s_tableSortAscendingSearch = sortAscending;
			break;
		case tableShared:
			s_tableSortAscendingShared = sortAscending;
			break;
		case tableServer:
			s_tableSortAscendingServer = sortAscending;
			break;
		case tableClientList:
			s_tableSortAscendingClientList = sortAscending;
			break;
		case tableNone:
		default:
			break;
	}
}

void CPreferences::RemoveCat(size_t index)
{
	if (index < catMap.GetCount()) {
		catMap.RemoveAt(index);
	}
}

// Jacobo221 - Several issues on the browsers:
// netscape is named Netscape on some systems
// MozillaFirebird is named mozilla-firebird and also firebird on some systems
// Niether Galeon tabs nor epiphany tabs have been tested
// Konqueror alternatives is (Open on current window, fails if no konq already open):
//	dcop `dcop konqueror-* | head -n1` konqueror-mainwindow#1 openURL '%s'
wxString CPreferences::GetBrowser()
{
	wxString cmd;
	if( s_BrowserTab )
                switch ( s_Browser ) {
                        case 0: cmd = wxT("kfmclient exec '%s'"); break;
                        case 1: cmd = wxT("sh -c \"if ! mozilla -remote 'openURL(%s, new-tab)'; then mozilla '%s'; fi\""); break;
                        case 2: cmd = wxT("sh -c \"if ! firefox -remote 'openURL(%s, new-tab)'; then firefox '%s'; fi\""); break;
                        case 3: cmd = wxT("sh -c \"if ! MozillaFirebird -remote 'openURL(%s, new-tab)'; then MozillaFirebird '%s'; fi\""); break;
                        case 4: cmd = wxT("opera --newpage '%s'"); break;
                        case 5: cmd = wxT("sh -c \"if ! netscape -remote 'openURLs(%s,new-tab)'; then netscape '%s'; fi\""); break;
                        case 6: cmd = wxT("galeon -n '%s'"); break;
                        case 7: cmd = wxT("epiphany -n '%s'"); break;
                        case 8: cmd = s_CustomBrowser; break;
                        default:
                                printf("Unable to determine selected browser!\n");
                }
        else
		switch ( s_Browser ) {
			case 0: cmd = wxT("konqueror '%s'"); break;
			case 1: cmd = wxT("sh -c 'mozilla %s'"); break;
			case 2: cmd = wxT("firefox '%s'"); break;
			case 3:	cmd = wxT("MozillaFirebird '%s'"); break;
			case 4:	cmd = wxT("opera '%s'"); break;
			case 5: cmd = wxT("netscape '%s'"); break;
			case 6: cmd = wxT("galeon '%s'"); break;
			case 7: cmd = wxT("epiphany '%s'"); break;
			case 8: cmd = s_CustomBrowser; break;
			default:
				printf("Unable to determine selected browser!\n");
		}
	
	return cmd;
}
