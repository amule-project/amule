//
// This file is part of the aMule Project.
//
// Copyright (c) 2006 Mikkel Schubert ( xaignar@amule.org / http://www.amule.org )
// Copyright (c) 2006 aMule Team ( admin@amule.org / http://www.amule.org )
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


#ifndef THREADSCHEDULER_H
#define THREADSCHEDULER_H

#include <wx/thread.h>

#include <deque>
#include <map>

#include "Types.h"


class CThreadTask;


//! The priority values of tasks.
enum ETaskPriority
{
	ETP_Low = 0,
	ETP_Normal,
	ETP_High,
	//! For tasks such as finding shared files and ipfilter.dat loading only.
	ETP_Critical
};


/**
 * This class mananges scheduling of background tasks.
 *
 * Currently it is assumed that all tasks are IO intensive,
 * so that only a single task is allowed to proceed at any
 * one time. All threads are run in lowest priority mode.
 * 
 * Tasks are sorted by priority (see ETaskPriority) and age.
 *  
 * Note that the scheduler starts in suspended mode, in
 * which tasks are queued but not executed. Call Start()
 * to begin execution of the tasks.
 */
class CThreadScheduler
{
public:
	/** Starts execution of queued tasks. */
	static void Start();
	
	/**
	 * Terminates task execution and frees the scheduler object.
	 *
	 * Tasks added after this are discarded.
	 */
	static void Terminate();
	
	
	/**
	 * Adds a new task to the queue, returning true if the task was queued.
	 * 
	 * Before the task is queued, it is checked against the
	 * existing tasks based on type and description. If an
	 * matching task already exists, this task-object is
	 * discarded. The task is also discarded if the scheduler
	 * has been terminated. If 'overwrite' is true, any
	 * existing duplicate task is dropped, and if already
	 * running, terminated.
	 *
	 * Note: This function takes ownership of the task.
	 *
	 * @see Start
	 * @see Terminate
	 */
	static bool AddTask(CThreadTask* task, bool overwrite = false);

private:
	CThreadScheduler();
	~CThreadScheduler();

	/** Returns the number of tasks on the queue. */
	size_t GetTaskCount() const;
	
	/** Tries to add the given task to the queue, returning true on success. */
	bool DoAddTask(CThreadTask* task, bool overwrite);
	
	/** Creates the actual scheduler thread if none exist. */
	void CreateSchedulerThread();

	/** Entry function called via internal thread-object. */
	void* Entry();
		
	//! Contains a task and its age.
	typedef std::pair<CThreadTask*, uint32> CEntryPair;
	
	//! List of currently scheduled tasks. 
	std::deque<CEntryPair> m_tasks;

	//! Specifies if tasks should be resorted by priority.
	bool	m_tasksDirty;
	
	typedef std::map<wxString, CThreadTask*> CDescMap;
	typedef std::map<wxString, CDescMap> CTypeMap;
	//! Map of current task by type -> desc. Used to avoid duplicate tasks.
	CTypeMap m_taskDescs;

	//! The actual worker thread.
	wxThread* m_thread;
	//! The currently running task, if any.
	CThreadTask* m_currentTask;
	
	friend class CTaskThread;
	friend struct CTaskSorter;
};


/**
 * Base-class of all threaded tasks.
 *
 * This class acts as a pseudo-thread, and is transparently
 * executed on a worker thread by the CThreadScheduler
 * class.
 *
 * Note that the task type should be an unique description
 * of the task type, as it is used to detect completion of
 * all tasks of a given type and in duplicate detection
 * with the description. The description should be unique
 * for the given task, such that duplicates can be discovered.
 */
class CThreadTask
{
public:
	/**
	 * @param type Should be a name constant among tasks of the type (hashing, completion, etc).
	 * @param desc Should be an unique description for this task, for detecting duplicates.
	 * @param priority Decides how soon the task will be carried out.
	 */	
	CThreadTask(const wxString& type, const wxString& desc, ETaskPriority priority = ETP_Normal);
	
	/** Needed since CThreadScheduler only works with CThreadTask pointers. */
	virtual ~CThreadTask();

	/** Returns the task type, used for debugging and duplicate detection. */
	const wxString& GetType() const;
	
	/** Returns the task description, used for debugging and duplicate detection. */
	const wxString& GetDesc() const;

	/** Returns the priority of the task. Used when selecting the next task. */
	ETaskPriority GetPriority() const;
	
protected:
	//! @see wxThread::Entry
	virtual void Entry() = 0;
	
	/** Called when the last task of a specific type has been completed. */
	virtual void OnLastTask();

	/** @see wxThread::OnExit */
	virtual void OnExit();

	/** @see wxThread::TestDestroy */
	bool TestDestroy() const;
	
private:
	wxString m_type;
	wxString m_desc;
	ETaskPriority m_priority;

	//! The owner (scheduler), used when calling TestDestroy.
	wxThread* m_owner;
	//! Specifies if the specifc task should be aborted.
	bool m_abort;
	
	friend class CThreadScheduler;
};

#endif
