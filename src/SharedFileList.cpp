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


#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>

#include "muuli_wdr.h"		// Needed for IDC_RELOADSHAREDFILES
#include "SharedFileList.h"	// Interface declarations
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "packets.h"		// Needed for Packet
#include "CMemFile.h"		// Needed for CMemFile
#include "sockets.h"		// Needed for CServerConnect
#include "SharedFilesCtrl.h"	// Needed for CSharedFilesCtrl
#include "KnownFile.h"		// Needed for CKnownFile
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "AddFileThread.h"	// Needed for CAddFileThread
#include "Preferences.h"	// Needed for CPreferences
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "amule.h"			// Needed for theApp
#include "CMD4Hash.h"		// Needed for CMD4Hash
#include "PartFile.h"		// Needed for PartFile
#include "server.h"		// Needed for CServer

CSharedFileList::CSharedFileList(CPreferences* in_prefs,CServerConnect* in_server,CKnownFileList* in_filelist){
	app_prefs = in_prefs;
	server = in_server;
	filelist = in_filelist;
	output = 0;

	FindSharedFiles();
}

CSharedFileList::~CSharedFileList(){
}

void CSharedFileList::FindSharedFiles() {
	/* Abort loading if we are shutting down. */
	if(!theApp.IsRunning()) {
		return;
	}
	/* All part files are automatically shared. */
  	if (!m_Files_map.empty()) {
		list_mut.Lock();
		m_Files_map.clear();
		list_mut.Unlock();
		theApp.downloadqueue->AddPartFilesToShare();
	}

	/* Global incoming dir and all category incoming directories are automatically shared. */
	AddFilesFromDirectory(theApp.glob_prefs->GetIncomingDir());
	for (uint32 i = 1;i < theApp.glob_prefs->GetCatCount();i++) {
		AddFilesFromDirectory(theApp.glob_prefs->GetCatPath(i));
	}
	//for (POSITION pos = app_prefs->shareddir_list.GetHeadPosition();pos != 0;app_prefs->shareddir_list.GetNext(pos))
	// remove bogus entries first
	for (unsigned int ij = 0; ij < app_prefs->shareddir_list.GetCount(); ++ij) {
		if(!wxFileName::DirExists(app_prefs->shareddir_list.Item(ij))) {
			app_prefs->shareddir_list.Remove(ij);
			--ij;
		}
	}
	for (unsigned int ii = 0; ii < app_prefs->shareddir_list.GetCount(); ++ii) {
		AddFilesFromDirectory(app_prefs->shareddir_list.Item(ii));
	}

	uint32 newFiles = CAddFileThread::GetCount();
	if (!newFiles) {
		AddLogLineF(false,_("Found %i known shared files"),m_Files_map.size());
	} else {
		AddLogLineF(false,_("Found %i known shared files, %i unknown"),m_Files_map.size(),newFiles);
	}
}

  #include <wx/encconv.h>
       #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>

void CSharedFileList::AddFilesFromDirectory(wxString directory)
{
		
	wxString fname=::wxFindFirstFile(directory,wxFILE);
  	
	if (fname.IsEmpty()) {
    		return;
	}
	while(!fname.IsEmpty()) {  

		uint32 fdate=wxFileModificationTime(fname);
		int b = open(unicode2char(fname), O_RDONLY | O_LARGEFILE);
		wxASSERT(b!=-1);
		wxFile new_file(b);
		wxASSERT(new_file.IsOpened());
		
		if(fname.Find(wxT('/'),TRUE) != -1) {  // starts at end
			// Take just the file from the path
			fname=fname.Mid(fname.Find('/',TRUE)+1);  
		}
		
		CKnownFile* toadd=filelist->FindKnownFile(fname,fdate,new_file.Length());
		//theApp.Yield();
		if (toadd) {
			if ( m_Files_map.find(toadd->GetFileHash()) == m_Files_map.end() ) {
				toadd->SetFilePath(directory);
				output->ShowFile(toadd);
				list_mut.Lock();
				m_Files_map[toadd->GetFileHash()] = toadd;
				list_mut.Unlock();
			} else {
				if (!fname.Cmp(toadd->GetFileName())) {
					printf("Warning: File '%s' already shared as '%s'\n", unicode2char(directory + fname), unicode2char(toadd->GetFileName()));
				}
			}
		} else {
			//not in knownfilelist - start adding thread to hash file
			CAddFileThread::AddFile(directory, fname);
		}
		fname=::wxFindNextFile();
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

	if (bOnlyAdd) {
		output->ShowFile(toadd);
		return;
	}
	if (output) {
		output->ShowFile(toadd);
	}
	
	// offer new file to server
	if (!server->IsConnected()) {
		return;
	}
	CMemFile* files = new CMemFile(100);

	files->Write((uint32)1); // filecount
	CreateOfferedFilePacket(toadd,files, true);
	Packet* packet = new Packet(files);
	packet->SetOpCode(OP_OFFERFILES);
	delete files;
	theApp.uploadqueue->AddUpDataOverheadServer(packet->GetPacketSize());
	server->SendPacket(packet,true);
}

// removes first occurrence of 'toremove' in 'list'
void CSharedFileList::RemoveFile(CKnownFile* toremove){
	output->RemoveFile(toremove);
	m_Files_map.erase(toremove->GetFileHash());
}

#define GetDlgItem(X) (wxStaticCast(wxWindow::FindWindowById((X)),wxButton))
void CSharedFileList::Reload(bool sendtoserver, bool firstload){
	// Madcat - Disable reloading if reloading already in progress.
	// Kry - Fixed to let non-english language users use the 'Reload' button :P
	// deltaHF - removed the old ugly button and changed the code to use the new small one
	GetDlgItem(ID_BTNRELSHARED)->Disable();
	output->DeleteAllItems();
	this->FindSharedFiles();
	if ((output) && (firstload == false)) {
		output->ShowFileList(this);
	}
	if (sendtoserver) {
		SendListToServer();
	}
	GetDlgItem(ID_BTNRELSHARED)->Enable();
	output->InitSort();
}

void CSharedFileList::SetOutputCtrl(CSharedFilesCtrl* in_ctrl){
	output = in_ctrl;
	output->ShowFileList(this);
}

void CSharedFileList::SendListToServer(){
	if (m_Files_map.empty() || !server->IsConnected())
		return;
	CMemFile* files = new CMemFile();

	files->Write((uint32)m_Files_map.size());
	
	for (CKnownFileMap::iterator pos = m_Files_map.begin();
	     pos != m_Files_map.end(); pos++ ) {
		CreateOfferedFilePacket(pos->second,files,true);
	}
	Packet* packet = new Packet(files);
	packet->SetOpCode(OP_OFFERFILES);
	delete files;
	theApp.uploadqueue->AddUpDataOverheadServer(packet->GetPacketSize());
	server->SendPacket(packet,true);
}

CKnownFile* CSharedFileList::GetFileByIndex(int index){
        int count=0;
 
	for (CKnownFileMap::iterator pos = m_Files_map.begin();
	     pos != m_Files_map.end(); pos++ ) {
                if (index==count) return pos->second;
                count++;
        }
        return 0;
}

void CSharedFileList::CreateOfferedFilePacket(CKnownFile* cur_file,CMemFile* files, bool fromserver){
	// This function is used for offering files to the local server and for sending
	// shared files to some other client. In each case we send our IP+Port only, if
	// we have a HighID.
	
	cur_file->SetPublishedED2K(true);
	files->Write((const uint8*)cur_file->GetFileHash());

	uint32 nClientID;
	uint16 nClientPort;
	
	if (!fromserver || (theApp.serverconnect->GetCurrentServer()->GetTCPFlags() & SRV_TCPFLG_COMPRESSION)) {
		#define FILE_COMPLETE_ID		0xfbfbfbfb
		#define FILE_COMPLETE_PORT	0xfbfb
		#define FILE_INCOMPLETE_ID	0xfcfcfcfc
		#define FILE_INCOMPLETE_PORT	0xfcfc
		// complete   file: ip 251.251.251 (0xfbfbfbfb) port 0xfbfb
		// incomplete file: op 252.252.252 (0xfcfcfcfc) port 0xfcfc
		if (cur_file->GetStatus() == PS_COMPLETE) {
//			printf("Publishing complete file\n");
			nClientID = FILE_COMPLETE_ID;
			nClientPort = FILE_COMPLETE_PORT;
		} else {
//			printf("Publishing incomplete file\n");			
			nClientID = FILE_INCOMPLETE_ID;
			nClientPort = FILE_INCOMPLETE_PORT;		
		}
	} else {
//		printf("Publishing standard file\n");
		if (!theApp.serverconnect->IsConnected() || theApp.serverconnect->IsLowID()){
			nClientID = 0;
			nClientPort = 0;
		} else {
			nClientID = theApp.serverconnect->GetClientID();
			nClientPort = theApp.glob_prefs->GetPort();
		}
	}
	
	files->Write(nClientID);
	files->Write(nClientPort);

	// files->Write(cur_file->GetFileTypePtr(),4);

	//uint32 uTagCount = tags.GetSize();
	uint32 uTagCount = 0; // File name and size right now
	
	if (cur_file->GetFileName()) { 
		uTagCount++;
	}

	if (cur_file->GetFileSize()>0) { 
		uTagCount++;
	}
	
	files->Write(uTagCount);
	
	if (cur_file->GetFileName()) {
		CTag* nametag = new CTag(FT_FILENAME,cur_file->GetFileName());
		nametag->WriteTagToFile(files);
		delete nametag;		
	}
	
	if (cur_file->GetFileSize()>0) { 	
		CTag* sizetag = new CTag(FT_FILESIZE,cur_file->GetFileSize());
		sizetag->WriteTagToFile(files);
		delete sizetag;
	}

	// TODO Import new CTag struct from eMule 0,30e and use their implementation
	// Mainly 'cos we lack most Tags!!!
	// This SHOULD be a correct packet anyway.
	// Either is ok or we are crashing clients that request ;)
}

uint64 CSharedFileList::GetDatasize() {
	uint64 fsize;
	fsize=0;

	for (CKnownFileMap::iterator pos = m_Files_map.begin();
	     pos != m_Files_map.end(); pos++ ) {
		fsize+=pos->second->GetFileSize();
	}
	return fsize;
}

CKnownFile*	CSharedFileList::GetFileByID(const CMD4Hash& filehash){
	if (m_Files_map.find(filehash) != m_Files_map.end())
		return m_Files_map[filehash];
	else
		return NULL;
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
	output->UpdateItem(toupdate);
}

void CSharedFileList::GetSharedFilesByDirectory(const wxString directory,
                            CTypedPtrList<CPtrList, CKnownFile*>& list)
{
	for (CKnownFileMap::iterator pos = m_Files_map.begin();
	     pos != m_Files_map.end(); pos++ ) {
		CKnownFile *cur_file = pos->second;

		if (directory.CompareTo(cur_file->GetFilePath())) {
			continue;
		}

		list.AddTail(cur_file);
	}
}

void CSharedFileList::ClearED2KPublishInfo(){
	CKnownFile* cur_file;

	for (CKnownFileMap::iterator pos = m_Files_map.begin(); pos != m_Files_map.end(); pos++ ) {
		cur_file = pos->second;
		cur_file->SetPublishedED2K(false);
	}

}
