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

#ifndef ADDFILETHREAD_H
#define ADDFILETHREAD_H

#include <wx/thread.h>		// Needed for wxThread

class CKnownFile;
class CPartFile;
class CFile;


/**
 * This class takes care of transparently hashing files in seperate threads and
 * throws events once a file has been completed. By default it only uses a 
 * single thread, however, this can be changed by incrementing the MAXTHREADCOUNT
 * constant in AddFileThread.cpp. Each thread works by reading a chunk of the 
 * file (PARTSIZE bytes, see AddFileThread.cpp) and hashing that. Once there 
 * are no more files to hash, the threads die.
 *
 * You need to call Start() before the class will start spawning new threads, 
 * however, it does not matter if you call it before or after adding the files.
 * To terminate the threads, simply call Stop().
 */
class CAddFileThread : private wxThread
{
public:
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
	 * @return The number of files in the queue.
	 */
	static int		GetFileCount();
	
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
	 * Constructor.
	 */
	CAddFileThread();
	
	/**
	 * Helper function that creates a new hashing-thread if needed.
	 */
	static void CreateNewThread();

	/**
	 * Helper function for hashing the next PARTSIZE chunk of a file.
	 *
	 * @param file The file to read from.
	 * @param owner The known- (or part) file representing that file.
	 * @bool createAICH Specifies if AICH hash-sets should be created as well.
	 * @return Returns false on read-errors, true otherwise.
	 *
	 * This function will create a MD4 hash and, if specified, a AICH hashset for 
	 * the next part of the file. This function makes the assumption that it wont
	 * be called for closed or EOF files.
	 */
	static bool CreateNextPartHash( CFile* file, CKnownFile* owner, bool createAICH );

	
	//! Main function
	virtual ExitCode 	Entry();
};

#endif // ADDFILETHREAD_H
