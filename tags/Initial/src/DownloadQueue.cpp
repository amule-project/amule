//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
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



#include <wx/defs.h>		// Needed before any other wx/*.h
#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/msw/winundef.h>
#else
	#include <netdb.h>		// Needed for gethostbyname_r
	#include <sys/socket.h>		//
	#include <netinet/in.h>		// These three are needed for inet_ntoa
	#include <arpa/inet.h>		//
#endif
#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/textfile.h>	// Needed for wxTextFile
#include <wx/filename.h>
#include <wx/listimpl.cpp>
#include <wx/file.h>
#include <wx/utils.h>
#include <wx/intl.h>		// Needed for _

#include "DownloadQueue.h"	// Interface declarations
#include "server.h"		// Needed for CServer
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "packets.h"		// Needed for Packet
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "ClientList.h"		// Needed for CClientList
#include "updownclient.h"	// Needed for CUpDownClient
#include "ServerList.h"		// Needed for CServerList
#include "sockets.h"		// Needed for CServerConnect
#include "SysTray.h"		// Needed for TBN_DLOAD
#include "ED2KLink.h"		// Needed for CED2KFileLink
#include "SearchList.h"		// Needed for CSearchFile
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "PartFile.h"		// Needed for CPartFile
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "CamuleAppBase.h"	// Needed for theApp
#include "opcodes.h"		// Needed for MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE
#include "otherfunctions.h"	// Needed for GetTickCount
#include "SearchDlg.h"		// Needed for CSearchDlg->GetCatChoice()

// Max. file IDs per UDP packet
// ----------------------------
// 576 - 30 bytes of header (28 for UDP, 2 for "E3 9A" edonkey proto) = 546 bytes
// 546 / 16 = 34

#define MAX_FILES_PER_UDP_PACKET	31	// 2+16*31 = 498 ... is still less than 512 bytes!!
#define MAX_REQUESTS_PER_SERVER		35


CDownloadQueue::CDownloadQueue(CPreferences* in_prefs,CSharedFileList* in_sharedfilelist)
{
	app_prefs = in_prefs;
	sharedfilelist = in_sharedfilelist;
	filesrdy = 0;
	datarate = 0;
	cur_udpserver = 0;
	lastfile = 0;
	lastcheckdiskspacetime = 0;
	lastudpsearchtime = 0;
	lastudpstattime = 0;
	udcounter = 0;
	m_iSearchedServers = 0;
	m_datarateMS=0;
	m_nDownDataRateMSOverhead = 0;
	m_nDownDatarateOverhead = 0;
	m_nDownDataOverheadSourceExchange = 0;
	m_nDownDataOverheadFileRequest = 0;
	m_nDownDataOverheadOther = 0;
	m_nDownDataOverheadServer = 0;
	m_nDownDataOverheadSourceExchangePackets = 0;
	m_nDownDataOverheadFileRequestPackets = 0;
	m_nDownDataOverheadOtherPackets = 0;
	m_nDownDataOverheadServerPackets = 0;
	m_nLastED2KLinkCheck = 0;
	m_lastRefreshedDLDisplay = 0;
	m_dwNextTCPSrcReq = 0;
	m_cRequestsSentToServer = 0;
}

void CDownloadQueue::UpdateDisplayedInfo(bool force)
{
	DWORD curTick = ::GetTickCount();

	if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+(uint32)(rand()/(RAND_MAX/1000))) {
		theApp.amuledlg->transferwnd->downloadlistctrl->UpdateItem(this);
		m_lastRefreshedDLDisplay = curTick;
	}
}

void CDownloadQueue::AddPartFilesToShare()
{
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)) {
		CPartFile* cur_file = filelist.GetAt(pos);
		if (cur_file->GetStatus(true) == PS_READY) {
			sharedfilelist->SafeAddKFile(cur_file,true);
			printf("Sharing %s\n",cur_file->GetFullName());
		}
	}
}

void CDownloadQueue::CompDownDatarateOverhead()
{
	m_AvarageDDRO_list.AddTail(m_nDownDataRateMSOverhead);
	if (m_AvarageDDRO_list.GetCount() > 150) {
		m_AvarageDDRO_list.RemoveAt(m_AvarageDDRO_list.GetHeadPosition());
	}
	m_nDownDatarateOverhead = 0;
	m_nDownDataRateMSOverhead = 0;
	for (POSITION pos = m_AvarageDDRO_list.GetHeadPosition();pos != 0;m_AvarageDDRO_list.GetNext(pos)) {
		m_nDownDatarateOverhead += m_AvarageDDRO_list.GetAt(pos);
	}
	if(m_AvarageDDRO_list.GetCount() > 10) {
		m_nDownDatarateOverhead = 10*m_nDownDatarateOverhead/m_AvarageDDRO_list.GetCount();
	} else {
		m_nDownDatarateOverhead = 0;
	}
	return;
}

void CDownloadQueue::Init()
{
	// find all part files, read & hash them if needed and store into a list
	int count = 0;

	wxString searchPath(app_prefs->GetTempDir());
	searchPath += "/*.part.met";

	// check all part.met files
	printf("Loading temp files from %s.\n",searchPath.GetData());
	wxString fileName=::wxFindFirstFile(searchPath,wxFILE);
	while(!fileName.IsEmpty()) {
		wxFileName myFileName(fileName);
		printf("Loading %s... ",myFileName.GetFullName().GetData());
		CPartFile* toadd = new CPartFile();
		if (toadd->LoadPartFile(app_prefs->GetTempDir(),(char*)myFileName.GetFullName().GetData())) {
			count++;
			printf("Done.\n");
			filelist.AddTail(toadd); // to downloadqueue
			if (toadd->GetStatus(true) == PS_READY) {
				sharedfilelist->SafeAddKFile(toadd); // part files are always shared files
			}
			theApp.amuledlg->transferwnd->downloadlistctrl->AddFile(toadd);// show in downloadwindow
		} else {
			printf("ERROR!\n");
			delete toadd;
		}
		fileName=::wxFindNextFile();
	}
	if(count == 0) {
		theApp.amuledlg->AddLogLine(false, CString(_("No part files found")));
	} else {
		theApp.amuledlg->AddLogLine(false, CString(_("Found %i part files")),count);
		SortByPriority();
		CheckDiskspace();
	}
}

CDownloadQueue::~CDownloadQueue()
{
	for (POSITION pos =filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)) {
		delete filelist.GetAt(pos);
	}

}

void CDownloadQueue::SavePartFiles(bool del /*= false*/)
{
	for (POSITION pos =filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)) {
		filelist.GetAt(pos)->m_hpartfile.Flush();
		filelist.GetAt(pos)->SavePartFile();
		if (del) {
			delete filelist.GetAt(pos);
		}
	}
}

void CDownloadQueue::AddSearchToDownload(CSearchFile* toadd,uint8 paused)
{
	if (IsFileExisting(toadd->GetFileHash())) {
		return;
	}
	CPartFile* newfile = new CPartFile(toadd);
	if (!newfile) {
		return;
	}
	if (newfile->GetStatus() == PS_ERROR) {
		delete newfile;
		return;
	}
	if (paused==2) {
		paused=(uint8)theApp.glob_prefs->AddNewFilesPaused();
	}
	AddDownload(newfile, (paused==1));
	newfile->SetCategory(theApp.amuledlg->searchwnd->GetCatChoice());
}

void CDownloadQueue::AddSearchToDownload(CString link,uint8 paused)
{
	CPartFile* newfile = new CPartFile(link);
	if (!newfile) {
		return;
	}
	if (newfile->GetStatus() == PS_ERROR){
		delete newfile;
		return;
	}
	if (paused==2) {
		paused=(uint8)theApp.glob_prefs->AddNewFilesPaused();
	}
	AddDownload(newfile, (paused==1));
	newfile->SetCategory(theApp.amuledlg->searchwnd->GetCatChoice());
}

void CDownloadQueue::StartNextFile()
{
	if(!theApp.glob_prefs->StartNextFile()) {
		return;
	}

	CPartFile*  pfile = NULL;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)) {
		CPartFile* cur_file = filelist.GetAt(pos);
		if (cur_file->GetStatus() == PS_PAUSED) {
			if (!pfile) {
				pfile = cur_file;
				if (pfile->GetDownPriority() == PR_HIGH) {
					break;
				}
			} else {
				if (cur_file->GetDownPriority() > pfile->GetDownPriority()) {
					pfile = cur_file;
					if (pfile->GetDownPriority() == PR_HIGH) {
						break;
					}
				}
			}
		}
	}
	if (pfile) {
		pfile->ResumeFile();
	}
}

void CDownloadQueue::AddFileLinkToDownload(CED2KFileLink* pLink)
{
	CPartFile* newfile = new CPartFile(pLink);
	if (!newfile) {
		return;
	}
	if (newfile->GetStatus() == PS_ERROR) {
		delete newfile;
		newfile=NULL;
	} else {
		AddDownload(newfile,theApp.glob_prefs->AddNewFilesPaused());
	}
	if (pLink->HasValidSources()) {
		if (newfile) {
			newfile->AddClientSources(pLink->SourcesList,1);
		} else {
			CPartFile* partfile = GetFileByID((uchar*)pLink->GetHashKey());
			if (partfile) {
				partfile->AddClientSources(pLink->SourcesList,1);
			}
		}
	}
	if(pLink->HasHostnameSources()) {
		for (POSITION pos = pLink->m_HostnameSourcesList.GetHeadPosition(); pos != NULL; pLink->m_HostnameSourcesList.GetNext(pos)) {
			AddToResolve((uchar*)pLink->GetHashKey(), pLink->m_HostnameSourcesList.GetAt(pos)->strHostname, pLink->m_HostnameSourcesList.GetAt(pos)->nPort);
		}
	}
}

void CDownloadQueue::AddDownload(CPartFile* newfile,bool paused)
{
	// Creteil - Add in paused mode if there is already file(s) downloading
	if ((paused) && filelist.GetHeadPosition() != 0) {
		newfile->StopFile();
	}

	filelist.AddTail(newfile);
	SortByPriority();
	CheckDiskspace();

	newfile->SetCategory(theApp.amuledlg->searchwnd->GetCatChoice());
	theApp.amuledlg->transferwnd->downloadlistctrl->AddFile(newfile);
	theApp.amuledlg->AddLogLine(true, CString(_("Downloading %s")),newfile->GetFileName().GetData());
	CString msgTemp;
	msgTemp.Format(CString(_("Downloading %s"))+"\n",newfile->GetFileName().GetData());
	theApp.amuledlg->ShowNotifier(msgTemp, TBN_DLOAD);
	// Kry - Get sources if not stopped
	if (!newfile->IsStopped()) {
		SendLocalSrcRequest(newfile);
	}
}

bool CDownloadQueue::IsFileExisting(uchar* fileid)
{
	if (CKnownFile* file = sharedfilelist->GetFileByID((uchar*)fileid)) {
		if (file->IsPartFile()) {
			theApp.amuledlg->AddLogLine(true, CString(_("You are already trying to download the file %s")), file->GetFileName().GetData());
		} else {
			theApp.amuledlg->AddLogLine(true, CString(_("You already have the file %s")), file->GetFileName().GetData());
		}
		return true;
	} else if ((file = this->GetFileByID((uchar*)fileid))) {
		theApp.amuledlg->AddLogLine(true, CString(_("You are already trying to download the file %s")), file->GetFileName().GetData());
		return true;
	}
	return false;
}


void CDownloadQueue::Process()
{
	ProcessLocalRequests(); // send src requests to local server
		
	uint32 downspeed = 0;
	if (app_prefs->GetMaxDownload() != UNLIMITED && datarate > 1500) {
		downspeed = (app_prefs->GetMaxDownload()*1024*100)/(datarate+1); //(uint16)((float)((float)(app_prefs->GetMaxDownload()*1024)/(datarate+1)) * 100);
		if (downspeed < 50) {
			downspeed = 50;
		} else if (downspeed > 200) {
			downspeed = 200;
		}
	}

	datarate = 0;
	for (POSITION pos =filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		CPartFile* cur_file =  filelist.GetAt(pos);
		if ((cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY) && (!cur_file->IsAutoDownPriority() && cur_file->GetDownPriority() == PR_HIGH)) {
			datarate += cur_file->Process(downspeed);
		}
	}
	for (POSITION pos =filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		CPartFile* cur_file =  filelist.GetAt(pos);
		if ((cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY) && (!cur_file->IsAutoDownPriority() && cur_file->GetDownPriority() == PR_NORMAL)) {
			datarate += cur_file->Process(downspeed);
		}
	}
	for (POSITION pos =filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		CPartFile* cur_file =  filelist.GetAt(pos);
		if ((cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY) && (cur_file->GetDownPriority() == PR_LOW || cur_file->IsAutoDownPriority())) {
			datarate += cur_file->Process(downspeed);
		}
	}

	udcounter++;
	if (udcounter == 5) {
		if (theApp.serverconnect->IsUDPSocketAvailable()) {
			if((!lastudpstattime) || (::GetTickCount() - lastudpstattime) > UDPSERVERSTATTIME) {
				lastudpstattime = ::GetTickCount();
				theApp.serverlist->ServerStats();
			}
		}
	}
	if (udcounter == 10) {
		udcounter = 0;
		if (theApp.serverconnect->IsUDPSocketAvailable()) {
			if ((!lastudpsearchtime) || (::GetTickCount() - lastudpsearchtime) > UDPSERVERREASKTIME) {
				SendNextUDPPacket();
			}
		}
	}

	if ((!lastcheckdiskspacetime) || (::GetTickCount() - lastcheckdiskspacetime) > DISKSPACERECHECKTIME) {
		CheckDiskspace();
	}

	// Check for new links once per second.
	if ((::GetTickCount() - m_nLastED2KLinkCheck) >= 1000) {
		wxString filename = filename.Format("%s/.aMule/ED2KLinks", getenv("HOME"));
		if (wxFile::Exists(filename)) {
			AddLinksFromFile();
		}
		m_nLastED2KLinkCheck = ::GetTickCount();
	}
}

CPartFile* CDownloadQueue::GetFileByID(uchar* filehash){
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		if (!memcmp(filehash,filelist.GetAt(pos)->GetFileHash(),16))
			return filelist.GetAt(pos);
	}
	return 0;
}

CPartFile* CDownloadQueue::GetFileByIndex(int index){
//      if (index>=filelist.GetCount()) return 0;
	int count=0;

	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		if (count==index) return filelist.GetAt(pos);
		count++;
	}
	return 0;
}

bool CDownloadQueue::IsPartFile(void* totest){
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos))
		if (totest == filelist.GetAt(pos))
			return true;
	return false;
}

void CDownloadQueue::CheckAndAddSource(CPartFile* sender,CUpDownClient* source)
{
	// if we block loopbacks at this point it should prevent us from connecting to ourself
	// Bodo: If one has running two clients running on the same system,
	// Bodo: does this prevent us from conneting to the other client?
	if(!source->HasLowID() && (source->GetUserID() & 0xFF) == 0x7F) {
		delete source;
		return;
	}

	/*
	if (IsBlacklisted(source)) {
		delete source;
		return;
	}
	*/

	if (sender->IsStopped()) {
		delete source;
		return;
	}

	// "Filter LAN IPs" and/or "IPfilter" is not required here,
	// because it was already done in parent functions

	// uses this only for temp. clients
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)) {
		CPartFile* cur_file = filelist.GetAt(pos);
		for (int sl=0;sl<SOURCESSLOTS;sl++) if (!cur_file->srclists[sl].IsEmpty())
		for (POSITION pos2 = cur_file->srclists[sl].GetHeadPosition();pos2 != 0; cur_file->srclists[sl].GetNext(pos2)) {
			if (cur_file->srclists[sl].GetAt(pos2)->Compare(source)) {
			//if (cur_file->srclists[sl].GetAt(pos2)->Compare(source) || cur_file->srclists[sl].GetAt(pos2)->Compare(source, false)) {
				if (cur_file == sender) { // this file has already this source
					delete source;
					return;
				}
				// set request for this source
				if (cur_file->srclists[sl].GetAt(pos2)->AddRequestForAnotherFile(sender)) {
					// add it to uploadlistctrl
					theApp.amuledlg->transferwnd->downloadlistctrl->AddSource(sender,cur_file->srclists[sl].GetAt(pos2),true);
					delete source;
					return;
				}
				else{
					delete source;
					return;
				}
			}
		}
	}

	//our new source is real new but maybe it is already uploading to us?
	//if yes the known client will be attached to the var "source"
	//and the old sourceclient will be deleted
	if (theApp.clientlist->AttachToAlreadyKnown(&source,0)) {
		source->reqfile = sender;
		// No more need for this
		// source->SetDownloadFile(sender);
	} else {
		theApp.clientlist->AddClient(source,true);
	}

	if (source->GetFileRate()>0 || source->GetFileComment().GetLength()>0) {
		sender->UpdateFileRatingCommentAvail();
	}
	sender->srclists[source->sourcesslot].AddTail(source);
	sender->IsCountDirty = true;
	theApp.amuledlg->transferwnd->downloadlistctrl->AddSource(sender,source,false);
	UpdateDisplayedInfo();
}

void CDownloadQueue::CheckAndAddKnownSource(CPartFile* sender,CUpDownClient* source)
{
	if(!source->HasLowID() && (source->GetUserID() & 0xFF) == 0x7F) {
		return;
	}

	/*
	if (sender->IsStopped()) {
		return;
	}
	*/

	// use this for client which are already know (downloading for example)
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)) {
		CPartFile* cur_file = filelist.GetAt(pos);
		for (int sl=0;sl<SOURCESSLOTS;sl++) {
			if (!cur_file->srclists[sl].IsEmpty()) {
				if (cur_file->srclists[sl].Find(source)) {
					if (cur_file == sender) {
						return;
					}
					if (source->AddRequestForAnotherFile(sender)) {
						theApp.amuledlg->transferwnd->downloadlistctrl->AddSource(sender,source,true);
					}
					return;
				}
			}
		}
	}
	source->reqfile = sender;
	// No more need for this
	// source->SetDownloadFile(sender);

	if (source->GetFileRate()>0 || source->GetFileComment().GetLength()>0) {
		sender->UpdateFileRatingCommentAvail();
	}

	sender->srclists[source->sourcesslot].AddTail(source);
	sender->IsCountDirty = true;
	theApp.amuledlg->transferwnd->downloadlistctrl->AddSource(sender,source,false);
	UpdateDisplayedInfo();
}

/* Creteil importing new method from eMule 0.30c */
bool CDownloadQueue::RemoveSource(CUpDownClient* toremove, bool	updatewindow, bool bDoStatsUpdate)
{
	bool removed = false;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)) {
		CPartFile* cur_file = filelist.GetAt(pos);
		for (int sl=0;sl<SOURCESSLOTS;sl++) if (!cur_file->srclists[sl].IsEmpty()) {
			for (POSITION pos2 = cur_file->srclists[sl].GetHeadPosition();pos2 != 0; cur_file->srclists[sl].GetNext(pos2)) {
				if (toremove == cur_file->srclists[sl].GetAt(pos2)) {
					cur_file->srclists[sl].RemoveAt(pos2);
					cur_file->IsCountDirty = true;
					removed = true;
					if ( bDoStatsUpdate ){
						cur_file->RemoveDownloadingSource(toremove);
						cur_file->NewSrcPartsInfo();
					}
					#if 0
					/* Razor 1a - Modif by MikaelB */
					if(!toremove->m_OtherRequests_list.IsEmpty())  {
						POSITION pos3, pos4;
						for(pos3=toremove->m_OtherRequests_list.GetHeadPosition();(pos4=pos3)!=NULL;) {
							toremove->m_OtherRequests_list.GetNext(pos3);
							POSITION pos5 = toremove->m_OtherRequests_list.GetAt(pos4)->A4AFSourcesList.Find(toremove);
							if(pos5) {
								toremove->m_OtherRequests_list.GetAt(pos4)->A4AFSourcesList.RemoveAt(pos5);
								theApp.amuledlg->transferwnd->downloadlistctrl->RemoveSource(toremove,toremove->m_OtherRequests_list.GetAt(pos4));
								toremove->m_OtherRequests_list.RemoveAt(pos4);
							}
						}
					}
					cur_file->RemoveDownloadingSource(toremove);
					if (updatewindow) {
						toremove->SetDownloadState(DS_NONE);
						theApp.amuledlg->transferwnd->downloadlistctrl->RemoveSource(toremove, cur_file);
					}
					toremove->ResetFileStatusInfo();
					toremove->reqfile = 0;				
					/* End modif */
					#endif
					break;
				}
			}
		}
		if (bDoStatsUpdate) {
			cur_file->UpdateAvailablePartsCount();
		}
	}

	/* Creteil Changes BEGIN */
	
	// remove this source on all files in the downloadqueue who link this source
	// pretty slow but no way arround, maybe using a Map is better, but that's slower on other parts
	POSITION pos3, pos4;
	for(pos3 = toremove->m_OtherRequests_list.GetHeadPosition();(pos4=pos3)!=NULL;) {
		toremove->m_OtherRequests_list.GetNext(pos3);
		POSITION pos5 = toremove->m_OtherRequests_list.GetAt(pos4)->A4AFsrclist.Find(toremove);
		if(pos5) { 
			toremove->m_OtherRequests_list.GetAt(pos4)->A4AFsrclist.RemoveAt(pos5);
			theApp.amuledlg->transferwnd->downloadlistctrl->RemoveSource(toremove,toremove->m_OtherRequests_list.GetAt(pos4));
			toremove->m_OtherRequests_list.RemoveAt(pos4);
		}
	}
	for(pos3 = toremove->m_OtherNoNeeded_list.GetHeadPosition();(pos4=pos3)!=NULL;) {
		toremove->m_OtherNoNeeded_list.GetNext(pos3);
		POSITION pos5 = toremove->m_OtherNoNeeded_list.GetAt(pos4)->A4AFsrclist.Find(toremove);
		if(pos5) { 
			toremove->m_OtherNoNeeded_list.GetAt(pos4)->A4AFsrclist.RemoveAt(pos5);
			theApp.amuledlg->transferwnd->downloadlistctrl->RemoveSource(toremove,toremove->m_OtherNoNeeded_list.GetAt(pos4));
			toremove->m_OtherNoNeeded_list.RemoveAt(pos4);
		}
	}

	if (toremove->GetFileComment().GetLength()>0 || toremove->GetFileRate()>0) {
		toremove->reqfile->UpdateFileRatingCommentAvail();
	}

	/* Creteil changes END */

	if (updatewindow) {
		toremove->SetDownloadState(DS_NONE);
		theApp.amuledlg->transferwnd->downloadlistctrl->RemoveSource(toremove,0);
	}
	toremove->ResetFileStatusInfo();
	toremove->reqfile = 0;
	return removed;
}

void CDownloadQueue::RemoveFile(CPartFile* toremove)
{
	/*
	POSITION pos1, pos2;
	// remove this file from the local server request queue
	for (pos1 = m_localServerReqQueue.GetHeadPosition(); ( pos2 = pos1 ) != NULL;) {
		m_localServerReqQueue.GetNext(pos1);
		CPartFile* cur_file = m_localServerReqQueue.GetAt(pos2);
		if (cur_file == toremove) {
			m_localServerReqQueue.RemoveAt(pos2);
		}
	}
	*/

	RemoveLocalServerRequest(toremove);

	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)) {
		if (toremove == filelist.GetAt(pos)) {
			filelist.RemoveAt(pos);
			return;
		}
	}
	SortByPriority();
	CheckDiskspace();
}

void CDownloadQueue::DeleteAll(){
	POSITION pos;
	for (pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		CPartFile* cur_file = filelist.GetAt(pos);
		for (int sl=0;sl<SOURCESSLOTS;sl++) if (!cur_file->srclists[sl].IsEmpty())
			cur_file->srclists[sl].RemoveAll();
			cur_file->IsCountDirty = true;
		// Barry - Should also remove all requested blocks
		// Don't worry about deleting the blocks, that gets handled 
		// when CUpDownClient is deleted in CClientList::DeleteAll()
		cur_file->RemoveAllRequestedBlocks();
	}
}

bool CDownloadQueue::SendNextUDPPacket()
{
	
	if (filelist.IsEmpty() || !theApp.serverconnect->IsUDPSocketAvailable()	|| !theApp.serverconnect->IsConnected()) {
		return false;
	}
	if (!cur_udpserver) {
		if (!(cur_udpserver = theApp.serverlist->GetNextServer(cur_udpserver))) {
			StopUDPRequests();
		}
		m_cRequestsSentToServer = 0;
	}

	// get max. file ids per packet for current server
	int iMaxFilesPerPacket = GetMaxFilesPerUDPServerPacket();

	// loop until the packet is filled or a packet was sent
	bool bSentPacket = false;
	CSafeMemFile dataGlobGetSources(16);
	int iFiles = 0;
	while (iFiles < iMaxFilesPerPacket && !bSentPacket) {
		// get next file to search sources for
		CPartFile* nextfile = NULL;
		while (!bSentPacket && !(nextfile && (nextfile->GetStatus() == PS_READY || nextfile->GetStatus() == PS_EMPTY))) {
			if (lastfile == NULL) {
				// we just started the global source searching
				// or have switched the server
				// get first file to search sources for
				nextfile = filelist.GetHead();
				lastfile = nextfile;
			} else {
				POSITION pos = filelist.Find(lastfile);
				if (pos == 0) {
					// the last file is no longer in the DL-list
					// (may have been finished or canceld)
					// get first file to search sources for
					nextfile = filelist.GetHead();
					lastfile = nextfile;
				} else {
					filelist.GetNext(pos);
					if (pos == 0) {
						// finished asking the current server for all files
						// if there are pending requests for the current server, send them
						if (dataGlobGetSources.GetLength() > 0) {
							if (SendGlobGetSourcesUDPPacket(dataGlobGetSources)) {
								bSentPacket = true;
							}
							dataGlobGetSources.SetLength(0);
						}

						// get next server to ask
						cur_udpserver = theApp.serverlist->GetNextServer(cur_udpserver);
						m_cRequestsSentToServer = 0;
						if (cur_udpserver == NULL) {
							// finished asking all servers for all files
							lastudpsearchtime = ::GetTickCount();
							lastfile = NULL;
							m_iSearchedServers = 0;
							return false; // finished (processed all file & all servers)
						}
						m_iSearchedServers++;

						// if we already sent a packet, switch to the next file at next function call
						if (bSentPacket) {
							lastfile = NULL;
							break;
						}

						// get max. file ids per packet for current server
						iMaxFilesPerPacket = GetMaxFilesPerUDPServerPacket();

						// have selected a new server; get first file to search sources for
						nextfile = filelist.GetHead();
						lastfile = nextfile;
					} else {
						nextfile = filelist.GetAt(pos);
						lastfile = nextfile;
					}
				}
			}
		}

		if (!bSentPacket && nextfile && nextfile->GetSourceCount() < theApp.glob_prefs->GetMaxSourcePerFileUDP()) {
			dataGlobGetSources.Write((const uint8*)nextfile->GetFileHash());
			iFiles++;
		}
	}

	//ASSERT( dataGlobGetSources.GetLength() == 0 || !bSentPacket );

	if (!bSentPacket && dataGlobGetSources.GetLength() > 0) {
		SendGlobGetSourcesUDPPacket(dataGlobGetSources);
	}

	// send max 40 udp request to one server per interval, if we have more than 40 files, we rotate the list and use it as Queue
	if (m_cRequestsSentToServer >= MAX_REQUESTS_PER_SERVER) {
		// move the last 40 files to the head
		if (filelist.GetCount() >= MAX_REQUESTS_PER_SERVER) {
			for (int i = 0; i != MAX_REQUESTS_PER_SERVER; i++) {
				filelist.AddHead(filelist.RemoveTail());
			}
		}

		// and next server
		cur_udpserver = theApp.serverlist->GetNextServer(cur_udpserver);
		m_cRequestsSentToServer = 0;
		if (cur_udpserver == NULL) {
			lastudpsearchtime = ::GetTickCount();
			lastfile = NULL;
			return false; // finished (processed all file & all servers)
		}
		m_iSearchedServers++;
		lastfile = NULL;
	}
	return true;
}

void CDownloadQueue::StopUDPRequests()
{
	cur_udpserver = 0;
	lastudpsearchtime = ::GetTickCount();
	lastfile = 0;
}
/*
void CDownloadQueue::SortByPriority()
{
	POSITION pos1, pos2;
	uint16 i = 0;
	for(pos1 = filelist.GetHeadPosition(); (pos2 = pos1 ) != NULL;) {
		filelist.GetNext(pos1);
		CPartFile* cur_file = filelist.GetAt(pos2);
		if (cur_file->GetDownPriority() == PR_HIGH) {
			filelist.AddHead(cur_file);
			filelist.RemoveAt(pos2);
		} else if (cur_file->GetDownPriority() == PR_LOW) {
			filelist.AddTail(cur_file);
			filelist.RemoveAt(pos2);
		}
		i++;
		if (i == filelist.GetCount()) {
			break;
		}
	}
}
*/

void CDownloadQueue::SortByPriority()
{
	uint16 n = filelist.GetCount();
	if (!n) {
		return;
	}
	uint16 i;
	for ( i = n/2; i--; ) {
		HeapSort(i, n-1);
	}
	for ( i = n; --i; ) {
		SwapParts(filelist.FindIndex(0), filelist.FindIndex(i));
		HeapSort(0, i-1);
	}
}

bool CDownloadQueue::CompareParts(POSITION pos1, POSITION pos2)
{
	CPartFile* file1 = filelist.GetAt(pos1);
	CPartFile* file2 = filelist.GetAt(pos2);
	if (file1->GetDownPriority() == file2->GetDownPriority()) {
		return (wxString::wxString(file1->GetPartMetFileName()).CmpNoCase(file2->GetPartMetFileName()))>=0;
	}
	if (file1->GetDownPriority() < file2->GetDownPriority()) {
		return true;
	}
	return false;
}

void CDownloadQueue::SwapParts(POSITION pos1, POSITION pos2)
{
	CPartFile* file1 = filelist.GetAt(pos1);
	CPartFile* file2 = filelist.GetAt(pos2);
	filelist.SetAt(pos1, file2);
	filelist.SetAt(pos2, file1);
}

void CDownloadQueue::HeapSort(uint16 first, uint16 last)
{
	uint16 r;
	POSITION pos1 = filelist.FindIndex(first);
	for ( r = first; !(r & 0x8000) && (r<<1) < last; ) {
		uint16 r2 = (r<<1)+1;
		POSITION pos2 = filelist.FindIndex(r2);
		if (r2 != last) {
			POSITION pos3 = pos2;
			filelist.GetNext(pos3);
			if (!CompareParts(pos2, pos3)) {
				pos2 = pos3;
				r2++;
			}
		}
		if (!CompareParts(pos1, pos2)) {
			SwapParts(pos1, pos2);
			r = r2;
			pos1 = pos2;
		} else {
			break;
		}
	}
}

void CDownloadQueue::ResetLocalServerRequests()
{
	m_dwNextTCPSrcReq = 0;
	m_localServerReqQueue.RemoveAll();

	POSITION pos = filelist.GetHeadPosition();
	while (pos != NULL)
	{ 
		CPartFile* pFile = filelist.GetNext(pos);
		UINT uState = pFile->GetStatus();
		if (uState == PS_READY || uState == PS_EMPTY)
			pFile->ResumeFile();
		pFile->m_bLocalSrcReqQueued = false;
	}
}

void CDownloadQueue::RemoveLocalServerRequest(CPartFile* pFile)
{
	POSITION pos1, pos2;
	for (pos1 = m_localServerReqQueue.GetHeadPosition(); (pos2 = pos1) != NULL;) {
		m_localServerReqQueue.GetNext(pos1);
		if (m_localServerReqQueue.GetAt(pos2) == pFile)	{
			m_localServerReqQueue.RemoveAt(pos2);
			pFile->m_bLocalSrcReqQueued = false;
			// could 'break' here.. fail safe: go through entire list..
		}
	}
}

void CDownloadQueue::ProcessLocalRequests()
{
	if ( (!m_localServerReqQueue.IsEmpty()) && (m_dwNextTCPSrcReq < ::GetTickCount()) )
	{
		CSafeMemFile dataTcpFrame(22);
		const int iMaxFilesPerTcpFrame = 15;
		int iFiles = 0;
		while (!m_localServerReqQueue.IsEmpty() && iFiles < iMaxFilesPerTcpFrame)
		{
			// find the file with the longest waitingtime
			POSITION pos1, pos2;
			uint32 dwBestWaitTime = 0xFFFFFFFF;
			POSITION posNextRequest = NULL;
			CPartFile* cur_file;
			for( pos1 = m_localServerReqQueue.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
				m_localServerReqQueue.GetNext(pos1);
				cur_file = m_localServerReqQueue.GetAt(pos2);
				if (cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY)
				{
					uint8 nPriority = cur_file->GetDownPriority();
					if (nPriority > PR_HIGH){
						wxASSERT(0);
						nPriority = PR_HIGH;
					}

					if (cur_file->lastsearchtime + (PR_HIGH-nPriority) < dwBestWaitTime ){
						dwBestWaitTime = cur_file->lastsearchtime + (PR_HIGH-nPriority);
						posNextRequest = pos2;
					}
				}
				else{
					m_localServerReqQueue.RemoveAt(pos2);
					cur_file->m_bLocalSrcReqQueued = false;
					theApp.amuledlg->AddDebugLogLine(false, "Local server source request for file \"%s\" not sent because of status '%s'", cur_file->GetFileName().GetData(), cur_file->getPartfileStatus().c_str());
				}
			}
			
			if (posNextRequest != NULL)
			{
				cur_file = m_localServerReqQueue.GetAt(posNextRequest);
				cur_file->m_bLocalSrcReqQueued = false;
				cur_file->lastsearchtime = ::GetTickCount();
				m_localServerReqQueue.RemoveAt(posNextRequest);
				iFiles++;
				
				// create request packet
				Packet* packet = new Packet(OP_GETSOURCES,16);
				md4cpy(packet->pBuffer,cur_file->GetFileHash());
				dataTcpFrame.WriteRaw(packet->GetPacket(), packet->GetRealPacketSize());
				delete packet;
				#if 0 
				// Needs new preferences
				if ( theApp.glob_prefs->GetDebugSourceExchange() )
					theApp.amuledlg->AddDebugLogLine( false, "Send:Source Request Server File(%s)", cur_file->GetFileName());
				#endif
			}
		}

		int iSize = dataTcpFrame.GetLength();
		if (iSize > 0)
		{
			// create one 'packet' which contains all buffered OP_GETSOURCES eD2K packets to be sent with one TCP frame
			// server credits: 16*iMaxFilesPerTcpFrame+1 = 241
			Packet* packet = new Packet(new char[iSize], dataTcpFrame.GetLength(), true, false);
			dataTcpFrame.Seek(0, wxFromStart);
			dataTcpFrame.ReadRaw(packet->GetPacket(), iSize);
			uint32 size = packet->size;
			theApp.serverconnect->SendPacket(packet, true);	// Deletes `packet'.
			theApp.uploadqueue->AddUpDataOverheadServer(size);
		}

		// next TCP frame with up to 15 source requests is allowed to be sent in..
		m_dwNextTCPSrcReq = ::GetTickCount() + SEC2MS(iMaxFilesPerTcpFrame*(16+4));
	}

}

void CDownloadQueue::SendLocalSrcRequest(CPartFile* sender)
{
	//ASSERT ( !m_localServerReqQueue.Find(sender) );
	//printf("Add Local Request\n");
	m_localServerReqQueue.AddTail(sender);
}

void CDownloadQueue::GetDownloadStats(int results[])
{
	
	results[0]=0;
	results[1]=0;

	for (POSITION pos =theApp.downloadqueue->filelist.GetHeadPosition();pos != 0;theApp.downloadqueue->filelist.GetNext(pos)) {
		CPartFile* cur_file =  filelist.GetAt(pos);
		results[0]+=cur_file->GetSourceCount();
		results[1]+=cur_file->GetTransferingSrcCount();
	}
}

CUpDownClient* CDownloadQueue::GetDownloadClientByIP(uint32 dwIP)
{
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)) {
		CPartFile* cur_file = filelist.GetAt(pos);
		for (int sl=0;sl<SOURCESSLOTS;sl++) if (!cur_file->srclists[sl].IsEmpty())
		for (POSITION pos2 = cur_file->srclists[sl].GetHeadPosition();pos2 != 0; cur_file->srclists[sl].GetNext(pos2)) {
			if (dwIP == cur_file->srclists[sl].GetAt(pos2)->GetIP()) {
				return cur_file->srclists[sl].GetAt(pos2);
			}
		}
	}
	return NULL;
}

    
void CDownloadQueue::AddLinksFromFile()
{
        wxString filename;
	wxString link;
	wxTextFile linksfile(wxString::Format("%s/.aMule/ED2KLinks", getenv("HOME")));
	
	if (linksfile.Open()) {
		link = linksfile.GetFirstLine();
		// GetLineCount returns the actual number of lines in file, but line numbering
		// starts from zero. Thus we must loop until i = GetLineCount()-1.
		for (unsigned int i = 0; i < linksfile.GetLineCount(); i++) {
			// Need the links to end with /, otherwise CreateLinkFromUrl crashes us.
			if (link.Right(1) != "/") {
				link+="/";
			}
			try {
				CED2KLink* pLink=CED2KLink::CreateLinkFromUrl(link);
				if(pLink->GetKind()==CED2KLink::kFile) {
					// All seems ok, add it to download queue.
					AddFileLinkToDownload(pLink->GetFileLink());
				} else {
					throw wxString(_("Bad link."));
				}
				delete pLink;
			} catch(wxString error) {
				char buffer[200];
				sprintf(buffer,_("This ed2k link is invalid (%s)"),error.GetData());
				theApp.amuledlg->AddLogLine(true,CString(_("Invalid link: %s")),buffer);
			}
			// We must double-check here where are we, because GetNextLine moves reading head
			// one line below, and we must make sure that line exists. Thus check if we are 
			// at least one line away from end before trying to read next line.
			if (i + 1 < linksfile.GetLineCount()) {
				link = linksfile.GetNextLine();
			}
		}
	} else {
		printf("Failed to open ED2KLinks file.\n");
	}
	// Save and Delete the file.
	linksfile.Write();
	wxRemoveFile(wxString::Format("%s/.aMule/ED2KLinks", getenv("HOME")));
}

/* Razor 1a - Modif by MikaelB
   RemoveSourceFromPartFile function */
   
void CDownloadQueue::RemoveSourceFromPartFile(CPartFile* file, CUpDownClient* client, POSITION position)
{
	file->srclists[client->sourcesslot].RemoveAt(position);
	file->IsCountDirty = true;
	file->NewSrcPartsInfo();
	file->UpdateAvailablePartsCount();
	client->SetDownloadState(DS_NONE);
	client->SetValidSource(false);
	file->RemoveDownloadingSource(client);
	theApp.amuledlg->transferwnd->downloadlistctrl->RemoveSource(client, file);
	client->reqfile = 0;
	if(! client->m_OtherRequests_list.IsEmpty()) {
		POSITION position, temp_position;
		for(position = client->m_OtherRequests_list.GetHeadPosition(); (temp_position = position) != NULL;) {
			client->m_OtherRequests_list.GetNext(position);
			POSITION position2 = client->m_OtherRequests_list.GetAt(temp_position)->A4AFSourcesList.Find(client);
			if(position2) {
				client->m_OtherRequests_list.GetAt(temp_position)->A4AFSourcesList.RemoveAt(position2);
				theApp.amuledlg->transferwnd->downloadlistctrl->RemoveSource(client, client->m_OtherRequests_list.GetAt(temp_position));  
				client->m_OtherRequests_list.RemoveAt(temp_position);
			}
		}
	}
	/*
	// If client's upload status id none
	if(client->GetUploadState() == US_NONE) {
		// Disconnect client
		client->Disconnected();
	}
	*/
}

/* End modif */

/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN ... */
void CDownloadQueue::DisableAllA4AFAuto(void)
{
	CPartFile* cur_file;
	for (POSITION pos = filelist.GetHeadPosition(); pos != NULL; filelist.GetNext(pos)) {
		cur_file = (CPartFile*)filelist.GetAt(pos);
		if (cur_file != NULL) {
			cur_file->SetA4AFAuto(false);
		}
	}
}
/* eMule 0.30c implementation, i give it a try (Creteil) END ... */

void CDownloadQueue::ResetCatParts(int cat)
{
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)) {
		CPartFile* cur_file = filelist.GetAt(pos);
		if (cur_file->GetCategory()==cat) {
			cur_file->SetCategory(0);
		} else if (cur_file->GetCategory() > cat) {
			cur_file->SetCategory(cur_file->GetCategory()-1);
		}
	}
}

void CDownloadQueue::SetCatPrio(int cat, uint8 newprio)
{
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)) {
		CPartFile* cur_file = filelist.GetAt(pos);
		if (cat==0 || cur_file->GetCategory()==cat) {
			if (newprio==PR_AUTO) {
				//cur_file->SetAutoPriority(true);
				//cur_file->SetPriority(PR_HIGH);
				cur_file->SetAutoDownPriority(true);
				cur_file->SetDownPriority(PR_HIGH);
			} else {
				//cur_file->SetAutoPriority(false);
				//cur_file->SetPriority(newprio);
				cur_file->SetAutoDownPriority(false);
				cur_file->SetDownPriority(newprio);
			}
		}
	}
}

void CDownloadQueue::SetCatStatus(int cat, int newstatus)
{
	bool reset=false;
	POSITION pos= filelist.GetHeadPosition();
	while (pos != 0) {
		CPartFile* cur_file = filelist.GetAt(pos);
		if (!cur_file) {
			continue;
		}
		if (cat == cur_file->GetCategory() || cat == 0) {
			switch (newstatus) {
				case MP_CANCEL:
					cur_file->Delete();
					reset=true;
					break;
				case MP_PAUSE:
					cur_file->PauseFile();
					break;
				case MP_STOP:
					cur_file->StopFile();
					break;
				case MP_RESUME: 
					if (cur_file->GetStatus()==PS_PAUSED) {
						cur_file->ResumeFile();
					}
					break;
			}
		}
		filelist.GetNext(pos);
		if (reset) {
			reset = false;
			pos = filelist.GetHeadPosition();
		}
	}
}

uint16 CDownloadQueue::GetDownloadingFileCount()
{
	uint16 result = 0;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;) {
		CPartFile* cur_file = filelist.GetNext(pos);
		if (cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY) {
			result++;
		}
	}
	return result;
}

uint16 CDownloadQueue::GetPausedFileCount()
{
	uint16 result = 0;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;) {
		CPartFile* cur_file = filelist.GetNext(pos);
		if (cur_file->GetStatus() == PS_PAUSED) {
			result++;
		}
	}
	return result;
}

void CDownloadQueue::CheckDiskspace(bool bNotEnoughSpaceLeft)
{
	lastcheckdiskspacetime = ::GetTickCount();

	// sorting the list could be done here,
	// but I prefer to "see" that function call in the calling functions.
	// SortByPriority();

	// If disabled, resume any previously paused files
	if (!theApp.glob_prefs->IsCheckDiskspaceEnabled()) {
		if (!bNotEnoughSpaceLeft) { // avoid worse case, if we already had 'disk full'
			for (POSITION pos1 = filelist.GetHeadPosition(); pos1 != NULL;) {
				CPartFile* cur_file = filelist.GetNext(pos1);
				switch(cur_file->GetStatus()) {
					case PS_ERROR:
					case PS_COMPLETING:
					case PS_COMPLETE:
						continue;
				}
				cur_file->ResumeFileInsufficient();
			}
		}
		return;
	}
	wxLongLong total, free;
	wxGetDiskSpace(theApp.glob_prefs->GetTempDir(), &total, &free);
	// 'bNotEnoughSpaceLeft' - avoid worse case, if we already had 'disk full'
	uint64 nTotalAvailableSpace = bNotEnoughSpaceLeft ? 0 : free.GetValue();
	if (theApp.glob_prefs->GetMinFreeDiskSpace() == 0) {
		for (POSITION pos1 = filelist.GetHeadPosition(); pos1 != NULL;) {
			CPartFile* cur_file = filelist.GetNext(pos1);
			switch(cur_file->GetStatus()) {
				case PS_ERROR:
				case PS_COMPLETING:
				case PS_COMPLETE:
					continue;
			}
			if (cur_file->IsStopped()) {
				continue;
			}
			// Pause the file only if it would grow in size and would exceed the currently available free space
			uint32 nSpaceToGo = cur_file->GetNeededSpace();
			if (nSpaceToGo <= nTotalAvailableSpace) {
				nTotalAvailableSpace -= nSpaceToGo;
				cur_file->ResumeFileInsufficient();
			} else {
				if (!cur_file->GetInsufficient()) {
					printf("Free Disk Space (Total): %lli\n", nTotalAvailableSpace);
					printf("File : %s, Needed Space : %i - PAUSED !!!\n", cur_file->GetFileName().GetData(),cur_file->GetNeededSpace());
					cur_file->PauseFile(true);
				}
			}
		}
	} else {
		for (POSITION pos1 = filelist.GetHeadPosition(); pos1 != NULL;) {
			CPartFile* cur_file = filelist.GetNext(pos1);
			switch(cur_file->GetStatus()) {
				case PS_ERROR:
				case PS_COMPLETING:
				case PS_COMPLETE:
					continue;
			}
			if (nTotalAvailableSpace < theApp.glob_prefs->GetMinFreeDiskSpace()) {
				if (cur_file->IsNormalFile()) {				
				// Normal files: pause the file only if it would still grow
					uint32 nSpaceToGrow = cur_file->GetNeededSpace();
					if (nSpaceToGrow) {
						if (!cur_file->GetInsufficient()) {						
							printf("Free Disk Space (Total): %lli\n", nTotalAvailableSpace);
							printf("File : %s, Needed Space : %i - PAUSED !!!\n", cur_file->GetFileName().GetData(),cur_file->GetNeededSpace());
							// cur_file->PauseFileInsufficient();
							cur_file->PauseFile(true/*bInsufficient*/);
						}
					}
				} else {
					if (!cur_file->GetInsufficient()) {											
						// Compressed/sparse files: always pause the file
						cur_file->PauseFile(true/*bInsufficient*/);
					}
				}
			} else {
				cur_file->ResumeFileInsufficient();
			}
		}
	}
}

int CDownloadQueue::GetMaxFilesPerUDPServerPacket() const
{
	int iMaxFilesPerPacket;
	if (cur_udpserver && cur_udpserver->GetUDPFlags() & SRV_UDPFLG_EXT_GETSOURCES) {
		// get max. file ids per packet
		if (m_cRequestsSentToServer < MAX_REQUESTS_PER_SERVER) {
			iMaxFilesPerPacket = std::min(MAX_FILES_PER_UDP_PACKET, MAX_REQUESTS_PER_SERVER - m_cRequestsSentToServer);
		} else {
			// ASSERT(0);
			iMaxFilesPerPacket = 0;
		}
	} else {
		iMaxFilesPerPacket = 1;
	}
	return iMaxFilesPerPacket;
}

bool CDownloadQueue::SendGlobGetSourcesUDPPacket(CSafeMemFile& data)
{
	bool bSentPacket = false;

	if (cur_udpserver && (theApp.serverconnect->GetCurrentServer() == NULL ||
	cur_udpserver != theApp.serverlist->GetServerByAddress(theApp.serverconnect->GetCurrentServer()->GetAddress(),theApp.serverconnect->GetCurrentServer()->GetPort()))) {
		int iFileIDs = data.GetLength() / 16;
		Packet packet(&data);

		packet.opcode = OP_GLOBGETSOURCES;
		theApp.uploadqueue->AddUpDataOverheadServer(packet.size);
		theApp.serverconnect->SendUDPPacket(&packet,cur_udpserver,false);
		
		m_cRequestsSentToServer += iFileIDs;
		bSentPacket = true;
	}

	return bSentPacket;
}


// Kry  - Implementation of HasHostNameSources using wxThread functions, taken from UDPSocket.cpp

class SourcesAsyncDNS : public wxThread
{
public:
	SourcesAsyncDNS();
	virtual ExitCode Entry();

	wxString ipName;
	uint16 port;
	const uchar* fileid;
};

SourcesAsyncDNS::SourcesAsyncDNS() : wxThread(wxTHREAD_DETACHED)
{
}

wxThread::ExitCode SourcesAsyncDNS::Entry()
{
	struct hostent ret,*result=NULL;
#ifndef __WXMSW__
	int errorno=0;
	char dataBuf[512]={0};
#endif

#if defined(__linux__)
	gethostbyname_r(ipName.GetData(),&ret,dataBuf,sizeof(dataBuf),&result,&errorno);
#elif defined(__WXMSW__)
	result = gethostbyname(ipName.GetData());
#else
	result = gethostbyname_r(ipName.GetData(),&ret,dataBuf,sizeof(dataBuf),&errorno); 
#endif

	if(result) {
		unsigned long addr=*(unsigned long*)ret.h_addr;
		struct sockaddr_in* newsi=(struct sockaddr_in*)malloc(sizeof(struct sockaddr_in)); // new struct sockaddr_in;
		newsi->sin_addr.s_addr=addr;
		wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,TM_SOURCESDNSDONE);
		evt.SetExtraLong((long)newsi);
		wxPostEvent(theApp.amuledlg,evt);
	}

	return NULL;
}


void CDownloadQueue::AddToResolve(uchar* fileid, CStringA pszHostname, uint16 port)
{
	bool bResolving = !m_toresolve.IsEmpty();

	// double checking
	if (!theApp.downloadqueue->GetFileByID(fileid)) {
		return;
	}
	Hostname_Entry* entry = new Hostname_Entry;
	md4cpy(entry->fileid, fileid);
	entry->strHostname = pszHostname;
	entry->port = port;
	m_toresolve.AddTail(entry);

	if (bResolving) {
		return;
	}
	printf("Opening thread for resolving %s\n",pszHostname.c_str());;
	SourcesAsyncDNS* dns=new SourcesAsyncDNS();
	if(dns->Create()!=wxTHREAD_NO_ERROR) {
		// Cannot create (Already there?)
		dns->Delete();
		return;
	}
	dns->ipName= pszHostname;
	dns->port=port;
	dns->fileid = fileid;
	if (dns->Run() != wxTHREAD_NO_ERROR) {
		// Cannot run (Already there?)
		dns->Delete();
		m_toresolve.RemoveHead();
		delete entry;
		return;
	}
}



bool CDownloadQueue::OnHostnameResolved(struct sockaddr_in* inaddr)
{
	Hostname_Entry* resolved = m_toresolve.RemoveHead();
	if (resolved) {
		printf("Thread finished, Hostname %s resolved to %s\n", resolved->strHostname.c_str(),inet_ntoa(inaddr->sin_addr));
		if (inaddr!=NULL) {
			CPartFile* file = theApp.downloadqueue->GetFileByID(resolved->fileid);
			if (file) {
				CSafeMemFile sources(1+4+2);
				sources.Write((uint8)1); // No. Sources
				sources.Write((uint32)inaddr->sin_addr.s_addr);
				sources.Write((uint16)resolved->port);
				sources.Seek(0,wxFromStart);
				file->AddSources(&sources,0,0);
			}
		}
		delete resolved;
	}
	if(inaddr) {
		// must free it
		//delete inaddr;
		free(inaddr);
	}
	while (!m_toresolve.IsEmpty()) {
		Hostname_Entry* entry = m_toresolve.GetHead();
		SourcesAsyncDNS* dns=new SourcesAsyncDNS();
		if(dns->Create()!=wxTHREAD_NO_ERROR) {
			// Cannot create (Already there?)
			dns->Delete();
			return false;
	 	}
		dns->ipName= entry->strHostname;
		dns->port=entry->port;
		dns->fileid = entry->fileid;
		if (dns->Run() != wxTHREAD_NO_ERROR) {
			// Cannot run (Already there?)
			dns->Delete();
			return false;
		}
		m_toresolve.RemoveHead();
		delete entry;
	}
	return TRUE;
}