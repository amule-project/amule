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
#include <wx/timer.h>		// Needed for wxStopWatch

#include "amule.h"			// Needed for theApp
#include "AddFileThread.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for nstrdup
#include "opcodes.h"		// Needed for TM_HASHTHREADFINISHED
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "KnownFile.h"		// Needed for CKnownFile

struct UnknownFile_Struct
{
	wxString		name;
	wxString		directory;
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
	DeadThread = true;
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

	printf("Signaling hashing thread to terminate... \n");

	if (DeadThread || m_endWaitingForHashList) {
		printf("Already dead\n");
	} else {
		m_lockWaitingForHashList.Lock();
		m_endWaitingForHashList = 1;
		m_lockWaitingForHashList.Unlock();
		wxStopWatch aika;

		while (!DeadThread) {
			if (aika.Time() > 20000) {
				printf("\tTimed out hashing thread signal\n");
				break;
			}
		}
		printf("\nDone\n");
	}

	printf("Sending death event to the app\n");
	wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,TM_HASHTHREADFINISHED);
	wxPostEvent(&theApp,evt);

}

void CAddFileThread::AddFile(const wxString path, const wxString name, CPartFile* part)
{
	dwLastAddTime = GetTickCount();
	if (m_endWaitingForHashList) {
		Setup();
	}
	UnknownFile_Struct* hashfile = new UnknownFile_Struct;
	hashfile->directory = path;
	hashfile->name = name;
	hashfile->owner = part;

	wxMutexLocker sLock(m_lockWaitingForHashList);
	m_sWaitingForHashList.AddTail(hashfile);

}

wxThread::ExitCode CAddFileThread::Entry()
{

	while (!m_endWaitingForHashList) {

		  if (m_sWaitingForHashList.IsEmpty()) {
			  if ((GetTickCount() - dwLastAddTime) > THREAD_ADDING_TIMEOUT) {
				printf("Hashing thread timed out with no aditions - removing thread\n");
				m_lockWaitingForHashList.Lock();
				m_endWaitingForHashList = 1;
				m_lockWaitingForHashList.Unlock();
				wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,TM_HASHTHREADFINISHED);
				wxPostEvent(&theApp,evt);
			  } else {
				this->Yield();
				this->Sleep(1);
			  }
			  continue;
		  }

		m_lockWaitingForHashList.Lock();
		UnknownFile_Struct* hashfile = m_sWaitingForHashList.RemoveHead();
		m_lockWaitingForHashList.Unlock();

		CKnownFile* newrecord = new CKnownFile();
		printf("Sharing %s/%s\n",unicode2char(hashfile->directory),unicode2char(hashfile->name));

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
			wxPostEvent(&theApp,evt);
			dwLastAddTime = GetTickCount();
		}

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
