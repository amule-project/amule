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

#ifndef ADDFILETHREAD_H
#define ADDFILETHREAD_H

#include <wx/thread.h>		// Needed for wxThread

#include <list>				// Needed for std::list

#include "types.h"			// Needed for uints


class CPartFile;
struct QueuedFile;


/**
 * This class takes care of transparently hashing files in seperate threads and
 * throws events once a file has been completed. By default it only uses a 
 * single thread, however, this can be changed by incrementing the MAXTHREADCOUNT
 * constant in AddFileThread.cpp. Each thread works by reading a chunk of the 
 * file (CRUMBSIZE bytes, see AddFileThread.cpp) and hashing that. Once there 
 * are no more files to hash, the threads die.
 *
 * You need to call Start() before the class will start spawning new threads, 
 * however, it does not matter if you call it before or after adding the files.
 * To terminate the threads, simply call Stop().
 *
 *
 * Note:
 *  I have been very careful to decrease the locking time of this class in order
 *  to avoid dead-locks. Should you wish to change this class, please keep this 
 *  in mind.
 */
class CAddFileThread : private wxThread
{
public:
	CAddFileThread();

	/**
	 * Starts the hasher.
	 *
	 * This function does nothing but set if s_running variable unless there are
	 * already files on the queue. If there are files on the queue, then the 
	 * maximum number of threads allowed will be created and started.
	 */
	static void		Start();

	/**
	 * Stops the hasher and all threads.
	 *
	 * This function sets the s_running variable and waits for the currently 
	 * existing threads to die. It will only return after all running threads 
	 * have terminated. In most cases it should return almost immediatly.
	 */
	static void		Stop();


	/**
	 * @return The number of files in the s_queue list.
	 *
	 * Please note that this function only returns the number of files not being 
	 * hashes, as any files that are being hashes are immediatly removed from the 
	 * queue. Therefore, the number of files will be aproximatly GetFileCount() +
	 * GetThreadCount()
	 */
	static int		GetFileCount();
	
	/**
	 * Speficies the number of existing threads.
	 *
	 * @return The current number of existing threads. 
	 *
	 * It is probably safe to assume that this reflects the number of files being
	 * hashed, as threads with no files to hash die immediatly.
	 */
	static uint8	GetThreadCount();
	
	/**
	 * Specifies if the hasher is active.
	 *
	 * @return True if the hashes is active and threads are allowed to run.
	 */
	static bool		IsRunning(); 


	/**
	 * Adds a file to the hashing queue.
	 *
	 * @param path The full path of the file.
	 * @param name The filename.
	 * @param part The CPartFile object which the file belongs to. Used when verifying completed files.
	 */
	static void		AddFile(const wxString& path, const wxString& name, const CPartFile* part = NULL);

private:
	/**
	 * Helper function that creates another thread.
	 */
	static void CreateNewThread();

	/**
	 * Helper function which returns the next non-busy file or NULL.
	 *
	 * @return A viable file on the queue or NULL if the list is empty.
	 *
	 * Returns the first non-busy file on the queue and sets it's busy flag. 
	 * This allows multiple threads to work on the list and also to check 
	 * for duplicates when adding files.
	 */
	static QueuedFile* GetNextFile();

	/**
	 * Helper function for removing a file from the queue.
	 *
	 * @param file The object to be removed from to the queue.
	 *
	 * Please not that this function will also delete the pointer, so 
	 * do not attempt to delete it afterwards.
	 */
	static void RemoveFromQueue(QueuedFile* file);
	 

	//! Sets the IsRunning status
	static void		SetRunning(bool running);
	//! Increments thread count
	static void		ThreadCountInc();
	//! Decrements thread count
	static void		ThreadCountDec();
	
	//! Main function
	virtual ExitCode 	Entry();
	
	//! Lock for the s_running variable
	static wxMutex		s_running_lock;
	//! Lock for the thread count
	static wxMutex		s_count_lock;
	//! Lock for the queue
	static wxMutex 		s_queue_lock;

	//! Is the hasher active. Does not mean that there are any threads running.
	static bool			s_running;

	//! Number of currently existing threads.
	static uint8		s_count;

	//! The queue-type
	typedef std::list<QueuedFile*> FileQueue;
	//! The queue of files to be hashed
	static FileQueue s_queue;
};

#endif // ADDFILETHREAD_H
