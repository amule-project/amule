//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "DownloadQueue.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h

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
#include "Statistics.h"		// Needed for CStatistics

#include <algorithm>
#include <numeric>
#include <sys/types.h>

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
	m_iSearchedServers = 0;
	m_nLastED2KLinkCheck = 0;
	m_dwNextTCPSrcReq = 0;
	m_cRequestsSentToServer = 0;
	do_not_sort_please = true;
}


void CDownloadQueue::AddPartFilesToShare()
{
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		if (cur_file->GetStatus(true) == PS_READY) {
			printf("Adding file %s to shares\n",unicode2char(cur_file->GetFullName()));
			sharedfilelist->SafeAddKFile(cur_file,true);
		}
	}
	do_not_sort_please = false;
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
			do_not_sort_please = true;
			filelist.push_back(toadd); // to downloadqueue
			do_not_sort_please = false;
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
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		delete filelist[i];
	}

}

void CDownloadQueue::SavePartFiles(bool del /*= false*/)
{
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		filelist[i]->m_hpartfile.Flush();
		filelist[i]->SavePartFile();
		if (del) {
			delete filelist[i];
		}
	}
	do_not_sort_please = false;
}

void CDownloadQueue::SaveSourceSeeds()
{
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		filelist[i]->SaveSourceSeeds();
	}
	do_not_sort_please = false;
}

void CDownloadQueue::LoadSourceSeeds()
{
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		filelist[i]->LoadSourceSeeds();
	}
	do_not_sort_please = false;
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
	do_not_sort_please = true;
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
	do_not_sort_please = false;
	if (pfile) {
		pfile->ResumeFile();
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
	wxString msgTemp = _("Downloading ") + newfile->GetFileName();	
	AddLogLineM(true, msgTemp);
	Notify_ShowNotifier(msgTemp, TBN_DLOAD, 0);
	// Kry - Get sources if not stopped
	if (!newfile->IsStopped()) {
		SendLocalSrcRequest(newfile);
	}
}

bool CDownloadQueue::IsFileExisting(const CMD4Hash& fileid) 
{
	if (const CKnownFile* file = sharedfilelist->GetFileByID(fileid)) {
		if (file->IsPartFile()) {
			AddLogLineM(true, _("You are already trying to download the file ") + file->GetFileName());
		} else {
			AddLogLineM(true, _("You already have the file ") + file->GetFileName());
		}
		return true;
	} else if ((file = GetFileByID(fileid))) {
		AddLogLineM(true, _("You are already trying to download the file ") + file->GetFileName());
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
	do_not_sort_please = true;
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
	do_not_sort_please = false;


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

CPartFile *CDownloadQueue::GetFileByID(const CMD4Hash& filehash) {
	do_not_sort_please = true;
	CPartFile* found = NULL;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		if (filehash == filelist[i]->GetFileHash()) {
			found = filelist[i];
			break;
		}
	}
	do_not_sort_please = false;
	return found;
}


CPartFile *CDownloadQueue::GetFileByIndex(unsigned int index) 
{
	CPartFile* found = NULL;
	do_not_sort_please = true;
	if ( index < filelist.size() ) {
		found = filelist[ index ];
	}
	do_not_sort_please = false;
	wxASSERT( found );
	return found;
}


bool CDownloadQueue::IsPartFile(const CKnownFile* totest) {
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		if (totest == filelist[i]) {
			do_not_sort_please = false;;
			return true;
		}
	}
	do_not_sort_please = false;
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
						Notify_DownloadCtrlAddSource(sender, *it, false);
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
			
			Notify_DownloadCtrlAddSource(sender, source, true);
		}
	} else {
		// Unknown client, add it to the clients list
		source->SetRequestFile( sender );

		theApp.clientlist->AddClient(source);
	
		if ( source->GetFileRate() || !source->GetFileComment().IsEmpty() ) {
			sender->UpdateFileRatingCommentAvail();
		}
	
		sender->AddSource( source );
	
		Notify_DownloadCtrlAddSource(sender, source, true);
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
				Notify_DownloadCtrlAddSource( sender, source, false );
			}
		}
	} else {
		source->SetRequestFile( sender );

		if ( source->GetFileRate() || !source->GetFileComment().IsEmpty() ) {
			sender->UpdateFileRatingCommentAvail();
		}

		sender->AddSource( source );
		Notify_DownloadCtrlAddSource( sender, source, true );
	}
}

/* Creteil importing new method from eMule 0.30c */
bool CDownloadQueue::RemoveSource(CUpDownClient* toremove, bool	WXUNUSED(updatewindow), bool bDoStatsUpdate)
{
	toremove->DeleteAllFileRequests();
	
	bool removed = false;
	do_not_sort_please = true;
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
	do_not_sort_please = false;

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
	do_not_sort_please = true;
	for ( std::deque<CPartFile*>::iterator it = filelist.begin(); it != filelist.end(); it++ ) {
		if (toremove == (*it)) {
			filelist.erase(it);
			return;
		}
	}
	do_not_sort_please = false;
	SortByPriority();
	CheckDiskspace();
}

void CDownloadQueue::DeleteAll(){
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		cur_file->m_SrcList.clear();
		cur_file->IsCountDirty = true;
		// Barry - Should also remove all requested blocks
		// Don't worry about deleting the blocks, that gets handled
		// when CUpDownClient is deleted in CClientList::DeleteAll()
		cur_file->RemoveAllRequestedBlocks();
	}
	do_not_sort_please = false;
}

bool CDownloadQueue::SendNextUDPPacket()
{
	do_not_sort_please = true;
	if (filelist.empty() || !theApp.serverconnect->IsUDPSocketAvailable()	|| !theApp.serverconnect->IsConnected()) {
		return false;
	}
	do_not_sort_please = false;
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
	do_not_sort_please = true;
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
	do_not_sort_please = false;
	
	if (!bSentPacket && dataGlobGetSources.GetLength() > 0) {
		SendGlobGetSourcesUDPPacket(dataGlobGetSources);
	}

	// send max 40 udp request to one server per interval, if we have more than 40 files, we rotate the list and use it as Queue
	if (m_cRequestsSentToServer >= MAX_REQUESTS_PER_SERVER) {
		// move the last 40 files to the head
		do_not_sort_please = true;
		if (filelist.size() > MAX_REQUESTS_PER_SERVER) {
			for (int i = 0; i != MAX_REQUESTS_PER_SERVER; i++) {
				filelist.push_front( filelist.back() );
				filelist.pop_back();
			}
		}
		do_not_sort_please = false;
		
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
		// Kry: Sorting for most non-current sourcing ratio
		
		uint16 sources_1 = file1->GetSourceCount() * 100;
		uint16 sources_2 = file2->GetSourceCount() * 100;
		
		if (!sources_1) {
			// No sources -> Less prio
			return false;
		}

		if (!sources_2) {
			// No sources -> Less prio
			return true;
		}
		
		uint16 not_current_1 = file1->GetNotCurrentSourcesCount() * 100;
		uint16 not_current_2 = file2->GetNotCurrentSourcesCount() * 100;
		
		if (!not_current_1) {
			// Sources avail. but none connected. Go up.
			return true;
		}
		
		if (!not_current_2) {
			// Sources avail. but none connected. Go up.
			return false;
		}	
		
		//bool ratio = file1->GetNotCurrentSourcesRatio() > file2->GetNotCurrentSourcesRatio();
		
		// Very simple algorithm: if (a) has a lower % of connected sources, go up
		bool ratio = (not_current_1/sources_1) > (not_current_2/sources_2);
			
		if (ratio) {
			return ratio;
		} else {		
			// If both files has same ratio (something incredibly absurd)		
			// Use normal alpha-numeric comparison here, lesser before greater*/
			return file1->GetPartMetFileName().CmpNoCase( file2->GetPartMetFileName() ) < 0;
		}
	}
}

void CDownloadQueue::SortByPriority()
{
	if (!do_not_sort_please) {
		do_not_sort_please = true;
		sort(filelist.begin(), filelist.end(), ComparePartFiles);
		do_not_sort_please = false;
	}
}

void CDownloadQueue::ResetLocalServerRequests()
{
	m_dwNextTCPSrcReq = 0;
	m_localServerReqQueue.clear();
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ )
	{
		CPartFile* pFile = filelist[i];
		UINT uState = pFile->GetStatus();
		if (uState == PS_READY || uState == PS_EMPTY)
			pFile->ResumeFile();
		pFile->m_bLocalSrcReqQueued = false;
	}
	do_not_sort_please = false;
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
			theApp.statistics->AddUpDataOverheadServer(size);
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
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile *cur_file = filelist[i];
		results[0]+=cur_file->GetSourceCount();
		results[1]+=cur_file->GetTransferingSrcCount();
	}
	do_not_sort_please = false;
}

CUpDownClient* CDownloadQueue::GetDownloadClientByIP(uint32 dwIP)
{
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		for (CPartFile::SourceSet::iterator it = cur_file->m_SrcList.begin(); it != cur_file->m_SrcList.end(); ++it ) {
			if (dwIP == (*it)->GetIP()) {
				do_not_sort_please = false;
				return *it;
			}
		}
	}
	do_not_sort_please = false;
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
			// Special case! used by a secondary running mule to raise this one.
			if (link.Cmp(wxT("RAISE_DIALOG")) == 0) {
				printf("Rising dialog at secondary aMule running request\n");
				Notify_ShowGUI();
				continue;
			}
			if (!link.IsEmpty()) {
				AddED2KLink( link );
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
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		cur_file = (CPartFile*)filelist[i];
		if (cur_file != NULL) {
			cur_file->SetA4AFAuto(false);
		}
	}
	do_not_sort_please = false;
}
/* eMule 0.30c implementation, i give it a try (Creteil) END ... */

void CDownloadQueue::ResetCatParts(int cat)
{
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		if (cur_file->GetCategory()==cat) {
			cur_file->SetCategory(0);
		} else if (cur_file->GetCategory() > cat) {
			cur_file->SetCategory(cur_file->GetCategory()-1);
		}
	}
	do_not_sort_please = false;
}

void CDownloadQueue::SetCatPrio(int cat, uint8 newprio)
{
	do_not_sort_please = true;
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
	do_not_sort_please = false;
}

void CDownloadQueue::SetCatStatus(int cat, int newstatus)
{
	std::list<CPartFile*> fileList;
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		
		if ( cur_file->CheckShowItemInGivenCat(cat) ) {
			fileList.push_back( cur_file );
		}
		
	}
	do_not_sort_please = false;
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
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		if (cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY) {
			result++;
		}
	}
	do_not_sort_please = false;
	return result;
}

uint16 CDownloadQueue::GetPausedFileCount()
{
	uint16 result = 0;
	do_not_sort_please = true;
	for ( uint16 i = 0, size = filelist.size(); i < size; i++ ) {
		CPartFile* cur_file = filelist[i];
		if (cur_file->GetStatus() == PS_PAUSED) {
			result++;
		}
	}
	do_not_sort_please = false;
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
			do_not_sort_please = true;
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
			do_not_sort_please = false;
		}
		return;
	}
	wxLongLong total = 0, free = 0;
	wxGetDiskSpace(thePrefs::GetTempDir(), &total, &free);
	// 'bNotEnoughSpaceLeft' - avoid worse case, if we already had 'disk full'
	uint64 nTotalAvailableSpace = bNotEnoughSpaceLeft ? 0 : free.GetValue();
	if (thePrefs::GetMinFreeDiskSpace() == 0) {
		do_not_sort_please = true;
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
		do_not_sort_please = false;
	} else {
		do_not_sort_please = true;
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
		do_not_sort_please = false;
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
	theApp.statistics->AddUpDataOverheadServer(packet.GetPacketSize());
	theApp.serverconnect->SendUDPPacket(&packet,cur_udpserver,false);

	m_cRequestsSentToServer += iFileIDs;
	return true;
}


void CDownloadQueue::AddToResolve(const CMD4Hash& fileid, const wxString& pszHostname, uint16 port)
{
	bool bResolving = !m_toresolve.empty();

	// double checking
	if (!GetFileByID(fileid)) {
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
			CPartFile* file = GetFileByID(resolved->fileid);
			if (file) {
				CSafeMemFile sources(1+4+2);
				sources.WriteUInt8(1); // No. Sources
				sources.WriteUInt32(ip);
				sources.WriteUInt16(resolved->port);
				sources.Seek(0,wxFromStart);
				file->AddSources(sources,0,0);
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

bool CDownloadQueue::AddED2KLink( const wxString& link, int category )
{
	wxASSERT( !link.IsEmpty() );
	wxString URI = link;
	
	// Need the links to end with /, otherwise CreateLinkFromUrl crashes us.
	if ( URI.Last() != wxT('/') ) {
		URI += wxT("/");
	}
	
	try {
		return AddED2KLink( CED2KLink::CreateLinkFromUrl(URI), category );
	} catch ( wxString err ) {
		AddLogLineM( true, _("Invalid ed2k link! Error: ") + err);
	}
	
	return false;
}


bool CDownloadQueue::AddED2KLink( const CED2KLink* link, int category )
{
	switch ( link->GetKind() ) {
		case CED2KLink::kFile:
			return AddED2KLink( dynamic_cast<const CED2KFileLink*>( link ), category );
			
		case CED2KLink::kServer:
			return AddED2KLink( dynamic_cast<const CED2KServerLink*>( link ) );
			
		case CED2KLink::kServerList:
			return AddED2KLink( dynamic_cast<const CED2KServerListLink*>( link ) );
			
		default:
			return false;
	}
}



bool CDownloadQueue::AddED2KLink( const CED2KFileLink* link, int category )
{
	// Check if the file already exists, in which case we just add the source
	CPartFile* file = GetFileByID( link->GetHashKey() );
		
	if ( !file ) {
		file = new CPartFile( link );
	
		if ( file->GetStatus() == PS_ERROR ) {
			delete file;

			return false;
		}
	
		AddDownload( file, thePrefs::AddNewFilesPaused(), category );
	}
	
	
	// Add specified sources, specified by IP
	if ( link->HasValidSources() ) {
		file->AddClientSources( link->m_sources, 1 );
	}
	
	// Add specified sources, specified by hostname
	if ( link->HasHostnameSources() ) {
		const CTypedPtrList<CPtrList, SUnresolvedHostname*>& list = link->m_hostSources;
		
		for ( POSITION pos = list.GetHeadPosition(); pos; list.GetNext(pos) ) {
			AddToResolve( link->GetHashKey(), list.GetAt(pos)->strHostname, list.GetAt(pos)->nPort );
		}
	}

	return true;	
}


bool CDownloadQueue::AddED2KLink( const CED2KServerLink* link )
{
	CServer *server = new CServer( link->GetPort(), Uint32toStringIP( link->GetIP() ) );
	
	server->SetListName( Uint32toStringIP( link->GetIP() ) );
	
	theApp.serverlist->AddServer(server);
	
	Notify_ServerAdd(server);

	return true;
}


bool CDownloadQueue::AddED2KLink( const CED2KServerListLink* WXUNUSED(link) )
{
	#warning AddED2KLink @ CED2KServerListLink not implemented!
	return false;
}
