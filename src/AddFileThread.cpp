// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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
#pragma implementation "AddFileThread.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/event.h>		// Needed for wxCommandEvent
#include <wx/timer.h>		// Needed for wxStopWatch

#if wxCHECK_VERSION(2, 5, 2)
#	include <wx/arrstr.h>		// Needed for wxArrayString
#endif

#include <wx/filename.h>	// Needed for wxFileName::GetPathSeperator()
#include <wx/utils.h>

#include "AddFileThread.h"	// Interface declarations
#include "StringFunctions.h"	// Needed for unicode2char
#include "amule.h"			// Needed for theApp
#include "opcodes.h"		// Needed for TM_HASHTHREADFINISHED
#include "PartFile.h"		// Needed for CKnownFile and CPartFile
#include "CFile.h"			// Needed for CFile
#include "AICHSyncThread.h"	// Needed for CAICHSyncThread

#include <algorithm>

//! Max number of threads. Change if you have > 1 CPUs ;)
const unsigned int MAXTHREADCOUNT = 1;

const uchar default_zero_hash[16] = { 0x31, 0xD6, 0xCF, 0xE0, 0xD1, 0x6A, 0xE9, 0x31, 
												0xB7, 0x3C, 0x59, 0xD7, 0xE0, 0xC0, 0x89, 0xC0 };

/**
 * Container for queued files.
 */
struct QueuedFile
{
	//! Signifies if a thread is currently working on this file.
	bool				m_busy;
	//! The full path to the file.
	wxString			m_path;
	//! The name of the file.
	wxString			m_name;
	//! The PartFile owning this file in case of a final hashing (completing).
	const CPartFile*	m_owner;
};


wxMutex CAddFileThread::s_running_lock;
wxMutex CAddFileThread::s_count_lock;
wxMutex CAddFileThread::s_queue_lock;


std::list<QueuedFile*> CAddFileThread::s_queue;
bool CAddFileThread::s_running;
uint8 CAddFileThread::s_count;


CAddFileThread::CAddFileThread()
	: wxThread(wxTHREAD_DETACHED)
{
	// Ensure that the AICH thread isn't running
	if ( CAICHSyncThread::IsRunning() )
		CAICHSyncThread::Stop();
}


CAddFileThread::~CAddFileThread()
{
	ThreadCountDec();
}


bool CAddFileThread::IsRunning()
{
	wxMutexLocker lock( s_running_lock );
	return s_running;
}


void CAddFileThread::SetRunning(bool running)
{
	wxMutexLocker lock( s_running_lock );
	s_running = running;
}


uint8 CAddFileThread::GetThreadCount()
{
	wxMutexLocker lock( s_count_lock );
	return s_count;
}


void CAddFileThread::ThreadCountInc()
{
	wxMutexLocker lock( s_count_lock );
	s_count++;
}


void CAddFileThread::ThreadCountDec()
{
	s_count_lock.Lock();

	if ( s_count ) {
		// Decrement and make a copy so we can unlock immediatly
		uint8 count = --s_count;
		s_count_lock.Unlock();

		// No threads left? Then let it be known.
		if ( count == 0 ) {
			wxMuleInternalEvent evt(wxEVT_CORE_FILE_HASHING_SHUTDOWN);
			wxPostEvent(&theApp, evt);
		}
	} else {
		// Just unlock
		s_count_lock.Unlock();

		// Some debug info
		printf("Hasher: Warning, ThreadCountDec() called while thread count was zero!\n");
	}
}


void CAddFileThread::CreateNewThread()
{
	printf("Hasher: Creating new thread.\n");

	ThreadCountInc();

	CAddFileThread* th = new CAddFileThread();
	wxThreadError create_error;
	if ((create_error = th->Create()) != wxTHREAD_NO_ERROR) {
		// Error!
		if (create_error == wxTHREAD_RUNNING) {
			printf("Attempt to create a already running thread!\n");
		} else if (create_error == wxTHREAD_NO_RESOURCE) {
			printf("Attempt to create a thread without resources!\n");
		} else {
			printf("Unkown error attempting to create a thread!\n");
		}
	}

	// The threads shouldn't be hugging the CPU, as it will already be hugging the HD
	th->SetPriority(WXTHREAD_MIN_PRIORITY);
	
	th->Run();
}


void CAddFileThread::Start()
{
	if ( IsRunning() ) {
		printf("Hasher: Warning, Start() called while already running.\n");
	} else {
		SetRunning(true);

		// No threads are running yet, so all files are non-busy.
		uint32 files = GetFileCount();
		// We wont start on more files than the max number of threads.
		if ( files > MAXTHREADCOUNT )
			files = MAXTHREADCOUNT;

		// Start as many threads as we can and as needed
		for ( ; files; files-- ) {
			CreateNewThread();
		}
	}
}


void CAddFileThread::Stop()
{
	if ( IsRunning() ) {
		// Are there any threads to kill?
		if ( GetThreadCount() ) {		
			printf("Hasher: Signaling for remaining threads to terminate.\n");
		
			SetRunning( false );

			// Wait for all threads to die
			while ( GetThreadCount() ) {
				// Sleep for 1/100 of a second to avoid clobbering the mutex
				// By doing this we ensure that this function only returns
				// once the thread has died.

				otherfunctions::MilliSleep(10);
				
			}
			
			printf("Hasher: Threads terminated.\n");
		} else {
			// No threads to kill, just stay quiet.
			SetRunning( false );
		}
	} else {
		printf("Hasher: Warning, attempted to stop already stopped hasher!\n");
	}
}


int	CAddFileThread::GetFileCount()
{
	wxMutexLocker lock( s_queue_lock );
	return s_queue.size();
}


QueuedFile* CAddFileThread::GetNextFile()
{
	wxMutexLocker lock( s_queue_lock );

	QueuedFile* file = NULL;

	// Get the first non-busy file
	FileQueue::iterator it = s_queue.begin();
	for ( ; it != s_queue.end(); ++it ) {
		if ( !(*it)->m_busy ) {
			file = (*it);
			// Mark the file as busy to avoid having multiple threads working on it
			file->m_busy = true;
			break;					
		}
	}
			
	return file;
}


void CAddFileThread::RemoveFromQueue(QueuedFile* file)
{
	// Some debugging information
	if ( !file ) {
		printf("Hasher: Error, tried to pass NULL pointer to RemoveFromQueue().\n");
		return;
	}

	// Some moredebugging information
	if ( !file->m_busy ) {
		printf("Hasher: Warning, non-busy file passed to RemoveFromQueue().\n");
	}

	// Remove a queued file which has been completed
	FileQueue::iterator it = std::find( s_queue.begin(), s_queue.end(), file );
			
	if ( it != s_queue.end() ) {
		s_queue.erase( it );
	} else {
		// Some debug info
		printf("Hasher: Warning, attempted to remove file from queue, but didn't find it.\n");
	}
			
	delete file;
}



void CAddFileThread::AddFile(const wxString& path, const wxString& name, const CPartFile* part)
{
	QueuedFile* hashfile = new QueuedFile;
	hashfile->m_busy = false;
	hashfile->m_path = path;
	hashfile->m_name = name;
	hashfile->m_owner = part;


	// New scope so that I can use wxMutexLocker ;) 
	{
		wxMutexLocker lock( s_queue_lock );

		// Avoid duplicate files
		FileQueue::iterator it = s_queue.begin();
		for ( ; it != s_queue.end(); ++it )
			if ( ( name == (*it)->m_name ) && ( path == (*it)->m_path ) )
				return;

		
		// If it's a partfile (part != NULL), then add it to the front so that
		// it gets hashed sooner, otherwise add it to the back
		if ( part == NULL ) {
			s_queue.push_back( hashfile );
		} else {
			s_queue.push_front( hashfile );
		}
	}


	// Should we start another thread?
	if ( GetThreadCount() < MAXTHREADCOUNT )
		CreateNewThread();
}


wxThread::ExitCode CAddFileThread::Entry()
{
	// Pointer to the queued file currently being hashed
	QueuedFile* current = NULL;
	// Pointer to the known-file used to store the results
	CKnownFile* knownfile = NULL;

	// Continue to loop until there's nothing to do, or someone kills the threads
	while ( IsRunning() ) {
		// Try to get the next file on queue
		current = GetNextFile();
			
		if ( !current ) {
			// Nothing to do, break
			printf("Hasher: No files on queue, stopping thread.\n");
			break;
		}
		

		wxString filename = current->m_path + wxFileName::GetPathSeparator() + current->m_name;
		
		// The file currently getting hashed
		CFile file;
	
		// Attempt to open the file
		if ( !file.Open( filename, CFile::read ) ) {
			printf("Hasher: Warning, failed to open file, skipping: %s\n", unicode2char(filename));
			
			// Whoops, something's wrong. Delete the item and continue
			RemoveFromQueue( current );
			continue;
		}


		// We only support files <= 4gigs
		if ( (uint64)file.GetLength() >= (uint64)(4294967295U) ) {
			printf("Hasher: Warning, file is bigger than 4GB, skipping: %s\n", unicode2char(filename));
			
			// Delete and continue 
			RemoveFromQueue( current );
			continue;
		}


		// Create a CKnownFile to contain the result
		knownfile = new CKnownFile();
		
		// Set initial values
		knownfile->m_strFilePath = current->m_path;
		knownfile->m_strFileName = current->m_name;
		
		knownfile->SetFileSize( file.GetLength() );
		knownfile->date = GetLastModificationTime( filename );
		knownfile->m_AvailPartFrequency.Insert(0, 0, knownfile->GetPartCount() );
	
		
		// We can only allow ourselves to create AICH hashsets for knownfiles.
		// This is because we do not know yet if a partfile was uncorrupted.
		bool needsAICH = !current->m_owner;
		bool error = false;
		
		
		// Prepare the AICH hashset if needed
		if ( needsAICH ) {
			knownfile->GetAICHHashset()->FreeHashSet();
		
			printf("Hasher: Starting to create MD4 and AICH hash for file: %s\n", unicode2char(current->m_name));
		} else {
			printf("Hasher: Starting to create MD4 hash for file: %s\n", unicode2char(current->m_name));
		}
		
		
		// This loops creates the part-hashes, loop-de-loop.
		while ( !error && ( file.GetPosition() < file.GetLength() ) && IsRunning() ) {
			error = !CreateNextPartHash( &file, knownfile, needsAICH );	
		}


		// Checking if something went wrong
		if ( error ) {
			printf("Hasher: Error while reading file, skipping: %s\n", unicode2char(current->m_name));
		
			// Remove the temp-result
			delete knownfile;
			
			// Delete and continue 
			RemoveFromQueue( current );
			continue;
		}
		


		if ( IsRunning() ) {
			// If the file is < PARTSIZE, then the filehash is that one hash,
			// otherwise, the filehash is the hash of the parthashes
			if ( knownfile->hashlist.GetCount() == 1 ) {
				knownfile->m_abyFileHash = knownfile->hashlist[0];
				knownfile->hashlist.Clear();
			} else {
				unsigned int len = knownfile->hashlist.GetCount() * 16;
				byte* data = new byte[ len ];
				
				for (size_t i = 0; i < knownfile->hashlist.GetCount(); i++) {
					memcpy( data + 16*i, knownfile->hashlist[i], 16 );
				}
	
				byte hash[16];
	
				knownfile->CreateHashFromString( data, len, hash, NULL );
				delete [] data;

				knownfile->m_abyFileHash.SetHash( (uchar*)hash );
			}
			
			
			// Did we create a AICH hashset?
			if ( needsAICH ) {
				CAICHHashSet* m_pAICHHashSet = knownfile->GetAICHHashset();

				m_pAICHHashSet->ReCalculateHash(false);
				if ( m_pAICHHashSet->VerifyHashTree(true) ) {
					m_pAICHHashSet->SetStatus(AICH_HASHSETCOMPLETE);
		
					if ( !m_pAICHHashSet->SaveHashSet() ) {
						printf("Hasher: Warning, failed to save AICH hashset for file: %s\n", unicode2char(current->m_name));
					}
				}
			}
			
			
			// Pass on the completion
			wxMuleInternalEvent evt(wxEVT_CORE_FILE_HASHING_FINISHED);
			evt.SetClientData( knownfile );
			evt.SetExtraLong( (long)current->m_owner );

			printf("Hasher: Finished hashing file: %s\n", unicode2char(current->m_name));
			
			RemoveFromQueue( current );
			wxPostEvent(&theApp, evt);
		}
	}

	// Ensure consistancy so we can start properly after a Stop()
	if ( current ) {
		delete knownfile;

		// Reset the busy-flag so the file will be hashed again
		current->m_busy = false;
	}
	
	
	// Notify that the thread has died
	printf("Hasher: A thread has died.\n");


	return 0;
}


bool CAddFileThread::CreateNextPartHash( CFile* file, CKnownFile* owner, bool createAICH )
{
	// Well read a PARTSIZE bytes per cycle
	uint32 cur_length = PARTSIZE;
	if ( file->GetPosition() + PARTSIZE > file->GetLength() ) {
		// Less than PARTSIZE left, reduce read-length		
		cur_length = file->GetLength() - file->GetPosition();
	}
	
	// Is this file EXACTLY (n * PARTSIZE) long?
	bool zero_hash = ((file->GetPosition() + PARTSIZE) == file->GetLength());

	// Sanity check, this shoulnd't happen due to the loop calling this function, but ...
	if ( cur_length == 0 ) {
		printf("Hasher: Warning, EOF in CreateNextPartHash!\n");
	
		return false;
	}

	byte* data = new byte[cur_length];
		
	// Check for read errors
	if ( file->Read(data, cur_length) != cur_length ) {
		delete[] data;
		
		return false;
	}


	// The MD4 hash
	byte hash[16];
	// The AICH hash
	CAICHHashTree* pBlockAICHHashTree = NULL;

	// Obtain the AICH hash if wanted 
	if ( createAICH ) {
		// Get the start-position of the current part
		uint32 position = file->GetPosition() - cur_length;

		pBlockAICHHashTree = owner->GetAICHHashset()->m_pHashTree.FindHash( position, cur_length);		
	}


	// Create the md4 hash and perhaps a AICH hash
	owner->CreateHashFromString( data, cur_length, hash, pBlockAICHHashTree );
	
	// Store the md4 hash
	owner->hashlist.Add( (uchar*)hash );

	
	// Kry This is because of the ed2k implementation for parts. A 2 * PARTSIZE 
	// file i.e. will have 3 parts (check CKnownFile::SetFileSize for comments). 
	// So we have to create the hash for the 0-size data, which will be the default
	// md4 hash for null data: 31D6CFE0D16AE931B73C59D7E0C089C0	
	if ( zero_hash ) {
		owner->hashlist.Add( default_zero_hash );
	}

	delete[] data;

	return true;
}
