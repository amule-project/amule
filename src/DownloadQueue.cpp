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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "DownloadQueue.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h

#include <wx/filefn.h>
#include <wx/textfile.h>	// Needed for wxTextFile
#include <wx/filename.h>
#include <wx/utils.h>

#include "DownloadQueue.h"	// Interface declarations
#include "Server.h"		// Needed for CServer
#include "Packet.h"		// Needed for CPacket
#include "MemFile.h"		// Needed for CMemFile
#include "ClientList.h"		// Needed for CClientList
#include "updownclient.h"	// Needed for CUpDownClient
#include "ServerList.h"		// Needed for CServerList
#include "ServerConnect.h"		// Needed for CServerConnect
#include "ED2KLink.h"		// Needed for CED2KFileLink
#include "SearchList.h"		// Needed for CSearchFile
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "PartFile.h"		// Needed for CPartFile
#include "Preferences.h"	// Needed for thePrefs
#include "amule.h"			// Needed for theApp
#include "GetTickCount.h"	// Needed for GetTickCount
#include "NetworkFunctions.h" // Needed for CAsyncDNS
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"
#include "Format.h"		// Needed for CFormat
#include "IPFilter.h"
#include "FileFunctions.h"	// Needed for CDirIterator
#include "OtherFunctions.h"	// Needed for CMutexUnlocker

#include "kademlia/kademlia/Kademlia.h"

#include <algorithm>
#include <numeric>
#include <vector>


// Max. file IDs per UDP packet
// ----------------------------
// 576 - 30 bytes of header (28 for UDP, 2 for "E3 9A" edonkey proto) = 546 bytes
// 546 / 16 = 34


#define MAX_FILES_PER_UDP_PACKET	31	// 2+16*31 = 498 ... is still less than 512 bytes!!
#define MAX_REQUESTS_PER_SERVER		35


CDownloadQueue::CDownloadQueue()
// Needs to be recursive that that is can own an observer assigned to itself
	: m_mutex( wxMUTEX_RECURSIVE )
{
	m_datarate = 0;
	m_udpserver = 0;
	m_lastsorttime = 0;
	m_lastudpsearchtime = 0;
	m_lastudpstattime = 0;
	m_udcounter = 0;
	m_nLastED2KLinkCheck = 0;
	m_dwNextTCPSrcReq = 0;
	m_cRequestsSentToServer = 0;
	m_lastDiskCheck = 0;
	SetLastKademliaFileRequest();
}


CDownloadQueue::~CDownloadQueue()
{
	if ( !m_filelist.empty() ) {
		for ( unsigned int i = 0; i < m_filelist.size(); i++ ) {
			printf("\rSaving PartFile %u of %u", i + 1, (unsigned int)m_filelist.size());
			fflush(stdout);
			delete m_filelist[i];
		}
		printf("\nAll PartFiles Saved.\n");
	}
}


void CDownloadQueue::LoadMetFiles( const wxString& path )
{
	printf("Loading temp files from %s.\n",
		(const char *)unicode2char(path));
	
	CDirIterator TempDir( path );
	std::vector<wxString> files;

	// Locate part-files to be loaded
	wxString fileName = TempDir.GetFirstFile( CDirIterator::File, wxT("*.part.met") );
	while ( !fileName.IsEmpty() ) {
		files.push_back( fileName );

		fileName = TempDir.GetNextFile();
	}
	std::sort( files.begin(), files.end() );

	// Load part-files	
	for ( size_t i = 0; i < files.size(); i++ ) {
		printf("\rLoading PartFile %u of %u", (unsigned int)(i + 1), (unsigned int)files.size());
			
		fileName = wxFileName( files[i] ).GetFullName();
		
		CPartFile* toadd = new CPartFile();
		bool result = toadd->LoadPartFile(path, fileName);
		if (!result) {
			// Try from backup
			result = toadd->LoadPartFile(path, fileName, true);
		}
		
		if (result) {
			{
				wxMutexLocker lock(m_mutex);
				m_filelist.push_back(toadd);
			}
			
			NotifyObservers( EventType( EventType::INSERTED, toadd ) );
			
			if ( toadd->GetStatus(true) == PS_READY ) {
				theApp.sharedfiles->SafeAddKFile( toadd, true ); 
			}
			
			Notify_DownloadCtrlAddFile(toadd);
		} else {
			// Newline so that the error stays visible.
			printf(": Failed to load PartFile '%s'\n", (const char*)unicode2char(fileName));
			delete toadd;
		}
	}

	printf("\nAll PartFiles Loaded.\n");
	
	if ( GetFileCount() == 0 ) {
		AddLogLineM(false, _("No part files found"));
	} else {
		AddLogLineM(false, wxString::Format(_("Found %u part files"), GetFileCount()) );
		
		DoSortByPriority();
		CheckDiskspace( path );
	}
}


uint16 CDownloadQueue::GetFileCount() const
{
	wxMutexLocker lock( m_mutex );
	
	return m_filelist.size();
}


CServer* CDownloadQueue::GetUDPServer() const
{
	wxMutexLocker lock( m_mutex );

	return m_udpserver;
}


void CDownloadQueue::SetUDPServer( CServer* server )
{
	wxMutexLocker lock( m_mutex );

	m_udpserver = server;
}

	
void CDownloadQueue::SaveSourceSeeds()
{
	for ( uint16 i = 0; i < GetFileCount(); i++ ) {
		GetFileByIndex( i )->SaveSourceSeeds();
	}
}


void CDownloadQueue::LoadSourceSeeds()
{
	for ( uint16 i = 0; i < GetFileCount(); i++ ) {
		GetFileByIndex( i )->LoadSourceSeeds();
	}
}


void CDownloadQueue::AddSearchToDownload(CSearchFile* toadd, uint8 category)
{
	if ( IsFileExisting(toadd->GetFileHash()) ) {
		return;
	}
	
	CPartFile* newfile = new CPartFile(toadd);
	if ( newfile && newfile->GetStatus() != PS_ERROR ) {
		AddDownload( newfile, thePrefs::AddNewFilesPaused(), category );
	}
}


void CDownloadQueue::StartNextFile(CPartFile* oldfile)
{
	if ( thePrefs::StartNextFile() ) {
		CPartFile* tFile = NULL;

		{
			wxMutexLocker lock(m_mutex);
			
			for ( uint16 i = 0; i < m_filelist.size(); i++ ) {
				CPartFile* file = m_filelist[i];
			
				if ( file->GetStatus() == PS_PAUSED ) {
					if ( !tFile || file->GetDownPriority() > tFile->GetDownPriority() ) {
						if (tFile && thePrefs::StartNextFileSame()) {
							if (tFile->GetCategory() == oldfile->GetCategory()) {
								// Already found a file for this category
								if (file->GetCategory() == oldfile->GetCategory()) {
									// But the new one is also for this category
									if (file->GetDownPriority() > tFile->GetDownPriority()) {
										// So, higher prio?
										tFile = file;
									} else {
										; // Lower prio, ignore this file.
									}
								} else {
									; // It's not from the same category, so nothing
								}
							} else {
								// Found no file yet with the same category
								if (file->GetDownPriority() > tFile->GetDownPriority()) {
									// So, higher prio?
									tFile = file;
								} else {
									; // Lower prio, ignore this file.
								}
							}
						} else {
							// There is no file found yet or no category selection.
							tFile = file;
						}
						
						if ( tFile->GetDownPriority() == PR_HIGH ) {
							if (thePrefs::StartNextFileSame()) {
								// It's from the same category?
								if (tFile->GetCategory() == oldfile->GetCategory()) {
									// There can't be any higher
									break;
								} else {
									; // Maybe we'll find another file from the same category
								}
							} else {
								// There can't be any higher
								break;							
							}
						}	
					}
				}
			}
		}
		
		if ( tFile ) {
			tFile->ResumeFile();
		}
	}
}


void CDownloadQueue::AddDownload(CPartFile* file, bool paused, uint8 category)
{
	if ( paused && GetFileCount() ) {
		file->StopFile();
	}

	{
		wxMutexLocker lock(m_mutex);
		m_filelist.push_back( file );
		DoSortByPriority();
	}

	NotifyObservers( EventType( EventType::INSERTED, file ) );

	file->SetCategory(category);
	Notify_DownloadCtrlAddFile( file );
	AddLogLineM(true, CFormat(_("Downloading %s")) % file->GetFileName() );
}


bool CDownloadQueue::IsFileExisting( const CMD4Hash& fileid ) const 
{
	if (const CKnownFile* file = theApp.sharedfiles->GetFileByID(fileid)) {
		if (file->IsPartFile()) {
			AddLogLineM(true, CFormat( _("You are already trying to download the file '%s'") ) % file->GetFileName());
		} else {
			AddLogLineM(true, CFormat( _("You already have the file '%s'") ) % file->GetFileName());
		}
		return true;
	} else if ((file = GetFileByID(fileid))) {
		AddLogLineM(true, CFormat( _("You are already trying to download the file %s") ) % file->GetFileName());
		return true;
	}
	return false;
}


void CDownloadQueue::Process()
{
	// send src requests to local server
	ProcessLocalRequests();
	
	{
		wxMutexLocker lock(m_mutex);

		uint32 downspeed = 0;
		if (thePrefs::GetMaxDownload() != UNLIMITED && m_datarate > 1500) {
			downspeed = (((uint32)thePrefs::GetMaxDownload())*1024*100)/(m_datarate+1); 
			if (downspeed < 50) {
				downspeed = 50;
			} else if (downspeed > 200) {
				downspeed = 200;
			}
		}
	
		m_datarate = 0;
		m_udcounter++;
		uint32 cur_datarate = 0;
		uint32 cur_udcounter = m_udcounter;
		
		for ( uint16 i = 0; i < m_filelist.size(); i++ ) {
			CPartFile* file = m_filelist[i];
	
			CMutexUnlocker unlocker(m_mutex);
			
			if ( file->GetStatus() == PS_READY || file->GetStatus() == PS_EMPTY ){
				cur_datarate += file->Process( downspeed, cur_udcounter );
			} else {
				//This will make sure we don't keep old sources to paused and stoped files..
				file->StopPausedFile();
			}
		}
	
		m_datarate += cur_datarate;
	
	
		if (m_udcounter == 5) {
			if (theApp.serverconnect->IsUDPSocketAvailable()) {
				if( (::GetTickCount() - m_lastudpstattime) > UDPSERVERSTATTIME) {
					m_lastudpstattime = ::GetTickCount();
					
					CMutexUnlocker unlocker(m_mutex);
					theApp.serverlist->ServerStats();
				}
			}
		}
	
		if (m_udcounter == 10) {
			m_udcounter = 0;
			if (theApp.serverconnect->IsUDPSocketAvailable()) {
				if ( (::GetTickCount() - m_lastudpsearchtime) > UDPSERVERREASKTIME) {
					SendNextUDPPacket();
				}
			}
		}
	
		if ( (::GetTickCount() - m_lastsorttime) > 10000 ) {
			
			
			DoSortByPriority();
		}
		// Check if any paused files can be resumed
			
		CheckDiskspace( thePrefs::GetTempDir() );
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


CPartFile* CDownloadQueue::GetFileByID(const CMD4Hash& filehash) const
{
	wxMutexLocker lock( m_mutex );
	
	for ( uint16 i = 0; i < m_filelist.size(); ++i ) {
		if ( filehash == m_filelist[i]->GetFileHash()) {
			return m_filelist[ i ];
		}
	}
	
	return NULL;
}


CPartFile* CDownloadQueue::GetFileByIndex(unsigned int index)  const
{
	wxMutexLocker lock( m_mutex );
	
	if ( index < m_filelist.size() ) {
		return m_filelist[ index ];
	}
	
	wxASSERT( false );
	return NULL;
}


bool CDownloadQueue::IsPartFile(const CKnownFile* file) const
{
	wxMutexLocker lock( m_mutex );

	for ( uint16 i = 0; i < m_filelist.size(); i++ ) {
		if ( file == m_filelist[i] ) {
			return true;
		}
	}
	
	return false;
}


void CDownloadQueue::CheckAndAddSource(CPartFile* sender, CUpDownClient* source)
{
	// if we block loopbacks at this point it should prevent us from connecting to ourself
	if ( source->HasValidHash() ) {
		if ( source->GetUserHash() == thePrefs::GetUserHash() ) {
			AddDebugLogLineM( false, logDownloadQueue, wxT("Tried to add source with matching hash to your own.") );
			source->Safe_Delete();
			return;
		}
	}

	if (sender->IsStopped()) {
		source->Safe_Delete();
		return;
	}

	// Filter sources which are known to be dead/useless
	if ( theApp.clientlist->IsDeadSource( source ) || sender->IsDeadSource(source) ) {
		source->Safe_Delete();
		return;
	}

	
	// Find all clients with the same hash
	if ( source->HasValidHash() ) {
		CClientList::SourceList found = theApp.clientlist->GetClientsByHash( source->GetUserHash() );

		CClientList::SourceList::iterator it = found.begin();
		for ( ; it != found.end(); it++ ) {
			CKnownFile* file = (*it)->GetRequestFile();

			// Only check files on the download-queue
			if ( file ) {
				// Is the found source queued for something else?
				if (  file != sender ) {
					// Try to add a request for the other file
					if ( (*it)->AddRequestForAnotherFile(sender)) {
						// Add it to downloadlistctrl
						Notify_DownloadCtrlAddSource(sender, *it, A4AF_SOURCE);
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
			// If we're already queued for the right file, then there's nothing to do
			if ( sender != source->GetRequestFile() ) {	
				// Add the new file to the request list
				source->AddRequestForAnotherFile( sender );
			}
		} else {
			// Source was known, but reqfile NULL. I'm not sure if this can 
			// happen, but it's best to be certain, so I handle this case as well
			source->SetRequestFile( sender );
			
			sender->AddSource( source );
			if ( source->GetFileRating() || !source->GetFileComment().IsEmpty() ) {
				sender->UpdateFileRatingCommentAvail();
			}
			
			Notify_DownloadCtrlAddSource(sender, source, UNAVAILABLE_SOURCE);
		}
	} else {
		// Unknown client, add it to the clients list
		source->SetRequestFile( sender );

		theApp.clientlist->AddClient(source);
	
		sender->AddSource( source );
		if ( source->GetFileRating() || !source->GetFileComment().IsEmpty() ) {
			sender->UpdateFileRatingCommentAvail();
		}
	
		Notify_DownloadCtrlAddSource(sender, source, UNAVAILABLE_SOURCE);
	}
}


void CDownloadQueue::CheckAndAddKnownSource(CPartFile* sender,CUpDownClient* source)
{
	// Kad reviewed
	
	if (sender->IsStopped()) {
		return;
	}

	// Filter sources which are known to be dead/useless
	if ( sender->IsDeadSource(source) ) {
		return;
	}

	// "Filter LAN IPs" -- this may be needed here in case we are connected to the internet and are also connected
	// to a LAN and some client from within the LAN connected to us. Though this situation may be supported in future
	// by adding that client to the source list and filtering that client's LAN IP when sending sources to
	// a client within the internet.
	//
	// "IPfilter" is not needed here, because that "known" client was already IPfiltered when receiving OP_HELLO.
	if (!source->HasLowID()) {
		uint32 nClientIP = wxUINT32_SWAP_ALWAYS(source->GetUserIDHybrid());
		if (!IsGoodIP(nClientIP, thePrefs::FilterLanIPs())) { // check for 0-IP, localhost and LAN addresses
			AddDebugLogLineM(false, logIPFilter, wxT("Ignored already known source with IP=%s") + Uint32toStringIP(nClientIP));
			return;
		}
	}	
	
	CPartFile* file = source->GetRequestFile();
	
	// Check if the file is already queued for something else
	if ( file ) {
		if ( file != sender ) {
			if ( source->AddRequestForAnotherFile( sender ) ) {
				Notify_DownloadCtrlAddSource( sender, source, A4AF_SOURCE );
			}
		}
	} else {
		source->SetRequestFile( sender );

		if ( source->GetFileRating() || !source->GetFileComment().IsEmpty() ) {
			sender->UpdateFileRatingCommentAvail();
		}

		source->SetSourceFrom(SF_PASSIVE);
		sender->AddSource( source );
		Notify_DownloadCtrlAddSource( sender, source, UNAVAILABLE_SOURCE);
	}
}


bool CDownloadQueue::RemoveSource(CUpDownClient* toremove, bool	WXUNUSED(updatewindow), bool bDoStatsUpdate)
{
	bool removed = false;
	toremove->DeleteAllFileRequests();
	
	for ( uint16 i = 0; i < GetFileCount(); i++ ) {
		CPartFile* cur_file = GetFileByIndex( i );
		
		// Remove from source-list
		if ( cur_file->DelSource( toremove ) ) {
			cur_file->RemoveDownloadingSource(toremove);
			removed = true;
			if ( bDoStatsUpdate ) {
				cur_file->UpdatePartsInfo();
			}
		}

		// Remove from A4AF-list
		cur_file->A4AFsrclist.erase( toremove );
	}


	if ( !toremove->GetFileComment().IsEmpty() || toremove->GetFileRating()>0) {
		toremove->GetRequestFile()->UpdateFileRatingCommentAvail();
	}
	
	toremove->SetRequestFile( NULL );
	toremove->SetDownloadState(DS_NONE);

	// Remove from downloadlist widget
	Notify_DownloadCtrlRemoveSource(toremove,0);
	toremove->ResetFileStatusInfo();

	return removed;
}


void CDownloadQueue::RemoveFile(CPartFile* file)
{
	RemoveLocalServerRequest( file );

	NotifyObservers( EventType( EventType::REMOVED, file ) );

	wxMutexLocker lock( m_mutex );

	EraseValue( m_filelist, file );
}


CUpDownClient* CDownloadQueue::GetDownloadClientByIP_UDP(uint32 dwIP, uint16 nUDPPort) const
{
	wxMutexLocker lock( m_mutex );

	for ( unsigned int i = 0; i < m_filelist.size(); i++ ) {
		const CPartFile::SourceSet& set = m_filelist[i]->GetSourceList();
		
		for ( CPartFile::SourceSet::iterator it = set.begin(); it != set.end(); it++ ) {
			if ( (*it)->GetIP() == dwIP && (*it)->GetUDPPort() == nUDPPort ) {
				return *it;
			}
		}
	}
	return NULL;
}


/**
 * Checks if the specified server is the one we are connected to.
 */
bool IsConnectedServer(const CServer* server)
{
	if (server && theApp.serverconnect->GetCurrentServer()) {
		wxString srvAddr = theApp.serverconnect->GetCurrentServer()->GetAddress();
		uint16 srvPort = theApp.serverconnect->GetCurrentServer()->GetPort();

		return server->GetAddress() == srvAddr && server->GetPort() == srvPort;
	}

	return false;
}


bool CDownloadQueue::SendNextUDPPacket()
{
	if ( m_filelist.empty() || !theApp.serverconnect->IsUDPSocketAvailable() || !theApp.IsConnectedED2K()) {
		return false;
	}

	// Start monitoring the server and the files list
	if ( !m_queueServers.IsActive() ) {
		AddObserver( &m_queueFiles );
		
		theApp.serverlist->AddObserver( &m_queueServers );
	}
	

 	bool packetSent = false;
 	while ( !packetSent ) {
 		// Get max files ids per packet for current server
 		int filesAllowed = GetMaxFilesPerUDPServerPacket();
 
 		if (filesAllowed < 1 || !m_udpserver || IsConnectedServer(m_udpserver)) {
 			// Select the next server to ask, must not be the connected server
 			do {
				m_udpserver = m_queueServers.GetNext();
			} while (IsConnectedServer(m_udpserver));
	
 			m_cRequestsSentToServer = 0;
 			filesAllowed = GetMaxFilesPerUDPServerPacket();
  		}
 
 		
 		// Check if we have asked all servers, in which case we are done
 		if (m_udpserver == NULL) {
 			DoStopUDPRequests();
 			
 			return false;
  		}
  
 		// Memoryfile containing the hash of every file to request
		// 20bytes allocation because 16b + 4b is the worse case scenario.
 		CMemFile hashlist( 20 );
 		
		CPartFile* file = m_queueFiles.GetNext();
		while ( file && filesAllowed ) {
 			uint8 status = file->GetStatus();
 			
			if ( ( status == PS_READY || status == PS_EMPTY ) && file->GetSourceCount() < thePrefs::GetMaxSourcePerFileUDP() ) {
 				hashlist.WriteHash( file->GetFileHash() );
					
				if ( m_udpserver->GetUDPFlags() & SRV_UDPFLG_EXT_GETSOURCES2 ) {
					hashlist.WriteUInt32( file->GetFileSize() );
				}

				--filesAllowed;
  			}
		
			// Avoid skipping a file if we can't send any more currently
			if ( filesAllowed ) {
				file = m_queueFiles.GetNext();
			}
		}
		
		// See if we have anything to send
 		if ( hashlist.GetLength() ) {
 			packetSent = SendGlobGetSourcesUDPPacket(hashlist);
		}
 		
 		
 		// Check if we've covered every file
		if ( file == NULL ) {
 			// Reset the list of asked files so that the loop will start over
			m_queueFiles.Reset();
			
 		
 			// Unset the server so that the next server will be used
 			m_udpserver = NULL;
  		}
  	}
	
	return true;
}


void CDownloadQueue::StopUDPRequests()
{
	wxMutexLocker lock( m_mutex );

	DoStopUDPRequests();
}


void CDownloadQueue::DoStopUDPRequests()
{
	// No need to observe when we wont be using the results
	theApp.serverlist->RemoveObserver( &m_queueServers );
	RemoveObserver( &m_queueFiles );
	
	m_udpserver = 0;
	m_lastudpsearchtime = ::GetTickCount();
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
		int sourcesA = file1->GetSourceCount();
		int sourcesB = file2->GetSourceCount();
		
		int notSourcesA = file1->GetNotCurrentSourcesCount();
		int notSourcesB = file2->GetNotCurrentSourcesCount();
		
		int cmp = CmpAny( sourcesA - notSourcesA, sourcesB - notSourcesB );

		if ( cmp == 0 ) {
			cmp = CmpAny( notSourcesA, notSourcesB );
		}

		return cmp < 0;
	}
}


void CDownloadQueue::DoSortByPriority()
{
	m_lastsorttime = ::GetTickCount();
	sort( m_filelist.begin(), m_filelist.end(), ComparePartFiles );
}


void CDownloadQueue::ResetLocalServerRequests()
{
	wxMutexLocker lock( m_mutex );

	m_dwNextTCPSrcReq = 0;
	m_localServerReqQueue.clear();

	for ( uint16 i = 0; i < m_filelist.size(); i++ ) {
		m_filelist[i]->m_bLocalSrcReqQueued = false;
	}
}


void CDownloadQueue::RemoveLocalServerRequest( CPartFile* file )
{
	wxMutexLocker lock( m_mutex );
	
	EraseValue( m_localServerReqQueue, file );

	file->m_bLocalSrcReqQueued = false;
}


void CDownloadQueue::ProcessLocalRequests()
{
	wxMutexLocker lock( m_mutex );
	
	if ( (!m_localServerReqQueue.empty()) && (m_dwNextTCPSrcReq < ::GetTickCount()) ) {
		CMemFile dataTcpFrame(22);
		const int iMaxFilesPerTcpFrame = 15;
		int iFiles = 0;
		while (!m_localServerReqQueue.empty() && iFiles < iMaxFilesPerTcpFrame) {
			// find the file with the longest waitingtime
			uint32 dwBestWaitTime = 0xFFFFFFFF;

			std::list<CPartFile*>::iterator posNextRequest = m_localServerReqQueue.end();
			std::list<CPartFile*>::iterator it = m_localServerReqQueue.begin();
			while( it != m_localServerReqQueue.end() ) {
				CPartFile* cur_file = (*it);
				if (cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY) {
					uint8 nPriority = cur_file->GetDownPriority();
					if (nPriority > PR_HIGH) {
						wxASSERT(0);
						nPriority = PR_HIGH;
					}

					if (cur_file->lastsearchtime + (PR_HIGH-nPriority) < dwBestWaitTime ){
						dwBestWaitTime = cur_file->lastsearchtime + (PR_HIGH - nPriority);
						posNextRequest = it;
					}

					it++;
				} else {
					it = m_localServerReqQueue.erase(it);
					cur_file->m_bLocalSrcReqQueued = false;
					AddDebugLogLineM( false, logDownloadQueue, wxString(wxT("Local server source request for file \"")) + cur_file->GetFileName() + wxString(wxT("\" not sent because of status '")) +  cur_file->getPartfileStatus() + wxT("'"));
				}
			}

			if (posNextRequest != m_localServerReqQueue.end()) {
				CPartFile* cur_file = (*posNextRequest);
				cur_file->m_bLocalSrcReqQueued = false;
				cur_file->lastsearchtime = ::GetTickCount();
				m_localServerReqQueue.erase(posNextRequest);
				iFiles++;

				// create request packet
				CMemFile data(20);
				data.WriteHash(cur_file->GetFileHash());
				// Kry - lugdunum extended protocol on 17.3 to handle filesize properly.
				// There is no need to check anything, old server ignore the extra 4 bytes.
				data.WriteUInt32(cur_file->GetFileSize());
				CPacket packet(&data);
				packet.SetOpCode(OP_GETSOURCES);
				dataTcpFrame.Write(packet.GetPacket(), packet.GetRealPacketSize());
			}
		}

		int iSize = dataTcpFrame.GetLength();
		if (iSize > 0) {
			// create one 'packet' which contains all buffered OP_GETSOURCES eD2K packets to be sent with one TCP frame
			// server credits: 16*iMaxFilesPerTcpFrame+1 = 241
			CPacket* packet = new CPacket(new char[iSize], dataTcpFrame.GetLength(), true, false);
			dataTcpFrame.Seek(0, wxFromStart);
			dataTcpFrame.Read(packet->GetPacket(), iSize);
			uint32 size = packet->GetPacketSize();
			theApp.serverconnect->SendPacket(packet, true);	// Deletes `packet'.
			theStats::AddUpOverheadServer(size);
		}

		// next TCP frame with up to 15 source requests is allowed to be sent in..
		m_dwNextTCPSrcReq = ::GetTickCount() + SEC2MS(iMaxFilesPerTcpFrame*(16+4));
	}

}


void CDownloadQueue::SendLocalSrcRequest(CPartFile* sender)
{
	wxMutexLocker lock( m_mutex );
	
	m_localServerReqQueue.push_back(sender);
}


void CDownloadQueue::AddLinksFromFile()
{
	wxTextFile file(theApp.ConfigDir + wxT("ED2KLinks"));

	if ( file.Open() ) {
		for ( unsigned int i = 0; i < file.GetLineCount(); i++ ) {
			wxString line = file.GetLine( i ).Strip( wxString::both );
			
			if ( !line.IsEmpty() ) {
				// Special case! used by a secondary running mule to raise this one.
				if ( line == wxT("RAISE_DIALOG")  ) {
					Notify_ShowGUI();
					continue;
				}
				
				AddED2KLink( line );
			}
		}

		file.Close();
	} else {
		printf("Failed to open ED2KLinks file.\n");
	}
	
	// Delete the file.
	wxRemoveFile(theApp.ConfigDir +  wxT("ED2KLinks"));
}


void CDownloadQueue::ResetCatParts(uint8 cat)
{
	for ( uint16 i = 0; i < GetFileCount(); i++ ) {
		CPartFile* file = GetFileByIndex( i );
		
		if ( file->GetCategory() == cat ) {
			// Reset the category
			file->SetCategory( 0 );
		} else if ( file->GetCategory() > cat ) {
			// Set to the new position of the original category
			file->SetCategory( file->GetCategory() - 1 );
		}
	}
}


void CDownloadQueue::SetCatPrio(uint8 cat, uint8 newprio)
{
	for ( uint16 i = 0; i < GetFileCount(); i++ ) {
		CPartFile* file = GetFileByIndex( i );
		
		if ( !cat || file->GetCategory() == cat ) {
			if ( newprio == PR_AUTO ) {
				file->SetAutoDownPriority(true);
			} else {
				file->SetAutoDownPriority(false);
				file->SetDownPriority(newprio);
			}
		}
	}
}


void CDownloadQueue::SetCatStatus(uint8 cat, int newstatus)
{
	std::list<CPartFile*> files;

	{
		wxMutexLocker lock(m_mutex);
	
		for ( uint16 i = 0; i < m_filelist.size(); i++ ) {
			if ( m_filelist[i]->CheckShowItemInGivenCat(cat) ) {
				files.push_back( m_filelist[i] );
			}
		}
	}
	
	std::list<CPartFile*>::iterator it = files.begin();
		
	for ( ; it != files.end(); it++ ) {
		switch ( newstatus ) {
			case MP_CANCEL:	(*it)->Delete(); 	break;
			case MP_PAUSE:	(*it)->PauseFile();	break;
			case MP_STOP:	(*it)->StopFile();	break;
			case MP_RESUME:	(*it)->ResumeFile();	break;
		}
	}
}


uint16 CDownloadQueue::GetDownloadingFileCount() const
{
	wxMutexLocker lock( m_mutex );
	
	uint16 count = 0;
	for ( uint16 i = 0; i < m_filelist.size(); i++ ) {
		uint8 status = m_filelist[i]->GetStatus();
		if ( status == PS_READY || status == PS_EMPTY ) {
			count++;
		}
	}
	
	return count;
}


uint16 CDownloadQueue::GetPausedFileCount() const
{
	wxMutexLocker lock( m_mutex );

	uint16 count = 0;
	for ( uint16 i = 0; i < m_filelist.size(); i++ ) {
		if ( m_filelist[i]->GetStatus() == PS_PAUSED ) {
			count++;
		}
	}
	
	return count;
}


void CDownloadQueue::CheckDiskspace( const wxString& path )
{
	if ( ::GetTickCount() - m_lastDiskCheck < DISKSPACERECHECKTIME ) {
		return;
	}
	
	m_lastDiskCheck = ::GetTickCount();

	uint32 min = 0;
	// Check if the user has set an explicit limit
	if ( thePrefs::IsCheckDiskspaceEnabled() ) {
		min = thePrefs::GetMinFreeDiskSpace();
	}

	// The very least acceptable diskspace is a single PART
	if ( min < PARTSIZE ) {
		min = PARTSIZE;
	}

	// Get the current free disc-space
	wxLongLong free = 0;
	if ( !wxGetDiskSpace( path, NULL, &free ) ) {
		return;
	}

	
	for ( unsigned int i = 0; i < m_filelist.size(); i++ ) {
		CPartFile* file = m_filelist[i];
			
		switch ( file->GetStatus() ) {
			case PS_ERROR:
			case PS_COMPLETING:
			case PS_COMPLETE:
				continue;
		}
	
		if ( free >= min && file->GetInsufficient() ) {
			// We'll try to resume files if there is enough free space
			if ( free - file->GetNeededSpace() > min ) {
				file->ResumeFile();
			}
		} else if ( free < min && !file->IsPaused() ) {
			// No space left, stop the files.
			file->PauseFile( true );
		}
	}
}


int CDownloadQueue::GetMaxFilesPerUDPServerPacket() const
{
	if ( m_udpserver ) {
		if ( m_udpserver->GetUDPFlags() & SRV_UDPFLG_EXT_GETSOURCES ) {
			// get max. file ids per packet
			if ( m_cRequestsSentToServer < MAX_REQUESTS_PER_SERVER ) {
				return std::min(
					MAX_FILES_PER_UDP_PACKET,
					MAX_REQUESTS_PER_SERVER - m_cRequestsSentToServer
				);
			}
		} else if ( m_cRequestsSentToServer < MAX_REQUESTS_PER_SERVER ) {
			return 1;
		}
	}

	return 0;
}


bool CDownloadQueue::SendGlobGetSourcesUDPPacket(CMemFile& data)
{
	if (!m_udpserver) {
		return false;
	}
	
	int item_size;
	if (m_udpserver->GetUDPFlags() & SRV_UDPFLG_EXT_GETSOURCES2) {
		item_size = (16 + 4); // (hash + size)
	} else {
		item_size = 16;
	}
	
	int iFileIDs = data.GetLength() / item_size;
	
	CPacket packet(&data);

	if (item_size == 16) {
		packet.SetOpCode(OP_GLOBGETSOURCES);
	} else {
		packet.SetOpCode(OP_GLOBGETSOURCES2);
	}
	
	theStats::AddUpOverheadServer(packet.GetPacketSize());
	theApp.serverconnect->SendUDPPacket(&packet,m_udpserver,false);

	m_cRequestsSentToServer += iFileIDs;
	return true;
}


void CDownloadQueue::AddToResolve(const CMD4Hash& fileid, const wxString& pszHostname, uint16 port)
{
	// double checking
	if ( !GetFileByID(fileid) ) {
		return;
	}

	wxMutexLocker lock( m_mutex );
		
	Hostname_Entry entry = { fileid, pszHostname, port };
	m_toresolve.push_front(entry);

	// Check if there are other DNS lookups on queue
	if ( m_toresolve.size() > 1 ) {
		return;
	}
	
	CAsyncDNS* dns = new CAsyncDNS(pszHostname, DNS_SOURCE);
	
	if ( dns->Create() == wxTHREAD_NO_ERROR ) {
		if ( dns->Run() != wxTHREAD_NO_ERROR ) {
			dns->Delete();
			m_toresolve.pop_front();
		}
	} else {
		dns->Delete();
		m_toresolve.pop_front();
	}
}


void CDownloadQueue::OnHostnameResolved(uint32 ip)
{
	wxMutexLocker lock( m_mutex );

	wxASSERT( m_toresolve.size() );
	
	Hostname_Entry resolved = m_toresolve.front();
	m_toresolve.pop_front();

	if ( ip ) {
		CPartFile* file = GetFileByID( resolved.fileid );
		if ( file ) {
			CMemFile sources(1+4+2);
			sources.WriteUInt8(1); // No. Sources
			sources.WriteUInt32(ip);
			sources.WriteUInt16(resolved.port);
			sources.Seek(0,wxFromStart);
			
			file->AddSources(sources, 0, 0, SF_LINK);
		}
	}
	
	while ( !m_toresolve.empty() ) {
		Hostname_Entry entry = m_toresolve.front();
		
		CAsyncDNS* dns = new CAsyncDNS(entry.strHostname, DNS_SOURCE);
	
		if ( dns->Create() == wxTHREAD_NO_ERROR ) {
			if ( dns->Run() == wxTHREAD_NO_ERROR ) {
				break;
			} else {
				dns->Delete();
				m_toresolve.pop_front();
			}
		} else {
			dns->Delete();
			m_toresolve.pop_front();
		}
	}		
	
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
		CED2KLink* uri = CED2KLink::CreateLinkFromUrl(URI);
		bool result = AddED2KLink( uri, category );
		delete uri;
		
		return result;
	} catch ( const wxString& err ) {
		AddLogLineM( true, CFormat( _("Invalid ed2k link! Error: %s")) % err);
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
	
	CPartFile* file = new CPartFile( link );

	if ( file->GetStatus() == PS_ERROR ) {
		// That file already exists
		delete file;
		file = GetFileByID( link->GetHashKey() );
		if (!file) {
			// This is a shared file, return an error.
			return false;
		}
	} else {
		AddDownload( file, thePrefs::AddNewFilesPaused(), category );
	}

	// Add specified sources, specified by IP
	if ( link->HasValidSources() ) {
		file->AddClientSources( link->m_sources, 1, SF_LINK );
	}

	// Add specified sources, specified by hostname
	if ( link->HasHostnameSources() ) {
		const std::deque<SUnresolvedHostname*>& list = link->m_hostSources;

		for ( std::deque<SUnresolvedHostname*>::const_iterator it = list.begin(); it != list.end(); ++it ) {
			AddToResolve( link->GetHashKey(), (*it)->strHostname, (*it)->nPort );
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


bool CDownloadQueue::AddED2KLink( const CED2KServerListLink* link )
{
	theApp.serverlist->UpdateServerMetFromURL( link->GetAddress() );

	return true;
}


void CDownloadQueue::ObserverAdded( ObserverType* o )
{
	CObservableQueue<CPartFile*>::ObserverAdded( o );
	
	EventType::ValueList list;

	{
		wxMutexLocker lock(m_mutex);
		list.reserve( m_filelist.size() );
		list.insert( list.begin(), m_filelist.begin(), m_filelist.end() );
	}

	NotifyObservers( EventType( EventType::INITIAL, &list ), o );
}

void CDownloadQueue::KademliaSearchFile(uint32 searchID, const Kademlia::CUInt128* pcontactID, const Kademlia::CUInt128* pbuddyID, uint8 type, uint32 ip, uint16 tcp, uint16 udp, uint32 serverip, uint16 serverport, uint32 clientid)
{
	#ifdef __COMPILE_KAD__
	
	AddDebugLogLineM(false, logKadSearch, wxString::Format(wxT("Search result sources (type %i)"),type));
	
	//Safty measure to make sure we are looking for these sources
	CPartFile* temp = GetFileByKadFileSearchID(searchID);
	if( !temp ) {
		AddDebugLogLineM(false, logKadSearch, wxT("This is not the file we're looking for..."));
		return;
	}
	
	//Do we need more sources?
	if(!(!temp->IsStopped() && thePrefs::GetMaxSourcePerFile() > temp->GetSourceCount())) {
		AddDebugLogLineM(false, logKadSearch, wxT("No more sources needed for this file"));
		return;
	}

	uint32 ED2Kip = wxUINT32_SWAP_ALWAYS(ip);
	
	if (theApp.ipfilter->IsFiltered(ED2Kip)) {
		AddDebugLogLineM(false, logKadSearch, wxT("Source ip got filtered"));
		AddDebugLogLineM(false, logIPFilter, CFormat(wxT("IPfiltered source IP=%s received from Kademlia")) % Uint32toStringIP(ED2Kip));
		return;
	}
	
	if( (ip == Kademlia::CKademlia::getIPAddress() || ED2Kip == theApp.serverconnect->GetClientID()) && tcp == thePrefs::GetPort()) {
		AddDebugLogLineM(false, logKadSearch, wxT("Trying to add myself as source, ignore"));
		return;
	}
	
	CUpDownClient* ctemp = NULL; 
	switch( type ) {
		case 1: {
			//NonFirewalled users
			if(!tcp) {
				AddDebugLogLineM(false, logKadSearch, CFormat(wxT("Ignored source (IP=%s) received from Kademlia, no tcp port received")) % Uint32toStringIP(ip));
				return;
			}
			if (!IsGoodIP(ED2Kip,thePrefs::FilterLanIPs())) {
				AddDebugLogLineM(false, logKadSearch, CFormat(wxT("%s got filtered")) % Uint32toStringIP(ED2Kip));
				AddDebugLogLineM(false, logIPFilter, CFormat(wxT("Ignored source (IP=%s) received from Kademlia, filtered")) % Uint32toStringIP(ED2Kip));
				return;
			}
			ctemp = new CUpDownClient(tcp,ip,0,0,temp,false, true);
			ctemp->SetSourceFrom(SF_KADEMLIA);
			ctemp->SetServerIP(serverip);
			ctemp->SetServerPort(serverport);
			ctemp->SetKadPort(udp);
			byte cID[16];
			pcontactID->toByteArray(cID);
			ctemp->SetUserHash(cID);
			break;
		}
		case 2: {
			//Don't use this type... Some clients will process it wrong..
			break;
		}
		case 3: {
			//This will be a firewaled client connected to Kad only.
			//We set the clientID to 1 as a Kad user only has 1 buddy.
			ctemp = new CUpDownClient(tcp,1,0,0,temp,false, true);
			//The only reason we set the real IP is for when we get a callback
			//from this firewalled source, the compare method will match them.
			ctemp->SetSourceFrom(SF_KADEMLIA);
			ctemp->SetKadPort(udp);
			byte cID[16];
			pcontactID->toByteArray(cID);
			ctemp->SetUserHash(cID);
			pbuddyID->toByteArray(cID);
			ctemp->SetBuddyID(cID);
			ctemp->SetBuddyIP(serverip);
			ctemp->SetBuddyPort(serverport);
			break;
		}
	}

	if (ctemp) {
		AddDebugLogLineM(false, logKadSearch, CFormat(wxT("Happily adding a source (%s) type %d")) % Uint32_16toStringIP_Port(ip, ctemp->GetUserPort()) % type);
		CheckAndAddSource(temp, ctemp);
	}
	#endif
}

CPartFile* CDownloadQueue::GetFileByKadFileSearchID(uint32 id) const
{
	
	wxMutexLocker lock( m_mutex );
	
	for ( uint16 i = 0; i < m_filelist.size(); ++i ) {
		if ( id == m_filelist[i]->GetKadFileSearchID()) {
			return m_filelist[ i ];
		}
	}
	
	return NULL;
	
}

bool CDownloadQueue::DoKademliaFileRequest()
{
	return ((::GetTickCount() - lastkademliafilerequest) > KADEMLIAASKTIME);
}
