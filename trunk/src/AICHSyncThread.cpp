//This file is part of aMule Project
//
// Copyright (c) 2003-2004 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2002-2004 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "AICHSyncThread.h"
#include "SHAHashSet.h"
#include "SafeFile.h"
#include "KnownFile.h"
#include "SHA.h"
#include "amule.h"
#include "amuleDlg.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "Preferences.h"
#include "SharedFilesWnd.h"
#include "SharedFilesCtrl.h"	
#include <wx/debug.h>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

using namespace std;

#warning FILE TODO: LOGLINES AND FIRST TIME WARNING 
// And well, IMHO first time warning should be on the checkversion.
// Grep TODO on file for changes needed.

wxMutex CamuleApp::hashing_mut;

/////////////////////////////////////////////////////////////////////////////////////////
///CAICHSyncThread

CAICHSyncThread::CAICHSyncThread()
{

}

bool CAICHSyncThread::InitInstance()
{
	#warning NET_UNICODE
	//InitThreadLocale();
	return TRUE;
}

void* CAICHSyncThread::Entry()
{

	if ( !theApp.IsRunning() )
		return 0;

	// we collect all masterhashs which we find in the known2.met and store them in a list
	std::list<CAICHHash> liKnown2Hashs;
	wxString fullpath=theApp.ConfigDir;
	fullpath.Append(KNOWN2_MET_FILENAME);
	CSafeFile file;
	uint32 nLastVerifiedPos = 0;
	if (file.Exists(fullpath)) {	
		if (!file.Open(fullpath, CFile::read_write)) {
			#warning: logline
			return false;
		}
	} else {
		if (!file.Create(fullpath, CFile::read_write)) {
			#warning: logline
			return false;			
		}		
	}
	
	try {

		uint32 nExistingSize = file.GetLength();
		uint16 nHashCount;
		while (file.GetPosition() < nExistingSize){
			liKnown2Hashs.push_back(CAICHHash(&file));
			nHashCount = file.ReadUInt16();
			if (file.GetPosition() + nHashCount*HASHSIZE > nExistingSize){
				throw fullpath;
			}
			// skip the rest of this hashset
			file.Seek(nHashCount*HASHSIZE, CFile::current);
			nLastVerifiedPos = file.GetPosition();
		}
	}
	catch(wxString error){
		if (file.Eof()) {
			   theApp.QueueLogLine(true,_("Thread Error: EOF on ") + error);
		} else {
				theApp.QueueLogLine(true,_("Thread Error: wrong format on ")  + error);
		}
		return false;
	}
	file.Close();

	// now we check that all files which are in the sharedfilelist have a corresponding hash in out list
	// those how don'T are added to the hashinglist
	for (uint32 i = 0; i < theApp.sharedfiles->GetCount(); i++){
		const CKnownFile* pCurFile = theApp.sharedfiles->GetFileByIndex(i);
		if (pCurFile != NULL && !pCurFile->IsPartFile() ){
			if (theApp.amuledlg==NULL || !theApp.IsRunning()) // in case of shutdown while still hashing
				return 0;
			if (pCurFile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE){
				bool bFound = false;
				for (std::list<CAICHHash>::iterator it = liKnown2Hashs.begin();it != liKnown2Hashs.end(); ++it)
				{
					if (*(it) == pCurFile->GetAICHHashset()->GetMasterHash()){
						bFound = true;
#ifdef _DEBUG_
						// in debugmode we load and verify all hashsets
						wxASSERT( pCurFile->GetAICHHashset()->LoadHashSet() );
						pCurFile->GetAICHHashset()->FreeHashSet();
#endif
						break;
					}
				}
				if (bFound) // hashset is available, everything fine with this file
					continue;
			}
			pCurFile->GetAICHHashset()->SetStatus(AICH_ERROR);
			m_liToHash.push_back((CKnownFile*)pCurFile);
		}
	}
	// warn the user if he just upgraded
// TODO
//	if (theApp.glob_prefs->Prefs.IsFirstStart() && !m_liToHash.empty()){
//		theApp.QueueLogLine(false, GetResString(IDS_AICH_WARNUSER));
//	}

	if (!m_liToHash.empty()){
// TODO		
//		theApp.QueueLogLine(true, GetResString(IDS_AICH_SYNCTOTAL), m_liToHash.GetCount() );
		theApp.amuledlg->sharedfileswnd->sharedfilesctrl->SetAICHHashing(m_liToHash.size());
		// let first all normal hashing be done before starting out synchashing
		while (theApp.sharedfiles->GetCount() != 0){
			Sleep(100);
		}
		wxMutexLocker sLock1(theApp.hashing_mut); // only one filehash at a time
		// sLock1.Lock(); // Mutex Locker locks on constructor.
		uint32 cDone = 0;
		for (KnownFilePtrList::iterator it = m_liToHash.begin();it != m_liToHash.end(); ++it)
		{
			if (theApp.amuledlg==NULL || !theApp.IsRunning()){ // in case of shutdown while still hashing
				return 0;
			}
			theApp.amuledlg->sharedfileswnd->sharedfilesctrl->SetAICHHashing(m_liToHash.size()-cDone);
			if (theApp.amuledlg->sharedfileswnd->sharedfilesctrl != NULL)
				theApp.amuledlg->sharedfileswnd->sharedfilesctrl->ShowFilesCount();
			
			CKnownFile* pCurFile = *(it);
			// just to be sure that the file hasnt been deleted lately
			if (!(theApp.knownfiles->IsKnownFile(pCurFile) && theApp.sharedfiles->GetFileByID(pCurFile->GetFileHash())) )
				continue;
// TODO			
//			theApp.QueueLogLine(false, GetResString(IDS_AICH_CALCFILE), pCurFile->GetFileName());
			if(!pCurFile->CreateAICHHashSetOnly()) 
				{}
// TODO					
				//theApp.QueueDebugLogLine(false, _T("Failed to create AICH Hashset while sync. for file %s"), pCurFile->GetFileName());
			cDone++;
		}

		theApp.amuledlg->sharedfileswnd->sharedfilesctrl->SetAICHHashing(0);
		if (theApp.amuledlg->sharedfileswnd->sharedfilesctrl != NULL)
			theApp.amuledlg->sharedfileswnd->sharedfilesctrl->ShowFilesCount();
		// sLock1.Unlock(); // And unlocks on destructor
	}
// TODO
	//theApp.QueueDebugLogLine(false, _("AICHSyncThread finished"));
	return 0;
}
