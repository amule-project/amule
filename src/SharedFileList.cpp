//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "SharedFileList.h"	// Interface declarations  // Do_not_auto_remove

#include <protocol/Protocols.h>
#include <protocol/kad/Constants.h>
#include <tags/FileTags.h>

#include <wx/utils.h>

#include "Packet.h"		// Needed for CPacket
#include "MemFile.h"		// Needed for CMemFile
#include "ServerConnect.h"	// Needed for CServerConnect
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "ThreadTasks.h"	// Needed for CThreadScheduler and CHasherTask
#include "Preferences.h"	// Needed for thePrefs
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amule.h"		// Needed for theApp
#include "PartFile.h"		// Needed for PartFile
#include "Server.h"		// Needed for CServer
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"
#include <common/Format.h>
#include <common/FileFunctions.h>
#include "GuiEvents.h"		// Needed for Notify_*
#include "SHAHashSet.h"		// Needed for CAICHHash


#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Search.h"
#include "ClientList.h"

typedef std::deque<CKnownFile*> KnownFileArray;

///////////////////////////////////////////////////////////////////////////////
// CPublishKeyword

class CPublishKeyword
{
public:
	CPublishKeyword(const wxString& rstrKeyword)
	{
		m_strKeyword = rstrKeyword;
		// min. keyword char is allowed to be < 3 in some cases (see also 'CSearchManager::getWords')
		//ASSERT( rstrKeyword.GetLength() >= 3 );
		wxASSERT( !rstrKeyword.IsEmpty() );
		KadGetKeywordHash(rstrKeyword, &m_nKadID);
		SetNextPublishTime(0);
		SetPublishedCount(0);
	}

	const Kademlia::CUInt128& GetKadID() const { return m_nKadID; }
	const wxString& GetKeyword() const { return m_strKeyword; }
	int GetRefCount() const { return m_aFiles.size(); }
	const KnownFileArray& GetReferences() const { return m_aFiles; }

	uint32 GetNextPublishTime() const { return m_tNextPublishTime; }
	void SetNextPublishTime(uint32 tNextPublishTime) { m_tNextPublishTime = tNextPublishTime; }

	uint32 GetPublishedCount() const { return m_uPublishedCount; }
	void SetPublishedCount(uint32 uPublishedCount) { m_uPublishedCount = uPublishedCount; }
	void IncPublishedCount() { m_uPublishedCount++; }

	bool AddRef(CKnownFile* pFile) {
		if (std::find(m_aFiles.begin(), m_aFiles.end(), pFile) != m_aFiles.end()) {
			wxFAIL;
			return false;
		}
		m_aFiles.push_back(pFile);
		return true;
	}

	int RemoveRef(CKnownFile* pFile) {
		KnownFileArray::iterator it = std::find(m_aFiles.begin(), m_aFiles.end(), pFile);
		if (it != m_aFiles.end()) {
			m_aFiles.erase(it);
		}
		return m_aFiles.size();
	}

	void RemoveAllReferences() {
		m_aFiles.clear();
	}

	void RotateReferences(unsigned iRotateSize) {
		wxCHECK_RET(m_aFiles.size(), wxT("RotateReferences: Rotating empty array"));
		
		unsigned shift = (iRotateSize % m_aFiles.size());
		std::rotate(m_aFiles.begin(), m_aFiles.begin() + shift, m_aFiles.end());
	}

protected:
	wxString m_strKeyword;
	Kademlia::CUInt128 m_nKadID;
	uint32 m_tNextPublishTime;
	uint32 m_uPublishedCount;
	KnownFileArray m_aFiles;
};


///////////////////////////////////////////////////////////////////////////////
// CPublishKeywordList

class CPublishKeywordList
{
public:
	CPublishKeywordList();
	~CPublishKeywordList();

	void AddKeyword(const wxString& keyword, CKnownFile *file);
	void AddKeywords(CKnownFile* pFile);
	void RemoveKeyword(const wxString& keyword, CKnownFile *file);
	void RemoveKeywords(CKnownFile* pFile);
	void RemoveAllKeywords();

	void RemoveAllKeywordReferences();
	void PurgeUnreferencedKeywords();

	int GetCount() const { return m_lstKeywords.size(); }

	CPublishKeyword* GetNextKeyword();
	void ResetNextKeyword();

	uint32 GetNextPublishTime() const { return m_tNextPublishKeywordTime; }
	void SetNextPublishTime(uint32 tNextPublishKeywordTime) { m_tNextPublishKeywordTime = tNextPublishKeywordTime; }

protected:
	// can't use a CMap - too many disadvantages in processing the 'list'
	//CTypedPtrMap<CMapStringToPtr, CString, CPublishKeyword*> m_lstKeywords;
	typedef std::list<CPublishKeyword*> CKeyWordList;
	CKeyWordList m_lstKeywords;
	CKeyWordList::iterator m_posNextKeyword;
	uint32 m_tNextPublishKeywordTime;

	CPublishKeyword* FindKeyword(const wxString& rstrKeyword, CKeyWordList::iterator* ppos = NULL);
};

CPublishKeywordList::CPublishKeywordList()
{
	ResetNextKeyword();
	SetNextPublishTime(0);
}

CPublishKeywordList::~CPublishKeywordList()
{
	RemoveAllKeywords();
}

CPublishKeyword* CPublishKeywordList::GetNextKeyword()
{
	if (m_posNextKeyword == m_lstKeywords.end()) {
		m_posNextKeyword = m_lstKeywords.begin();
		if (m_posNextKeyword == m_lstKeywords.end()) {
			return NULL;
		}
	}
	return *m_posNextKeyword++;
}

void CPublishKeywordList::ResetNextKeyword()
{
	m_posNextKeyword = m_lstKeywords.begin();
}

CPublishKeyword* CPublishKeywordList::FindKeyword(const wxString& rstrKeyword, CKeyWordList::iterator* ppos)
{
	CKeyWordList::iterator it = m_lstKeywords.begin();
	for (; it != m_lstKeywords.end(); ++it) {
		CPublishKeyword* pPubKw = *it;
		if (pPubKw->GetKeyword() == rstrKeyword) {
			if (ppos) {
				(*ppos) = it;
			}

			return pPubKw;
		}
	}
	
	return NULL;
}

void CPublishKeywordList::AddKeyword(const wxString& keyword, CKnownFile *file)
{
	CPublishKeyword* pubKw = FindKeyword(keyword);
	if (pubKw == NULL) {
		pubKw = new CPublishKeyword(keyword);
		m_lstKeywords.push_back(pubKw);
		SetNextPublishTime(0);
	}
	pubKw->AddRef(file);
}

void CPublishKeywordList::AddKeywords(CKnownFile* pFile)
{
	const Kademlia::WordList& wordlist = pFile->GetKadKeywords();

	Kademlia::WordList::const_iterator it;
	for (it = wordlist.begin(); it != wordlist.end(); ++it) {
		AddKeyword(*it, pFile);
	}
}

void CPublishKeywordList::RemoveKeyword(const wxString& keyword, CKnownFile *file)
{
	CKeyWordList::iterator pos;
	CPublishKeyword* pubKw = FindKeyword(keyword, &pos);
	if (pubKw != NULL) {
		if (pubKw->RemoveRef(file) == 0) {
			if (pos == m_posNextKeyword) {
				++m_posNextKeyword;
			}
			m_lstKeywords.erase(pos);
			delete pubKw;
			SetNextPublishTime(0);
		}
	}
}

void CPublishKeywordList::RemoveKeywords(CKnownFile* pFile)
{
	const Kademlia::WordList& wordlist = pFile->GetKadKeywords();
	Kademlia::WordList::const_iterator it;
	for (it = wordlist.begin(); it != wordlist.end(); ++it) {
		RemoveKeyword(*it, pFile);
	}
}


void CPublishKeywordList::RemoveAllKeywords()
{
	DeleteContents(m_lstKeywords);
	ResetNextKeyword();
	SetNextPublishTime(0);
}


void CPublishKeywordList::RemoveAllKeywordReferences()
{
	CKeyWordList::iterator it = m_lstKeywords.begin();
	for (; it != m_lstKeywords.end(); ++it) {
		(*it)->RemoveAllReferences();
	}
}


void CPublishKeywordList::PurgeUnreferencedKeywords()
{
	CKeyWordList::iterator it = m_lstKeywords.begin();
	while (it != m_lstKeywords.end()) {
		CPublishKeyword* pPubKw = *it;
		if (pPubKw->GetRefCount() == 0) {
			if (it == m_posNextKeyword) {
				++m_posNextKeyword;
			}
			m_lstKeywords.erase(it++);
			delete pPubKw;
			SetNextPublishTime(0);
		} else {
			++it;
		}
	}
}


CSharedFileList::CSharedFileList(CKnownFileList* in_filelist){
	filelist = in_filelist;
	reloading = false;
	m_lastPublishED2K = 0;
	m_lastPublishED2KFlag = true;
	/* Kad Stuff */
	m_keywords = new CPublishKeywordList;
	m_currFileSrc = 0;
	m_currFileNotes = 0;
	m_lastPublishKadSrc = 0;
	m_lastPublishKadNotes = 0;
	m_currFileKey = 0;
}


CSharedFileList::~CSharedFileList()
{
	delete m_keywords;
}


void CSharedFileList::FindSharedFiles() 
{
	/* Abort loading if we are shutting down. */
	if(theApp->IsOnShutDown()) {
		return;
	}

	// Clear statistics.
	theStats::ClearSharedFilesInfo();

	// Reload shareddir.dat
	theApp->glob_prefs->ReloadSharedFolders();

	{
		wxMutexLocker lock(list_mut);
		m_Files_map.clear();
	}

	// All part files are automatically shared.
	for ( uint32 i = 0; i < theApp->downloadqueue->GetFileCount(); ++i ) {
		CPartFile* file = theApp->downloadqueue->GetFileByIndex( i );
		
		if ( file->GetStatus(true) == PS_READY ) {
			AddLogLineNS(CFormat(_("Adding file %s to shares"))
				% file->GetFullName().GetPrintable());
			AddFile(file);
		}
	}
	
	// Create a list of all shared paths and weed out duplicates.
	std::list<CPath> sharedPaths;
	
	// Global incoming dir and all category incoming directories are automatically shared.
	sharedPaths.push_back(thePrefs::GetIncomingDir());
	for (unsigned int i = 1;i < theApp->glob_prefs->GetCatCount(); ++i) {
		sharedPaths.push_back(theApp->glob_prefs->GetCatPath(i));
	}

	const thePrefs::PathList& shared = theApp->glob_prefs->shareddir_list;
	sharedPaths.insert(sharedPaths.end(), shared.begin(), shared.end());

	sharedPaths.sort();
	sharedPaths.unique();

	filelist->PrepareIndex();
	unsigned addedFiles = 0;
	std::list<CPath>::iterator it = sharedPaths.begin();
	for (; it != sharedPaths.end(); ++it) {
		addedFiles += AddFilesFromDirectory(*it);
	}
	filelist->ReleaseIndex();
	
	if (addedFiles == 0) {
		AddLogLineN(CFormat(wxPLURAL("Found %i known shared file", "Found %i known shared files", GetCount())) % GetCount());

		// Make sure the AICH-hashes are up to date.
		CThreadScheduler::AddTask(new CAICHSyncTask());
	} else {	
		// New files, AICH thread will be run at the end of the hashing thread.
		AddLogLineN(CFormat(wxPLURAL("Found %i known shared file, %i unknown", "Found %i known shared files, %i unknown", GetCount())) % GetCount() % addedFiles);
	}
}


// Checks if the dir a is the same as b. If they are, then logs the message and returns true.
bool CheckDirectory(const wxString& a, const CPath& b)
{
	if (CPath(a).IsSameDir(b)) {
		AddLogLineC(CFormat( _("ERROR: Attempted to share %s") ) % a);

		return true;
	}

	return false;
}
		

unsigned CSharedFileList::AddFilesFromDirectory(const CPath& directory)
{
	// Do not allow these folders to be shared:
	//  - The .aMule folder
	//  - The Temp folder
	//  - The users home-dir
	if (CheckDirectory(wxGetHomeDir(), directory)) {
		return 0;
	} else if (CheckDirectory(theApp->ConfigDir, directory)) {
		return 0;
	} else if (CheckDirectory(thePrefs::GetTempDir().GetRaw(), directory)) {
		return 0;
	}

	if (!directory.DirExists()) {
		AddLogLineNS(CFormat(_("Shared directory not found, skipping: %s"))
			% directory.GetPrintable());
		
		return 0;
	}
	
	CDirIterator::FileType searchFor = CDirIterator::FileNoHidden;
	if (thePrefs::ShareHiddenFiles()) {
		 searchFor = CDirIterator::File;
	}

	unsigned knownFiles = 0;
	unsigned addedFiles = 0;

	CDirIterator SharedDir(directory); 

	CPath fname = SharedDir.GetFirstFile(searchFor);
	while (fname.IsOk()) {
		CPath fullPath = directory.JoinPaths(fname);
	
		if (!fullPath.FileExists()) {
			AddDebugLogLineN(logKnownFiles,
				CFormat(wxT("Shared file does not exist (possibly a broken link): %s")) % fullPath);
			
			fname = SharedDir.GetNextFile();
			continue;
		}

		AddDebugLogLineN(logKnownFiles,
			CFormat(wxT("Found shared file: %s")) % fullPath);

		time_t fdate = CPath::GetModificationTime(fullPath);
		sint64 fsize = fullPath.GetFileSize();

		// This will also catch files with too strict permissions.
		if ((fdate == (time_t)-1) || (fsize == wxInvalidOffset)) {
			AddDebugLogLineN(logKnownFiles,
				CFormat(wxT("Failed to retrive modification time or size for '%s', skipping.")) % fullPath);
			
			fname = SharedDir.GetNextFile();
			continue;
		}


		CKnownFile* toadd = filelist->FindKnownFile(fname, fdate, fsize);
		if (toadd) {
			knownFiles++;
			if (AddFile(toadd)) {
				AddDebugLogLineN(logKnownFiles,
					CFormat(wxT("Added known file '%s' to shares"))
						% fname);

				toadd->SetFilePath(directory);
			} else {
				AddDebugLogLineN(logKnownFiles,
					CFormat(wxT("File already shared, skipping: %s"))
						% fname);
			}
		} else {
			//not in knownfilelist - start adding thread to hash file
			AddDebugLogLineN(logKnownFiles,
				CFormat(wxT("Hashing new unknown shared file '%s'")) % fname);
			
			if (CThreadScheduler::AddTask(new CHashingTask(directory, fname))) {
				addedFiles++;
			}
		}

		fname = SharedDir.GetNextFile();
	}

	if ((addedFiles == 0) && (knownFiles == 0)) {
		AddLogLineNS(CFormat(_("No shareable files found in directory: %s"))
			% directory.GetPrintable());
	}

	return addedFiles;
}


bool CSharedFileList::AddFile(CKnownFile* pFile)
{
	wxASSERT(pFile->GetHashCount() == pFile->GetED2KPartHashCount());
	
	wxMutexLocker lock(list_mut);

	CKnownFileMap::value_type entry(pFile->GetFileHash(), pFile);
	if (m_Files_map.insert(entry).second) {
		/* Keywords to publish on Kad */
		m_keywords->AddKeywords(pFile);
		theStats::AddSharedFile(pFile->GetFileSize());
		return true;
	}
	return false;
}


void CSharedFileList::SafeAddKFile(CKnownFile* toadd, bool bOnlyAdd)
{
	// TODO: Check if the file is already known - only with another date
	
	if (AddFile(toadd)) {
		Notify_SharedFilesShowFile(toadd);
	}
	
	if (!bOnlyAdd && theApp->IsConnectedED2K()) {		
		// Publishing of files is not anymore handled here.
		// Instead, the timer does it by itself.
		m_lastPublishED2KFlag = true;
	}
}


// removes first occurrence of 'toremove' in 'list'
void CSharedFileList::RemoveFile(CKnownFile* toremove){
	Notify_SharedFilesRemoveFile(toremove);
	wxMutexLocker lock(list_mut);
	if (m_Files_map.erase(toremove->GetFileHash()) > 0) {
		theStats::RemoveSharedFile(toremove->GetFileSize());
	}
	/* This file keywords must not be published to kad anymore */
	m_keywords->RemoveKeywords(toremove);
}


void CSharedFileList::Reload()
{
	// Madcat - Disable reloading if reloading already in progress.
	// Kry - Fixed to let non-english language users use the 'Reload' button :P
	// deltaHF - removed the old ugly button and changed the code to use the new small one
	// Kry - bah, let's use a var. 
	if (!reloading) {
		reloading = true;
		Notify_SharedFilesRemoveAllItems();
		
		/* All Kad keywords must be removed */
		m_keywords->RemoveAllKeywordReferences();

		/* Public identifiers must be erased as they might be invalid now */
		m_PublicSharedDirNames.clear();

		FindSharedFiles();
		
		/* And now the unreferenced keywords must be removed also */
		m_keywords->PurgeUnreferencedKeywords();
		
		Notify_SharedFilesShowFileList();
	
		reloading = false;
	}
}


const CKnownFile *CSharedFileList::GetFileByIndex(unsigned int index) const
{
	wxMutexLocker lock(list_mut);
	if ( index >= m_Files_map.size() ) {
		return NULL;
	}
	CKnownFileMap::const_iterator pos = m_Files_map.begin();
	std::advance(pos, index);
	return pos->second;
}


CKnownFile*	CSharedFileList::GetFileByID(const CMD4Hash& filehash)
{
	wxMutexLocker lock(list_mut);
	CKnownFileMap::iterator it = m_Files_map.find(filehash);
	
	if ( it != m_Files_map.end() ) {
		return it->second;
	} else {
		return NULL;
	}
}

short CSharedFileList::GetFilePriorityByID(const CMD4Hash& filehash)
{
	CKnownFile* tocheck = GetFileByID(filehash);
	if (tocheck)
		return tocheck->GetUpPriority();
	else
		return -10;	// file doesn't exist
}


void CSharedFileList::CopyFileList(std::vector<CKnownFile*>& out_list) const
{
	wxMutexLocker lock(list_mut);

	out_list.reserve(m_Files_map.size());
	for (
		CKnownFileMap::const_iterator it = m_Files_map.begin();
		it != m_Files_map.end();
		++it
		) {
		out_list.push_back(it->second);
	}
}



void CSharedFileList::UpdateItem(CKnownFile* toupdate)
{
	Notify_SharedFilesUpdateItem(toupdate);
}


void CSharedFileList::GetSharedFilesByDirectory(const wxString& directory,
				CKnownFilePtrList& list)
{
	wxMutexLocker lock(list_mut);

	const CPath dir = CPath(directory);
	for (CKnownFileMap::iterator pos = m_Files_map.begin();
	     pos != m_Files_map.end(); ++pos ) {
		CKnownFile *cur_file = pos->second;

		if (dir.IsSameDir(cur_file->GetFilePath())) {
			continue;
		}

		list.push_back(cur_file);
	}
}

/* ---------------- Network ----------------- */

void CSharedFileList::ClearED2KPublishInfo(){
	CKnownFile* cur_file;
	m_lastPublishED2KFlag = true;
	wxMutexLocker lock(list_mut);
	for (CKnownFileMap::iterator pos = m_Files_map.begin(); pos != m_Files_map.end(); ++pos ) {
		cur_file = pos->second;
		cur_file->SetPublishedED2K(false);
	}
}

void CSharedFileList::ClearKadSourcePublishInfo()
{
	wxMutexLocker lock(list_mut);
	CKnownFile* cur_file;
	for (CKnownFileMap::iterator pos = m_Files_map.begin(); pos != m_Files_map.end(); ++pos ) {
		cur_file = pos->second;
		cur_file->SetLastPublishTimeKadSrc(0,0);
	}
}

void CSharedFileList::RepublishFile(CKnownFile* pFile)
{
	CServer* server = theApp->serverconnect->GetCurrentServer();
	if (server && (server->GetTCPFlags() & SRV_TCPFLG_COMPRESSION)) {
		m_lastPublishED2KFlag = true;
		pFile->SetPublishedED2K(false); // FIXME: this creates a wrong 'No' for the ed2k shared info in the listview until the file is shared again.
	}
}

uint8 GetRealPrio(uint8 in)
{
	switch(in) {
		case 4 : return 0;
		case 0 : return 1;
		case 1 : return 2;
		case 2 : return 3;
		case 3 : return 4;
	}
	return 0;
}

bool SortFunc( const CKnownFile* fileA, const CKnownFile* fileB )
{
    return GetRealPrio(fileA->GetUpPriority()) < GetRealPrio(fileB->GetUpPriority());
}

void CSharedFileList::SendListToServer(){
	std::vector<CKnownFile*> SortedList;

	{	
		wxMutexLocker lock(list_mut);

		if (m_Files_map.empty() || !theApp->IsConnectedED2K() ) {
			return;
		}

		// Getting a sorted list of the non-published files.
		SortedList.reserve( m_Files_map.size() );

		CKnownFileMap::iterator it = m_Files_map.begin();
		for ( ; it != m_Files_map.end(); ++it ) {
			if (!it->second->GetPublishedED2K()) {
				SortedList.push_back( it->second );
			}
		}
	}

	std::sort( SortedList.begin(), SortedList.end(), SortFunc ); 
	
	// Limits for the server. 
	
	CServer* server = theApp->serverconnect->GetCurrentServer();	
	
	uint32 limit = server ? server->GetSoftFiles() : 0;
	if( limit == 0 || limit > 200 ) {
		limit = 200;
	}
	
	if( (uint32)SortedList.size() < limit ) {
		limit = SortedList.size();
		if (limit == 0) {
			m_lastPublishED2KFlag = false;
			return;
		}
	}

	CMemFile files;
	
	// Files sent.
	files.WriteUInt32(limit);	
	
	uint16 count = 0;	
	// Add to packet
	std::vector<CKnownFile*>::iterator sorted_it = SortedList.begin();
	for ( ; (sorted_it != SortedList.end()) && (count < limit); ++sorted_it ) {
		CKnownFile* file = *sorted_it;
		if (!file->IsLargeFile() || (server && server->SupportsLargeFilesTCP())) {
			file->CreateOfferedFilePacket(&files, server, NULL);
		}
		file->SetPublishedED2K(true);	
		++count;
	}
	
	wxASSERT(count == limit);
	
	CPacket* packet = new CPacket(files, OP_EDONKEYPROT, OP_OFFERFILES);
	// compress packet
	//   - this kind of data is highly compressable (N * (1 MD4 and at least 3 string meta data tags and 1 integer meta data tag))
	//   - the min. amount of data needed for one published file is ~100 bytes
	//   - this function is called once when connecting to a server and when a file becomes shareable - so, it's called rarely.
	//   - if the compressed size is still >= the original size, we send the uncompressed packet
	// therefor we always try to compress the packet
	if (server->GetTCPFlags() & SRV_TCPFLG_COMPRESSION){
		packet->PackPacket();
	}

	theStats::AddUpOverheadServer(packet->GetPacketSize());
	theApp->serverconnect->SendPacket(packet,true);
}


void CSharedFileList::Process()
{
	Publish();
	if( !m_lastPublishED2KFlag || ( ::GetTickCount() - m_lastPublishED2K < ED2KREPUBLISHTIME ) ) {
		return;
	}
	SendListToServer();
	m_lastPublishED2K = ::GetTickCount();
}

void CSharedFileList::Publish()
{
	// Variables to save cpu.
	unsigned int tNow = time(NULL);
	bool IsFirewalled = theApp->IsFirewalled();

	if( Kademlia::CKademlia::IsConnected() && ( !IsFirewalled || ( IsFirewalled && theApp->clientlist->GetBuddyStatus() == Connected)) && GetCount() && Kademlia::CKademlia::GetPublish()) { 
		//We are connected to Kad. We are either open or have a buddy. And Kad is ready to start publishing.

		if( Kademlia::CKademlia::GetTotalStoreKey() < KADEMLIATOTALSTOREKEY) {

			//We are not at the max simultaneous keyword publishes 
			if (tNow >= m_keywords->GetNextPublishTime()) {

				//Enough time has passed since last keyword publish

				//Get the next keyword which has to be (re)-published
				CPublishKeyword* pPubKw = m_keywords->GetNextKeyword();
				if (pPubKw) {

					//We have the next keyword to check if it can be published

					//Debug check to make sure things are going well.
					wxASSERT( pPubKw->GetRefCount() != 0 );

					if (tNow >= pPubKw->GetNextPublishTime()) {
						//This keyword can be published.
						Kademlia::CSearch* pSearch = Kademlia::CSearchManager::PrepareLookup(Kademlia::CSearch::STOREKEYWORD, false, pPubKw->GetKadID());
						if (pSearch) {
							//pSearch was created. Which means no search was already being done with this HashID.
							//This also means that it was checked to see if network load wasn't a factor.

							//This sets the filename into the search object so we can show it in the gui.
							pSearch->SetFileName(pPubKw->GetKeyword());

							//Add all file IDs which relate to the current keyword to be published
							const KnownFileArray& aFiles = pPubKw->GetReferences();
							uint32 count = 0;
							for (unsigned int f = 0; f < aFiles.size(); ++f) {

								//Only publish complete files as someone else should have the full file to publish these keywords.
								//As a side effect, this may help reduce people finding incomplete files in the network.
								if( !aFiles[f]->IsPartFile() ) {
									count++;
									pSearch->AddFileID(Kademlia::CUInt128(aFiles[f]->GetFileHash().GetHash()));
									if( count > 150 ) {
										//We only publish up to 150 files per keyword publish then rotate the list.
										pPubKw->RotateReferences(f);
										break;
									}
								}
							}

							if( count ) {
								//Start our keyword publish
								pPubKw->SetNextPublishTime(tNow+(KADEMLIAREPUBLISHTIMEK));
								pPubKw->IncPublishedCount();
								Kademlia::CSearchManager::StartSearch(pSearch);
							} else {
								//There were no valid files to publish with this keyword.
								delete pSearch;
							}
						}
					}
				}
				m_keywords->SetNextPublishTime(KADEMLIAPUBLISHTIME+tNow);
			}
		}
		
		if( Kademlia::CKademlia::GetTotalStoreSrc() < KADEMLIATOTALSTORESRC) {
			if(tNow >= m_lastPublishKadSrc) {
				if(m_currFileSrc > GetCount()) {
					m_currFileSrc = 0;
				}
				CKnownFile* pCurKnownFile = const_cast<CKnownFile*>(GetFileByIndex(m_currFileSrc));
				if(pCurKnownFile) {
					if(pCurKnownFile->PublishSrc()) {
						Kademlia::CUInt128 kadFileID;
						kadFileID.SetValueBE(pCurKnownFile->GetFileHash().GetHash());
						if(Kademlia::CSearchManager::PrepareLookup(Kademlia::CSearch::STOREFILE, true, kadFileID )==NULL) {
							pCurKnownFile->SetLastPublishTimeKadSrc(0,0);
						}
					}	
				}
				m_currFileSrc++;

				// even if we did not publish a source, reset the timer so that this list is processed
				// only every KADEMLIAPUBLISHTIME seconds.
				m_lastPublishKadSrc = KADEMLIAPUBLISHTIME+tNow;
			}
		}

		if( Kademlia::CKademlia::GetTotalStoreNotes() < KADEMLIATOTALSTORENOTES) {
			if(tNow >= m_lastPublishKadNotes) {
				if(m_currFileNotes > GetCount()) {
					m_currFileNotes = 0;
				}
				CKnownFile* pCurKnownFile = const_cast<CKnownFile*>(GetFileByIndex(m_currFileNotes));
				if(pCurKnownFile) {
					if(pCurKnownFile->PublishNotes()) {
						Kademlia::CUInt128 kadFileID;
						kadFileID.SetValueBE(pCurKnownFile->GetFileHash().GetHash());
						if(Kademlia::CSearchManager::PrepareLookup(Kademlia::CSearch::STORENOTES, true, kadFileID )==NULL)
							pCurKnownFile->SetLastPublishTimeKadNotes(0);
					}	
				}
				m_currFileNotes++;

				// even if we did not publish a source, reset the timer so that this list is processed
				// only every KADEMLIAPUBLISHTIME seconds.
				m_lastPublishKadNotes = KADEMLIAPUBLISHTIME+tNow;
			}
		}
	}
}


void CSharedFileList::AddKeywords(CKnownFile* pFile)
{
	m_keywords->AddKeywords(pFile);
}


void CSharedFileList::RemoveKeywords(CKnownFile* pFile)
{
	m_keywords->RemoveKeywords(pFile);
}


bool CSharedFileList::RenameFile(CKnownFile* file, const CPath& newName)
{
	if (file->IsPartFile()) {
		CPartFile* pfile = dynamic_cast<CPartFile*>(file);
		
		if (file->GetStatus() != PS_COMPLETING) {
			pfile->SetFileName(newName);
			pfile->SavePartFile();
	
			Notify_SharedFilesUpdateItem(file);
			Notify_DownloadCtrlUpdateItem(file);
			
			return true;
		}
	} else {
		CPath oldPath = file->GetFilePath().JoinPaths(file->GetFileName());
		CPath newPath = file->GetFilePath().JoinPaths(newName);

		if (CPath::RenameFile(oldPath, newPath)) {
			// Must create a copy of the word list because:
			// 1) it will be reset on SetFileName()
			// 2) we will want to edit it
			Kademlia::WordList oldwords = file->GetKadKeywords();
			file->SetFileName(newName);
			theApp->knownfiles->Save();
			UpdateItem(file);
			RepublishFile(file);

			const Kademlia::WordList& newwords = file->GetKadKeywords();
			Kademlia::WordList::iterator itold;
			Kademlia::WordList::const_iterator itnew;
			// compare keywords in old and new names
			for (itnew = newwords.begin(); itnew != newwords.end(); ++itnew) {
				for (itold = oldwords.begin(); itold != oldwords.end(); ++itold) {
					if (*itold == *itnew) {
						break;
					}
				}
				if (itold != oldwords.end()) {
					// Remove keyword from old name which also exist in new name
					oldwords.erase(itold);
				} else {
					// This is a new keyword not present in the old name
					m_keywords->AddKeyword(*itnew, file);
				}
			}
			// Remove all remaining old keywords not present in the new name
			for (itold = oldwords.begin(); itold != oldwords.end(); ++itold) {
				m_keywords->RemoveKeyword(*itold, file);
			}

			Notify_DownloadCtrlUpdateItem(file);
			Notify_SharedFilesUpdateItem(file);
			
			return true;
		}
	}
	
	return false;
}


const CPath* CSharedFileList::GetDirForPublicSharedDirName(const wxString& strSharedDir) const
{
	StringPathMap::const_iterator it = m_PublicSharedDirNames.find(strSharedDir);

	if (it != m_PublicSharedDirNames.end()) {
		return &(it->second);
	} else {
		return NULL;
	}
}


wxString CSharedFileList::GetPublicSharedDirName(const CPath& dir)
{
	// safety check: is the directory supposed to be shared after all?
	if (!IsShared(dir))	{
		wxFAIL;
		return wxT("");
	}
	// check if the public name for the directory is cached in our Map
	StringPathMap::const_iterator it;
	for (it = m_PublicSharedDirNames.begin(); it != m_PublicSharedDirNames.end(); ++it) {
		if (it->second.IsSameDir(dir)) {
			// public name for directory was determined earlier
			return it->first;
		}
	}

	// we store the path separator (forward or back slash) for quick access
	wxChar cPathSepa = wxFileName::GetPathSeparator();

	// determine and cache the public name for "dir" ...
	// We need to use the 'raw' filename, so the receiving client can recognize it.
	wxString strDirectoryTmp = dir.GetRaw();
	if (strDirectoryTmp.EndsWith(&cPathSepa)) {
		strDirectoryTmp.RemoveLast();
	}
	
	wxString strPublicName;
	int iPos;
	// check all the subdirectories in the path for being shared
	// the public name will consist of these concatenated
	while ((iPos = strDirectoryTmp.Find( cPathSepa, true )) != wxNOT_FOUND)	{
		strPublicName = strDirectoryTmp.Right(strDirectoryTmp.Length() - iPos) + strPublicName;
		strDirectoryTmp.Truncate(iPos);
		if (!IsShared(CPath(strDirectoryTmp)))
			break;
	}
	if (!strPublicName.IsEmpty()) {
		// remove first path separator ???
		wxASSERT( strPublicName.GetChar(0) == cPathSepa );
		strPublicName = strPublicName.Right(strPublicName.Length() - 1);
	} else {
		// must be a rootdirectory on Windos
		wxASSERT( strDirectoryTmp.Length() == 2 );
		strPublicName = strDirectoryTmp;
	}
	// we have the name, make sure it is unique by appending an index if necessary
	if (m_PublicSharedDirNames.find(strPublicName) != m_PublicSharedDirNames.end())	{
		wxString strUniquePublicName;
		for (iPos = 2; ; ++iPos) {
			strUniquePublicName = CFormat(wxT("%s_%i")) % strPublicName % iPos;

			if (m_PublicSharedDirNames.find(strUniquePublicName) == m_PublicSharedDirNames.end()) {
				AddDebugLogLineN(logClient, CFormat(wxT("Using public name '%s' for directory '%s'"))
				                            % strUniquePublicName
				                            % dir.GetPrintable());
				m_PublicSharedDirNames.insert(std::pair<wxString, CPath> (strUniquePublicName, dir));
				return strUniquePublicName;
			}
			// This is from eMule and it checks if there are more than 200 shared folders with the same public name.
			// The condition can be true if many shared subfolders with the same name exist in folders that are not
			// shared. So they get the names of each shared subfolders concatenated. But those might all be the same!
			// It's here for safety reasons so we should not run out of memory.
			else if (iPos > 200)  // Only 200 identical names are indexed.
			{
				wxASSERT( false );
				return wxT("");
			}
		}
	} else {
		AddDebugLogLineN(logClient, CFormat(wxT("Using public name '%s' for directory '%s'")) % strPublicName % dir.GetPrintable());
		m_PublicSharedDirNames.insert(std::pair<wxString, CPath> (strPublicName, dir));
		return strPublicName;
	}
}


bool CSharedFileList::IsShared(const CPath& path) const
{ 
	if( path.IsDir(CPath::exists) ) {
		// check if it's a shared folder
		const unsigned folderCount = theApp->glob_prefs->shareddir_list.size();
		for (unsigned i = 0; i < folderCount; ++i) {
			if (path.IsSameDir(theApp->glob_prefs->shareddir_list[i])) {
				return true;
			}
		}
	
		// check if it's one of the categories folders (category 0 = incoming)
		for (unsigned i = 0; i < theApp->glob_prefs->GetCatCount(); ++i) {
			if (path.IsSameDir(theApp->glob_prefs->GetCategory(i)->path)) {
				return true;
			}
		}
	}

	return false;
}


void CSharedFileList::CheckAICHHashes(const std::list<CAICHHash>& hashes)
{
	wxMutexLocker locker(list_mut);

	// Now we check that all files which are in the sharedfilelist have a
	// corresponding hash in our list. Those how don't are queued for hashing.
	CKnownFileMap::iterator it = m_Files_map.begin();
	for (; it != m_Files_map.end(); ++it) {
		const CKnownFile* file = it->second;
	
		if (file->IsPartFile() == false) {
			CAICHHashSet* hashset = file->GetAICHHashset();

			if (hashset->GetStatus() == AICH_HASHSETCOMPLETE) {
				if (std::find(hashes.begin(), hashes.end(), hashset->GetMasterHash()) != hashes.end()) {
					continue;
				}
			}

			hashset->SetStatus(AICH_ERROR);

			CThreadScheduler::AddTask(new CHashingTask(file));
		}
	}

}

// File_checked_for_headers
