//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/event.h>		// Needed for wxCommandEvent
#include <wx/timer.h>		// Needed for wxStopWatch
#include <wx/arrstr.h>		// Needed for wxArrayString
#include <wx/filename.h>	// Needed for wxFileName::GetPathSeperator()
#include <wx/utils.h>

#include "AddFileThread.h"	// Interface declarations
#include <common/StringFunctions.h>	// Needed for unicode2char
#include "amule.h"		// Needed for theApp
#include "OPCodes.h"		// Needed for TM_HASHTHREADFINISHED
#include "PartFile.h"		// Needed for CKnownFile and CPartFile
#include "CFile.h"		// Needed for CFile
#include "FileFunctions.h" // Needed for GetLastModificationTime()
#include "AICHSyncThread.h"	// Needed for CAICHSyncThread
#include "Logger.h"		// Needed for AddLogLine
#include <common/Format.h>
#include "InternalEvents.h"	// Needed for wxMuleInternalEvent

#include <algorithm>
#include <list>

const byte default_zero_hash[16] = { 0x31, 0xD6, 0xCF, 0xE0, 0xD1, 0x6A, 0xE9, 0x31, 
												0xB7, 0x3C, 0x59, 0xD7, 0xE0, 0xC0, 0x89, 0xC0 };



//! The mutex used to protect variables and datastructures used by the thread.
static wxMutex		s_mutex;

//! Is the hasher active. Does not mean that there are any threads running.
static bool			s_running;

//
static CAddFileThread*		s_thread;


/**
 * Container for queued files.
 */
struct QueuedFile
{
	//! The full path to the file.
	wxString			m_path;
	//! The name of the file.
	wxString			m_name;
	//! The PartFile owning this file in case of a final hashing (completing).
	const CPartFile*	m_owner;
};


//! The queue-type
typedef std::list<QueuedFile> FileQueue;
//! The queue of files to be hashed
static FileQueue s_queue;




CAddFileThread::CAddFileThread()
	: wxThread(wxTHREAD_DETACHED)
{
}


bool CAddFileThread::IsRunning()
{
	wxMutexLocker lock( s_mutex );
	return s_running;
}


void CAddFileThread::CreateNewThread()
{
	if ( !s_thread && !s_queue.empty() ) {
		// Ensure that the AICH thread isn't running
		if ( CAICHSyncThread::IsRunning() ) {
			CAICHSyncThread::Stop();
		}
		
		AddLogLineM( false, _("Hasher: Creating new thread.") );
		
		s_thread = new CAddFileThread();

		wxThreadError state = s_thread->Create();
		if (state != wxTHREAD_NO_ERROR) {
			AddDebugLogLineM(true, logHasher, wxString::Format(wxT("Error while creating file-hashing thread: %d"), state));
		
			// Delete() must be called in this case acording to the docs.
			s_thread->Delete();
			delete s_thread;
			s_thread = NULL;
		}
		
		// The threads shouldn't be hugging the CPU, as it will already be hugging the HD
		s_thread->SetPriority(WXTHREAD_MIN_PRIORITY);
		
		state = s_thread->Run();
		if (state != wxTHREAD_NO_ERROR) {
			AddDebugLogLineM(true, logHasher, wxString::Format(wxT("Error while attempting to run file-hashing thread: %d"), state));
			
			// Delete() must be called in this case acording to the docs.
			s_thread->Delete();
			delete s_thread;
			s_thread = NULL;
		}		
	}	
}


void CAddFileThread::Start()
{
	wxASSERT( wxThread::IsMain() );
	
	wxMutexLocker lock( s_mutex );

	if ( s_running ) {
		AddDebugLogLineM( true, logHasher, wxT("Warning, Start() called while already running.") );		
	} else {
		s_running = true;

		// No threads are running yet, so all files are non-busy.
		CreateNewThread();		
	}
}


void CAddFileThread::Stop()
{
	wxASSERT( wxThread::IsMain() );

	{
		wxMutexLocker lock(s_mutex);
	
		if ( !s_running ) {
			AddDebugLogLineM( true, logHasher, wxT("Warning, Attempted to stop already stopped hasher!") );
			return;
		}
	
		s_running = false;
	}
	
	if ( s_thread ) {
		AddLogLineM( false, _("Hasher: Signaling for remaining threads to terminate.") );
		
		// We will be blocking the main thread, so we need to leave the
		// gui mutex, so that events can still be processed while we are
		// waiting.
		wxMutexGuiLeave();
		
		// Wait for all threads to die
		while ( s_thread ) {
			// Sleep for 1/100 of a second to avoid clobbering the mutex
			MilliSleep(10);

			// Flush any pending log-entries caused by the thread.
			CLogger::FlushPendingEntries();
		}

#ifdef __WXGTK__
		// Re-claim the GUI mutex.
		wxMutexGuiEnter();
#endif		
	}
}


int	CAddFileThread::GetFileCount()
{
	wxMutexLocker lock( s_mutex );
	return s_queue.size();
}


void CAddFileThread::AddFile(const wxString& path, const wxString& name, const CPartFile* part)
{
	AddDebugLogLineM( false, logHasher, wxString( wxT("Adding file to queue: ") ) + path );
	
	wxMutexLocker lock( s_mutex );
	
	// Avoid duplicate files
	for ( FileQueue::iterator it = s_queue.begin(); it != s_queue.end(); ++it ) {
		if ( it->m_path == path && it->m_name == name ) {
			return;
		}
	}
	
	QueuedFile newfile = { path, name, part };
	
	// If it's a partfile (part != NULL), then add it to the front so that
	// it gets hashed sooner, otherwise add it to the back
	if ( part == NULL ) {
		s_queue.push_back( newfile );
	} else {
		s_queue.push_front( newfile );
	}

	
	// Check if we need to start an actual thread.
	CreateNewThread();
}


wxThread::ExitCode CAddFileThread::Entry()
{
	// The current file being hashed.	
	QueuedFile current;

	// Continue to loop until there's nothing to do, or someone kills the threads
	while ( IsRunning() ) {
		{
			wxMutexLocker lock(s_mutex);

			if ( s_queue.empty() ) {
				AddLogLineM( false, _("Hasher: No files on queue, stopping thread.") );
				break;
			}

			current = s_queue.front();
			s_queue.pop_front();
		}


		wxString filename = current.m_path + wxFileName::GetPathSeparator() + current.m_name;
		
		// The file currently getting hashed
		CFile file;
	
		// Attempt to open the file
		if ( !file.Open( filename, CFile::read ) ) {
			AddDebugLogLineM( true, logHasher, wxT("Warning, failed to open file, skipping: " ) + filename );
			continue;
		}


		// We only support files <= 4gigs
		if (file.GetLength() > MAX_FILE_SIZE) {
			AddDebugLogLineM( true, logHasher, wxT("Warning, file is bigger than 4GB, skipping: ") + filename );
			continue;
		}

		// Zero-size partfiles should be hashed, but not zero-sized shared-files.
		if (!file.GetLength() && !current.m_owner) {
			AddDebugLogLineM( true, logHasher, wxT("Warning, 0-size file, skipping: ") + filename );
			continue;			
		}
		

		// Create a CKnownFile to contain the result
		CKnownFile* knownfile = new CKnownFile();
		
		// Set initial values
		knownfile->m_strFilePath = current.m_path;
		knownfile->SetFileName(current.m_name);
		
		knownfile->SetFileSize( file.GetLength() );
		knownfile->date = GetLastModificationTime( filename );
		knownfile->m_AvailPartFrequency.Insert(0, 0, knownfile->GetPartCount() );
	
		
		// We can only create the AICH hashset if the file is a knownfile or 
		// if the partfile is complete, since the MD4 hashset is checked first,
		// so that the AICH hashset only gets assigned if the MD4 hashset 
		// matches what we expected. Due to the rareity of post-completion
		// corruptions, this gives us a nice speedup in almost all cases.
		bool needsAICH = !current.m_owner || current.m_owner->GetGapList().IsEmpty();
		bool error = false;
		
		
		// Prepare the AICH hashset if needed
		if ( needsAICH ) {
			knownfile->GetAICHHashset()->FreeHashSet();
		
			AddLogLineM( false, CFormat( _("Hasher: Starting to create MD4 and AICH hash for file: %s")) % current.m_name );
		} else {
			AddLogLineM( false, CFormat( _("Hasher: Starting to create MD4 hash for file: %s")) % current.m_name );
		}
		
		
		// This loops creates the part-hashes, loop-de-loop.
		while ( !error && ( file.GetPosition() < file.GetLength() ) && IsRunning() ) {
			error = !CreateNextPartHash( &file, knownfile, needsAICH );	
		}


		// Checking if something went wrong
		if ( error ) {
			AddDebugLogLineM( true, logHasher, wxT("Error while reading file, skipping: ") + current.m_name );
		
			// Remove the temp-result
			delete knownfile;
			
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
				byte data[len];
				
				for (size_t i = 0; i < knownfile->hashlist.GetCount(); i++) {
					memcpy( data + 16*i, knownfile->hashlist[i].GetHash(), 16 );
				}
	
				byte hash[16];
	
				knownfile->CreateHashFromString( data, len, hash, NULL );

				knownfile->m_abyFileHash.SetHash( (byte*)hash );
			}
			
			
			// Did we create a AICH hashset?
			if ( needsAICH ) {
				CAICHHashSet* m_pAICHHashSet = knownfile->GetAICHHashset();

				m_pAICHHashSet->ReCalculateHash(false);
				if ( m_pAICHHashSet->VerifyHashTree(true) ) {
					m_pAICHHashSet->SetStatus(AICH_HASHSETCOMPLETE);
		
					if ( !m_pAICHHashSet->SaveHashSet() ) {
						AddDebugLogLineM( true, logHasher, wxT("Warning, failed to save AICH hashset for file: ") + current.m_name );
					}
				}
			}
			
			
			// Pass on the completion
			wxMuleInternalEvent evt(wxEVT_CORE_FILE_HASHING_FINISHED);
			evt.SetClientData( knownfile );
			evt.SetExtraLong( (long)current.m_owner );

			AddLogLineM( false, CFormat( _("Hasher: Finished hashing file: %s")) % current.m_name );
			
			wxPostEvent(&theApp, evt);
		} else {
			delete knownfile;
		}
	}

	// Notify that the thread has died
	AddLogLineM( false, _("Hasher: A thread has died.") );

	
	{
		wxMutexLocker lock(s_mutex);
		s_thread = NULL;
	}


	// Notify the core.
	wxMuleInternalEvent evt(wxEVT_CORE_FILE_HASHING_SHUTDOWN);
	wxPostEvent(&theApp, evt);


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
		AddDebugLogLineM( true, logHasher, wxT("Warning, EOF in CreateNextPartHash!") );
	
		return false;
	}

	byte* data = new byte[cur_length];
		
	// Check for read errors
	try {
		file->Read(data, cur_length);
	} catch (const CIOFailureException& e) {
		delete[] data;
		AddDebugLogLineM(true, logHasher, wxT("IO failure while hashing file: ") + e.what());
		
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
	delete[] data;
	
	// Store the md4 hash
	owner->hashlist.Add( CMD4Hash(hash) );

	
	// Kry This is because of the ed2k implementation for parts. A 2 * PARTSIZE 
	// file i.e. will have 3 parts (check CKnownFile::SetFileSize for comments). 
	// So we have to create the hash for the 0-size data, which will be the default
	// md4 hash for null data: 31D6CFE0D16AE931B73C59D7E0C089C0	
	if ( zero_hash ) {
		owner->hashlist.Add( CMD4Hash(default_zero_hash) );
	}

	return true;
}
