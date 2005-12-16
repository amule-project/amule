//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 Angel Vidal (Kry) ( kry@amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
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


	//! Mutex used to protect the s_tread variable.
	static	wxMutex s_mutex;

	//! Pointer to the currently running thread.
	static	CAICHSyncThread* s_thread;
};

#endif

