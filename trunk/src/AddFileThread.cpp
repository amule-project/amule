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

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/event.h>		// Needed for wxCommandEvent
#include <sys/time.h>		// Needed for gettimeofday

#include "AddFileThread.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for nstrdup
#include "opcodes.h"		// Needed for TM_HASHTHREADFINISHED
#include "amule.h"			// Needed for theApp
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "KnownFile.h"		// Needed for CKnownFile

struct UnknownFile_Struct
{
	char*		name;
	char*		directory;
	CPartFile*	owner;
};

wxMutex CAddFileThread::m_lockWaitingForHashList;
CTypedPtrList<CPtrList, UnknownFile_Struct*> CAddFileThread::m_sWaitingForHashList;
volatile int CAddFileThread::m_endWaitingForHashList;

uint32 CAddFileThread::dwLastAddTime;
bool CAddFileThread::DeadThread;

CAddFileThread::CAddFileThread() : wxThread(wxTHREAD_DETACHED)
{
	dwLastAddTime = GetTickCount();
	m_endWaitingForHashList = 0;
}

void CAddFileThread::Setup()
{
	dwLastAddTime = GetTickCount();
	m_endWaitingForHashList = 0;
	DeadThread = false;
	CAddFileThread* th = new CAddFileThread();
	th->Create();
	th->Run();
}

void CAddFileThread::Shutdown()
{

	printf("Signaling hashing thread to terminate... ");

	if (DeadThread || m_endWaitingForHashList) {
		printf("Already dead\n");
	} else {		
		m_lockWaitingForHashList.Lock();
		m_endWaitingForHashList = 1;
		m_lockWaitingForHashList.Unlock();
		struct timeval aika;
		gettimeofday(&aika,NULL);
		long secs = aika.tv_sec;

		while (!DeadThread) {
			gettimeofday(&aika,NULL);
			if (aika.tv_sec > (secs + 20)) {
				printf("Timed out hashing thread signal\n");
				break;
			}
		}
		printf("OK\n");
	}

	printf("Sending death event to main dialog\n");
	wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,TM_HASHTHREADFINISHED);
	wxPostEvent(theApp.amuledlg,evt);		

}

void CAddFileThread::AddFile(const char *path, const char *name, CPartFile* part)
{
	dwLastAddTime = GetTickCount();
	if (m_endWaitingForHashList) {
		Setup();
	}
	UnknownFile_Struct* hashfile = new UnknownFile_Struct;
	hashfile->directory = nstrdup(path);
	hashfile->name = nstrdup(name);
	hashfile->owner = part;

	wxMutexLocker sLock(m_lockWaitingForHashList);
	m_sWaitingForHashList.AddTail(hashfile);

}

wxThread::ExitCode CAddFileThread::Entry()
{
	
	while (!m_endWaitingForHashList) {
	
		if (theApp.amuledlg) {
			// Sanity check
		    if (theApp.amuledlg->m_app_state == APP_STATE_SHUTINGDOWN) {
			    printf("App is shutting down, hashing thread died\n");		    	
				break;
		    }		    
		}
		   
		  if (m_sWaitingForHashList.IsEmpty()) {
			  if ((GetTickCount() - dwLastAddTime) > THREAD_ADDING_TIMEOUT) {
				printf("Hashing thread timed out with no aditions - removing thread\n");
				m_lockWaitingForHashList.Lock();
				m_endWaitingForHashList = 1;
				m_lockWaitingForHashList.Unlock();
				wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,TM_HASHTHREADFINISHED);
				wxPostEvent(theApp.amuledlg,evt);
			  } else {
				wxYield();
				wxSleep(1);
			  }				  
			  continue;	  
		  }
			
		m_lockWaitingForHashList.Lock();
		UnknownFile_Struct* hashfile = m_sWaitingForHashList.RemoveHead();
		m_lockWaitingForHashList.Unlock();
	
		CKnownFile* newrecord = new CKnownFile();
		printf("Sharing %s/%s\n",hashfile->directory,hashfile->name);
	
		// TODO: What we are supposed to do if the following does fail?
		// Kry - Exit, afaik
		bool finished = newrecord->CreateFromFile(hashfile->directory,hashfile->name,&m_endWaitingForHashList);
		
		if (!finished) {
			// Kry -Hashing thread interrupted
			m_lockWaitingForHashList.Lock();
			m_endWaitingForHashList = 1;
			m_lockWaitingForHashList.Unlock();
			continue;
		}
		
		if (!m_endWaitingForHashList) {
			wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,TM_FINISHEDHASHING);
			evt.SetClientData(newrecord);
			evt.SetExtraLong((long)hashfile->owner);
			wxPostEvent(theApp.amuledlg,evt);
			dwLastAddTime = GetTickCount();
		}
	
		delete[] hashfile->name;
		delete[] hashfile->directory;
		delete hashfile;
    }
    
    // Just to be sure
	m_lockWaitingForHashList.Lock();
	m_endWaitingForHashList = 1;
	m_lockWaitingForHashList.Unlock();
	DeadThread = true;
	return 0;
}

int CAddFileThread::GetCount()
{
	return m_sWaitingForHashList.GetCount();
}
