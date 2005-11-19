//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2004 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "AICHSyncThread.h"
#include <common/StringFunctions.h>	// Needed for unicode2char
#include "SHAHashSet.h"
#include "CFile.h"				// Needed for CFile
#include "KnownFile.h"
#include "SHA.h"
#include "amule.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "Preferences.h"
#include "Logger.h"
#include <common/Format.h>

#include <list>
#include <algorithm>


wxMutex			CAICHSyncThread::s_mutex;
CAICHSyncThread*	CAICHSyncThread::s_thread;


bool CAICHSyncThread::Start()
{
	wxMutexLocker lock( s_mutex );

	// Check if the hasher is running already.
	if ( s_thread ) {
		AddDebugLogLineM( true, logAICHThread, wxT("Start() called while thread is active!") );
		return false;
	}
	
	s_thread = new CAICHSyncThread();

	wxThreadError state = s_thread->Create();
	if (state != wxTHREAD_NO_ERROR) {
		AddDebugLogLineM(true, logAICHThread, wxString::Format(wxT("Error while creating AICH thread: %d"), state));
	
		// Delete() must be called in this case acording to the docs.
		s_thread->Delete();
		delete s_thread;
		s_thread = NULL;
	
		return false;
	}
	
	// The threads shouldn't be hugging the CPU, as it will already be hugging the HD
	s_thread->SetPriority(WXTHREAD_MIN_PRIORITY);
	
	state = s_thread->Run();
	if (state != wxTHREAD_NO_ERROR) {
		AddDebugLogLineM(true, logAICHThread, wxString::Format(wxT("Error while attempting to run AICH thread: %d"), state));
		
		// Delete() must be called in this case acording to the docs.
		s_thread->Delete();
		delete s_thread;
		s_thread = NULL;

		return false;
	}

	return true;
}


bool CAICHSyncThread::Stop()
{
	{
		wxMutexLocker lock(s_mutex);

		// Are there any threads to kill?
		if ( !s_thread ) {
			AddDebugLogLineM( true, logAICHThread, wxT("Warning, attempted to stop non-existing thread!") );
			return false;
		}

		AddLogLineM( false, _("AICH Thread: Signaling for thread to terminate.") );

		// Tell the thread to terminate, this function returns immediatly
		s_thread->Delete();
	}

	// Wait for all threads to die
	while ( IsRunning() ) {
		// Sleep for 1/100 of a second to avoid clobbering the mutex
		MilliSleep(10);

		// Flush any pending log-entries caused by the thread.
		CLogger::FlushPendingEntries();
	}

	return true;
}


bool CAICHSyncThread::IsRunning()
{
	wxMutexLocker lock( s_mutex );

	return s_thread;
}


CAICHSyncThread::CAICHSyncThread()
	: wxThread( wxTHREAD_DETACHED )
{
}


CAICHSyncThread::~CAICHSyncThread()
{
	wxMutexLocker lock( s_mutex );
	s_thread = NULL;	
	
	AddLogLineM( false, _("AICH Thread: Terminated.") );
}


void* CAICHSyncThread::Entry()
{
	AddLogLineM( false, _("AICH Thread: Syncronization thread started.") );

	// We collect all masterhashs which we find in the known2.met and store them in a list
	typedef std::list<CAICHHash> HashList;
	HashList hashlist;

	wxString fullpath = theApp.ConfigDir + KNOWN2_MET_FILENAME;

	CFile file;
	if ( wxFileExists( fullpath ) ) {
		if ( !file.Open( fullpath, CFile::read_write ) ) {
			AddDebugLogLineM( true, logAICHThread, wxT("Error, failed to open hashlist file!") );
			return 0;
		}
	} else {
		if (!file.Create(fullpath)) {
			AddDebugLogLineM( true, logAICHThread, wxT("Error, failed to create hashlist file!") );
			return 0;
		}
	}

	try {
		while ( !file.Eof() ) {
			// Read the next hash
			hashlist.push_back( CAICHHash( &file ) );

			// skip the rest of this hashset
			uint16 nHashCount = file.ReadUInt16();
			file.Seek( nHashCount * HASHSIZE, wxFromCurrent );
		}
	} catch (const CEOFException&) {
		AddDebugLogLineM(true, logAICHThread, wxT("Hashlist corrupted, removing file."));
		file.Close();
		wxRemoveFile(fullpath);
		
		return 0;
	} catch (const CIOFailureException&) {
		AddDebugLogLineM(true, logAICHThread, wxT("IO failure while reading hashlist. Aborting."));
		
		return 0;		
	}

	file.Close();

	AddLogLineM( false, _("AICH Thread: Masterhashes of known files have been loaded.") );


	std::list<CKnownFile*> queue;

	// Now we check that all files which are in the sharedfilelist have a corresponding hash in our list
	// those how don't are added to the hashinglist
	for ( uint32 i = 0; i < theApp.sharedfiles->GetCount(); ++i) {
		// Check for termination
		if ( TestDestroy() ) {
			return 0;
		}

		const CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(i);
		if ( cur_file && !cur_file->IsPartFile() ) {
			CAICHHashSet* hashset = cur_file->GetAICHHashset();

			if ( hashset->GetStatus() == AICH_HASHSETCOMPLETE ) {

				HashList::iterator it = std::find( hashlist.begin(), hashlist.end(), hashset->GetMasterHash() );

				if ( it != hashlist.end() )
					continue;
			}

			hashset->SetStatus( AICH_ERROR );

			queue.push_back( (CKnownFile*)cur_file );
		}
	}

	// warn the user if he just upgraded
	// TODO
	//	if ( theApp.glob_prefs->Prefs.IsFirstStart() && !queue.empty() ) {
	//		theApp.QueueLogLine(false, GetResString(IDS_AICH_WARNUSER));
	//	}

	if ( !queue.empty() ) {
		AddLogLineM( false, wxString::Format( _("AICH Thread: Starting to hash files. %li files found."), (long int)queue.size() ) );
		while ( !queue.empty() && !TestDestroy() ) {
			CKnownFile* pCurFile = queue.front();
			queue.pop_front();

			AddLogLineM( false, CFormat( _("AICH Thread: Hashing file: %s, total files left: %li") ) % pCurFile->GetFileName() % queue.size() );

			// Just to be sure that the file hasnt been deleted lately
			if ( !(theApp.knownfiles->IsKnownFile(pCurFile) &&
				theApp.sharedfiles->GetFileByID(pCurFile->GetFileHash())) )
				continue;

			if ( !pCurFile->CreateAICHHashSetOnly() ) {
				AddDebugLogLineM( true, logAICHThread, wxT("Failed to create hashset for file ") + pCurFile->GetFileName() );
			}
		}

		AddLogLineM( false, _("AICH Thread: Hashing completed.") );
	} else {
		AddLogLineM( false, _("AICH Thread: No new files found.") );
	}

	return 0;
}

