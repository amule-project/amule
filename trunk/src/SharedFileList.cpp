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
#pragma implementation "SharedFileList.h"
#endif

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
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "ServerConnect.h"		// Needed for CServerConnect
#include "KnownFile.h"		// Needed for CKnownFile
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "AddFileThread.h"	// Needed for CAddFileThread
#include "Preferences.h"	// Needed for CPreferences
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amule.h"			// Needed for theApp
#include "CMD4Hash.h"		// Needed for CMD4Hash
#include "PartFile.h"		// Needed for PartFile
#include "Server.h"		// Needed for CServer
#include "updownclient.h"
#include "StringFunctions.h" // Needed for unicode2char
#include "Statistics.h"		// Needed for CStatistics
#include "Logger.h"
#include "Format.h"

#ifndef AMULE_DAEMON
	#include "muuli_wdr.h"		// Needed for IDC_RELOADSHAREDFILES
	#include <wx/msgdlg.h>
#endif

#warning Theres a lot of Kad code missing from this file and the .h that must be addressed.

CSharedFileList::CSharedFileList(CKnownFileList* in_filelist){
	filelist = in_filelist;
	reloading = false;
	m_lastPublishED2K = 0;
	m_lastPublishED2KFlag = true;	
}

CSharedFileList::~CSharedFileList(){
}

static int wxStat( const wxChar *file_name, wxStructStat *buf, wxMBConv &wxConv )
{
	return stat( wxConv.cWX2MB( file_name ), buf );
}

static bool wxDirExists(const wxChar *pszPathName, wxMBConv &wxConv)
{
	wxString strPath(pszPathName);
	wxStructStat st;
	return wxStat(strPath.c_str(), &st, wxConv) == 0 && ((st.st_mode & S_IFMT) == S_IFDIR);
}

void CSharedFileList::FindSharedFiles() {
	/* Abort loading if we are shutting down. */
	if(!theApp.IsOnShutDown()) {
		return;
	}
	
	// Reload shareddir.dat
	theApp.glob_prefs->ReloadSharedFolders();
	
	/* All part files are automatically shared. */
  	if (!m_Files_map.empty()) {
		list_mut.Lock();
		m_Files_map.clear();
		list_mut.Unlock();

		for ( uint32 i = 0; i < theApp.downloadqueue->GetFileCount(); ++i ) {
			CPartFile* file = theApp.downloadqueue->GetFileByIndex( i );
			
			if ( file->GetStatus(true) == PS_READY ) {
				printf("Adding file %s to shares\n",
					(const char *)unicode2char( file->GetFullName() ) );
				
				SafeAddKFile( file, true );
			}
		}
	}

	/* Global incoming dir and all category incoming directories are automatically shared. */

	AddFilesFromDirectory(thePrefs::GetIncomingDir());
	for (unsigned int i = 1;i < theApp.glob_prefs->GetCatCount(); ++i) {
		AddFilesFromDirectory(theApp.glob_prefs->GetCatPath(i));
	}

	// remove bogus entries first
	for (unsigned int i = 0; i < theApp.glob_prefs->shareddir_list.GetCount(); ) {
		const wxString &fileName = theApp.glob_prefs->shareddir_list.Item(i);
		// In ANSI builds, the first call is enough. In UNICODE builds,
		// the second call makes sure we are able to read an extended
		// ANSI chars directory name.
		if(	!wxFileName::DirExists(fileName) &&
			!wxDirExists(fileName, aMuleConv)) {
			theApp.glob_prefs->shareddir_list.RemoveAt(i);
		} else {
			++i;
		}
	}

	for (unsigned int i = 0; i < theApp.glob_prefs->shareddir_list.GetCount(); ++i) {
		AddFilesFromDirectory(theApp.glob_prefs->shareddir_list.Item(i));
	}

	uint32 newFiles = CAddFileThread::GetFileCount();
	if (!newFiles) {
		AddLogLineM(false,
			wxString::Format(_("Found %i known shared files"),
				m_Files_map.size()));
		// No new files, run AICH thread
		theApp.RunAICHThread();
	} else {	
		// New files, AICH thread will be run at the end of the hashing thread.
		AddLogLineM(false,
			wxString::Format(_("Found %i known shared files, %i unknown"),
				m_Files_map.size(),newFiles));
	}
}


// Checks if the dir a is the same as b. If they are, then logs the message and returns true.
bool CheckDirectory( const wxString& a, const wxString& b, bool fatal )
{
	wxString tmp = a;
	if ( tmp.Last() != wxFileName::GetPathSeparator() ) {
		tmp += wxFileName::GetPathSeparator();
	}
	
	if ( tmp == b ) {
		wxString msg;

		if ( fatal ) {
			msg = CFormat( _("ERROR! Attempted to share %s") ) % a;
		} else {
			msg = CFormat( _("WARNING! Sharing the following directory is not recommended: %s") ) % a;
		}
		
		AddLogLineM(true, msg);
		
		return true;
	}

	return false;
}
		

void CSharedFileList::AddFilesFromDirectory(wxString directory)
{
	if ( !wxDirExists( directory ) ) {
		return;
	}

	if (directory.Last() != wxFileName::GetPathSeparator()) {
		directory += wxFileName::GetPathSeparator();
	}


	// Do not allow these folders to be shared:
	//  - The .aMule folder
	//  - The Temp folder
	// The following dirs just result in a warning.
	//  - The users home-dir
	CheckDirectory( wxGetHomeDir(),	directory, false );
		
	if ( CheckDirectory( theApp.ConfigDir,	directory, true ) )
		return;
		
	if ( CheckDirectory( thePrefs::GetTempDir(), directory, true ) )
		return;


	CDirIterator SharedDir(directory); 
	
	wxString fname = SharedDir.FindFirstFile(CDirIterator::File); // We just want files

	if (fname.IsEmpty()) {
		printf("Empty dir %s shared\n", (const char *)unicode2char(directory));
    		return;
	}
	while(!fname.IsEmpty()) {
		
		AddDebugLogLineM(false, logKnownFiles, wxT("Found file ")+fname + wxT(" on shared folder"));

		uint32 fdate=GetLastModificationTime(fname);

		if (::wxDirExists(fname)) {
			// Woops, is a dir!
			AddDebugLogLineM(false, logKnownFiles, wxT("Shares: ") + fname + wxT(" is a directory, skipping"));
			fname = SharedDir.FindNextFile();
			continue;
		}
		
		CFile new_file(fname, CFile::read);

		if (!new_file.IsOpened()) {
			AddDebugLogLineM(false, logKnownFiles, wxT("No permisions to open") + fname + wxT(", skipping"));
			fname = SharedDir.FindNextFile();
			continue;
		}

		if(fname.Find(wxFileName::GetPathSeparator(),TRUE) != -1) {  // starts at end
			// Take just the file from the path
			fname=fname.Mid(fname.Find(wxFileName::GetPathSeparator(),TRUE)+1);
		}

		if (!thePrefs::ShareHiddenFiles() && fname.StartsWith(wxT("."))) {
			AddDebugLogLineM(false, logKnownFiles, wxT("Ignored file ") + fname + wxT(" (Hidden)"));
			fname = SharedDir.FindNextFile();
			continue;			
		}
		
		CKnownFile* toadd=filelist->FindKnownFile(fname,fdate,new_file.Length());
		//theApp.Yield();
		if (toadd) {
			if ( m_Files_map.find(toadd->GetFileHash()) == m_Files_map.end() ) {
				AddDebugLogLineM(false, logKnownFiles, wxT("Added known file ") + fname + wxT(" to shares"));
				toadd->SetFilePath(directory);
				Notify_SharedFilesShowFile(toadd);
				list_mut.Lock();
				m_Files_map[toadd->GetFileHash()] = toadd;
				list_mut.Unlock();
			} else {
				if (fname.Cmp(toadd->GetFileName())) {
					AddDebugLogLineM(false, logKnownFiles, wxT("Warning: File '") + directory + fname + wxT("' already shared as '") + toadd->GetFileName() + wxT("'"));
				} else {
					AddDebugLogLineM(false, logKnownFiles, wxT("File '") + fname + wxT("' is already shared"));
				}
			}
		} else {
			//not in knownfilelist - start adding thread to hash file
			AddDebugLogLineM(false, logKnownFiles, wxT("Hashing new unknown shared file ") + fname);
			CAddFileThread::AddFile(directory, fname);
		}
		fname = SharedDir.FindNextFile();
	}
}

void CSharedFileList::SafeAddKFile(CKnownFile* toadd, bool bOnlyAdd){
	// TODO: Check if the file is already known - only with another date
	
	//CSingleLock sLock(&list_mut,true);
	list_mut.Lock();
	if ( m_Files_map.find(toadd->GetFileHash()) != m_Files_map.end() )
	{
		list_mut.Unlock();
		return;
	}
	m_Files_map[toadd->GetFileHash()] = toadd;
	//sLock.Unlock();
	list_mut.Unlock();

	Notify_SharedFilesShowFile(toadd);
	
	if (!bOnlyAdd) {
		
		// offer new file to server
		if (!theApp.serverconnect->IsConnected()) {
			return;
		}
	
		m_lastPublishED2KFlag = true;
		
		// Publishing of files is not anymore handled here. Instead, the timer does it by itself.

	}
	
}

// removes first occurrence of 'toremove' in 'list'
void CSharedFileList::RemoveFile(CKnownFile* toremove){
	Notify_SharedFilesRemoveFile(toremove);
	m_Files_map.erase(toremove->GetFileHash());
}

void CSharedFileList::Reload(bool firstload){
	// Madcat - Disable reloading if reloading already in progress.
	// Kry - Fixed to let non-english language users use the 'Reload' button :P
	// deltaHF - removed the old ugly button and changed the code to use the new small one
	// Kry - bah, let's use a var. 
	if (!reloading) {
		reloading = true;
		Notify_SharedFilesRemoveAllItems();
		FindSharedFiles();
		if (firstload == false) {
			Notify_SharedFilesShowFileList(this);
		}
		Notify_SharedFilesSort();
		reloading = false;
	}
}

const CKnownFile *CSharedFileList::GetFileByIndex(unsigned int index) const {
	if ( index >= m_Files_map.size() ) {
		return NULL;
	}
	unsigned int count = 0;
	for ( 	CKnownFileMap::const_iterator pos = m_Files_map.begin();
		pos != m_Files_map.end();
		++pos ) {
		if ( index == count ) {
			return pos->second;
		}
		++count;
        }
	// Should never return here
	wxASSERT(0);
	return NULL;
}

uint64 CSharedFileList::GetDatasize() {
	uint64 fsize;
	fsize=0;

	for (CKnownFileMap::iterator pos = m_Files_map.begin();
	     pos != m_Files_map.end(); ++pos ) {
		fsize+=pos->second->GetFileSize();
	}
	return fsize;
}

CKnownFile*	CSharedFileList::GetFileByID(const CMD4Hash& filehash)
{
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


void CSharedFileList::UpdateItem(CKnownFile* toupdate)
{
	Notify_SharedFilesUpdateItem(toupdate);
}

void CSharedFileList::GetSharedFilesByDirectory(const wxString directory,
                            CTypedPtrList<CPtrList, CKnownFile*>& list)
{
	for (CKnownFileMap::iterator pos = m_Files_map.begin();
	     pos != m_Files_map.end(); ++pos ) {
		CKnownFile *cur_file = pos->second;

		if (directory.CompareTo(cur_file->GetFilePath())) {
			continue;
		}

		list.AddTail(cur_file);
	}
}

/* ---------------- Network ----------------- */

void CSharedFileList::ClearED2KPublishInfo(){
	CKnownFile* cur_file;
	m_lastPublishED2KFlag = true;
	for (CKnownFileMap::iterator pos = m_Files_map.begin(); pos != m_Files_map.end(); ++pos ) {
		cur_file = pos->second;
		cur_file->SetPublishedED2K(false);
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
	
	if (m_Files_map.empty() || !theApp.serverconnect->IsConnected() ) {
		return;
	}
	
	// Gettting a sorted list of the non-published files.

	std::vector<CKnownFile*> SortedList;
	SortedList.reserve( m_Files_map.size() );

	CKnownFileMap::iterator it = m_Files_map.begin();
	for ( ; it != m_Files_map.end(); ++it ) {
		if (!it->second->GetPublishedED2K()) {
			SortedList.push_back( it->second );
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

	CSafeMemFile files;
	
	// Files sent.
	files.WriteUInt32(limit);	
	
	uint16 count = 0;	
	// Add to packet
	std::vector<CKnownFile*>::iterator sorted_it = SortedList.begin();
	for ( ; (sorted_it != SortedList.end()) && (count < limit); ++sorted_it ) {
		CKnownFile* file = *sorted_it;
		CreateOfferedFilePacket(file, &files, server, NULL);
		file->SetPublishedED2K(true);	
		++count;
	}
	
	wxASSERT(count == limit);
	
	CPacket* packet = new CPacket(&files);
	packet->SetOpCode(OP_OFFERFILES);
	// compress packet
	//   - this kind of data is highly compressable (N * (1 MD4 and at least 3 string meta data tags and 1 integer meta data tag))
	//   - the min. amount of data needed for one published file is ~100 bytes
	//   - this function is called once when connecting to a server and when a file becomes shareable - so, it's called rarely.
	//   - if the compressed size is still >= the original size, we send the uncompressed packet
	// therefor we always try to compress the packet
	if (server->GetTCPFlags() & SRV_TCPFLG_COMPRESSION){
		packet->PackPacket();
	}

	theApp.statistics->AddUpDataOverheadServer(packet->GetPacketSize());
	theApp.serverconnect->SendPacket(packet,true);
}


void CSharedFileList::CreateOfferedFilePacket(
	CKnownFile *cur_file,
	CSafeMemFile *files,
	CServer *pServer,
	CUpDownClient *pClient) {
		
	// This function is used for offering files to the local server and for sending
	// shared files to some other client. In each case we send our IP+Port only, if
	// we have a HighID.

	wxASSERT(!(pClient && pServer));
		
	cur_file->SetPublishedED2K(true);
	files->WriteHash16(cur_file->GetFileHash());

	uint32 nClientID;
	uint16 nClientPort;

	if (pServer && (pServer->GetTCPFlags() & SRV_TCPFLG_COMPRESSION)) {
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
		if (!theApp.serverconnect->IsConnected() || theApp.serverconnect->IsLowID()){
			nClientID = 0;
			nClientPort = 0;
		} else {
			nClientID = theApp.serverconnect->GetClientID();
			nClientPort = thePrefs::GetPort();
		}
	}

	files->WriteUInt32(nClientID);
	files->WriteUInt16(nClientPort);
	
	TagPtrList tags;

	tags.push_back(new CTag(FT_FILENAME, cur_file->GetFileName()));
	tags.push_back(new CTag(FT_FILESIZE, cur_file->GetFileSize()));
	
	// NOTE: Archives and CD-Images are published with file type "Pro"
	wxString strED2KFileType(otherfunctions::GetED2KFileTypeSearchTerm(otherfunctions::GetED2KFileTypeID(cur_file->GetFileName())));
	if (!strED2KFileType.IsEmpty()) {
		tags.push_back(new CTag(FT_FILETYPE, strED2KFileType));
	}

	wxString strExt;
	int iExt = cur_file->GetFileName().Find(wxT('.'), true);
	if (iExt != -1){
		strExt = cur_file->GetFileName().Mid(iExt);
		if (!strExt.IsEmpty()){
			strExt = strExt.Mid(1);
			if (!strExt.IsEmpty()){
				strExt.MakeLower();
				tags.push_back(new CTag(FT_FILEFORMAT, strExt)); // file extension without a "."
			}
		}
	}

		
	// There, we could add MetaData info, if we ever get to have that.
	
	EUtf8Str eStrEncode;

#if wxUSE_UNICODE
	bool unicode_support = 
		// eservers that support UNICODE.
		(pServer && (pServer->GetTCPFlags() & SRV_TCPFLG_UNICODE))
		||
		// clients that support unicode
		(pClient && !pClient->GetUnicodeSupport());
	eStrEncode = unicode_support ? utf8strRaw : utf8strNone;
#else
	eStrEncode = utf8strNone;
#endif
	
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
	#warning Kad
	//Publish();
	if( !m_lastPublishED2KFlag || ( ::GetTickCount() - m_lastPublishED2K < ED2KREPUBLISHTIME ) ) {
		return;
	}
	SendListToServer();
	m_lastPublishED2K = ::GetTickCount();
}
