//
// This file is part of aMule Project
//
// Copyright (c) 2003-2004 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2002-2004 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include "AICHSyncThread.h"
#include "SHAHashSet.h"
#include "SafeFile.h"
#include "KnownFile.h"
#include "SHA.h"
#include "amule.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "Preferences.h"

#include <list>
#include <algorithm>


wxMutex				CAICHSyncThread::s_thread_mutex;
CAICHSyncThread*	CAICHSyncThread::s_thread;


bool CAICHSyncThread::Start()
{
	// Check if we already have a thread
	if ( GetThread() )
		return false;

	CAICHSyncThread* thread = new CAICHSyncThread();
	if ( thread->Create() != wxTHREAD_NO_ERROR ) {
		AddLogLineM( true, _("Failed to create AICH thread!") );
		return false;
	}

	thread->SetPriority( WXTHREAD_DEFAULT_PRIORITY - 10 ); // slightly less than main
	thread->Run();

	return true;
}


bool CAICHSyncThread::Stop()
{
	if ( GetThread() ) {
		GetThread()->Delete();
		delete GetThread();
		return true;
	}

	return false;
}


bool CAICHSyncThread::IsRunning()
{
	return ( GetThread() != NULL );
}


CAICHSyncThread* CAICHSyncThread::GetThread()
{
	wxMutexLocker lock( s_thread_mutex );

	return s_thread;
}


void CAICHSyncThread::SetThread( CAICHSyncThread* ptr )
{
	wxMutexLocker lock( s_thread_mutex );

	s_thread = ptr;
}


CAICHSyncThread::CAICHSyncThread()
	: wxThread( wxTHREAD_JOINABLE )
{
	// Some sainity checking, this should never happen
	if ( GetThread() ) {
		printf("AICH Thread: Warning, thread has already been started!\n");
	}

	SetThread( this );
}


CAICHSyncThread::~CAICHSyncThread()
{
	// Some sainity checking, this should never happen
	if ( GetThread() != this ) {
		printf("AICH Thread: Warning, mismatch between running thread and static pointer!\n");
		return;
	}

	SetThread( NULL );

	printf("AICH Thread: Thread terminated\n");
}


void* CAICHSyncThread::Entry()
{
	printf("AICH Thread: Syncronization thread started...\n");

	// We collect all masterhashs which we find in the known2.met and store them in a list
	typedef std::list<CAICHHash> HashList;
	HashList hashlist;

	wxString fullpath = theApp.ConfigDir + KNOWN2_MET_FILENAME;

	CSafeFile file;
	if ( file.Exists( fullpath ) ) {
		if ( !file.Open( fullpath, CFile::read_write ) ) {
			printf("AICH Thread: Error, failed to open hashlist file!\n");

			return 0;
		}
	} else {
		if ( !file.Create( fullpath, CFile::read_write ) ) {
			printf("AICH Thread: Error, failed to create hashlist file!\n");

			return 0;
		}
	}


	try {
		while ( !file.Eof() ) {
			// Read the next hash
			hashlist.push_back( CAICHHash( &file ) );

			// skip the rest of this hashset
			uint16 nHashCount = file.ReadUInt16();
			file.Seek( nHashCount * HASHSIZE, CFile::current );
		}
	} catch ( ... ) {
		if ( file.Eof() ) {
			AddLogLineM( true, _("AICH Thread: Error, EOF while reading hashlist!") );
		} else {
			AddLogLineM( true, _("AICH Thread: Error, Corrupt file encountered while reading hashlist!") );
		}

		return 0;
	}

	file.Close();

	printf("AICH Thread: Masterhash for known files loaded...\n");


	std::list<CKnownFile*> queue;

	// Now we check that all files which are in the sharedfilelist have a corresponding hash in our list
	// those how don't are added to the hashinglist
	for ( uint32 i = 0; i < theApp.sharedfiles->GetCount(); i++) {
		// Check for termination
		if ( TestDestroy() ) {
			printf("AICH Thread: Thread interrupted, terminating\n");
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
		printf("AICH Thread: Starting to hash files. %i files found.\n", queue.size() );
		while ( !queue.empty() ) {
			// Check for termination
			if ( TestDestroy() ) {
				printf("AICH Thread: Thread interrupted, terminating\n");
				return 0;
			}

			CKnownFile* pCurFile = queue.front();

			printf("AICH Thread: Hashing file: %s, total files left: %i\n", unicode2char( pCurFile->GetFileName() ), queue.size() );

			// Just to be sure that the file hasnt been deleted lately
			if ( !(theApp.knownfiles->IsKnownFile(pCurFile) && theApp.sharedfiles->GetFileByID(pCurFile->GetFileHash())) )
				continue;

			if ( !pCurFile->CreateAICHHashSetOnly() ) {
				printf("AICH Thread: Failed to create hashset for file %s\n", unicode2char( pCurFile->GetFileName() ) );
			}

			queue.pop_front();
		}

		printf("AICH Thread: Hashing completed...\n");
	}

	return 0;
}

