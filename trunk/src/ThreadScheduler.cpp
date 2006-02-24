//
// This file is part of the aMule Project.
//
// Copyright (c) 2006 Mikkel Schubert ( xaignar@amule.org / http://www.amule.org )
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

#include "ThreadScheduler.h"	// Interface declarations
#include "Logger.h"				// Needed for Add(Debug)LogLineM
#include "GetTickCount.h"		// Needed for GetTickCount
#include <common/Format.h>		// Needed for CFormat
#include "ScopedPtr.h"			// Needed for CScopedPtr

#include <algorithm>


//! Global lock the scheduler and its thread.
static wxMutex s_lock;
//! Pointer to the global scheduler instance (automatically instantiated).
static CThreadScheduler* s_scheduler = NULL;
//! Specifies if the scheduler is running.
static bool	s_running = false;
//! Specifies if the gobal scheduler has been terminated.
static bool s_terminated = false;


/**
 * This class is used in a custom implementation of wxThreadHelper.
 *
 * The reason for not using wxThreadHelper are as follows:
 *  - wxThreadHelper makes use of wxThread:Kill, which is warned against
 *    serveral times in the docs, and even calls it in its destructor.
 *  - Managing the thread-object is difficult, since the only way to 
 *    destroy it is to create a new thread.
 */
class CTaskThread : public wxThread
{
public:
	CTaskThread(CThreadScheduler* owner)
		: wxThread(wxTHREAD_JOINABLE),
		  m_owner(owner)
	{
	}

	//! For simplicity's sake, all code is placed in CThreadScheduler::Entry
	void* Entry() {
		return m_owner->Entry();
	}

private:
	// The scheduler owning this thread.
	CThreadScheduler* m_owner;
};


void CThreadScheduler::Start()
{
	wxMutexLocker lock(s_lock);

	s_running = true;
	s_terminated = false;

	// Ensures that a thread is started if tasks are already waiting.
	if (s_scheduler) {
		AddDebugLogLineM(false, logThreads, wxT("Starting scheduler"));
		s_scheduler->CreateSchedulerThread();
	}
}


void CThreadScheduler::Terminate()
{
	AddDebugLogLineM(false, logThreads, wxT("Terminating scheduler"));
	CThreadScheduler* ptr = NULL;

	{	
		wxMutexLocker lock(s_lock);
		
		// Safely unlink the scheduler, as to avoid race-conditions.
		ptr = s_scheduler;
		s_running = false;
		s_terminated = true;
		s_scheduler = NULL;
	}
		
	delete ptr;
	AddDebugLogLineM(false, logThreads, wxT("Scheduler terminated"));
}


bool CThreadScheduler::AddTask(CThreadTask* task)
{
	wxMutexLocker lock(s_lock);

	// When terminated (on shutdown), all tasks are ignored.
	if (s_terminated) {
		AddDebugLogLineM(false, logThreads, wxT("Task discared: ") + task->GetDesc());
		delete task;
		return false;
	} else if (s_scheduler == NULL) {
		s_scheduler = new CThreadScheduler();
		AddDebugLogLineM(false, logThreads, wxT("Scheduler created."));
	}

	return s_scheduler->DoAddTask(task);
}


/** Returns string representation of error code. */
wxString GetErrMsg(wxThreadError err)
{
	switch (err) {
		case wxTHREAD_NO_ERROR:		return wxT("wxTHREAD_NO_ERROR");
		case wxTHREAD_NO_RESOURCE:	return wxT("wxTHREAD_NO_RESOURCE");
		case wxTHREAD_RUNNING:		return wxT("wxTHREAD_RUNNING");
		case wxTHREAD_NOT_RUNNING:	return wxT("wxTHREAD_NOT_RUNNING");
		case wxTHREAD_KILLED:		return wxT("wxTHREAD_KILLED");
		case wxTHREAD_MISC_ERROR:	return wxT("wxTHREAD_MISC_ERROR");
		default:
			return wxT("Unknown error");
	}	
}


void CThreadScheduler::CreateSchedulerThread()
{
	if ((m_thread and m_thread->IsAlive()) || m_tasks.empty()) {
		return;
	}

	// A thread can only be run once, so the old one must be safely disposed of
	if (m_thread) {
		AddDebugLogLineM(false, logThreads, wxT("CreateSchedulerThread: Disposing of old thread."));
		m_thread->Delete();
		delete m_thread;
	}
	
	m_thread = new CTaskThread(this);

	wxThreadError err = m_thread->Create();
	if (err == wxTHREAD_NO_ERROR) {
		// Try to avoid reducing the latency of the main thread
		m_thread->SetPriority(WXTHREAD_MIN_PRIORITY);
		
		err = m_thread->Run();
		if (err == wxTHREAD_NO_ERROR) {
			AddDebugLogLineM(false, logThreads, wxT("Scheduler thread started"));
			return;
		} else {
			AddDebugLogLineM(true, logThreads, wxT("Error while starting scheduler thread: ") + GetErrMsg(err));
		}
	} else {
		AddDebugLogLineM(true, logThreads, wxT("Error while creating scheduler thread: ") + GetErrMsg(err));
	}
	
	// Creation or running failed.
	m_thread->Delete();
	delete m_thread;
	m_thread = NULL;
}


/** This is the sorter functor for the task-queue. */
struct CTaskSorter 
{
	bool operator()(const CThreadScheduler::CEntryPair& a, const CThreadScheduler::CEntryPair& b) {
		if (a.first->GetPriority() != b.first->GetPriority()) {
			return a.first->GetPriority() < b.first->GetPriority();
		}

		// Compare timestamps.
		return a.second < b.second;
	}
};



CThreadScheduler::CThreadScheduler()
	: m_tasksDirty(false),
	  m_thread(NULL)
{

}


CThreadScheduler::~CThreadScheduler()
{
	if (m_thread) {
		m_thread->Delete();
		delete m_thread;
	}
}


size_t CThreadScheduler::GetTaskCount() const
{
	wxMutexLocker lock(s_lock);

	return m_tasks.size();
}


bool CThreadScheduler::DoAddTask(CThreadTask* task)
{
	// Get the map for this task type, implicitly creating it as needed.
	CDescMap& map = m_taskDescs[task->GetType()];
	
	CDescMap::value_type entry(task->GetDesc(), task);
	if (map.insert(entry).second) {
		AddDebugLogLineM(false, logThreads, wxT("Task scheduled: ") + task->GetType() + wxT(" - ") + task->GetDesc());
		m_tasks.push_back(CEntryPair(task, GetTickCount()));
		m_tasksDirty = true;
	} else if (map[task->GetDesc()] != task) {
		AddDebugLogLineM(false, logThreads, wxT("Duplicate task, discarding: ") + task->GetType() + wxT(" - ") + task->GetDesc());
		delete task;
		return false;
	}

	if (s_running) {
		CreateSchedulerThread();
	}

	return true;
}


void* CThreadScheduler::Entry()
{
	AddDebugLogLineM(false, logThreads, wxT("Entering scheduling loop"));
	
	while (!m_thread->TestDestroy()) {
		CScopedPtr<CThreadTask> task(NULL);

		{
			wxMutexLocker lock(s_lock);	
			
			// Resort tasks by priority/age if list has been modified.
			if (m_tasksDirty) {
				AddDebugLogLineM(false, logThreads, wxT("Resorting tasks"));
				std::sort(m_tasks.begin(), m_tasks.end(), CTaskSorter());
				m_tasksDirty = false;
			} else if (m_tasks.empty()) {
				AddDebugLogLineM(false, logThreads, wxT("No more tasks, stopping"));
				break;
			}
			
			// Select the next task
			task.reset(m_tasks.front().first);
			m_tasks.pop_front();
		}

		AddDebugLogLineM(false, logThreads, wxT("Current task: ") + task->GetType() + wxT(" - ") + task->GetDesc());
		// Execute the task
		task->m_owner = m_thread;
		task->Entry();
		task->OnExit();
		
		AddLogLineM(false, logThreads,
			CFormat(wxT("Completed task '%s%s', %u tasks remaining.")) 
				% task->GetType()
				% (task->GetDesc().IsEmpty() ? wxString() : (wxT(" - ") + task->GetDesc()))
				% GetTaskCount() );
	
		// Check if this was the last task of this type
		bool isLastTask = false;
		
		{
			wxMutexLocker lock(s_lock);

			CDescMap& map = m_taskDescs[task->GetType()];
			if (!map.erase(task->GetDesc())) {
				wxASSERT(0);
			} else if (map.empty()) {
				m_taskDescs.erase(task->GetType());
				isLastTask = true;
			}
		}

		if (isLastTask) {
			// Allow the task to signal that all sub-tasks have been completed
			AddDebugLogLineM(false, logThreads, wxT("Last task, calling OnLastTask"));
			task->OnLastTask();
		}
	}
	
	AddDebugLogLineM(false, logThreads, wxT("Leaving scheduling loop"));

	return 0;
}



CThreadTask::CThreadTask(const wxString& type, const wxString& desc, ETaskPriority priority)
	: m_type(type),
	  m_desc(desc),
	  m_priority(priority),
	  m_owner(NULL)
{
}


CThreadTask::~CThreadTask()
{
}


void CThreadTask::OnLastTask()
{
	// Does nothing by default.
}


void CThreadTask::OnExit()
{
	// Does nothing by default.
}


bool CThreadTask::TestDestroy() const
{
	wxCHECK(m_owner, false);

	return m_owner->TestDestroy();
}


const wxString& CThreadTask::GetType() const
{
	return m_type;
}


const wxString& CThreadTask::GetDesc() const
{
	return m_desc;
}


ETaskPriority CThreadTask::GetPriority() const
{
	return m_priority;
}


