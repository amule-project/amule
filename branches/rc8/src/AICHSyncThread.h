//
// This file is part of aMule Project.
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

#ifndef __AICHSYNCTHREAD_H__
#define __AICHSYNCTHREAD_H__


#include <wx/thread.h>


class CKnownFile;


/**
 * This thread takes care of updating the known2.met file.
 * 
 * It should never be run while the CAddFileThread is running.
 */
class CAICHSyncThread : public wxThread
{
public:
	/**
	 * Starts the AICH thread if it isn't running.
	 *
	 * @return Returns true if the thread was started, false otherwise.	 
	 */
	static bool Start();

	/**
	 * Stops the AICH thread if it is running.
	 *
	 * @return Returns true if the thread was stopped, false otherwise.
	 *
	 * This function only returns once the AICH thread has been terminated.
	 */
	static bool Stop();

	/**
	 * Specifies the current state of the thread.
	 *
	 * @return Returns true if the thread is running, false otherwise.
	 */
	static bool IsRunning();

private:
	/**
	 * Constructor.
	 *
	 * Creates a detached thread.
	 */
	CAICHSyncThread();

	/**
	 * Destructor.
	 */
	~CAICHSyncThread();

	/**
	 * Main function.
	 */
	virtual void* Entry();


	/**
	 * Helper-function which returns the current thread or NULL.
	 */
	static CAICHSyncThread* GetThread();

	/**
	 * Helper-function used to set the currently running thread to avoid multiple threads.
	 */
	static void SetThread( CAICHSyncThread* ptr );


	//! Mutex used to protect the s_tread variable.
	static	wxMutex s_thread_mutex;

	//! Pointer to the currently running thread.
	static	CAICHSyncThread* s_thread;
};

#endif

