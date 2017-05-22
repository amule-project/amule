//
// This file is part of the aMule Project.
//
// Copyright (c) 2006-2011 Mikkel Schubert ( xaignar@amule.org / http:://www.amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef TASKS_H
#define TASKS_H

#include "ThreadScheduler.h"
#include <common/Path.h>

class CKnownFile;
class CPartFile;
class CFileAutoClose;


/**
 * This task performs MD4 and/or AICH hashings of a file,
 * depending on the type. For new shared files (using the
 * first constructor, with part == NULL), both MD4 and
 * AICH hashes are created. For incomplete partfiles
 * (rehashed due to changed timestamps), only MD4 hashing
 * is done. For complete partfiles, both MD4 and AICH
 * hashing is done.
 *
 * For existing shared files (using the second constructor),
 * only an AICH hash is created.
 *
 * @see CHashingEvent
 * @see CAICHSyncTask
 */
class CHashingTask : public CThreadTask
{
public:
	/**
	 * Schedules a partfile or new shared file for hashing.
	 *
	 * @param path The full path, without filename.
	 * @param filename The actual filename.
	 * @param part Used to identify the owner in the event-handler (PartFiles only).
	 *
	 * CHashingEvents sent by this type of tasks have the id MULE_EVT_HASHING.
	 * @see EVT_MULE_HASHING
	 */
	CHashingTask(const CPath& path, const CPath& filename, const CPartFile* part = NULL);

	/**
	 * Schedules a KnownFile to have a AICH hashset created, used by CAICHSyncTask.
	 *
	 * CHashingEvents sent by this type of tasks have the id MULE_EVT_AICH_HASHING.
	 * @see EVT_MULE_AICH_HASHING
	 **/
	CHashingTask(const CKnownFile* toAICHHash);

protected:
	//! Specifies which hashes should be calculated when the task is executed.
	enum EHashes {
		EH_AICH = 1,
		EH_MD4 = 2
	};

	//! @see CThreadTask::OnLastTask
	virtual void OnLastTask();

	//! @see CThreadTask::Entry
	virtual void Entry();

	/**
	 * Helper function for hashing a PARTSIZE chunk of a file.
	 *
	 * @param file The file to read from.
	 * @param part The number of the part to hash.
	 * @param owner The known- (or part) file representing that file.
	 * @bool createAICH Specifies if AICH hash-sets should be created as well.
	 * @return Returns false on read-errors, true otherwise.
	 *
	 * This function will create a MD4 hash and, if specified, a AICH hashset for
	 * the next part of the file. This function makes the assumption that it wont
	 * be called for closed or EOF files.
	 */
	bool CreateNextPartHash(CFileAutoClose& file, uint16 part, CKnownFile* owner, EHashes toHash);


	//! The path to the file to be hashed (shared or part), without filename.
	CPath m_path;
	//! The filename of the file to be hashed (filename only).
	CPath m_filename;
	//! Specifies which hash-types should be calculated
	EHashes m_toHash;
	//! If a partfile or an AICH hashing, this pointer stores it for callbacks.
	const CKnownFile* m_owner;

private:
	void SetHashingProgress(uint16 part);
};


/**
 * This task synchronizes the AICH hashlist.
 *
 * Shared files that are lacking a AICH-hash are scheduled for hashing.
 */
class CAICHSyncTask : public CThreadTask
{
public:
	CAICHSyncTask();

protected:
	/** See CThreadTask::Entry */
	virtual void Entry();

	/** Converts old known2.met files to known2_64.met files. */
	bool ConvertToKnown2ToKnown264();
};


/**
 * This task performs the final tasks on a complete download.
 *
 * This includes finding a usable destination filename, removing
 * old data files and moving the part-file (potentially to a
 * different partition).
 **/
class CCompletionTask : public CThreadTask
{
public:
	/**
	 * Creates a thread which will complete the given download.
	 */
	CCompletionTask(const CPartFile* file);

protected:
	/** See CThreadTask::Entry */
	virtual void Entry();

	/** See CThreadTask::OnExit */
	virtual void OnExit();

	//! The target filename.
	CPath		m_filename;
	//! The full path to the .met-file
	CPath		m_metPath;
	//! The category of the download.
	uint8		m_category;
	//! Owner of the file, used when sending completion-event.
	const CPartFile*	m_owner;
	//! Specifies if an error occured during completion.
	bool		m_error;
	//! The resulting full path. File may be be renamed.
	CPath		m_newName;
};


/**
 * This task preallocates space for a newly created partfile.
 */
class CAllocateFileTask : public CThreadTask
{
      public:
	/** Creates a thread that will allocate disk space for the full file. */
	CAllocateFileTask(CPartFile *file, bool pause);

      protected:
	/** See CThreadTask::Entry */
	virtual void Entry();

	/** See CThreadTask::OnExit */
	virtual void OnExit();

      private:
	//! The partfile for which this task allocates space.
	CPartFile *	m_file;

	//! Should this download start paused?
	bool		m_pause;

	//! Result of the preallocation.
	long		m_result;
};


/**
 * This event is used to signal the completion of a hashing event.
 *
 * @see CHashingTask
 */
class CHashingEvent : public wxEvent
{
public:
	/**
	 * @param type MULE_EVT_HASHING or MULE_EVT_AICH_HASHING.
	 * @param result
	 */
	CHashingEvent(wxEventType type, CKnownFile* result, const CKnownFile* owner = NULL);

	/** @see wxEvent::Clone */
	virtual wxEvent* Clone() const;

	/** Returns the owner (may be NULL) of the hashing result. */
	const CKnownFile* GetOwner() const;
	/** Returns a CKnownfile used to store the results of the hashing. */
	CKnownFile* GetResult() const;

private:
	//! The file owner.
	const CKnownFile* m_owner;
	//! The hashing results.
	CKnownFile* m_result;
};


/**
 * This event is sent when a part-file has been completed.
 */
class CCompletionEvent : public wxEvent
{
public:
	/** Constructor, see getter funtion for description of parameters. */
	CCompletionEvent(bool errorOccured, const CPartFile* owner, const CPath& fullPath);

	/** @see wxEvent::Clone */
	virtual wxEvent* Clone() const;

	/** Returns true if completion failed. */
	bool ErrorOccured() const;

	/** Returns the owner of the file that was being completed. */
	const CPartFile* GetOwner() const;

	/** Returns the full path to the completed file (empty on failure). */
	const CPath& GetFullPath() const;
private:
	//! The full path to the completed file.
	CPath m_fullPath;

	//! The owner of the completed .part file.
	const CPartFile* m_owner;

	//! Specifies if completion failed.
	bool m_error;
};


/**
 * This event is sent when preallocation of a new partfile is finished.
 */
DECLARE_LOCAL_EVENT_TYPE(MULE_EVT_ALLOC_FINISHED, -1);
class CAllocFinishedEvent : public wxEvent
{
      public:
	/** Constructor, see getter function for description of parameters. */
	CAllocFinishedEvent(CPartFile *file, bool pause, long result)
		: wxEvent(-1, MULE_EVT_ALLOC_FINISHED),
		  m_file(file), m_pause(pause), m_result(result)
	{}

	/** @see wxEvent::Clone */
	virtual wxEvent *Clone() const;

	/** Returns the partfile for which preallocation was requested. */
	CPartFile *GetFile() const throw()	{ return m_file; }

	/** Returns whether the partfile should start paused. */
	bool	IsPaused() const throw()	{ return m_pause; }

	/** Returns the result of preallocation: true on success, false otherwise. */
	bool	Succeeded() const throw()	{ return m_result == 0; }

	/** Returns the result of the preallocation. */
	long	GetResult() const throw()	{ return m_result; }

      private:
	//! The partfile for which preallocation was requested.
	CPartFile *	m_file;

	//! Should the download start paused?
	bool		m_pause;

	//! Result of preallocation
	long		m_result;
};

DECLARE_LOCAL_EVENT_TYPE(MULE_EVT_HASHING, -1)
DECLARE_LOCAL_EVENT_TYPE(MULE_EVT_AICH_HASHING, -1)
DECLARE_LOCAL_EVENT_TYPE(MULE_EVT_FILE_COMPLETED, -1)


typedef void (wxEvtHandler::*MuleHashingEventFunction)(CHashingEvent&);
typedef void (wxEvtHandler::*MuleCompletionEventFunction)(CCompletionEvent&);
typedef void (wxEvtHandler::*MuleAllocFinishedEventFunction)(CAllocFinishedEvent&);

//! Event-handler for completed hashings of new shared files and partfiles.
#define EVT_MULE_HASHING(func) \
	DECLARE_EVENT_TABLE_ENTRY(MULE_EVT_HASHING, -1, -1, \
	(wxObjectEventFunction) (wxEventFunction) \
	wxStaticCastEvent(MuleHashingEventFunction, &func), (wxObject*) NULL),

//! Event-handler for completed hashings of files that were missing a AICH hash.
#define EVT_MULE_AICH_HASHING(func) \
	DECLARE_EVENT_TABLE_ENTRY(MULE_EVT_AICH_HASHING, -1, -1, \
	(wxObjectEventFunction) (wxEventFunction) \
	wxStaticCastEvent(MuleHashingEventFunction, &func), (wxObject*) NULL),

//! Event-handler for completion of part-files.
#define EVT_MULE_FILE_COMPLETED(func) \
	DECLARE_EVENT_TABLE_ENTRY(MULE_EVT_FILE_COMPLETED, -1, -1, \
	(wxObjectEventFunction) (wxEventFunction) \
	wxStaticCastEvent(MuleCompletionEventFunction, &func), (wxObject*) NULL),

//! Event-handler for partfile preallocation finished events.
#define EVT_MULE_ALLOC_FINISHED(func) \
	DECLARE_EVENT_TABLE_ENTRY(MULE_EVT_ALLOC_FINISHED, -1, -1, \
	(wxObjectEventFunction) (wxEventFunction) \
	wxStaticCastEvent(MuleAllocFinishedEventFunction, &func), (wxObject*) NULL),


#endif // TASKS_H
// File_checked_for_headers
