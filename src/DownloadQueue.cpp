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

#include <wx/defs.h>		// Needed before any other wx/*.h
#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/textfile.h>	// Needed for wxTextFile
#include <wx/filename.h>
#include <wx/listimpl.cpp>
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
#include "Preferences.h"
#include "amule.h"			// Needed for theApp
#include "opcodes.h"		// Needed for MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE
#include "otherfunctions.h"	// Needed for GetTickCount
#include "NetworkFunctions.h" // Needed for CAsyncDNS
#include <algorithm>
#include <numeric>

// Max. file IDs per UDP packet
// ----------------------------
// 576 - 30 bytes of header (28 for UDP, 2 for "E3 9A" edonkey proto) = 546 bytes
// 546 / 16 = 34

#define MAX_FILES_PER_UDP_PACKET	31	// 2+16*31 = 498 ... is still less than 512 bytes!!
#define MAX_REQUESTS_PER_SERVER		35


CDownloadQueue::CDownloadQueue(CSharedFileList* in_sharedfilelist)
{
	sharedfilelist = in_sharedfilelist;
	filesrdy = 0;
	datarate = 0;
	cur_udpserver = 0;
	lastfile = 0;
	lastcheckdiskspacetime = 0;
	lastudpsearchtime = 0;
	lastudpstattime = 0;
	udcounter = 0;
	completedFilesExist = false;
	m_iSearchedServers = 0;
	m_datarateMS=0;
	m_nDownDataRateMSOverhead = 0;
	m_nDownDatarateTotal = 0;
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


void CDownloadQueue::AddPartFilesToShare()
{
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		if (cur_file->GetStatus(true) == PS_READY) {
			printf("Adding file %s to shares\n",unicode2char(cur_file->GetFullName()));
			sharedfilelist->SafeAddKFile(cur_file,true);
		}
	}
}

void CDownloadQueue::CompDownDatarateOverhead()
{
	// Adding the new overhead
	m_nDownDatarateTotal += m_nDownDataRateMSOverhead * 10;
	m_AverageDDRO_list.push_back( m_nDownDataRateMSOverhead * 10 );
	
	// Reset the overhead count
	m_nDownDataRateMSOverhead = 0;
	
	// We want at least 11 elements before we will start doing averages
	if ( m_AverageDDRO_list.size() > 10 ) {
		
		// We want 150 elements at most
		if ( m_AverageDDRO_list.size() > 150 ) {
			m_nDownDatarateTotal -= m_AverageDDRO_list.front();
		
			m_AverageDDRO_list.pop_front();
		
			m_nDownDatarateOverhead = m_nDownDatarateTotal / 150.0f;
		} else {
			m_nDownDatarateOverhead = m_nDownDatarateTotal / (double)m_AverageDDRO_list.size();
		}
	} else if ( m_AverageDDRO_list.size() == 10 ) {
		// Create the starting average once we have 10 items
		m_nDownDatarateTotal = std::accumulate( m_AverageDDRO_list.begin(),
		                                      m_AverageDDRO_list.end(), 0 );
	
		m_nDownDatarateOverhead = m_nDownDatarateTotal / 10.0;
		
	} else {
		m_nDownDatarateOverhead = 0;
	}
}


void CDownloadQueue::Init()
{
	// find all part files, read & hash them if needed and store into a list
	int count = 0;
	
	CDirIterator TempDir(thePrefs::GetTempDir());
	
	// check all part.met files
	printf("Loading temp files from %s.\n",unicode2char(thePrefs::GetTempDir()));
	
	wxString fileName=TempDir.FindFirstFile(CDirIterator::File,wxT("*.part.met"));
	
	while(!fileName.IsEmpty()) {
		wxFileName myFileName(fileName);
		printf("Loading %s... ",unicode2char(myFileName.GetFullName()));
		CPartFile* toadd = new CPartFile();
		bool result = toadd->LoadPartFile(thePrefs::GetTempDir(),myFileName.GetFullName());
		if (!result) {
			// Try from backup
			result = toadd->LoadPartFile(thePrefs::GetTempDir(),myFileName.GetFullName(), true);
		}
		if (result) {
			count++;
			printf("Done.\n");
			filelist.push_back(toadd); // to downloadqueue
			if (toadd->GetStatus(true) == PS_READY) {
				sharedfilelist->SafeAddKFile(toadd); // part files are always shared files
			}
			Notify_DownloadCtrlAddFile(toadd);// show in downloadwindow
		} else {
			printf("ERROR!\n");
			delete toadd;
		}
		fileName=TempDir.FindNextFile();
		// Dont leave the gui blank while loading the files, so ugly...
#ifndef AMULE_DAEMON		
		if ( !(count % 10) ) ::wxSafeYield();
#endif
	}
	if(count == 0) {
		AddLogLineM(false, _("No part files found"));
	} else {
		AddLogLineM(false, wxString::Format(_("Found %i part files"), count));
		SortByPriority();
		CheckDiskspace();
	}
}

CDownloadQueue::~CDownloadQueue()
{
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		delete filelist[i];
	}

}

void CDownloadQueue::SavePartFiles(bool del /*= false*/)
{
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		filelist[i]->m_hpartfile.Flush();
		filelist[i]->SavePartFile();
		if (del) {
			delete filelist[i];
		}
	}
}

void CDownloadQueue::SaveSourceSeeds()
{
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		filelist[i]->SaveSourceSeeds();
	}
}

void CDownloadQueue::LoadSourceSeeds()
{
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		filelist[i]->LoadSourceSeeds();
	}
}

void CDownloadQueue::AddSearchToDownload(CSearchFile* toadd, uint8 category)
{
	if (IsFileExisting(toadd->GetFileHash())) {
		return;
	}
	CPartFile* newfile = new CPartFile(toadd);
	if (!newfile) {
		return;
	}
	AddSearchToDownloadCommon(newfile, category);
}

void CDownloadQueue::AddSearchToDownload(const wxString& link, uint8 category)
{
	CPartFile* newfile = new CPartFile(link);
	if (!newfile) {
		return;
	}
	AddSearchToDownloadCommon(newfile, category);
}

void CDownloadQueue::AddSearchToDownloadCommon(CPartFile *newfile, uint8 category)
{
	if (newfile->GetStatus() == PS_ERROR) {
		delete newfile;
		return;
	}
	AddDownload(newfile, thePrefs::AddNewFilesPaused(), category);
	newfile->SetCategory(category);
}

void CDownloadQueue::StartNextFile()
{
	if(!thePrefs::StartNextFile()) {
		return;
	}

	CPartFile*  pfile = NULL;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
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

void CDownloadQueue::AddFileLinkToDownload(CED2KFileLink* pLink, uint8 category)
{
	CPartFile* newfile = new CPartFile(pLink);
	if (!newfile) {
		// Very Bad Things Happened
		printf("Really evil thing happened trying to create a partfile\n");
		return;
	}
	if (newfile->GetStatus() == PS_ERROR) {
		// This can be a ed2k link for a currently downloading file
		// so we'll try to add the sources to the file anyway.
		delete newfile;
		newfile=NULL;
	} else {
		AddDownload(newfile,thePrefs::AddNewFilesPaused(), category);
	}
	
	if (pLink->HasValidSources()) {
		if (newfile) {
			newfile->AddClientSources(pLink->SourcesList,1);
		} else {
			CPartFile* partfile = GetFileByID(pLink->GetHashKey());
			if (partfile) {
				partfile->AddClientSources(pLink->SourcesList,1);
			}
		}
	}
	
	if(pLink->HasHostnameSources()) {
		for (POSITION pos = pLink->m_HostnameSourcesList.GetHeadPosition(); pos != NULL; pLink->m_HostnameSourcesList.GetNext(pos)) {
			AddToResolve(pLink->GetHashKey(), pLink->m_HostnameSourcesList.GetAt(pos)->strHostname, pLink->m_HostnameSourcesList.GetAt(pos)->nPort);
		}
	}
}

void CDownloadQueue::AddDownload(CPartFile* newfile, bool paused, uint8 category)
{
	// Creteil - Add in paused mode if there is already file(s) downloading
	if (paused && !filelist.empty()) {
		newfile->StopFile();
	}

	filelist.push_back(newfile);
	SortByPriority();
	CheckDiskspace();

	newfile->SetCategory(category);
	Notify_DownloadCtrlAddFile(newfile);
	AddLogLineM(true, wxString::Format(_("Downloading %s"), newfile->GetFileName().c_str()));
	wxString msgTemp;
	msgTemp.Printf(_("Downloading %s\n"), newfile->GetFileName().c_str());
	Notify_ShowNotifier(msgTemp, TBN_DLOAD, 0);
	// Kry - Get sources if not stopped
	if (!newfile->IsStopped()) {
		SendLocalSrcRequest(newfile);
	}
}

bool CDownloadQueue::IsFileExisting(const CMD4Hash& fileid) const
{
	if (const CKnownFile* file = sharedfilelist->GetFileByID(fileid)) {
		if (file->IsPartFile()) {
			AddLogLineM(true, _("You are already trying to download the file ") + file->GetFileName());
		} else {
			AddLogLineM(true, _("You already have the file ") + file->GetFileName());
		}
		return true;
	} else if ((file = GetFileByID(fileid))) {
		AddLogLineM(true, _("You are already trying to download the file %s") + file->GetFileName());
		return true;
	}
	return false;
}


void CDownloadQueue::Process()
{
	ProcessLocalRequests(); // send src requests to local server

	uint32 downspeed = 0;
	if (thePrefs::GetMaxDownload() != UNLIMITED && datarate > 1500) {
		downspeed = (thePrefs::GetMaxDownload()*1024*100)/(datarate+1); //(uint16)((float)((float)(thePrefs::GetMaxDownload()*1024)/(datarate+1)) * 100);
		if (downspeed < 50) {
			downspeed = 50;
		} else if (downspeed > 200) {
			downspeed = 200;
		}
	}

	datarate = 0;
	udcounter++;

	// Since we imported the SortByPriority, there's no point on having separate loops
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {

		CPartFile* cur_file = filelist[i];
		if (cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY){
			datarate += cur_file->Process(downspeed, udcounter);
		} else {
			//This will make sure we don't keep old sources to paused and stoped files..
			cur_file->StopPausedFile();
		}
	}


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
		wxString filename = theApp.ConfigDir + wxT("ED2KLinks");
		if (wxFile::Exists(filename)) {
			AddLinksFromFile();
		}
		m_nLastED2KLinkCheck = ::GetTickCount();
	}
}

CPartFile *CDownloadQueue::GetFileByID(const CMD4Hash& filehash) const {
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		if (filehash == filelist[i]->GetFileHash())
			return filelist[i];
	}
	return NULL;
}

CPartFile *CDownloadQueue::GetFileByIndex(unsigned int index) const {
	if ( index >= filelist.size() ) {
		return NULL;
	}
	for ( unsigned int i = 0; i < filelist.size(); ++i ) {
		if ( i == index ) {
			return filelist[i];
		}
	}
	// Should never return here
	wxASSERT(0);
	return NULL;
}

bool CDownloadQueue::IsPartFile(const CKnownFile* totest) const{
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		if (totest == filelist[i]) {
			return true;
		}
	}
	return false;
}

void CDownloadQueue::CheckAndAddSource(CPartFile* sender,CUpDownClient* source)
{
	// if we block loopbacks at this point it should prevent us from connecting to ourself
	if ( source->HasValidHash() ) {
		if ( source->GetUserHash() == thePrefs::GetUserHash() ) {
			AddDebugLogLineM(false, _("Tried to add source with matching hash to your own."));
			source->Safe_Delete();
			return;
		}
	}

	if (sender->IsStopped()) {
		source->Safe_Delete();
		return;
	}

	
	// Find all clients with the same hash
	if ( source->HasValidHash() ) {
		CClientList::SourceList found = theApp.clientlist->GetClientsByHash( source->GetUserHash() );

		CClientList::SourceList::iterator it = found.begin();
		for ( ; it != found.end(); it++ ) {
			CKnownFile* cur_file = (*it)->GetRequestFile();

			// Only check files on the download-queue
			if ( cur_file ) {
				// Is the found source queued for something else?
				if (  cur_file != sender ) {
					// Try to add a request for the other file
					if ( (*it)->AddRequestForAnotherFile(sender)) {
						// Add it to downloadlistctrl
						Notify_DownloadCtrlAddSource(sender, *it, true);
					}
				}
				
				source->Safe_Delete();
				return;
			}
		}
	}



	// Our new source is real new but maybe it is already uploading to us?
	// If yes the known client will be attached to the var "source" and the old
	// source-client will be deleted. However, if the request file of the known 
	// source is NULL, then we have to treat it almost like a new source and if
	// it isn't NULL and not "sender", then we shouldn't move it, but rather add
	// a request for the new file.
	if ( theApp.clientlist->AttachToAlreadyKnown(&source, 0) ) {
		// Already queued for another file?
		if ( source->GetRequestFile() ) {
			// If we're already queued for the rigth file, then there's nothing to do
			if ( sender != source->GetRequestFile() ) {	
				// Add the new file to the request list
				source->AddRequestForAnotherFile( sender );
			}
		} else {
			// Source was known, but reqfile NULL. I'm not sure if this can 
			// happen, but it's best to be certain, so I handle this case as well
			source->SetRequestFile( sender );
			
			if ( source->GetFileRate() || !source->GetFileComment().IsEmpty() ) {
				sender->UpdateFileRatingCommentAvail();
			}
	
			sender->AddSource( source );
			
			Notify_DownloadCtrlAddSource(sender, source, false);
		}
	} else {
		// Unknown client, add it to the clients list
		source->SetRequestFile( sender );

		theApp.clientlist->AddClient(source);
	
		if ( source->GetFileRate() || !source->GetFileComment().IsEmpty() ) {
			sender->UpdateFileRatingCommentAvail();
		}
	
		sender->AddSource( source );
	
		Notify_DownloadCtrlAddSource(sender, source, false);
	}
}

void CDownloadQueue::CheckAndAddKnownSource(CPartFile* sender,CUpDownClient* source)
{
	if(!source->HasLowID() && (source->GetUserID() & 0xFF) == 0x7F) {
		return;
	}

	if (sender->IsStopped()) {
		return;
	}


	CPartFile* cur_file = source->GetRequestFile();
	
	// Check if the file is already queued for something else
	if ( cur_file ) {
		if ( cur_file != sender ) {
			if ( source->AddRequestForAnotherFile( sender ) ) {
				Notify_DownloadCtrlAddSource( sender, source, true );
			}
		}
	} else {
		source->SetRequestFile( sender );

		if ( source->GetFileRate() || !source->GetFileComment().IsEmpty() ) {
			sender->UpdateFileRatingCommentAvail();
		}

		sender->AddSource( source );
		Notify_DownloadCtrlAddSource( sender, source, false );
	}
}

/* Creteil importing new method from eMule 0.30c */
bool CDownloadQueue::RemoveSource(CUpDownClient* toremove, bool	WXUNUSED(updatewindow), bool bDoStatsUpdate)
{
	toremove->DeleteAllFileRequests();
	
	bool removed = false;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		
		// Remove from source-list
		if ( cur_file->DelSource( toremove ) ) {
			cur_file->RemoveDownloadingSource(toremove);
			cur_file->IsCountDirty = true;
			removed = true;
			if ( bDoStatsUpdate ) {
				cur_file->UpdatePartsInfo();
			}
		}

		// Remove from A4AF-list
		cur_file->A4AFsrclist.erase( toremove );
	}

	if ( !toremove->GetFileComment().IsEmpty() || toremove->GetFileRate()>0) {
		toremove->GetRequestFile()->UpdateFileRatingCommentAvail();
	}
	
	toremove->SetRequestFile( NULL );
	toremove->SetDownloadState(DS_NONE);

	// Remove from downloadlist widget
	Notify_DownloadCtrlRemoveSource(toremove,0);
	toremove->ResetFileStatusInfo();
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

	for ( std::deque<CPartFile*>::iterator it = filelist.begin(); it != filelist.end(); it++ ) {
		if (toremove == (*it)) {
			filelist.erase(it);
			return;
		}
	}
	SortByPriority();
	CheckDiskspace();
}

void CDownloadQueue::DeleteAll(){
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		cur_file->m_SrcList.clear();
		cur_file->IsCountDirty = true;
		// Barry - Should also remove all requested blocks
		// Don't worry about deleting the blocks, that gets handled
		// when CUpDownClient is deleted in CClientList::DeleteAll()
		cur_file->RemoveAllRequestedBlocks();
	}
}

bool CDownloadQueue::SendNextUDPPacket()
{

	if (filelist.empty() || !theApp.serverconnect->IsUDPSocketAvailable()	|| !theApp.serverconnect->IsConnected()) {
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
				nextfile = filelist.front();
				lastfile = nextfile;
			} else {
				std::deque<CPartFile*>::iterator it;
				it = std::find(filelist.begin(), filelist.end(), lastfile);
				if (it == filelist.end()) {
					// the last file is no longer in the DL-list
					// (may have been finished or canceld)
					// get first file to search sources for
					nextfile = filelist.front();
					lastfile = nextfile;
				} else {
					it++;
					if (it == filelist.end()) {
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
						nextfile = filelist.front();
						lastfile = nextfile;
					} else {
						nextfile = (*it);
						lastfile = nextfile;
					}
				}
			}
		}

		if (!bSentPacket && nextfile && nextfile->GetSourceCount() < thePrefs::GetMaxSourcePerFileUDP()) {
			dataGlobGetSources.WriteHash16(nextfile->GetFileHash());
			iFiles++;
		}
	}

	//ASSERT( dataGlobGetSources.Length() == 0 || !bSentPacket );

	if (!bSentPacket && dataGlobGetSources.GetLength() > 0) {
		SendGlobGetSourcesUDPPacket(dataGlobGetSources);
	}

	// send max 40 udp request to one server per interval, if we have more than 40 files, we rotate the list and use it as Queue
	if (m_cRequestsSentToServer >= MAX_REQUESTS_PER_SERVER) {
		// move the last 40 files to the head
		if (filelist.size() > MAX_REQUESTS_PER_SERVER) {
			for (int i = 0; i != MAX_REQUESTS_PER_SERVER; i++) {
				filelist.push_front( filelist.back() );
				filelist.pop_back();
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

// Comparison function needed by sort. Returns true if file1 preceeds file2
bool ComparePartFiles(const CPartFile* file1, const CPartFile* file2) {
	if (file1->GetDownPriority() != file2->GetDownPriority()) {
		// To place high-priority files before low priority files we have to 
		// invert this test, since PR_LOW is lower than PR_HIGH, and since 
		// placing a PR_LOW file before a PR_HIGH file would mean that
		// the PR_LOW file gets sources before the PR_HIGH file ...
		return (file1->GetDownPriority() > file2->GetDownPriority());
	} else {
		// Use normal alpha-numeric comparison here, lesser before greater
		return file1->GetPartMetFileName().CmpNoCase( file2->GetPartMetFileName() ) < 0;
	}
}

void CDownloadQueue::SortByPriority()
{
	sort(filelist.begin(), filelist.end(), ComparePartFiles);
}

void CDownloadQueue::ResetLocalServerRequests()
{
	m_dwNextTCPSrcReq = 0;
	m_localServerReqQueue.clear();

	for ( uint16 i = 0, size = filelist.size(); i < size; i++ )
	{
		CPartFile* pFile = filelist[i];
		UINT uState = pFile->GetStatus();
		if (uState == PS_READY || uState == PS_EMPTY)
			pFile->ResumeFile();
		pFile->m_bLocalSrcReqQueued = false;
	}
}

void CDownloadQueue::RemoveLocalServerRequest(CPartFile* pFile)
{
	std::list<CPartFile*>::iterator it = m_localServerReqQueue.begin();
	while ( it != m_localServerReqQueue.end() ) {
		if ( (*it) == pFile ) {
			it = m_localServerReqQueue.erase( it );
			pFile->m_bLocalSrcReqQueued = false;
			// could 'break' here.. fail safe: go through entire list..
		} else {
			it++;
		}
	}
}

void CDownloadQueue::ProcessLocalRequests()
{
	if ( (!m_localServerReqQueue.empty()) && (m_dwNextTCPSrcReq < ::GetTickCount()) )
	{
		CSafeMemFile dataTcpFrame(22);
		const int iMaxFilesPerTcpFrame = 15;
		int iFiles = 0;
		while (!m_localServerReqQueue.empty() && iFiles < iMaxFilesPerTcpFrame)
		{
			// find the file with the longest waitingtime
			uint32 dwBestWaitTime = 0xFFFFFFFF;

			std::list<CPartFile*>::iterator posNextRequest = m_localServerReqQueue.end();
			std::list<CPartFile*>::iterator it = m_localServerReqQueue.begin();
			while( it != m_localServerReqQueue.end() ) {
				CPartFile* cur_file = (*it);
				if (cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY)
				{
					uint8 nPriority = cur_file->GetDownPriority();
					if (nPriority > PR_HIGH){
						wxASSERT(0);
						nPriority = PR_HIGH;
					}

					if (cur_file->lastsearchtime + (PR_HIGH-nPriority) < dwBestWaitTime ){
						dwBestWaitTime = cur_file->lastsearchtime + (PR_HIGH - nPriority);
						posNextRequest = it;
					}

					it++;
				}
				else{
					it = m_localServerReqQueue.erase(it);
					cur_file->m_bLocalSrcReqQueued = false;
					AddDebugLogLineM(false, wxString(wxT("Local server source request for file \"")) + cur_file->GetFileName() + wxString(wxT("\" not sent because of status '")) +  cur_file->getPartfileStatus() + wxT("'"));
				}
			}

			if (posNextRequest != m_localServerReqQueue.end())
			{
				CPartFile* cur_file = (*posNextRequest);
				cur_file->m_bLocalSrcReqQueued = false;
				cur_file->lastsearchtime = ::GetTickCount();
				m_localServerReqQueue.erase(posNextRequest);
				iFiles++;

				// create request packet
				Packet* packet = new Packet(OP_GETSOURCES,16);
				packet->Copy16ToDataBuffer((const char *)cur_file->GetFileHash().GetHash());
				dataTcpFrame.Write(packet->GetPacket(), packet->GetRealPacketSize());
				delete packet;
			}
		}

		int iSize = dataTcpFrame.GetLength();
		if (iSize > 0)
		{
			// create one 'packet' which contains all buffered OP_GETSOURCES eD2K packets to be sent with one TCP frame
			// server credits: 16*iMaxFilesPerTcpFrame+1 = 241
			Packet* packet = new Packet(new char[iSize], dataTcpFrame.GetLength(), true, false);
			dataTcpFrame.Seek(0, wxFromStart);
			dataTcpFrame.Read(packet->GetPacket(), iSize);
			uint32 size = packet->GetPacketSize();
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
	m_localServerReqQueue.push_back(sender);
}

void CDownloadQueue::GetDownloadStats(uint32 results[])
{

	results[0]=0;
	results[1]=0;

	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile *cur_file = filelist[i];
		results[0]+=cur_file->GetSourceCount();
		results[1]+=cur_file->GetTransferingSrcCount();
	}
}

CUpDownClient* CDownloadQueue::GetDownloadClientByIP(uint32 dwIP)
{
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		for (CPartFile::SourceSet::iterator it = cur_file->m_SrcList.begin(); it != cur_file->m_SrcList.end(); ++it ) {
			if (dwIP == (*it)->GetIP()) {
				return *it;
			}
		}
	}
	return NULL;
}


void CDownloadQueue::AddLinksFromFile()
{
        wxString filename;
	wxString link;
	wxTextFile linksfile(theApp.ConfigDir + wxT("ED2KLinks"));

	if (linksfile.Open()) {
		link = linksfile.GetFirstLine();
		// GetLineCount returns the actual number of lines in file, but line numbering
		// starts from zero. Thus we must loop until i = GetLineCount()-1.
		for (unsigned int i = 0; i < linksfile.GetLineCount(); i++) {
			// Need the links to end with /, otherwise CreateLinkFromUrl crashes us.
			
			// Special case! used by a secondary running mule to raise this one.
			if (link.Cmp(wxT("RAISE_DIALOG")) == 0) {
				printf("Rising dialog at secondary aMule running request\n");
				Notify_ShowGUI();
				continue;
			}
			
			if (link.Right(1) != wxT("/")) {
				link+=wxT("/");
			}
			try {
				CED2KLink* pLink=CED2KLink::CreateLinkFromUrl(unicode2char(link));
				if(pLink->GetKind()==CED2KLink::kFile) {
					// All seems ok, add it to download queue.
					// lfroen - using category 0
					AddFileLinkToDownload(pLink->GetFileLink(), 0);
				} else {
					throw wxString(_("Bad link."));
				}
				delete pLink;
			} catch(wxString error) {
				AddLogLineM(true, wxString::Format(_("Invalid link: %s"), wxString::Format(_("This ed2k link is invalid (%s)"), error.c_str()).c_str()));
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
	wxRemoveFile(theApp.ConfigDir +  wxT("ED2KLinks"));
}

/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN ... */
void CDownloadQueue::DisableAllA4AFAuto(void)
{
	CPartFile* cur_file;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		cur_file = (CPartFile*)filelist[i];
		if (cur_file != NULL) {
			cur_file->SetA4AFAuto(false);
		}
	}
}
/* eMule 0.30c implementation, i give it a try (Creteil) END ... */

void CDownloadQueue::ResetCatParts(int cat)
{
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		if (cur_file->GetCategory()==cat) {
			cur_file->SetCategory(0);
		} else if (cur_file->GetCategory() > cat) {
			cur_file->SetCategory(cur_file->GetCategory()-1);
		}
	}
}

void CDownloadQueue::SetCatPrio(int cat, uint8 newprio)
{
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		if (cat==0 || cur_file->GetCategory()==cat) {
			if (newprio==PR_AUTO) {
				cur_file->SetAutoDownPriority(true);
				cur_file->SetDownPriority(PR_HIGH);
			} else {
				cur_file->SetAutoDownPriority(false);
				cur_file->SetDownPriority(newprio);
			}
		}
	}
}

void CDownloadQueue::SetCatStatus(int cat, int newstatus)
{
	std::list<CPartFile*> fileList;

	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		
		if ( cur_file->CheckShowItemInGivenCat(cat) ) {
			fileList.push_back( cur_file );
		}
		
	}
	if ( !fileList.empty() ) {
		std::list<CPartFile*>::iterator it = fileList.begin();
		
		for ( ; it != fileList.end(); it++ ) {
			switch ( newstatus ) {
				case MP_CANCEL:
					(*it)->Delete();
					break;
				case MP_PAUSE:
					(*it)->PauseFile();
					break;
				case MP_STOP:
					(*it)->StopFile();
					break;
				case MP_RESUME:
					if ( (*it)->GetStatus() == PS_PAUSED ) {
						(*it)->ResumeFile();
					}
					break;
			}
		}
	}
	
}


uint16 CDownloadQueue::GetDownloadingFileCount()
{
	uint16 result = 0;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		if (cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY) {
			result++;
		}
	}
	return result;
}

uint16 CDownloadQueue::GetPausedFileCount()
{
	uint16 result = 0;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
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
	if (!thePrefs::IsCheckDiskspaceEnabled()) {
		if (!bNotEnoughSpaceLeft) { // avoid worse case, if we already had 'disk full'
			for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
				CPartFile* cur_file = filelist[i];
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
	wxLongLong total = 0, free = 0;
	wxGetDiskSpace(thePrefs::GetTempDir(), &total, &free);
	// 'bNotEnoughSpaceLeft' - avoid worse case, if we already had 'disk full'
	uint64 nTotalAvailableSpace = bNotEnoughSpaceLeft ? 0 : free.GetValue();
	if (thePrefs::GetMinFreeDiskSpace() == 0) {
		for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
			CPartFile* cur_file = filelist[i];
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
					AddLogLineM(false, wxString::Format(_("Free Disk Space (Total): %lli\n"), nTotalAvailableSpace));
					AddLogLineM(true, wxString::Format(_("File : %s, Needed Space : %i - PAUSED !!!\n"), unicode2char(cur_file->GetFileName()), cur_file->GetNeededSpace()));
					cur_file->PauseFile(true);
				}
			}
		}
	} else {
		for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
			CPartFile* cur_file = filelist[i];
			switch(cur_file->GetStatus()) {
				case PS_ERROR:
				case PS_COMPLETING:
				case PS_COMPLETE:
					continue;
			}
			if (nTotalAvailableSpace < thePrefs::GetMinFreeDiskSpace()) {
				if (cur_file->IsNormalFile()) {
				// Normal files: pause the file only if it would still grow
					uint32 nSpaceToGrow = cur_file->GetNeededSpace();
					if (nSpaceToGrow) {
						if (!cur_file->GetInsufficient()) {
							AddLogLineM(false, wxString::Format(_("Free Disk Space (Total): %lli\n"), nTotalAvailableSpace));
							AddLogLineM(true, wxString::Format(_("File : %s, Needed Space : %i - PAUSED !!!\n"), unicode2char(cur_file->GetFileName()), cur_file->GetNeededSpace()));
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

	if (!cur_udpserver) {
		return false;
	}
	

	if (theApp.serverconnect->GetCurrentServer() != NULL) {
		wxString srvaddr = theApp.serverconnect->GetCurrentServer()->GetAddress();
		if (cur_udpserver == theApp.serverlist->GetServerByAddress(srvaddr,theApp.serverconnect->GetCurrentServer()->GetPort())) {
			return false;	
		}
	}	
		
	int iFileIDs = data.GetLength() / 16;
	Packet packet(&data);

	packet.SetOpCode(OP_GLOBGETSOURCES);
	theApp.uploadqueue->AddUpDataOverheadServer(packet.GetPacketSize());
	theApp.serverconnect->SendUDPPacket(&packet,cur_udpserver,false);

	m_cRequestsSentToServer += iFileIDs;
	return true;
}


void CDownloadQueue::AddToResolve(const CMD4Hash& fileid, const wxString& pszHostname, uint16 port)
{
	bool bResolving = !m_toresolve.empty();

	// double checking
	if (!theApp.downloadqueue->GetFileByID(fileid)) {
		return;
	}
	Hostname_Entry* entry = new Hostname_Entry;
	entry->fileid = fileid;
	entry->strHostname = pszHostname;
	entry->port = port;
	m_toresolve.push_back(entry);

	if (bResolving) {
		return;
	}
	printf("Opening thread for resolving %s\n",unicode2char(pszHostname));
	CAsyncDNS* dns=new CAsyncDNS();
	if(dns->Create()!=wxTHREAD_NO_ERROR) {
		// Cannot create (Already there?)
		dns->Delete();
		return;
	}
	dns->ipName= pszHostname;
	if (dns->Run() != wxTHREAD_NO_ERROR) {
		// Cannot run (Already there?)
		dns->Delete();
		m_toresolve.pop_front();
		delete entry;
		return;
	}
}



bool CDownloadQueue::OnHostnameResolved(uint32 ip)
{
	Hostname_Entry* resolved = m_toresolve.front();
	m_toresolve.pop_front();

	if (resolved) {		
		if (ip!=0) {
			CPartFile* file = theApp.downloadqueue->GetFileByID(resolved->fileid);
			if (file) {
				CSafeMemFile sources(1+4+2);
				sources.WriteUInt8(1); // No. Sources
				sources.WriteUInt32(ip);
				sources.WriteUInt16(resolved->port);
				sources.Seek(0,wxFromStart);
				file->AddSources(&sources,0,0);
			}
		}
		delete resolved;
	}
	while (!m_toresolve.empty()) {
		Hostname_Entry* entry = m_toresolve.front();
		CAsyncDNS* dns=new CAsyncDNS();
		if(dns->Create()!=wxTHREAD_NO_ERROR) {
			// Cannot create (Already there?)
			dns->Delete();
			return false;
	 	}
		dns->ipName= entry->strHostname;
		if (dns->Run() != wxTHREAD_NO_ERROR) {
			// Cannot run (Already there?)
			dns->Delete();
			return false;
		}
		m_toresolve.pop_front();
		delete entry;
	}
	return TRUE;
}

wxString CDownloadQueue::getTextList( const wxString& file_to_search ) const
{
	// Search for file(s)
	// This file can be searched through:
	// wxString -> Will search through the download queue's filenames
	// FileId number -> Will return the info about the number of file provided
	// If no file is specified, then all the downloads-queue list is dumped

	wxString out;
	int i = 0;
	long filenumber = -1;
	// values from 0 to infinity are valid FileId values
	if ( file_to_search.IsNumber() && ! file_to_search.IsEmpty() ) {
		file_to_search.ToLong(&filenumber);
	}
	for ( std::deque<CPartFile *>::const_iterator it = filelist.begin();
		it != filelist.end();
		++it, ++i) {
		CPartFile *file = *it;
		if ( i == filenumber || ( filenumber == -1 && ( file_to_search.IsEmpty() || file->GetFileName().Lower().Find(file_to_search) != -1 ) ) ) {
			out +=
				wxString::Format(wxT("%i: "), i) +
				file->GetFileName() +
				wxString::Format(wxT("\t [%.1f%%] %i/%i - "),
					file->GetPercentCompleted(),
					file->GetTransferingSrcCount(), 
					file->GetSourceCount() ) +
				file->getPartfileStatus();
			if (file->GetKBpsDown() > 0.001) {
				out += wxString::Format(wxT(" %.1f "),(float)file->GetKBpsDown()) + _("kB/s");
			}
			out += wxT("\n");
		}
	}
	if ( filelist.begin() == filelist.end() )
		out = wxString(_("Download queue is empty."));
	else if ( out.IsEmpty() ) {
		if ( filenumber != -1 )
			out = wxString::Format(_("There is no slot %li in your download queue."), filenumber);
		else
			out = wxString(_("No results were found matching '")) + file_to_search + wxT("'"); 
	}
	
	return out;
}
