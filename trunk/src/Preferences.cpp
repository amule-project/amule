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
#include "otherfunctions.h"	// Needed for atoll
//#include "Preferences.h"
#include "ini2.h"			// Needed for CIni
#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
#endif
#include "PrefsUnifiedDlg.h"			// Needed for Rse
#include "opcodes.h"		// Needed for PREFFILE_VERSION
#include "color.h"			// Needed for RGB

#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!

WX_DEFINE_OBJARRAY(ArrayOfCategory_Struct);

/// new implementation
CPreferences::CPreferences()
{
	srand((uint32)time(0)); // we need random numbers sometimes
	prefs = new Preferences_Struct;	
	memset(prefs,0,sizeof(Preferences_Struct));
	prefsExt=new Preferences_Ext_Struct;
	
	memset(prefsExt,0,sizeof(Preferences_Ext_Struct));

	//get application start directory
	char buffer[490];

	// Use home directory to save preferences 
	snprintf(buffer,400,"%s/." PACKAGE_NAME,getenv("HOME"));

	if (!wxFileName::DirExists(buffer)) {
		wxFileName::Mkdir(buffer,0777);
	}
	strncat(buffer,"/",1);

	appdir = nstrdup(buffer);
	CreateUserHash();
	md4cpy(&prefs->userhash,&userhash);
	
	// load preferences.dat or set standart values
	char* fullpath = new char[strlen(appdir)+16];
	sprintf(fullpath,"%spreferences.dat",appdir);
	FILE* preffile = fopen(fullpath,"rb");
	delete[] fullpath;

#ifdef UNIFIED_PREF_HANDLING
	PrefsUnifiedDlg::BuildItemList(prefs, appdir);
#endif
	LoadPreferences();
	
	if (!preffile) {
		SetStandartValues();
		//if (Ask4RegFix(true)) Ask4RegFix(false);
	} else {
		fread(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		if (ferror(preffile)) {
			SetStandartValues();
		}
		// import old pref-files
		if (prefsExt->version<20) {
			if (prefsExt->version>17) { // v0.20b+
				prefsImport20b=new Preferences_Import20b_Struct;
				memset(prefsImport20b,0,sizeof(Preferences_Import20b_Struct));
				fseek(preffile,0,0);
				fread(prefsImport20b,sizeof(Preferences_Import20b_Struct),1,preffile);
				memcpy(&prefs->userhash,&prefsImport20b->userhash,16);
				memcpy(&prefs->incomingdir,&prefsImport20b->incomingdir,510);
				memcpy(&prefs->tempdir,&prefsImport20b->tempdir,510);
				sprintf(prefs->nick,"%s", prefsImport20b->nick);
				prefs->totalDownloadedBytes=prefsImport20b->totalDownloadedBytes;
				prefs->totalUploadedBytes=prefsImport20b->totalUploadedBytes;
			} else if (prefsExt->version>7) { // v0.20a
				prefsImport20a=new Preferences_Import20a_Struct;
				memset(prefsImport20a,0,sizeof(Preferences_Import20a_Struct));
				fseek(preffile,0,0);
				fread(prefsImport20a,sizeof(Preferences_Import20a_Struct),1,preffile);
				memcpy(&prefs->userhash,&prefsImport20a->userhash,16);
				memcpy(&prefs->incomingdir,&prefsImport20a->incomingdir,510);
				memcpy(&prefs->tempdir,&prefsImport20a->tempdir,510);
				sprintf(prefs->nick,"%s", prefsImport20a->nick);
				prefs->totalDownloadedBytes=prefsImport20a->totalDownloaded;
				prefs->totalUploadedBytes=prefsImport20a->totalUploaded;
			} else { //v0.19c-
				prefsImport19c=new Preferences_Import19c_Struct;
				memset(prefsImport19c,0,sizeof(Preferences_Import19c_Struct));
				fseek(preffile,0,0);
				fread(prefsImport19c,sizeof(Preferences_Import19c_Struct),1,preffile);
				if (prefsExt->version<3) {
					CreateUserHash();
					memcpy(&prefs->userhash,&userhash,16);
				} else {
					memcpy(&prefs->userhash,&prefsImport19c->userhash,16);
				}
				memcpy(&prefs->incomingdir,&prefsImport19c->incomingdir,510);memcpy(&prefs->tempdir,&prefsImport19c->tempdir,510);
				sprintf(prefs->nick,"%s",prefsImport19c->nick);
			}
 		} else {
			memcpy(&prefs->userhash,&prefsExt->userhash,16);
			prefs->EmuleWindowPlacement=prefsExt->EmuleWindowPlacement;
		}
		fclose(preffile);
		memcpy(&userhash,&prefs->userhash,16);
		prefs->smartidstate=0;
	}

	// shared directories
	fullpath = new char[strlen(appdir)+MAX_PATH]; // i_a
	sprintf(fullpath,"%sshareddir.dat",appdir);
	//CStdioFile* sdirfile = new CStdioFile();
	FILE* sdirfile=fopen(fullpath,"r");
	//if (sdirfile->Open(fullpath,CFile::modeRead)){
	if(sdirfile) {
		// CString toadd;
		char buffer[4096];
		while(!feof(sdirfile)) {
			memset(buffer,0,sizeof(buffer));
			fgets(buffer,sizeof(buffer)-1,sdirfile);
			char* ptr=strchr(buffer,'\n');
			if(ptr) {
				*ptr=0;
			}
			if(strlen(buffer)>1) {
				shareddir_list.Add(buffer);//new CString(buffer));
			}
		}
		fclose(sdirfile);
	}
	// delete[] sdirfile;
	delete[] fullpath;

	// serverlist adresses
	fullpath = new char[strlen(appdir)+20];
	sprintf(fullpath,"%saddresses.dat",appdir);
	// sdirfile = new CStdioFile();
	sdirfile=fopen(fullpath,"r");
	// if (sdirfile->Open(fullpath,CFile::modeRead)){
	if(sdirfile) {
		// CString toadd;
		char buffer[4096];
		while(!feof(sdirfile)) {
			memset(buffer,0,sizeof(buffer));
			fgets(buffer,sizeof(buffer)-1,sdirfile);
			char* ptr=strchr(buffer,'\n');
			if(ptr) {
				*ptr=0;
			}
			if(strlen(buffer)>1) {
				adresses_list.Append(new CString(buffer));
			}
		}
		fclose(sdirfile);
	}
	// delete[] sdirfile;
	delete[] fullpath;	
	fullpath=NULL;
	userhash[5] = 14;
	userhash[14] = 111;
	if (!wxFileName::DirExists(GetIncomingDir())) {
		wxFileName::Mkdir(GetIncomingDir(),0777);
	}
	if (!wxFileName::DirExists(GetTempDir())) {
		wxFileName::Mkdir(GetTempDir(),0777);
	}

	if (((int*)prefs->userhash)[0] == 0 && ((int*)prefs->userhash)[1] == 0 && ((int*)prefs->userhash)[2] == 0 && ((int*)prefs->userhash)[3] == 0) {
		CreateUserHash();
	}
}

void CPreferences::SetStandartValues()
{
	CreateUserHash();
	md4cpy(&prefs->userhash,&userhash);
	WINDOWPLACEMENT defaultWPM;
	defaultWPM.length = sizeof(WINDOWPLACEMENT);
	defaultWPM.rcNormalPosition.left=10;defaultWPM.rcNormalPosition.top=10;
	defaultWPM.rcNormalPosition.right=700;defaultWPM.rcNormalPosition.bottom=500;
	defaultWPM.showCmd=0;
	prefs->EmuleWindowPlacement=defaultWPM;
	prefs->versioncheckLastAutomatic=0;
	Save();
}

uint16 CPreferences::GetMaxDownload()
{
	return prefs->maxdownload;
}

bool CPreferences::Save()
{
	bool error = false;
	char* fullpath = new char[strlen(appdir)+MAX_PATH]; // i_a
	sprintf(fullpath,"%spreferences.dat",appdir);

	FILE* preffile = fopen(fullpath,"wb");
	prefsExt->version = PREFFILE_VERSION;

	if (preffile) {
		prefsExt->version=PREFFILE_VERSION;
		prefsExt->EmuleWindowPlacement=prefs->EmuleWindowPlacement;
		memcpy(&prefsExt->userhash,&prefs->userhash,16);
		error = fwrite(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		fclose(preffile);
	} else {
		error = true;
	}

	SavePreferences();
	delete[] fullpath;

	fullpath = new char[strlen(appdir)+14];
	sprintf(fullpath,"%sshareddir.dat",appdir);
	FILE* sdirfile=fopen(fullpath,"w");
	if(sdirfile) {
		for(unsigned int ii = 0; ii < shareddir_list.GetCount(); ++ii) {
			fprintf(sdirfile,"%s\n",shareddir_list[ii].GetData());
		}
		fclose(sdirfile);
	} else {
		error = true;
	}
	delete[] fullpath;
	if (!wxFileName::DirExists(GetIncomingDir())) {
		wxFileName::Mkdir(GetIncomingDir(),0777);
	}
	if (!wxFileName::DirExists(GetTempDir())) {
		wxFileName::Mkdir(GetTempDir(),0777);
	}
	return error;
}

void CPreferences::CreateUserHash()
{
	for (int i = 0;i != 8; i++) {
		uint16	random = rand();
		memcpy(&userhash[i*2],&random,2);
	}
	// mark as emule client. that will be need in later version
	userhash[5] = 14;
	userhash[14] = 111;
}

int32 CPreferences::GetColumnWidth(Table t, int index) const
{
	switch(t) {
	case tableDownload:
		return prefs->downloadColumnWidths[index];
	case tableUpload:
		return prefs->uploadColumnWidths[index];
	case tableQueue:
		return prefs->queueColumnWidths[index];
	case tableSearch:
		return prefs->searchColumnWidths[index];
	case tableShared:
		return prefs->sharedColumnWidths[index];
	case tableServer:
		return prefs->serverColumnWidths[index];
	case tableClientList:
		return prefs->clientListColumnWidths[index];
	case tableNone:
	default:
		return 0;
	}
}

void CPreferences::SetColumnWidth(Table t, int index, int32 width)
{
	switch(t) {
	case tableDownload:
		prefs->downloadColumnWidths[index] = width;
		break;
	case tableUpload:
		prefs->uploadColumnWidths[index] = width;
		break;
	case tableQueue:
		prefs->queueColumnWidths[index] = width;
		break;
	case tableSearch:
		prefs->searchColumnWidths[index] = width;
		break;
	case tableShared:
		prefs->sharedColumnWidths[index] = width;
		break;
	case tableServer:
		prefs->serverColumnWidths[index] = width;
		break;
	case tableClientList:
		prefs->clientListColumnWidths[index] = width;
		break;
	case tableNone:
	default:
		break;
	}
}

bool CPreferences::GetColumnHidden(Table t, int index) const
{
	switch(t) {
	case tableDownload:
		return prefs->downloadColumnHidden[index];
	case tableUpload:
		return prefs->uploadColumnHidden[index];
	case tableQueue:
		return prefs->queueColumnHidden[index];
	case tableSearch:
		return prefs->searchColumnHidden[index];
	case tableShared:
		return prefs->sharedColumnHidden[index];
	case tableServer:
		return prefs->serverColumnHidden[index];
	case tableClientList:
		return prefs->clientListColumnHidden[index];
	case tableNone:
	default:
		return FALSE;
	}
}

void CPreferences::SetColumnHidden(Table t, int index, bool bHidden)
{
	switch(t) {
	case tableDownload:
		prefs->downloadColumnHidden[index] = bHidden;
		break;
	case tableUpload:
		prefs->uploadColumnHidden[index] = bHidden;
		break;
	case tableQueue:
		prefs->queueColumnHidden[index] = bHidden;
		break;
	case tableSearch:
		prefs->searchColumnHidden[index] = bHidden;
		break;
	case tableShared:
		prefs->sharedColumnHidden[index] = bHidden;
		break;
	case tableServer:
		prefs->serverColumnHidden[index] = bHidden;
		break;
	case tableClientList:
		prefs->clientListColumnHidden[index] = bHidden;
		break;
	case tableNone:
	default:
		break;
	}
}

int32 CPreferences::GetColumnOrder(Table t, int index) const
{
	switch(t) {
	case tableDownload:
		return prefs->downloadColumnOrder[index];
	case tableUpload:
		return prefs->uploadColumnOrder[index];
	case tableQueue:
		return prefs->queueColumnOrder[index];
	case tableSearch:
		return prefs->searchColumnOrder[index];
	case tableShared:
		return prefs->sharedColumnOrder[index];
	case tableServer:
		return prefs->serverColumnOrder[index];
	case tableClientList:
		return prefs->clientListColumnOrder[index];
	case tableNone:
	default:
		return 0;
	}
}

void CPreferences::SetColumnOrder(Table t, INT *piOrder)
{
	switch(t) {
	case tableDownload:
		memcpy(prefs->downloadColumnOrder, piOrder, sizeof(prefs->downloadColumnOrder));
		break;
	case tableUpload:
		memcpy(prefs->uploadColumnOrder, piOrder, sizeof(prefs->uploadColumnOrder));
		break;
	case tableQueue:
		memcpy(prefs->queueColumnOrder, piOrder, sizeof(prefs->queueColumnOrder));
		break;
	case tableSearch:
		memcpy(prefs->searchColumnOrder, piOrder, sizeof(prefs->searchColumnOrder));
		break;
	case tableShared:
		memcpy(prefs->sharedColumnOrder, piOrder, sizeof(prefs->sharedColumnOrder));
		break;
	case tableServer:
		memcpy(prefs->serverColumnOrder, piOrder, sizeof(prefs->serverColumnOrder));
		break;
	case tableClientList:
		memcpy(prefs->clientListColumnOrder, piOrder, sizeof(prefs->clientListColumnOrder));
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
	
	if (adresses_list.GetCount()>0) {
		// the 'true' tells wxList to do 'delete' on the nodes.
		adresses_list.DeleteContents(true); 
	}
	
	delete[] appdir;
	delete prefs;
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
	CString buffer;
	char* fullpath = new char[strlen(appdir)+MAX_PATH]; // i_a
	sprintf(fullpath,"%spreferences.ini",appdir);
	
	wxString fp(fullpath), bf("eMule");
	
	printf("Preferences saving in %s\n",fullpath);
	
	CIni ini(fp, bf);
	delete[] fullpath;
	fullpath=NULL;
	//---
	ini.WriteString("AppVersion", PACKAGE_STRING);
	//---

#ifdef	UNIFIED_PREF_HANDLING
	PrefsUnifiedDlg::SaveAllItems(ini);
#else
	buffer.Format("%s",prefs->nick);
	ini.WriteString("Nick",buffer);

	buffer.Format("%s",prefs->incomingdir);
	ini.WriteString("IncomingDir",buffer );

	buffer.Format("%s",prefs->tempdir);
	ini.WriteString("TempDir",buffer );

	ini.WriteInt("MaxUpload",prefs->maxupload);
	ini.WriteInt("MaxDownload",prefs->maxdownload);

	/* BigBob - Slot Allocation */
	ini.WriteInt("SlotAllocation",prefs->slotallocation);

	ini.WriteInt("MaxConnections",prefs->maxconnections);
	ini.WriteInt("RemoveDeadServer",prefs->deadserver);
	ini.WriteInt("Port",prefs->port);
	ini.WriteInt("UDPPort",prefs->udpport);
	ini.WriteInt("MaxSourcesPerFile",prefs->maxsourceperfile );
	ini.WriteWORD("Language",prefs->languageID);
	ini.WriteInt("SeeShare",prefs->m_iSeeShares);
	ini.WriteInt("ToolTipDelay",prefs->m_iToolDelayTime);
	ini.WriteInt("StatGraphsInterval",prefs->trafficOMeterInterval);
	ini.WriteInt("StatsInterval",prefs->statsInterval);
	ini.WriteInt("DownloadCapacity",prefs->maxGraphDownloadRate);
	ini.WriteInt("UploadCapacity",prefs->maxGraphUploadRate);
	ini.WriteInt("DeadServerRetry",prefs->deadserverretries);
	ini.WriteInt("ServerKeepAliveTimeout",prefs->m_dwServerKeepAliveTimeout);
	ini.WriteInt("ListRefresh",prefs->m_dwListRefresh);
	ini.WriteInt("SplitterbarPosition",prefs->splitterbarPosition+2);
	ini.WriteInt("VariousStatisticsMaxValue",prefs->statsMax);
	ini.WriteInt("StatsAverageMinutes",prefs->statsAverageMinutes);
	ini.WriteInt("MaxConnectionsPerFiveSeconds",prefs->MaxConperFive);
	ini.WriteInt("Check4NewVersionDelay",prefs->versioncheckdays);

	ini.WriteBool("Reconnect",prefs->reconnect);
	ini.WriteBool("Scoresystem",prefs->scorsystem);
	ini.WriteBool("ICH",prefs->ICH);
	ini.WriteBool("Serverlist",prefs->autoserverlist);
	ini.WriteString("LRUServermetURL",prefs->m_szLRUServermetURL);
	ini.WriteBool("UpdateNotify",prefs->updatenotify);
	ini.WriteBool("MinToTray",prefs->mintotray);
	ini.WriteBool("AddServersFromServer",prefs->addserversfromserver);
	ini.WriteBool("AddServersFromClient",prefs->addserversfromclient);
	ini.WriteBool("Splashscreen",prefs->splashscreen);
	ini.WriteBool("BringToFront",prefs->bringtoforeground);
	ini.WriteBool("TransferDoubleClick",prefs->transferDoubleclick);
	ini.WriteBool("BeepOnError",prefs->beepOnError);
	ini.WriteBool("ConfirmExit",prefs->confirmExit);
	ini.WriteBool("FilterBadIPs",prefs->filterBadIP);
	ini.WriteBool("Autoconnect",prefs->autoconnect);
	ini.WriteBool("OnlineSignature",prefs->onlineSig);
	ini.WriteBool("StartupMinimized",prefs->startMinimized);
	ini.WriteBool("SafeServerConnect",prefs->safeServerConnect);
	ini.WriteBool("ShowRatesOnTitle",prefs->showRatesInTitle);
	ini.WriteBool("IndicateRatings",prefs->indicateratings);
	ini.WriteBool("WatchClipboard4ED2kFilelinks",prefs->watchclipboard);
	ini.WriteBool("CheckDiskspace",prefs->checkDiskspace);
	ini.WriteInt("MinFreeDiskSpace",prefs->m_uMinFreeDiskSpace);
	buffer.Format("%s",prefs->yourHostname);
	ini.WriteString("YourHostname",buffer);

	// Barry - New properties...
	ini.WriteBool("AutoConnectStaticOnly", prefs->autoconnectstaticonly);  
	ini.WriteBool("AutoTakeED2KLinks", prefs->autotakeed2klinks);  
	ini.WriteBool("AddNewFilesPaused", prefs->addnewfilespaused);  
	ini.WriteInt ("3DDepth", prefs->depth3D);  

	ini.WriteBool("NotifierUseSound",prefs->useSoundInNotifier);
	ini.WriteBool("NotifyOnLog",prefs->useLogNotifier);
	ini.WriteBool("NotifyOnChat",prefs->useChatNotifier);		  
	ini.WriteBool("NotifierPopEveryChatMessage",prefs->notifierPopsEveryChatMsg);
	ini.WriteBool("NotifyOnDownload",prefs->useDownloadNotifier);
	ini.WriteBool("NotifierPopNewVersion",prefs->notifierNewVersion);
	ini.WriteBool("NotifyOnImportantError", prefs->notifierImportantError);
	ini.WriteBool("NotifyByMail", prefs->sendEmailNotifier);

	buffer.Format("%s",prefs->notifierSoundFilePath);
	ini.WriteString("NotifierSoundPath",buffer);
	buffer.Format("%s",prefs->notifierConfiguration);
	ini.WriteString("NotifierConfiguration",buffer);
	ini.WriteString("TxtEditor",prefs->TxtEditor);
	ini.WriteString("VideoPlayer",prefs->VideoPlayer);
	ini.WriteString("MessageFilter",prefs->messageFilter);
	ini.WriteString("CommentFilter",prefs->commentFilter);
	ini.WriteString("DateTimeFormat",GetDateTimeFormat());
	ini.WriteString("WebTemplateFile",prefs->m_sTemplateFile);

	ini.WriteString("DefaultIRCServer",prefs->m_sircserver);
	ini.WriteString("IRCNick",prefs->m_sircnick);
	ini.WriteBool("IRCAddTimestamp", prefs->m_bircaddtimestamp);
	ini.WriteString("IRCFilterName", prefs->m_sircchannamefilter);
	ini.WriteInt("IRCFilterUser", prefs->m_iircchanneluserfilter);
	ini.WriteBool("IRCUseFilter", prefs->m_bircusechanfilter);
	ini.WriteString("IRCPerformString", prefs->m_sircperformstring);
	ini.WriteBool("IRCUsePerform", prefs->m_bircuseperform);
	ini.WriteBool("IRCListOnConnect", prefs->m_birclistonconnect);
	ini.WriteBool("IRCAcceptLinks", prefs->m_bircacceptlinks);
	ini.WriteBool("IRCIgnoreInfoMessage", prefs->m_bircignoreinfomessage);
	ini.WriteBool("IRCIgnoreEmuleProtoInfoMessage", prefs->m_bircignoreemuleprotoinfomessage);
	ini.WriteBool("SmartIdCheck", prefs->smartidcheck);
	ini.WriteBool("Verbose", prefs->m_bVerbose);
	ini.WriteBool("PreviewPrio", prefs->m_bpreviewprio);
	ini.WriteBool("UpdateQueueListPref", prefs->m_bupdatequeuelist);
	ini.WriteBool("ManualHighPrio", prefs->m_bmanualhighprio);
	ini.WriteBool("FullChunkTransfers", prefs->m_btransferfullchunks);
	ini.WriteBool("StartNextFile", prefs->m_bstartnextfile);
	ini.WriteBool("ShowOverhead", prefs->m_bshowoverhead);
	ini.WriteBool("VideoPreviewBackupped", prefs->moviePreviewBackup);
	ini.WriteInt("FileBufferSizePref", prefs->m_iFileBufferSize);
	ini.WriteInt("QueueSizePref", prefs->m_iQueueSize);
	ini.WriteBool("DAPPref", prefs->m_bDAP);
	ini.WriteBool("UAPPref", prefs->m_bUAP);
	ini.WriteInt("AllcatType",prefs->allcatType);
	ini.WriteBool("ShowAllNotCats", prefs->showAllNotCats);
	ini.WriteBool("FilterServersByIP",prefs->filterserverbyip);
	ini.WriteBool("DisableKnownClientList",prefs->m_bDisableKnownClientList);
	ini.WriteBool("DisableQueueList",prefs->m_bDisableQueueList);
	ini.WriteBool("UseCreditSystem",prefs->m_bCreditSystem);
	ini.WriteBool("SaveLogToDisk",prefs->log2disk);
	ini.WriteBool("SaveDebugToDisk",prefs->debug2disk);
	ini.WriteBool("EnableScheduler",prefs->scheduler);
	ini.WriteBool("MessagesFromFriendsOnly",prefs->msgonlyfriends);
	ini.WriteBool("MessageFromValidSourcesOnly",prefs->msgsecure);
	ini.WriteBool("ShowInfoOnCatTabs",prefs->showCatTabInfos);
	ini.WriteBool("ResumeNextFromSameCat",prefs->resumeSameCat);
	ini.WriteBool("DontRecreateStatGraphsOnResize",prefs->dontRecreateGraphs);

	ini.WriteInt("VersionCheckLastAutomatic", prefs->versioncheckLastAutomatic);
	ini.WriteInt("FilterLevel",prefs->filterlevel);


	ini.SerGet(false, prefs->downloadColumnWidths,
		ELEMENT_COUNT(prefs->downloadColumnWidths), "DownloadColumnWidths");
	ini.SerGet(false, prefs->downloadColumnHidden,
		ELEMENT_COUNT(prefs->downloadColumnHidden), "DownloadColumnHidden");
	ini.SerGet(false, prefs->downloadColumnOrder,
		ELEMENT_COUNT(prefs->downloadColumnOrder), "DownloadColumnOrder");
	ini.SerGet(false, prefs->uploadColumnWidths,
		ELEMENT_COUNT(prefs->uploadColumnWidths), "UploadColumnWidths");
	ini.SerGet(false, prefs->uploadColumnHidden,
		ELEMENT_COUNT(prefs->uploadColumnHidden), "UploadColumnHidden");
	ini.SerGet(false, prefs->uploadColumnOrder,
		ELEMENT_COUNT(prefs->uploadColumnOrder), "UploadColumnOrder");
	ini.SerGet(false, prefs->queueColumnWidths,
		ELEMENT_COUNT(prefs->queueColumnWidths), "QueueColumnWidths");
	ini.SerGet(false, prefs->queueColumnHidden,
		ELEMENT_COUNT(prefs->queueColumnHidden), "QueueColumnHidden");
	ini.SerGet(false, prefs->queueColumnOrder,
		ELEMENT_COUNT(prefs->queueColumnOrder), "QueueColumnOrder");
	ini.SerGet(false, prefs->searchColumnWidths,
		ELEMENT_COUNT(prefs->searchColumnWidths), "SearchColumnWidths");
	ini.SerGet(false, prefs->searchColumnHidden,
		ELEMENT_COUNT(prefs->searchColumnHidden), "SearchColumnHidden");
	ini.SerGet(false, prefs->searchColumnOrder,
		ELEMENT_COUNT(prefs->searchColumnOrder), "SearchColumnOrder");
	ini.SerGet(false, prefs->sharedColumnWidths,
		ELEMENT_COUNT(prefs->sharedColumnWidths), "SharedColumnWidths");
	ini.SerGet(false, prefs->sharedColumnHidden,
		ELEMENT_COUNT(prefs->sharedColumnHidden), "SharedColumnHidden");
	ini.SerGet(false, prefs->sharedColumnOrder,
		ELEMENT_COUNT(prefs->sharedColumnOrder), "SharedColumnOrder");
	ini.SerGet(false, prefs->serverColumnWidths,
		ELEMENT_COUNT(prefs->serverColumnWidths), "ServerColumnWidths");
	ini.SerGet(false, prefs->serverColumnHidden,
		ELEMENT_COUNT(prefs->serverColumnHidden), "ServerColumnHidden");
	ini.SerGet(false, prefs->serverColumnOrder,
		ELEMENT_COUNT(prefs->serverColumnOrder), "ServerColumnOrder");
	ini.SerGet(false, prefs->clientListColumnWidths,
		ELEMENT_COUNT(prefs->clientListColumnWidths), "ClientListColumnWidths");
	ini.SerGet(false, prefs->clientListColumnHidden,
		ELEMENT_COUNT(prefs->clientListColumnHidden), "ClientListColumnHidden");
	ini.SerGet(false, prefs->clientListColumnOrder,
		ELEMENT_COUNT(prefs->clientListColumnOrder), "ClientListColumnOrder");

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	ini.WriteInt("TableSortItemDownload", prefs->tableSortItemDownload);
	ini.WriteInt("TableSortItemUpload", prefs->tableSortItemUpload);
	ini.WriteInt("TableSortItemQueue", prefs->tableSortItemQueue);
	ini.WriteInt("TableSortItemSearch", prefs->tableSortItemSearch);
	ini.WriteInt("TableSortItemShared", prefs->tableSortItemShared);
	ini.WriteInt("TableSortItemServer", prefs->tableSortItemServer);
	ini.WriteInt("TableSortItemClientList", prefs->tableSortItemClientList);
	ini.WriteBool("TableSortAscendingDownload", prefs->tableSortAscendingDownload);
	ini.WriteBool("TableSortAscendingUpload", prefs->tableSortAscendingUpload);
	ini.WriteBool("TableSortAscendingQueue", prefs->tableSortAscendingQueue);
	ini.WriteBool("TableSortAscendingSearch", prefs->tableSortAscendingSearch);
	ini.WriteBool("TableSortAscendingShared", prefs->tableSortAscendingShared);
	ini.WriteBool("TableSortAscendingServer", prefs->tableSortAscendingServer);
	ini.WriteBool("TableSortAscendingClientList", prefs->tableSortAscendingClientList);

	// deadlake PROXYSUPPORT
	ini.WriteBool("ProxyEnablePassword",prefs->proxy.EnablePassword,"Proxy");
	ini.WriteBool("ProxyEnableProxy",prefs->proxy.UseProxy,"Proxy");
	ini.WriteString("ProxyName",prefs->proxy.name,"Proxy");
	ini.WriteString("ProxyPassword",prefs->proxy.password,"Proxy");
	ini.WriteString("ProxyUser",prefs->proxy.user,"Proxy");
	ini.WriteInt("ProxyPort",prefs->proxy.port,"Proxy");
	ini.WriteInt("ProxyType",prefs->proxy.type,"Proxy");

	CString buffer2;
	for (int i=0;i<13;i++) {
		buffer.Format("%lu",(uint32)GetStatsColor(i));
		buffer2.Format("StatColor%i",i);
		ini.WriteString(buffer2,buffer,"eMule");
	}

	buffer.Format("%llu",(uint64)prefs->totalDownloadedBytes);
	ini.WriteString("TotalDownloadedBytes",buffer ,"Statistics");

	buffer.Format("%llu",(uint64)prefs->totalUploadedBytes);
	ini.WriteString("TotalUploadedBytes",buffer ,"Statistics");

	// my own :)
	ini.WriteInt("DesktopMode",prefs->desktopMode);


	// Web Server
	ini.WriteString("Password", GetWSPass(), "WebServer");
	ini.WriteString("PasswordLow", GetWSLowPass());
	ini.WriteInt("Port", prefs->m_nWebPort);
	ini.WriteBool("Enabled", prefs->m_bWebEnabled);
	ini.WriteBool("UseGzip", prefs->m_bWebUseGzip);
	ini.WriteInt("PageRefreshTime", prefs->m_nWebPageRefresh);
	ini.WriteBool("UseLowRightsUser", prefs->m_bWebLowEnabled);
	
	// Madcat - Sources Dropping Tweaks
	ini.WriteBool("NoNeededSources", prefs->DropNoNeededSources, "Razor_Preferences");
	ini.WriteBool("SwapNoNeededSources", prefs->SwapNoNeededSources);
	ini.WriteBool("FullQueueSources", prefs->DropFullQueueSources);
	ini.WriteBool("HighQueueRankingSources", prefs->DropHighQueueRankingSources);
	ini.WriteInt("HighQueueRanking", prefs->HighQueueRanking);
	ini.WriteInt("AutoDropTimer", prefs->AutoDropTimer);
	
	// Kry  - External connections
	ini.WriteBool("AcceptExternalConnections", prefs->AcceptExternalConnections, "ExternalConnect");
	ini.WriteBool("ECUseTCPPort", prefs->ECUseTCPPort);	
	ini.WriteInt("ECPort", prefs->ECPort);	
	ini.WriteString("ECPassword", prefs->ECPassword);	

	// Madcat - Toggle Fast ED2K Links Handler
	ini.WriteBool("FastED2KLinksHandler", prefs->FastED2KLinksHandler);
#endif  // UNIFIED_PREF_HANDLING
}

void CPreferences::SaveCats()
{
	CIni ini( wxString(appdir)+"preferences.ini" , "Category" );

	// Cats
	CString catinif,ixStr,buffer;
	catinif.Format("%sCategory.ini",appdir);
	if(wxFileName::FileExists(catinif)) {
		BackupFile(catinif, ".old");
	}
	if (GetCatCount()>0) {
		CIni catini( catinif, "Category" );
		printf("Opening %s\n",catinif.GetData());
		catini.WriteInt("Count",catMap.GetCount()-1,"General");
		for (size_t ix=1;ix<catMap.GetCount();ix++) {
			ixStr.Format("Cat#%i",ix);
			catini.WriteString("Title",catMap[ix]->title,(char*)ixStr.GetData());
			catini.WriteString("Incoming",catMap[ix]->incomingpath,(char*)ixStr.GetData());
			catini.WriteString("Comment",catMap[ix]->comment,(char*)ixStr.GetData());
			buffer.Format("%lu",(uint32)catMap[ix]->color);
			catini.WriteString("Color",buffer,(char*)ixStr.GetData());
			catini.WriteInt("Priority",catMap[ix]->prio,(char*)ixStr.GetData());
		}
	}
}

#ifndef	UNIFIED_PREF_HANDLING
void CPreferences::ResetStatsColor(int index)
{
	switch(index) {
		case 0 : prefs->statcolors[0]=RGB(0,0,64);break;
		case 1 : prefs->statcolors[1]=RGB(192,192,255);break;
		case 2 : prefs->statcolors[2]=RGB(128, 255, 128);break;
		case 3 : prefs->statcolors[3]=RGB(0, 210, 0);break;
		case 4 : prefs->statcolors[4]=RGB(0, 128, 0);break;
		case 5 : prefs->statcolors[5]=RGB(255, 128, 128);break;
		case 6 : prefs->statcolors[6]=RGB(200, 0, 0);break;
		case 7 : prefs->statcolors[7]=RGB(140, 0, 0);break;
		case 8 : prefs->statcolors[8]=RGB(150, 150, 255);break;
		case 9 : prefs->statcolors[9]=RGB(192,   0, 192);break;
		case 10 : prefs->statcolors[10]=RGB(255, 255, 128);break;
		case 11 : prefs->statcolors[11]=RGB(0, 0, 0);break;
		case 12 : prefs->statcolors[12]=RGB(255, 255, 255);break;

		default:break;
	}
}
#endif

void CPreferences::LoadPreferences()
{
	//--- Quick hack to add version tag to preferences.ini-file and solve the issue with the FlatStatusBar tag...
	CString strFileName;
	strFileName.Format("%spreferences.ini", appdir);
	CIni* pIni = new CIni(strFileName, "eMule");

	CString strCurrVersion, strPrefsVersion;

	strCurrVersion = CURRENT_VERSION_LONG;
	strPrefsVersion = CString(pIni->GetString("AppVersion").GetData());
	delete pIni;
	prefs->m_bFirstStart = false;

	if ((strCurrVersion != strPrefsVersion) && wxFileName::FileExists(strFileName)) {
		// CFile file;
	  	BackupFile(strFileName, ".old");
		strFileName += ".old";
	}
	
	printf("Loading preferences from %s\n",strFileName.c_str());
	
	CIni ini(strFileName, "eMule");
	//--- end Ozon :)

#ifdef	UNIFIED_PREF_HANDLING
	PrefsUnifiedDlg::LoadAllItems(ini);
#else
	
	char buffer[200];

	sprintf(prefs->nick,"%s",ini.GetString("Nick","Someone using aMule (http://amule.sourceforge.net)").GetData());
	
	sprintf(buffer,"%sIncoming",appdir);
	sprintf(prefs->incomingdir,"%s",ini.GetString("IncomingDir",buffer ).GetData());
	MakeFoldername(prefs->incomingdir);

	sprintf(buffer,"%sTemp",appdir);
	sprintf(prefs->tempdir,"%s",ini.GetString("TempDir",buffer).GetData());
	MakeFoldername(prefs->tempdir);

	prefs->maxupload=ini.GetInt("MaxUpload", UNLIMITED);
	prefs->maxdownload=ini.GetInt("MaxDownload",UNLIMITED);

	/* BigBob - Slot Allocation */
	prefs->slotallocation=ini.GetInt("SlotAllocation",3);

	prefs->maxconnections=ini.GetInt("MaxConnections",GetRecommendedMaxConnections());
	prefs->deadserver=ini.GetInt("RemoveDeadServer",2);
	prefs->port=ini.GetInt("Port",4662);
	prefs->udpport=ini.GetInt("UDPPort",prefs->port+10);
	prefs->maxsourceperfile=ini.GetInt("MaxSourcesPerFile",400 );
	prefs->languageID=ini.GetWORD("Language",0);
	prefs->m_iSeeShares=ini.GetInt("SeeShare",2);
	prefs->m_iToolDelayTime=ini.GetInt("ToolTipDelay",1);
	prefs->trafficOMeterInterval=ini.GetInt("StatGraphsInterval",3);
	prefs->statsInterval=ini.GetInt("statsInterval",30);
	prefs->maxGraphDownloadRate=ini.GetInt("DownloadCapacity",96);
	prefs->maxGraphUploadRate=ini.GetInt("UploadCapacity",16);
	prefs->deadserverretries=ini.GetInt("DeadServerRetry",1);
	prefs->m_dwServerKeepAliveTimeout=ini.GetInt("ServerKeepAliveTimeout",0);
	prefs->m_dwListRefresh=ini.GetInt("ListRefresh",0);
	prefs->splitterbarPosition=ini.GetInt("SplitterbarPosition",75);
	prefs->statsMax=ini.GetInt("VariousStatisticsMaxValue",100);
	prefs->statsAverageMinutes=ini.GetInt("StatsAverageMinutes",5);
	prefs->MaxConperFive=ini.GetInt("MaxConnectionsPerFiveSeconds",GetDefaultMaxConperFive());

	prefs->reconnect=ini.GetBool("Reconnect",true);
	prefs->scorsystem=ini.GetBool("Scoresystem",true);
	prefs->ICH=ini.GetBool("ICH",true);
	prefs->autoserverlist=ini.GetBool("Serverlist",false);
	snprintf(prefs->m_szLRUServermetURL,sizeof prefs->m_szLRUServermetURL,"%s",ini.GetString("LRUServermetURL").GetData());

	prefs->updatenotify=ini.GetBool("UpdateNotify",false);
	prefs->mintotray=ini.GetBool("MinToTray",false);
	prefs->addserversfromserver=ini.GetBool("AddServersFromServer",true);
	prefs->addserversfromclient=ini.GetBool("AddServersFromClient",true);
	prefs->splashscreen=ini.GetBool("Splashscreen",true);
	prefs->bringtoforeground=ini.GetBool("BringToFront",true);
	prefs->transferDoubleclick=ini.GetBool("TransferDoubleClick",true);
	prefs->beepOnError=ini.GetBool("BeepOnError",true);
	prefs->confirmExit=ini.GetBool("ConfirmExit",false);
	prefs->filterBadIP=ini.GetBool("FilterBadIPs",true);
	prefs->autoconnect=ini.GetBool("Autoconnect",false);
	prefs->showRatesInTitle=ini.GetBool("ShowRatesOnTitle",false);

	prefs->onlineSig=ini.GetBool("OnlineSignature",false);
	prefs->startMinimized=ini.GetBool("StartupMinimized",false);
	prefs->safeServerConnect =ini.GetBool("SafeServerConnect",false);

	prefs->filterserverbyip=ini.GetBool("FilterServersByIP",false);
	prefs->filterlevel=ini.GetInt("FilterLevel",127);
	prefs->checkDiskspace=ini.GetBool("CheckDiskspace",true);
	prefs->m_uMinFreeDiskSpace=ini.GetInt("MinFreeDiskSpace",0);
	sprintf(prefs->yourHostname,"%s",ini.GetString("YourHostname","").GetData());

	// Barry - New properties...
	prefs->autoconnectstaticonly = ini.GetBool("AutoConnectStaticOnly",false); 
	prefs->autotakeed2klinks = ini.GetBool("AutoTakeED2KLinks",true); 
	prefs->addnewfilespaused = ini.GetBool("AddNewFilesPaused",false); 
	prefs->depth3D = ini.GetInt("3DDepth", 0);

	// as temporarial converter for previous versions
	if (strPrefsVersion < "0.25a") { // before 0.25a
		if (ini.GetBool("FlatStatusBar",false)) {
			prefs->depth3D = 0;
		} else {
			prefs->depth3D = 5;
		}
	}
	prefs->useSoundInNotifier=ini.GetBool("NotifierUseSound",false);
	prefs->useLogNotifier=ini.GetBool("NotifyOnLog",false);
	prefs->useChatNotifier=ini.GetBool("NotifyOnChat",false);
	prefs->notifierPopsEveryChatMsg=ini.GetBool("NotifierPopEveryChatMessage",false);
	prefs->useDownloadNotifier=ini.GetBool("NotifyOnDownload",false); // Added by enkeyDEV
	prefs->notifierNewVersion=ini.GetBool("NotifierPopNewVersion",false);
	prefs->notifierImportantError=ini.GetBool("NotifyOnImportantError",false);
	prefs->sendEmailNotifier=ini.GetBool("NotifyByMail",false);
	sprintf(prefs->notifierSoundFilePath,"%s",ini.GetString("NotifierSoundPath","").GetData());
	sprintf(prefs->notifierConfiguration,"%s",ini.GetString("NotifierConfiguration","").GetData()); // Added by enkeyDEV
	sprintf(prefs->datetimeformat,"%s",ini.GetString("DateTimeFormat","%A, %x, %X").GetData());

	sprintf(prefs->m_sircserver,"%s",ini.GetString("DefaultIRCServer","irc.emule-project.net").GetData());
	sprintf(prefs->m_sircnick,"%s",ini.GetString("IRCNick","eMule").GetData());
	prefs->m_bircaddtimestamp=ini.GetBool("IRCAddTimestamp",true);
	sprintf(prefs->m_sircchannamefilter,"%s",ini.GetString("IRCFilterName", "" ).GetData());
	prefs->m_bircusechanfilter=ini.GetBool("IRCUseFilter", false);
	prefs->m_iircchanneluserfilter=ini.GetInt("IRCFilterUser", 0);
	sprintf(prefs->m_sircperformstring,"%s",ini.GetString("IRCPerformString", "/join #emule" ).GetData());
	prefs->m_bircuseperform=ini.GetBool("IRCUsePerform", false);
	prefs->m_birclistonconnect=ini.GetBool("IRCListOnConnect", true);
	prefs->m_bircacceptlinks=ini.GetBool("IRCAcceptLinks", false);
	prefs->m_bircignoreinfomessage=ini.GetBool("IRCIgnoreInfoMessage", false);
	prefs->m_bircignoreemuleprotoinfomessage=ini.GetBool("IRCIgnoreEmuleProtoInfoMessage", true);
	prefs->smartidcheck=ini.GetBool("SmartIdCheck",true);
	prefs->m_bVerbose=ini.GetBool("Verbose",false);
	prefs->m_bpreviewprio=ini.GetBool("PreviewPrio",false);
	prefs->m_bupdatequeuelist=ini.GetBool("UpdateQueueListPref",false);
	prefs->m_bmanualhighprio=ini.GetBool("ManualHighPrio",false);
	prefs->m_btransferfullchunks=ini.GetBool("FullChunkTransfers",true);
	prefs->m_bstartnextfile=ini.GetBool("StartNextFile",false);
	prefs->m_bshowoverhead=ini.GetBool("ShowOverhead",false);
	prefs->moviePreviewBackup=ini.GetBool("VideoPreviewBackupped",true);
	prefs->m_iFileBufferSize=ini.GetInt("FileBufferSizePref",16);
	prefs->m_iQueueSize=ini.GetInt("QueueSizePref",50);
	prefs->versioncheckdays=ini.GetInt("Check4NewVersionDelay",5);
	prefs->m_bDAP=ini.GetBool("DAPPref",true);
	prefs->m_bUAP=ini.GetBool("UAPPref",true);
	prefs->indicateratings=ini.GetBool("IndicateRatings",true);
	prefs->allcatType=ini.GetInt("AllcatType",0);
	prefs->showAllNotCats=ini.GetBool("ShowAllNotCats",false);
	prefs->watchclipboard=ini.GetBool("WatchClipboard4ED2kFilelinks",false);
	prefs->log2disk=ini.GetBool("SaveLogToDisk",false);
	prefs->debug2disk=ini.GetBool("SaveDebugToDisk",false);
	prefs->iMaxLogMessages = ini.GetInt("MaxLogMessages",1000);
	prefs->showCatTabInfos=ini.GetBool("ShowInfoOnCatTabs",false);
	prefs->resumeSameCat=ini.GetBool("ResumeNextFromSameCat",false);
	prefs->resumeSameCat=ini.GetBool("DontRecreateStatGraphsOnResize",false);

	prefs->versioncheckLastAutomatic=ini.GetInt("VersionCheckLastAutomatic",0);
	prefs->m_bDisableKnownClientList=ini.GetInt("DisableKnownClientList",false);
	prefs->m_bDisableQueueList=ini.GetInt("DisableQueueList",false);
	prefs->m_bCreditSystem=ini.GetInt("UseCreditSystem",true);
	prefs->scheduler=ini.GetBool("EnableScheduler",false);
	prefs->msgonlyfriends=ini.GetBool("MessagesFromFriendsOnly",false);
	prefs->msgsecure=ini.GetBool("MessageFromValidSourcesOnly",true);
	prefs->maxmsgsessions=ini.GetInt("MaxMessageSessions",50);

	sprintf(prefs->TxtEditor,"%s",ini.GetString("TxtEditor","notepad.exe").GetData());
	sprintf(prefs->VideoPlayer,"%s",ini.GetString("VideoPlayer","").GetData());
	
	sprintf(prefs->m_sTemplateFile,"%s",ini.GetString("WebTemplateFile","eMule.tmpl").GetData());

	sprintf(prefs->messageFilter,"%s",ini.GetString("MessageFilter","Your client has an infinite queue").GetData());
	sprintf(prefs->commentFilter,"%s",ini.GetString("CommentFilter","http://").GetData());
	
	ini.SerGet(true, prefs->downloadColumnWidths,
		ELEMENT_COUNT(prefs->downloadColumnWidths), "DownloadColumnWidths", NULL, DEFAULT_COL_SIZE);
	ini.SerGet(true, prefs->downloadColumnHidden,
		ELEMENT_COUNT(prefs->downloadColumnHidden), "DownloadColumnHidden");
	ini.SerGet(true, prefs->downloadColumnOrder,
		ELEMENT_COUNT(prefs->downloadColumnOrder), "DownloadColumnOrder");
	ini.SerGet(true, prefs->uploadColumnWidths,
		ELEMENT_COUNT(prefs->uploadColumnWidths), "UploadColumnWidths", NULL, DEFAULT_COL_SIZE);
	ini.SerGet(true, prefs->uploadColumnHidden,
		ELEMENT_COUNT(prefs->uploadColumnHidden), "UploadColumnHidden");
	ini.SerGet(true, prefs->uploadColumnOrder,
		ELEMENT_COUNT(prefs->uploadColumnOrder), "UploadColumnOrder");
	ini.SerGet(true, prefs->queueColumnWidths,
		ELEMENT_COUNT(prefs->queueColumnWidths), "QueueColumnWidths", NULL, DEFAULT_COL_SIZE);
	ini.SerGet(true, prefs->queueColumnHidden,
		ELEMENT_COUNT(prefs->queueColumnHidden), "QueueColumnHidden");
	ini.SerGet(true, prefs->queueColumnOrder,
		ELEMENT_COUNT(prefs->queueColumnOrder), "QueueColumnOrder");
	ini.SerGet(true, prefs->searchColumnWidths,
		ELEMENT_COUNT(prefs->searchColumnWidths), "SearchColumnWidths", NULL, DEFAULT_COL_SIZE);
	ini.SerGet(true, prefs->searchColumnHidden,
		ELEMENT_COUNT(prefs->searchColumnHidden), "SearchColumnHidden");
	ini.SerGet(true, prefs->searchColumnOrder,
		ELEMENT_COUNT(prefs->searchColumnOrder), "SearchColumnOrder");
	ini.SerGet(true, prefs->sharedColumnWidths,
		ELEMENT_COUNT(prefs->sharedColumnWidths), "SharedColumnWidths", NULL, DEFAULT_COL_SIZE);
	ini.SerGet(true, prefs->sharedColumnHidden,
		ELEMENT_COUNT(prefs->sharedColumnHidden), "SharedColumnHidden");
	ini.SerGet(true, prefs->sharedColumnOrder,
		ELEMENT_COUNT(prefs->sharedColumnOrder), "SharedColumnOrder");
	ini.SerGet(true, prefs->serverColumnWidths,
		ELEMENT_COUNT(prefs->serverColumnWidths), "ServerColumnWidths", NULL, DEFAULT_COL_SIZE);
	ini.SerGet(true, prefs->serverColumnHidden,
		ELEMENT_COUNT(prefs->serverColumnHidden), "ServerColumnHidden");
	ini.SerGet(true, prefs->serverColumnOrder,
		ELEMENT_COUNT(prefs->serverColumnOrder), "ServerColumnOrder");
	ini.SerGet(true, prefs->clientListColumnWidths,
		ELEMENT_COUNT(prefs->clientListColumnWidths), "ClientListColumnWidths", NULL, DEFAULT_COL_SIZE);
	ini.SerGet(true, prefs->clientListColumnHidden,
		ELEMENT_COUNT(prefs->clientListColumnHidden), "ClientListColumnHidden");
	ini.SerGet(true, prefs->clientListColumnOrder,
		ELEMENT_COUNT(prefs->clientListColumnOrder), "ClientListColumnOrder");

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	prefs->tableSortItemDownload = ini.GetInt("TableSortItemDownload", 0);
	prefs->tableSortItemUpload = ini.GetInt("TableSortItemUpload", 0);
	prefs->tableSortItemQueue = ini.GetInt("TableSortItemQueue", 0);
	prefs->tableSortItemSearch = ini.GetInt("TableSortItemSearch", 0);
	prefs->tableSortItemShared = ini.GetInt("TableSortItemShared", 0);
	prefs->tableSortItemServer = ini.GetInt("TableSortItemServer", 0);
	prefs->tableSortItemClientList = ini.GetInt("TableSortItemClientList", 0);
	prefs->tableSortAscendingDownload = ini.GetBool("TableSortAscendingDownload", true);
	prefs->tableSortAscendingUpload = ini.GetBool("TableSortAscendingUpload", true);
	prefs->tableSortAscendingQueue = ini.GetBool("TableSortAscendingQueue", true);
	prefs->tableSortAscendingSearch = ini.GetBool("TableSortAscendingSearch", true);
	prefs->tableSortAscendingShared = ini.GetBool("TableSortAscendingShared", true);
	prefs->tableSortAscendingServer = ini.GetBool("TableSortAscendingServer", true);
	prefs->tableSortAscendingClientList = ini.GetBool("TableSortAscendingClientList", true);

	// deadlake PROXYSUPPORT
	prefs->proxy.EnablePassword = ini.GetBool("ProxyEnablePassword",false,"Proxy");
	prefs->proxy.UseProxy = ini.GetBool("ProxyEnableProxy",false,"Proxy");
	*buffer = '\0';
	sprintf(prefs->proxy.name,"%s",ini.GetString("ProxyName",buffer,"Proxy").GetData());
	sprintf(prefs->proxy.password,"%s",ini.GetString("ProxyPassword",buffer,"Proxy").GetData());
	sprintf(prefs->proxy.user,"%s",ini.GetString("ProxyUser",buffer,"Proxy").GetData());
	prefs->proxy.port = ini.GetInt("ProxyPort",1080,"Proxy");
	prefs->proxy.type = ini.GetInt("ProxyType",PROXYTYPE_NOPROXY,"Proxy");

	CString buffer2;
	bool NoColor = true;
	for (int i=0;i<cntStatColors;i++) {
		buffer2.Format("StatColor%i",i);
		sprintf(buffer,"%s", ini.GetString(buffer2,"0","eMule").c_str());
		if ((prefs->statcolors[i]=atoll(buffer))) {
			NoColor = false;
		}
	}
	if (NoColor) {
		for ( int i=0; i<13; i++ ) {
			ResetStatsColor(i);
		}
	}

	sprintf(buffer,"%s",ini.GetString("TotalDownloadedBytes",0,"Statistics").c_str());
	prefs->totalDownloadedBytes=atoll(buffer);

	sprintf(buffer,"%s", ini.GetString("TotalUploadedBytes",0,"Statistics").c_str());
	prefs->totalUploadedBytes=atoll(buffer);
	
	prefs->desktopMode=ini.GetInt("DesktopMode",4);

	// Web Server
	sprintf(prefs->m_sWebPassword,"%s",ini.GetString("Password", "","WebServer").GetData());
	sprintf(prefs->m_sWebLowPassword,"%s",ini.GetString("PasswordLow", "").GetData());
	prefs->m_nWebPort=ini.GetInt("Port", 4711);
	prefs->m_bWebEnabled=ini.GetBool("Enabled", false);
	prefs->m_bWebUseGzip=ini.GetBool("UseGzip", true);
	prefs->m_bWebLowEnabled=ini.GetBool("UseLowRightsUser", false);
	prefs->m_nWebPageRefresh=ini.GetInt("PageRefreshTime", 120);

	prefs->dontcompressavi=ini.GetBool("DontCompressAvi",false);

	prefs->DropNoNeededSources=ini.GetBool("NoNeededSources", false,"Razor_Preferences");
	prefs->SwapNoNeededSources=ini.GetBool("SwapNoNeededSources", false);
	prefs->DropFullQueueSources=ini.GetBool("FullQueueSources", false);
	prefs->DropHighQueueRankingSources=ini.GetBool("HighQueueRankingSources", false);
	prefs->HighQueueRanking=ini.GetInt("HighQueueRanking", 1200);
	prefs->AutoDropTimer=ini.GetInt("AutoDropTimer", 240);

	// Kry - External Connections
	prefs->AcceptExternalConnections=ini.GetBool("AcceptExternalConnections", true,"ExternalConnect");
	prefs->ECUseTCPPort=ini.GetBool("ECUseTCPPort", false,"ExternalConnect");
	prefs->ECPort=ini.GetInt("ECPort",4712,"ExternalConnect");
	sprintf(prefs->ECPassword,"%s",ini.GetString("ECPassword", "","ExternalConnect").GetData());
	// Madcat - Toggle Fast ED2K Links Handler
	prefs->FastED2KLinksHandler=ini.GetBool("FastED2KLinksHandler", true);
	
#endif  // UNIFIED_PREF_HANDLING

	LoadCats();
}

void CPreferences::LoadCats() {
	CString ixStr,catinif,cat_a,cat_b,cat_c;
	char buffer[100];

	catinif.Format("%sCategory.ini",appdir);

	// default cat
	Category_Struct* newcat=new Category_Struct;
	newcat->title[0] = 0;
	newcat->incomingpath[0] = 0;
	newcat->comment[0] = 0;
	newcat->prio=0;
	newcat->color=0;
	AddCat(newcat);

	// if (!PathFileExists(catinif)) return;
	// surprise, surprise it won't exist
	// the system is now stupid enough to put everything in .eMule :(
	// if(!wxFileName::FileExists(catinif)) return;

	CIni catini( catinif, "Category" );
	int max=catini.GetInt("Count",0,"General");

	for (int ix=1;ix<=max;ix++) {
		ixStr.Format("Cat#%i",ix);

		Category_Struct* newcat=new Category_Struct;
		sprintf(newcat->title,"%s",catini.GetString("Title","",(char*)ixStr.GetData()).c_str());
		sprintf(newcat->incomingpath,"%s",catini.GetString("Incoming","",(char*)ixStr.GetData()).c_str());
		MakeFoldername(newcat->incomingpath);
		sprintf(newcat->comment,"%s",catini.GetString("Comment","",(char*)ixStr.GetData()).c_str());
		newcat->prio =catini.GetInt("Priority",0,(char*)ixStr.GetData());
		sprintf(buffer,"%s",catini.GetString("Color","0",(char*)ixStr.GetData()).c_str());
		newcat->color=atoll(buffer);

		AddCat(newcat);
		if (!wxFileName::DirExists(newcat->incomingpath)) {
			wxFileName::Mkdir(newcat->incomingpath,0777);
		}
	}
}

uint16 CPreferences::GetDefaultMaxConperFive()
{
	return MAXCONPER5SEC;
}


// Barry - Provide a mechanism for all tables to store/retrieve sort order
int32 CPreferences::GetColumnSortItem(Table t) const
{
	switch(t) {
		case tableDownload:
			return prefs->tableSortItemDownload;
		case tableUpload:
			return prefs->tableSortItemUpload;
		case tableQueue:
			return prefs->tableSortItemQueue;
		case tableSearch:
			return prefs->tableSortItemSearch;
		case tableShared:
			return prefs->tableSortItemShared;
		case tableServer:
			return prefs->tableSortItemServer;
		case tableClientList:
			return prefs->tableSortItemClientList;
		case tableNone:
		default:
			return 0;
	}
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
bool CPreferences::GetColumnSortAscending(Table t) const
{
	switch(t) {
		case tableDownload:
			return prefs->tableSortAscendingDownload;
		case tableUpload:
			return prefs->tableSortAscendingUpload;
		case tableQueue:
			return prefs->tableSortAscendingQueue;
		case tableSearch:
			return prefs->tableSortAscendingSearch;
		case tableShared:
			return prefs->tableSortAscendingShared;
		case tableServer:
			return prefs->tableSortAscendingServer;
		case tableClientList:
			return prefs->tableSortAscendingClientList;
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
			prefs->tableSortItemDownload = sortItem;
			break;
		case tableUpload:
			prefs->tableSortItemUpload = sortItem;
			break;
		case tableQueue:
			prefs->tableSortItemQueue = sortItem;
			break;
		case tableSearch:
			prefs->tableSortItemSearch = sortItem;
			break;
		case tableShared:
			prefs->tableSortItemShared = sortItem;
			break;
		case tableServer:
			prefs->tableSortItemServer = sortItem;
			break;
		case tableClientList:
			prefs->tableSortItemClientList = sortItem;
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
			prefs->tableSortAscendingDownload = sortAscending;
			break;
		case tableUpload:
			prefs->tableSortAscendingUpload = sortAscending;
			break;
		case tableQueue:
			prefs->tableSortAscendingQueue = sortAscending;
			break;
		case tableSearch:
			prefs->tableSortAscendingSearch = sortAscending;
			break;
		case tableShared:
			prefs->tableSortAscendingShared = sortAscending;
			break;
		case tableServer:
			prefs->tableSortAscendingServer = sortAscending;
			break;
		case tableClientList:
			prefs->tableSortAscendingClientList = sortAscending;
			break;
		case tableNone:
		default:
			break;
	}
}

void CPreferences::RemoveCat(size_t index)
{
	if (index>=0 && index<catMap.GetCount()) {
		catMap.RemoveAt(index);
	}
}
