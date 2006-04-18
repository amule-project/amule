//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <ctime>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wx/utils.h>
#include <wx/file.h>
#include <wx/filename.h>

#include "SharedFileList.h"	// Interface declarations
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "Packet.h"		// Needed for CPacket
#include "Tag.h"		// Needed for CTag
#include "MemFile.h"		// Needed for CMemFile
#include "ServerConnect.h"	// Needed for CServerConnect
#include "KnownFile.h"		// Needed for CKnownFile
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "ThreadTasks.h"	// Needed for CThreadScheduler and CHasherTask
#include "Preferences.h"	// Needed for thePrefs
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amule.h"		// Needed for theApp
#include "MD4Hash.h"		// Needed for CMD4Hash
#include "PartFile.h"		// Needed for PartFile
#include "Server.h"		// Needed for CServer
#include "updownclient.h"
#include <common/StringFunctions.h>	// Needed for unicode2char
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"
#include <common/Format.h>
#include "FileFunctions.h"
#include "GuiEvents.h"		// Needed for Notify_*

#ifndef AMULE_DAEMON
	#include "muuli_wdr.h"		// Needed for IDC_RELOADSHAREDFILES
	#include <wx/msgdlg.h>
#endif

#include <algorithm>		// Needed for std::find, std::sort

#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Search.h"
#include "kademlia/kademlia/Prefs.h"
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
			wxASSERT(0);
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

	void AddKeywords(CKnownFile* pFile);
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

void CPublishKeywordList::AddKeywords(CKnownFile* pFile)
{
	const Kademlia::WordList& wordlist = pFile->GetKadKeywords();

	Kademlia::WordList::const_iterator it;
	for (it = wordlist.begin(); it != wordlist.end(); ++it) {
		const wxString& strKeyword = *it;
		CPublishKeyword* pPubKw = FindKeyword(strKeyword);
		if (pPubKw == NULL) {
			pPubKw = new CPublishKeyword(strKeyword);
			m_lstKeywords.push_back(pPubKw);
			SetNextPublishTime(0);
		}
		pPubKw->AddRef(pFile);
	}
}

void CPublishKeywordList::RemoveKeywords(CKnownFile* pFile)
{
	const Kademlia::WordList& wordlist = pFile->GetKadKeywords();
	Kademlia::WordList::const_iterator it;
	for (it = wordlist.begin(); it != wordlist.end(); ++it) {
		const wxString& strKeyword = *it;
		CKeyWordList::iterator pos;
		CPublishKeyword* pPubKw = FindKeyword(strKeyword, &pos);
		if (pPubKw != NULL) {
			if (pPubKw->RemoveRef(pFile) == 0) {
				if (pos == m_posNextKeyword) {
					++m_posNextKeyword;
				}
				m_lstKeywords.erase(pos);
				delete pPubKw;
				SetNextPublishTime(0);
			}
		}
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


wxString ReadyPath(const wxString& path)
{
	if (path.Last() != wxFileName::GetPathSeparator()) {
		path + wxFileName::GetPathSeparator();
	}

	return path;
}


void CSharedFileList::FindSharedFiles() 
{
	/* Abort loading if we are shutting down. */
	if(theApp.IsOnShutDown()) {
		return;
	}

	// Clear statistics.
	theStats::ClearSharedFilesInfo();

	// Reload shareddir.dat
	theApp.glob_prefs->ReloadSharedFolders();

	{
		wxMutexLocker lock(list_mut);
		m_Files_map.clear();
	}

	// All part files are automatically shared.
	for ( uint32 i = 0; i < theApp.downloadqueue->GetFileCount(); ++i ) {
		CPartFile* file = theApp.downloadqueue->GetFileByIndex( i );
		
		if ( file->GetStatus(true) == PS_READY ) {
			printf("Adding file %s to shares\n",
				(const char *)unicode2char( file->GetFullName() ) );
			
			AddFile(file);
		}
	}

	
	// Create a list of all shared paths and weed out duplicates.
	std::list<wxString> sharedPaths;

	
	// Global incoming dir and all category incoming directories are automatically shared.
	sharedPaths.push_back(ReadyPath(thePrefs::GetIncomingDir()));
	for (unsigned int i = 1;i < theApp.glob_prefs->GetCatCount(); ++i) {
		sharedPaths.push_back(ReadyPath(theApp.glob_prefs->GetCatPath(i)));
	}

	// Also remove bogus entries
	for (size_t i = 0; i < theApp.glob_prefs->shareddir_list.GetCount(); ) {
		const wxString& path = theApp.glob_prefs->shareddir_list.Item(i);

		if (CheckDirExists(path)) {
			sharedPaths.push_back(ReadyPath(path));
			++i;
		} else {
			theApp.glob_prefs->shareddir_list.RemoveAt(i);
		}
	}

	sharedPaths.sort();
	sharedPaths.unique();

	unsigned addedFiles = 0;
	std::list<wxString>::iterator it = sharedPaths.begin();
	for (; it != sharedPaths.end(); ++it) {
		addedFiles += AddFilesFromDirectory(*it);
	}
	
	if (addedFiles == 0) {
		AddLogLineM(false, wxString::Format(_("Found %i known shared files"), 
			GetCount()));
		
		// Make sure the AICH-hashes are up to date.
		CThreadScheduler::AddTask(new CAICHSyncTask());
	} else {	
		// New files, AICH thread will be run at the end of the hashing thread.
		AddLogLineM(false,
			wxString::Format(_("Found %i known shared files, %i unknown"),
				GetCount(), addedFiles));
	}
}


// Checks if the dir a is the same as b. If they are, then logs the message and returns true.
bool CheckDirectory(const wxString& a, const wxString& b)
{
	if (ReadyPath(a) == ReadyPath(b)) {
		AddLogLineM(true, CFormat( _("ERROR! Attempted to share %s") ) % a);
		
		return true;
	}

	return false;
}
		

unsigned CSharedFileList::AddFilesFromDirectory(wxString directory)
{
	if ( !wxDirExists( directory ) ) {
		return 0;
	}

	directory = ReadyPath(directory);

	// Do not allow these folders to be shared:
	//  - The .aMule folder
	//  - The Temp folder
	// The following dirs just result in a warning.
	//  - The users home-dir
	if (CheckDirectory(wxGetHomeDir(), directory)) {
		return 0;
	}
		
	if (CheckDirectory(theApp.ConfigDir, directory)) {
		return 0;
	}
		
	if (CheckDirectory(thePrefs::GetTempDir(), directory)) {
		return 0;
	}

	CDirIterator SharedDir(directory); 
	
	wxString fname = SharedDir.GetFirstFile(CDirIterator::File); // We just want files

	if (fname.IsEmpty()) {
		printf("Empty dir %s shared\n", (const char *)unicode2char(directory));
    		return 0;
	}

	unsigned addedFiles = 0;
	while(!fname.IsEmpty()) {		
		AddDebugLogLineM(false, logKnownFiles, wxT("Found file ")+fname + wxT(" on shared folder"));

		uint32 fdate=GetLastModificationTime(fname);

		if (::wxDirExists(fname)) {
			// Woops, is a dir!
			AddDebugLogLineM(false, logKnownFiles, wxT("Shares: ") + fname + wxT(" is a directory, skipping"));
			fname = SharedDir.GetNextFile();
			continue;
		}
		
		CFile new_file(fname, CFile::read);
		if (!new_file.IsOpened()) {
			AddDebugLogLineM(false, logKnownFiles, wxT("No permisions to open") + fname + wxT(", skipping"));
			fname = SharedDir.GetNextFile();
			continue;
		}
		
		// Take just the file from the path
		fname = wxFileName(fname).GetFullName();

		if (!thePrefs::ShareHiddenFiles() && fname.StartsWith(wxT("."))) {
			AddDebugLogLineM(false, logKnownFiles, wxT("Ignored file ") + fname + wxT(" (Hidden)"));
			fname = SharedDir.GetNextFile();
			continue;
		}

		uint64 fileLength = 0;
		try {
			fileLength = new_file.GetLength();
		} catch (const CIOFailureException& e) {
			AddDebugLogLineM(true, logKnownFiles, wxT("Failed to get filesize, skipping: ") + fname);
			fname = SharedDir.GetNextFile();
			continue;
		}
		
		CKnownFile* toadd = filelist->FindKnownFile(fname, fdate, fileLength);
		if (toadd) {
			if ( AddFile(toadd) ) {
				AddDebugLogLineM(false, logKnownFiles, wxT("Added known file ") + fname + wxT(" to shares"));
				toadd->SetFilePath(directory);
			} else {
				if (fname != toadd->GetFileName()) {
					AddDebugLogLineM(false, logKnownFiles, wxT("Warning: File '") + directory + fname + wxT("' already shared as '") + toadd->GetFileName() + wxT("'"));
				} else {
					AddDebugLogLineM(false, logKnownFiles, wxT("File '") + fname + wxT("' is already shared"));
				}
			}
		} else {
			//not in knownfilelist - start adding thread to hash file
			AddDebugLogLineM(false, logKnownFiles, wxT("Hashing new unknown shared file ") + fname);
			
			if (CThreadScheduler::AddTask(new CHashingTask(directory, fname))) {
				addedFiles++;
			}
		}
		fname = SharedDir.GetNextFile();
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
	
	if (!bOnlyAdd && theApp.IsConnectedED2K()) {		
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


void CSharedFileList::CopyFileList(std::vector<CKnownFile*>& out_list)
{
	wxMutexLocker lock(list_mut);

	out_list.reserve(m_Files_map.size());
	for (
		CKnownFileMap::iterator it = m_Files_map.begin();
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

	for (CKnownFileMap::iterator pos = m_Files_map.begin();
	     pos != m_Files_map.end(); ++pos ) {
		CKnownFile *cur_file = pos->second;

		if (MakeFoldername(directory).CompareTo(MakeFoldername(cur_file->GetFilePath()))) {
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
	CServer* server = theApp.serverconnect->GetCurrentServer();
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

		if (m_Files_map.empty() || !theApp.IsConnectedED2K() ) {
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
	
	CServer* server = theApp.serverconnect->GetCurrentServer();	
	
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
			CreateOfferedFilePacket(file, &files, server, NULL);
		}
		file->SetPublishedED2K(true);	
		++count;
	}
	
	wxASSERT(count == limit);
	
	CPacket* packet = new CPacket(&files, OP_EDONKEYPROT, OP_OFFERFILES);
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
	theApp.serverconnect->SendPacket(packet,true);
}


void CSharedFileList::CreateOfferedFilePacket(
	CKnownFile *cur_file,
	CMemFile *files,
	CServer *pServer,
	CUpDownClient *pClient) {
		
	// This function is used for offering files to the local server and for sending
	// shared files to some other client. In each case we send our IP+Port only, if
	// we have a HighID.

	wxASSERT(!(pClient && pServer));
		
	cur_file->SetPublishedED2K(true);
	files->WriteHash(cur_file->GetFileHash());

	uint32 nClientID = 0;
	uint16 nClientPort = 0;

	if (pServer) {
		if (pServer->GetTCPFlags() & SRV_TCPFLG_COMPRESSION) {
			#define FILE_COMPLETE_ID		0xfbfbfbfb
			#define FILE_COMPLETE_PORT	0xfbfb
			#define FILE_INCOMPLETE_ID	0xfcfcfcfc
			#define FILE_INCOMPLETE_PORT	0xfcfc
			// complete   file: ip 251.251.251 (0xfbfbfbfb) port 0xfbfb
			// incomplete file: op 252.252.252 (0xfcfcfcfc) port 0xfcfc
			if (cur_file->GetStatus() == PS_COMPLETE) {
				nClientID = FILE_COMPLETE_ID;
				nClientPort = FILE_COMPLETE_PORT;
			} else {
				nClientID = FILE_INCOMPLETE_ID;
				nClientPort = FILE_INCOMPLETE_PORT;
			}
		} else {
			if (theApp.IsConnectedED2K() && !::IsLowID(theApp.GetED2KID())){
				nClientID = theApp.GetID();
				nClientPort = thePrefs::GetPort();
			}
		}
	} else {
		// Do not merge this with the above case - this one
		// also checks Kad status.
		if (theApp.IsConnected() && !theApp.IsFirewalled()) {
			nClientID = theApp.GetID();
			nClientPort = thePrefs::GetPort();
		}		
	}

	files->WriteUInt32(nClientID);
	files->WriteUInt16(nClientPort);
	
	TagPtrList tags;

	tags.push_back(new CTagString(FT_FILENAME, cur_file->GetFileName()));
	
	if (pClient && pClient->GetVBTTags()) {
		tags.push_back(new CTagVarInt(FT_FILESIZE, cur_file->GetFileSize()));
	} else {
		if (!cur_file->IsLargeFile()){
			tags.push_back(new CTagInt32(FT_FILESIZE, cur_file->GetFileSize()));
		} else {
			// Large file
			// we send 2*32 bit tags to servers, but a real 64 bit tag to other clients.
			if (pServer) {
				if (!pServer->SupportsLargeFilesTCP()){
					wxASSERT( false );
					tags.push_back(new CTagInt32(FT_FILESIZE, 0));
				}else {
					tags.push_back(new CTagInt32(FT_FILESIZE, (uint32)cur_file->GetFileSize()));
					tags.push_back(new CTagInt32(FT_FILESIZE_HI, (uint32)(cur_file->GetFileSize() >> 32)));
				}
			} else {
				if (!pClient->SupportsLargeFiles()) {
					wxASSERT( false );
					tags.push_back(new CTagInt32(FT_FILESIZE, 0));
				} else {
					tags.push_back(new CTagInt64(FT_FILESIZE, cur_file->GetFileSize()));
				}
			}		
		}
	}
	
	if (cur_file->GetFileRating()) {
		tags.push_back(new CTagVarInt(FT_FILERATING, cur_file->GetFileRating(), (pClient && pClient->GetVBTTags()) ? 0 : 32));
	}

	// NOTE: Archives and CD-Images are published+searched with file type "Pro"
	bool bAddedFileType = false;
	if (pServer && (pServer->GetTCPFlags() & SRV_TCPFLG_TYPETAGINTEGER)) {
		// Send integer file type tags to newer servers
		EED2KFileType eFileType = GetED2KFileTypeSearchID(GetED2KFileTypeID(cur_file->GetFileName()));
		if (eFileType >= ED2KFT_AUDIO && eFileType <= ED2KFT_CDIMAGE) {
			tags.push_back(new CTagInt32(FT_FILETYPE, eFileType));
			bAddedFileType = true;
		}
	}
	if (!bAddedFileType) {
		// Send string file type tags to:
		//	- newer servers, in case there is no integer type available for the file type (e.g. emulecollection)
		//	- older servers
		//	- all clients
		wxString strED2KFileType(GetED2KFileTypeSearchTerm(GetED2KFileTypeID(cur_file->GetFileName())));
		if (!strED2KFileType.IsEmpty()) {
			tags.push_back(new CTagString(FT_FILETYPE, strED2KFileType));
		}
	}

	// There, we could add MetaData info, if we ever get to have that.
	
	EUtf8Str eStrEncode;

	bool unicode_support = 
		// eservers that support UNICODE.
		(pServer && (pServer->GetUnicodeSupport()))
		||
		// clients that support unicode
		(pClient && pClient->GetUnicodeSupport());
	eStrEncode = unicode_support ? utf8strRaw : utf8strNone;
	
	files->WriteUInt32(tags.size());

	// Sadly, eMule doesn't use a MISCOPTIONS flag on hello packet for this, so we
	// have to identify the support for new tags by version.
	bool new_ed2k = 	
		// eMule client > 0.42f
		(pClient && pClient->IsEmuleClient() && pClient->GetVersion()  >= MAKE_CLIENT_VERSION(0,42,7))
		||
		// aMule >= 2.0.0rc8. Sadly, there's no way to check the rcN number, so I checked
		// the rc8 changelog. On rc8 OSInfo was introduced, so...
		(pClient && pClient->GetClientSoft() == SO_AMULE && !pClient->GetClientOSInfo().IsEmpty())
		||
		// eservers use a flag for this, at least.
		(pServer && (pServer->GetTCPFlags() & SRV_TCPFLG_NEWTAGS));
	
	for (TagPtrList::iterator it = tags.begin(); it != tags.end(); ++it ) {
		CTag* pTag = *it;
		if (new_ed2k) {
			pTag->WriteNewEd2kTag(files, eStrEncode);
		} else {
			pTag->WriteTagToFile(files, eStrEncode);
		}
		delete pTag;		
	}
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
	bool IsFirewalled = theApp.IsFirewalled();

	if( Kademlia::CKademlia::IsConnected() && ( !IsFirewalled || ( IsFirewalled && theApp.clientlist->GetBuddyStatus() == Connected)) && GetCount() && Kademlia::CKademlia::GetPublish()) { 
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
								pSearch->PreparePacket();
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
				CKnownFile* pCurKnownFile = (CKnownFile*)GetFileByIndex(m_currFileSrc);
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
				CKnownFile* pCurKnownFile = (CKnownFile*) GetFileByIndex(m_currFileNotes);
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


bool CSharedFileList::RenameFile(CKnownFile* file, const wxString& newName)
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
#warning Renaming of completed files causes problems on kad. Enable when reviewed.
#if 0
		wxString oldPath = JoinPaths(file->GetFilePath(), file->GetFileName());
		wxString newPath = JoinPaths(file->GetFilePath(), newName);

		if (UTF8_MoveFile(oldPath, newPath)) {
			RemoveKeywords(file);
			file->SetFileName(newName);
			AddKeywords(file);
			theApp.knownfiles->Save();
			UpdateItem(file);
			RepublishFile(file);

			Notify_DownloadCtrlUpdateItem(file);
			Notify_SharedFilesUpdateItem(file);
			
			return true;
		}
#endif
	}
	
	return false;
}
