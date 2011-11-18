//
// This file is part of the aMule Project.
//
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

#include <wx/app.h>

#include "PartFileConvert.h"

#include "amule.h"
#include "DownloadQueue.h"
#include <common/Format.h>
#include "Logger.h"
#include "PartFile.h"
#include "Preferences.h"
#include "SharedFileList.h"
#include <common/FileFunctions.h>
#include "OtherFunctions.h"
#include "GuiEvents.h"
#include "DataToText.h"

static unsigned s_nextJobId = 0;


struct ConvertJob {
	unsigned	id;
	CPath		folder;
	CPath		filename;
	wxString	filehash;
	int		format;
	ConvStatus	state;
	uint32_t	size;
	uint32_t	spaceneeded;
	uint8		partmettype;
	bool		removeSource;
	ConvertJob()	{ id=s_nextJobId++; size=0; spaceneeded=0; partmettype=PMT_UNKNOWN; removeSource=true; }
};

ConvertInfo::ConvertInfo(ConvertJob *job)
	: id(job->id),
	  folder(job->folder), filename(job->filename), filehash(job->filehash),
	  state(job->state), size(job->size), spaceneeded(job->spaceneeded)
{}


wxThread*		CPartFileConvert::s_convertPfThread = NULL;
std::list<ConvertJob*>	CPartFileConvert::s_jobs;
ConvertJob*		CPartFileConvert::s_pfconverting = NULL;
wxMutex			CPartFileConvert::s_mutex;


int CPartFileConvert::ScanFolderToAdd(const CPath& folder, bool deletesource)
{
	int count = 0;
	CDirIterator finder(folder);

	CPath file = finder.GetFirstFile(CDirIterator::File, wxT("*.part.met"));
	while (file.IsOk()) {
		ConvertToeMule(folder.JoinPaths(file), deletesource);
		file = finder.GetNextFile();
		count++;
	}
	/* Shareaza
	file = finder.GetFirstFile(CDirIterator::File, wxT("*.sd"));
	while (!file.IsEmpty()) {
		ConvertToeMule(file, deletesource);
		file = finder.GetNextFile();
		count++;
	}
	*/

	file = finder.GetFirstFile(CDirIterator::Dir, wxT("*.*"));
	while (file.IsOk()) {
		ScanFolderToAdd(folder.JoinPaths(file), deletesource);

		file = finder.GetNextFile();
	}

	return count;
}

void CPartFileConvert::ConvertToeMule(const CPath& file, bool deletesource)
{
	if (!file.FileExists()) {
		return;
	}
	
	ConvertJob* newjob = new ConvertJob();
	newjob->folder = file;
	newjob->removeSource = deletesource;
	newjob->state = CONV_QUEUE;

	wxMutexLocker lock(s_mutex);

	s_jobs.push_back(newjob);

	Notify_ConvertUpdateJobInfo(newjob);

	StartThread();
}

void CPartFileConvert::StartThread()
{
	if (!s_convertPfThread) {
		s_convertPfThread = new CPartFileConvert();
	
		switch ( s_convertPfThread->Create() ) {
			case wxTHREAD_NO_ERROR:
				AddDebugLogLineN( logPfConvert, wxT("A new thread has been created.") );
				break;
			case wxTHREAD_RUNNING:
				AddDebugLogLineC( logPfConvert, wxT("Error, attempt to create an already running thread!") );
				break;
			case wxTHREAD_NO_RESOURCE:
				AddDebugLogLineC( logPfConvert, wxT("Error, attempt to create a thread without resources!") );
				break;
			default:
				AddDebugLogLineC( logPfConvert, wxT("Error, unknown error attempting to create a thread!") );
		}

		// The thread shouldn't hog the CPU, as it will already be hogging the HD
		s_convertPfThread->SetPriority(WXTHREAD_MIN_PRIORITY);
		
		s_convertPfThread->Run();
	}
}

void CPartFileConvert::StopThread()
{
	if (s_convertPfThread) {
		s_convertPfThread->Delete();
	} else {
		return;
	}

	AddLogLineNS(_("Waiting for partfile convert thread to die..."));
	while (s_convertPfThread) {
		wxSleep(1);
	}
}

wxThread::ExitCode CPartFileConvert::Entry()
{
	int imported = 0;

	for (;;)
	{
		// search next queued job and start it
		{
			wxMutexLocker lock(s_mutex);
			s_pfconverting = NULL;
			for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
				if ((*it)->state == CONV_QUEUE) {
					s_pfconverting = *it;
					break;
				}
			}
		}

		if (s_pfconverting) {
			{
				wxMutexLocker lock(s_mutex);
				s_pfconverting->state = CONV_INPROGRESS;
			}

			Notify_ConvertUpdateJobInfo(s_pfconverting);

			ConvStatus convertResult = performConvertToeMule(s_pfconverting->folder);
			{
				wxMutexLocker lock(s_mutex);
				s_pfconverting->state = convertResult;
			}

			if (s_pfconverting->state == CONV_OK) {
				++imported;
			}

			Notify_ConvertUpdateJobInfo(s_pfconverting);

			AddLogLineC(CFormat(_("Importing %s: %s")) % s_pfconverting->folder % GetConversionState(s_pfconverting->state));

			if (TestDestroy()) {
				wxMutexLocker lock(s_mutex);
				DeleteContents(s_jobs);
				break;
			}
		} else {
			break; // nothing more to do now
		}
	}

	// clean up
	Notify_ConvertClearInfos();

	if (imported) {
		theApp->sharedfiles->PublishNextTurn();
	}

	AddDebugLogLineN(logPfConvert, wxT("No more jobs on queue, exiting from thread."));

	s_convertPfThread = NULL;

	return NULL;
}

ConvStatus CPartFileConvert::performConvertToeMule(const CPath& fileName)
{
	wxString filepartindex, buffer;
	unsigned fileindex;
	
	CPath folder	= fileName.GetPath();
	CPath partfile	= fileName.GetFullName();
	CPath newfilename;

	CDirIterator finder(folder);

	Notify_ConvertUpdateProgressFull(0, _("Reading temp folder"), s_pfconverting->folder.GetPrintable());

	filepartindex = partfile.RemoveAllExt().GetRaw();

	Notify_ConvertUpdateProgress(4, _("Retrieving basic information from download info file"));

	CPartFile* file = new CPartFile();
	s_pfconverting->partmettype = file->LoadPartFile(folder, partfile, false, true);

	switch (s_pfconverting->partmettype) {
		case PMT_UNKNOWN:
		case PMT_BADFORMAT:
			delete file;
			return CONV_BADFORMAT;
	}

	CPath oldfile = folder.JoinPaths(partfile.RemoveExt());

	{
		wxMutexLocker lock(s_mutex);
		s_pfconverting->size = file->GetFileSize();
		s_pfconverting->filename = file->GetFileName();
		s_pfconverting->filehash = file->GetFileHash().Encode();
	}

	Notify_ConvertUpdateJobInfo(s_pfconverting);

	if (theApp->downloadqueue->GetFileByID(file->GetFileHash())) {
		delete file;
		return CONV_ALREADYEXISTS;
	}

	if (s_pfconverting->partmettype == PMT_SPLITTED) {
		char *ba = new char [PARTSIZE];

		try {			
			CFile inputfile;

			// just count
			unsigned maxindex = 0;
			unsigned partfilecount = 0;
			CPath filePath = finder.GetFirstFile(CDirIterator::File, filepartindex + wxT(".*.part"));
			while (filePath.IsOk()) {
				long l;
				++partfilecount;
				filePath.GetFullName().RemoveExt().GetExt().ToLong(&l);
				fileindex = (unsigned)l;
				filePath = finder.GetNextFile();
				if (fileindex > maxindex) maxindex = fileindex;
			}
			float stepperpart;
			{
				wxMutexLocker lock(s_mutex);
				if (partfilecount > 0) {
					stepperpart = (80.0f / partfilecount);
					if (maxindex * PARTSIZE <= s_pfconverting->size) {
						s_pfconverting->spaceneeded = maxindex * PARTSIZE;
					} else {
						s_pfconverting->spaceneeded = s_pfconverting->size;
					}
				} else {
					stepperpart = 80.0f;
					s_pfconverting->spaceneeded = 0;
				}
			}

			Notify_ConvertUpdateJobInfo(s_pfconverting);

			sint64 freespace = CPath::GetFreeSpaceAt(thePrefs::GetTempDir());
			if (freespace != wxInvalidOffset) {
				if (static_cast<uint64>(freespace) < maxindex * PARTSIZE) {
					delete file;
					delete [] ba;
					return CONV_OUTOFDISKSPACE;
				}
			}

			// create new partmetfile, and remember the new name
			file->CreatePartFile();
			newfilename = file->GetFullName();

			Notify_ConvertUpdateProgress(8, _("Creating destination file"));

			file->m_hpartfile.SetLength( s_pfconverting->spaceneeded );

			unsigned curindex = 0;
			CPath filename = finder.GetFirstFile(CDirIterator::File, filepartindex + wxT(".*.part"));
			while (filename.IsOk()) {
				// stats
				++curindex;
				buffer = CFormat(_("Loading data from old download file (%u of %u)")) % curindex % partfilecount;

				Notify_ConvertUpdateProgress(10 + (curindex * stepperpart), buffer);

				long l;
				filename.GetFullName().RemoveExt().GetExt().ToLong(&l);
				fileindex = (unsigned)l;
				if (fileindex == 0) {
					filename = finder.GetNextFile();
					continue;
				}

				uint32 chunkstart = (fileindex - 1) * PARTSIZE;
				
				// open, read data of the part-part-file into buffer, close file
				inputfile.Open(filename, CFile::read);
				uint64 toReadWrite = std::min<uint64>(PARTSIZE, inputfile.GetLength());
				inputfile.Read(ba, toReadWrite);
				inputfile.Close();

				buffer = CFormat(_("Saving data block into new single download file (%u of %u)")) % curindex % partfilecount;

				Notify_ConvertUpdateProgress(10 + (curindex * stepperpart), buffer);

				// write the buffered data
				file->m_hpartfile.WriteAt(ba, chunkstart, toReadWrite);

				filename = finder.GetNextFile();
			}
			delete[] ba;
		} catch (const CSafeIOException& e) {
			AddDebugLogLineC(logPfConvert, wxT("IO error while converting partfiles: ") + e.what());
			
			delete[] ba;
			file->Delete();
			return CONV_IOERROR;
		}
		
		file->m_hpartfile.Close();
	}
	// import an external common format partdownload
	else //if (pfconverting->partmettype==PMT_DEFAULTOLD || pfconverting->partmettype==PMT_NEWOLD || Shareaza  ) 
	{
		if (!s_pfconverting->removeSource) {
			wxMutexLocker lock(s_mutex);
			s_pfconverting->spaceneeded = oldfile.GetFileSize();
		}

		Notify_ConvertUpdateJobInfo(s_pfconverting);

		sint64 freespace = CPath::GetFreeSpaceAt(thePrefs::GetTempDir());
		if (freespace == wxInvalidOffset) {
			delete file;
			return CONV_IOERROR;
		} else if (freespace < s_pfconverting->spaceneeded) {
			delete file;
			return CONV_OUTOFDISKSPACE;
		}

		file->CreatePartFile();
		newfilename = file->GetFullName();

		file->m_hpartfile.Close();

		bool ret = false;

		Notify_ConvertUpdateProgress(92, _("Copy"));

		CPath::RemoveFile(newfilename.RemoveExt());
		if (!oldfile.FileExists()) {
			// data file does not exist. well, then create a 0 byte big one
			CFile datafile;
			ret = datafile.Create(newfilename.RemoveExt());
		} else if (s_pfconverting->removeSource) {
			ret = CPath::RenameFile(oldfile, newfilename.RemoveExt());
		} else {
			ret = CPath::CloneFile(oldfile, newfilename.RemoveExt(), false);
		}
		if (!ret) {
			file->Delete();
			//delete file;
			return CONV_FAILED;
		}

	}

	Notify_ConvertUpdateProgress(94, _("Retrieving source downloadfile information"));

	CPath::RemoveFile(newfilename);
	if (s_pfconverting->removeSource) {
		CPath::RenameFile(folder.JoinPaths(partfile), newfilename);
	} else {
		CPath::CloneFile(folder.JoinPaths(partfile), newfilename, false);
	}

	file->m_hashlist.clear();

	if (!file->LoadPartFile(thePrefs::GetTempDir(), file->GetPartMetFileName(), false)) {
		//delete file;
		file->Delete();
		return CONV_BADFORMAT;
	}

	if (s_pfconverting->partmettype == PMT_NEWOLD || s_pfconverting->partmettype == PMT_SPLITTED) {
		file->SetCompletedSize(file->transferred);
		file->m_iGainDueToCompression = 0;
		file->m_iLostDueToCorruption = 0;
	}

	Notify_ConvertUpdateProgress(100, _("Adding download and saving new partfile"));

	theApp->downloadqueue->AddDownload(file, thePrefs::AddNewFilesPaused(), 0);
	file->SavePartFile();
	
	if (file->GetStatus(true) == PS_READY) {
		theApp->sharedfiles->SafeAddKFile(file); // part files are always shared files
	}

	if (s_pfconverting->removeSource) {
		CPath oldFile = finder.GetFirstFile(CDirIterator::File, filepartindex + wxT(".*"));
		while (oldFile.IsOk()) {
			CPath::RemoveFile(folder.JoinPaths(oldFile));
			oldFile = finder.GetNextFile();
		}

		if (s_pfconverting->partmettype == PMT_SPLITTED) {
			CPath::RemoveDir(folder);
		}
	}

	return CONV_OK;
}

// Notification handlers

void CPartFileConvert::RemoveJob(unsigned id)
{
	wxMutexLocker lock(s_mutex);
	for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
		if ((*it)->id == id && (*it)->state != CONV_INPROGRESS) {
			ConvertJob *job = *it;
			s_jobs.erase(it);
			Notify_ConvertRemoveJobInfo(id);
			delete job;
			break;
		}
	}
}

void CPartFileConvert::RetryJob(unsigned id)
{
	wxMutexLocker lock(s_mutex);
	for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
		if ((*it)->id == id && (*it)->state != CONV_INPROGRESS && (*it)->state != CONV_OK) {
			(*it)->state = CONV_QUEUE;
			Notify_ConvertUpdateJobInfo(*it);
			StartThread();
			break;
		}
	}
}

void CPartFileConvert::ReaddAllJobs()
{
	wxMutexLocker lock(s_mutex);
	for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
		Notify_ConvertUpdateJobInfo(*it);
	}
}
