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

#include "CTypedPtrList.h"	// Needed for CTypedPtrList
#include "types.h"
#include "GetTickCount.h"

#define THREAD_ADDING_TIMEOUT	60000 // 1 min timeout

class CPartFile;
class UnknownFile_Struct;

class CAddFileThread : protected wxThread
{
public:
	CAddFileThread();

	static void		Setup();
	static void		Shutdown();

	static void		AddFile(const char *path, const char *name, CPartFile* = NULL);
	static int		GetCount();
	static bool		IsRunning() { return !DeadThread; }

protected:
	virtual	bool		InitInstance()
		{return true;}
	virtual wxThread::ExitCode 	Entry();
	virtual void 		OnExit()
		{}

private:
	wxThread*		m_Thread;

	// Setted to non-zero to end the thread
	static volatile int	m_endWaitingForHashList;

	// Lock for the wait list and the conditions
	static wxMutex 		m_lockWaitingForHashList;

	static bool DeadThread;

	static uint32 dwLastAddTime;

	// The wait list itself
	static CTypedPtrList<CPtrList, UnknownFile_Struct*>
				m_sWaitingForHashList;
};

#endif // ADDFILETHREAD_H
