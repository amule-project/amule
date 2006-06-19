//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "PartFileConvert.h"

#ifdef CLIENT_GUI

// This allows us to compile muuli_wdr

CConvertListCtrl::CConvertListCtrl(
			 wxWindow* WXUNUSED(parent),
			 wxWindowID WXUNUSED(winid),
			 const wxPoint& WXUNUSED(pos),
			 const wxSize& WXUNUSED(size),
			 long WXUNUSED(style),
			 const wxValidator& WXUNUSED(validator),
			 const wxString& WXUNUSED(name))
			 {};
				 
#else 

// Normal compilation

#include "amule.h"
#include "DownloadQueue.h"
#include <common/Format.h>
#include "Logger.h"
#include "PartFile.h"
#include "Preferences.h"
#include "SharedFileList.h"
#include "FileFunctions.h"

#include <common/PlatformSpecific.h>
#include "muuli_wdr.h"



enum convstatus{
	CONV_OK			= 0,
	CONV_QUEUE,
	CONV_INPROGRESS,
	CONV_OUTOFDISKSPACE,
	CONV_PARTMETNOTFOUND,
	CONV_IOERROR,
	CONV_FAILED,
	CONV_BADFORMAT,
	CONV_ALREADYEXISTS
};

struct ConvertJob {
	wxString	folder;
	wxString	filename;
	wxString	filehash;
	int		format;
	int		state;
	uint32		size;
	uint32		spaceneeded;
	uint8		partmettype;
	bool		removeSource;
	ConvertJob()	{ size=0; spaceneeded=0; partmettype=PMT_UNKNOWN; removeSource = true; }
	//~ConvertJob() {}
};

wxThread*		CPartFileConvert::s_convertPfThread = NULL;
std::list<ConvertJob*>	CPartFileConvert::s_jobs;
ConvertJob*		CPartFileConvert::s_pfconverting = NULL;
wxMutex			CPartFileConvert::s_mutex;

CPartFileConvertDlg*	CPartFileConvert::s_convertgui = NULL;

int CPartFileConvert::ScanFolderToAdd(wxString folder, bool deletesource)
{
	int count = 0;
	CDirIterator finder(folder);
	wxString file;

	file = finder.GetFirstFile(CDirIterator::File, wxT("*.part.met"));
	while (!file.IsEmpty()) {
		ConvertToeMule(file, deletesource);
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
	while (!file.IsEmpty()) {
		if (file != wxT(".") && file != wxT("..")) {
			ScanFolderToAdd(file, deletesource);
		}
		file = finder.GetNextFile();
	}

	return count;
}

void CPartFileConvert::ConvertToeMule(wxString folder, bool deletesource)
{
	if (!wxFileName::FileExists(folder))
		return;
	
	ConvertJob* newjob = new ConvertJob();
	newjob->folder = folder;
	newjob->removeSource = deletesource;
	newjob->state = CONV_QUEUE;

	wxMutexLocker lock(s_mutex);

	s_jobs.push_back(newjob);

	if (s_convertgui) {
		s_convertgui->AddJob(newjob);
	}

	StartThread();
}

void CPartFileConvert::StartThread()
{
	if (!s_convertPfThread) {
		s_convertPfThread = new CPartFileConvert();
	
		switch ( s_convertPfThread->Create() ) {
			case wxTHREAD_NO_ERROR:
				AddDebugLogLineM( false, logPfConvert, wxT("A new thread has been created.") );
				break;
			case wxTHREAD_RUNNING:
				AddDebugLogLineM( true, logPfConvert, wxT("Error, attempt to create a already running thread!") );
				break;
			case wxTHREAD_NO_RESOURCE:
				AddDebugLogLineM( true, logPfConvert, wxT("Error, attempt to create a thread without resources!") );
				break;
			default:
				AddDebugLogLineM( true, logPfConvert, wxT("Error, unknown error attempting to create a thread!") );
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

	printf("Waiting for partfile convert thread to die...\n");
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
				s_pfconverting = *it;
				if (s_pfconverting->state == CONV_QUEUE) {
					break;
				} else {
					s_pfconverting = NULL;
				}
			}
		}

		if (s_pfconverting) {
			{
				wxMutexLocker lock(s_mutex);
				s_pfconverting->state = CONV_INPROGRESS;
			}

			UpdateGUI(s_pfconverting);

			int convertResult = performConvertToeMule(s_pfconverting->folder);
			{
				wxMutexLocker lock(s_mutex);
				s_pfconverting->state = convertResult;
			}

			if (s_pfconverting->state == CONV_OK) {
				++imported;
			}

			if (TestDestroy()) {
				wxMutexLocker lock(s_mutex);
				for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
					delete *it;
				}
				s_jobs.clear();
				break;
			}


			UpdateGUI(s_pfconverting);

			AddLogLineM(true, CFormat(_("Importing %s: %s")) % s_pfconverting->folder % GetReturncodeText(s_pfconverting->state));
		} else {
			break; // nothing more to do now
		}
	}

	// clean up
	UpdateGUI(NULL);

	if (imported) {
		theApp.sharedfiles->PublishNextTurn();
	}

	AddDebugLogLineM(false, logPfConvert, wxT("No more jobs on queue, exiting from thread."));

	s_convertPfThread = NULL;

	return NULL;
}

int CPartFileConvert::performConvertToeMule(wxString folder)
{
	wxString filepartindex, newfilename;
	wxString buffer;
	unsigned fileindex;
	wxString partfile = folder;
	folder = folder.BeforeLast(wxFileName::GetPathSeparator());
	partfile = partfile.AfterLast(wxFileName::GetPathSeparator());
	CDirIterator finder(folder);

	UpdateGUI(0, _("Reading temp folder"), true);

	filepartindex = partfile.BeforeFirst(wxT('.'));
	//int pos=filepartindex.ReverseFind('\\');
	//if (pos>-1) filepartindex=filepartindex.Mid(pos+1,filepartindex.GetLength()-pos);

	UpdateGUI(4, _("Retrieving basic information from download info file"));

	CPartFile* file = new CPartFile();
	s_pfconverting->partmettype = file->LoadPartFile(folder, partfile, false, true);

	switch (s_pfconverting->partmettype) {
		case PMT_UNKNOWN:
		case PMT_BADFORMAT:
			delete file;
			return CONV_BADFORMAT;
	}

	wxString oldfile = JoinPaths(folder, partfile.Left(partfile.Length() - ((s_pfconverting->partmettype == PMT_SHAREAZA) ? 3 : 4)));

	{
		wxMutexLocker lock(s_mutex);
		s_pfconverting->size = file->GetFileSize();
		s_pfconverting->filename = file->GetFileName();
		s_pfconverting->filehash = file->GetFileHash().Encode();
	}

	UpdateGUI(s_pfconverting);

	if (theApp.downloadqueue->GetFileByID(file->GetFileHash())) {
		delete file;
		return CONV_ALREADYEXISTS;
	}

	if (s_pfconverting->partmettype == PMT_SPLITTED) {
		char *ba = new char [PARTSIZE];

		try {			
			CFile inputfile;
			wxString filename;

			// just count
			unsigned maxindex = 0;
			unsigned partfilecount = 0;
			buffer = finder.GetFirstFile(CDirIterator::File, filepartindex + wxT(".*.part"));
			while (!buffer.IsEmpty()) {
				long l;
				++partfilecount;
				buffer.AfterLast(wxFileName::GetPathSeparator()).AfterFirst(wxT('.')).BeforeLast(wxT('.')).ToLong(&l);
				fileindex = (unsigned)l;
				buffer = finder.GetNextFile();
				// GonoszTopi - why the hell does eMule need this??
				//if (fileindex == 0) continue;
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
						s_pfconverting->spaceneeded = ((s_pfconverting->size / PARTSIZE) * PARTSIZE) + (s_pfconverting->size % PARTSIZE);
					}
				} else {
					stepperpart = 80.0f;
					s_pfconverting->spaceneeded = 0;
				}
			}

			UpdateGUI(s_pfconverting);

			wxLongLong freespace;
			if (wxGetDiskSpace(thePrefs::GetTempDir(), NULL, &freespace)) {
				if (freespace < (maxindex * PARTSIZE)) {
					delete file;
					delete [] ba;
					return CONV_OUTOFDISKSPACE;
				}
			}

			// create new partmetfile, and remember the new name
			file->CreatePartFile();
			newfilename = file->GetFullName();

			UpdateGUI(8, _("Creating destination file"));

			file->m_hpartfile.SetLength( s_pfconverting->spaceneeded );

			unsigned curindex = 0;
			filename = finder.GetFirstFile(CDirIterator::File, filepartindex + wxT(".*.part"));
			while (!filename.IsEmpty()) {
				// stats
				++curindex;
				buffer = wxString::Format(_("Loading data from old download file (%u of %u)"), curindex, partfilecount);

				UpdateGUI(10 + (curindex * stepperpart), buffer);

				long l;
				filename.AfterLast(wxFileName::GetPathSeparator()).AfterFirst(wxT('.')).BeforeLast(wxT('.')).ToLong(&l);
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

				buffer = wxString::Format(_("Saving data block into new single download file (%u of %u)"), curindex, partfilecount);

				UpdateGUI(10 + (curindex * stepperpart), buffer);

				// write the buffered data
				file->m_hpartfile.Seek(chunkstart, wxFromStart);
				file->m_hpartfile.Write(ba, toReadWrite);

				filename = finder.GetNextFile();
			}
			delete[] ba;
		} catch (const CSafeIOException& e) {
			AddDebugLogLineM(true, logPfConvert, wxT("IO error while converting partfiles: ") + e.what());
			
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
			wxFile f(oldfile);
			wxMutexLocker lock(s_mutex);
			s_pfconverting->spaceneeded = f.Length();
		}

		UpdateGUI(s_pfconverting);

		wxLongLong freespace;
		if (!wxGetDiskSpace(thePrefs::GetTempDir(), NULL, &freespace)) {
			delete file;
			return CONV_IOERROR;
		}
		if (!s_pfconverting->removeSource && (freespace < s_pfconverting->spaceneeded)) {
			delete file;
			return CONV_OUTOFDISKSPACE;
		}

		file->CreatePartFile();
		newfilename = file->GetFullName();

		file->m_hpartfile.Close();

		bool ret = false;

		UpdateGUI(92, _("Copy"));

		wxRemoveFile(newfilename.Left(newfilename.Length()-4));

		if (!wxFileName::FileExists(oldfile)) {
			// data file does not exist. well, then create a 0 byte big one
			CFile datafile;
			ret = datafile.Create(newfilename.Left(newfilename.Length() - 4));
		} else if (s_pfconverting->removeSource) {
			ret = wxRenameFile(oldfile, newfilename.Left(newfilename.Length() - 4));
		} else {
			ret = wxCopyFile(oldfile, newfilename.Left(newfilename.Length() - 4), false);
		}
		if (!ret) {
			file->Delete();
			//delete file;
			return CONV_FAILED;
		}

	}

	UpdateGUI(94, _("Retrieving source downloadfile information"));

	wxRemoveFile(newfilename);
	if (s_pfconverting->removeSource) {
		wxRenameFile(JoinPaths(folder, partfile), newfilename);
	} else {
		wxCopyFile(JoinPaths(folder, partfile), newfilename, false);
	}

	file->m_hashlist.clear();

	DeleteContents(file->m_gaplist);

	if (!file->LoadPartFile(thePrefs::GetTempDir(), file->GetPartMetFileName(), false)) {
		//delete file;
		file->Delete();
		return CONV_BADFORMAT;
	}

	if (s_pfconverting->partmettype == PMT_NEWOLD || s_pfconverting->partmettype == PMT_SPLITTED ) {
		file->SetCompletedSize(file->transfered);
		file->m_iGainDueToCompression = 0;
		file->m_iLostDueToCorruption = 0;
	}

	UpdateGUI(100, _("Adding download and saving new partfile"));

	theApp.downloadqueue->AddDownload(file, thePrefs::AddNewFilesPaused(), 0);
	file->SavePartFile();
	
	if (file->GetStatus(true) == PS_READY) {
		theApp.sharedfiles->SafeAddKFile(file); // part files are always shared files
	}

	if (s_pfconverting->removeSource) {
		buffer = finder.GetFirstFile(CDirIterator::File, filepartindex + wxT(".*"));
		while (!buffer.IsEmpty()) {
			wxRemoveFile(buffer);
			buffer = finder.GetNextFile();
		}

		if (s_pfconverting->partmettype == PMT_SPLITTED) {
			#ifndef __VMS__
				wxRmdir(folder);
			#else
				#warning wxRmdir does not work under VMS !
			#endif
		}
	}

	return CONV_OK;
}

void CPartFileConvert::UpdateGUI(float percent, wxString text, bool fullinfo)
{
	if (!IsMain()) {
		wxMutexGuiEnter();
	}

	if (s_convertgui) {
		s_convertgui->m_pb_current->SetValue((int)percent);
		wxString buffer = wxString::Format(wxT("%.2f %%"), percent);
		wxStaticText* percentlabel = dynamic_cast<wxStaticText*>(s_convertgui->FindWindow(IDC_CONV_PROZENT));
		percentlabel->SetLabel(buffer);

		if (!text.IsEmpty()) {
			dynamic_cast<wxStaticText*>(s_convertgui->FindWindow(IDC_CONV_PB_LABEL))->SetLabel(text);
		}

		percentlabel->GetParent()->Layout();

		if (fullinfo) {
			dynamic_cast<wxStaticBoxSizer*>(IDC_CURJOB)->GetStaticBox()->SetLabel(s_pfconverting->folder);
		}
	}

	if (!IsMain()) {
		wxMutexGuiLeave();
	}
}

void CPartFileConvert::UpdateGUI(ConvertJob* job)
{
	if (!IsMain()) {
		wxMutexGuiEnter();
	}
	if (s_convertgui) {
		s_convertgui->UpdateJobInfo(job);
	}
	if (!IsMain()) {
		wxMutexGuiLeave();
	}
}

void CPartFileConvert::ShowGUI(wxWindow* parent)
{
	if (s_convertgui) {
		s_convertgui->Show(true);
		s_convertgui->Raise();
	} else {
		s_convertgui = new CPartFileConvertDlg(parent);
		s_convertgui->Show(true);

		wxMutexLocker lock(s_mutex);
		if (s_pfconverting) {
			UpdateGUI(s_pfconverting);
			UpdateGUI(50, _("Fetching status..."), true);
		}

		// fill joblist
		for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
			s_convertgui->AddJob(*it);
			UpdateGUI(*it);
		}
	}
}

void CPartFileConvert::CloseGUI()
{
	if (s_convertgui) {
		s_convertgui->Show(false);
		s_convertgui->Destroy();
		s_convertgui = NULL;
	}
}

void CPartFileConvert::RemoveAllJobs()
{
	wxMutexLocker lock(s_mutex);
	for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
		if (s_convertgui) {
			s_convertgui->RemoveJob(*it);
		}
		delete *it;
	}
	s_jobs.clear();
}

void CPartFileConvert::RemoveJob(ConvertJob* job)
{
	wxMutexLocker lock(s_mutex);
	for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
		if (*it == job) {
			if (s_convertgui) {
				s_convertgui->RemoveJob(job);
			}
			s_jobs.erase(it);
			delete *it;
		}
	}
}

void CPartFileConvert::RemoveAllSuccJobs()
{
	wxMutexLocker lock(s_mutex);
	for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
		if ((*it)->state == CONV_OK) {
			if (s_convertgui) {
				s_convertgui->RemoveJob(*it);
			}
			s_jobs.erase(it);
			delete *it;
		}
	}
}

wxString CPartFileConvert::GetReturncodeText(int ret)
{
	switch (ret) {
		case CONV_OK			: return _("Completed");
		case CONV_INPROGRESS		: return _("In progress");
		case CONV_OUTOFDISKSPACE	: return _("Error: Out of diskspace");
		case CONV_PARTMETNOTFOUND	: return _("Error: Partmet not found");
		case CONV_IOERROR		: return _("Error: IO error!");
		case CONV_FAILED		: return _("Error: Failed!");
		case CONV_QUEUE			: return _("Queued");
		case CONV_ALREADYEXISTS		: return _("Already downloading");
		case CONV_BADFORMAT		: return _("Unknown or bad tempfile format.");
		default: return wxT("?");
	}
}


CConvertListCtrl::CConvertListCtrl(wxWindow* parent, wxWindowID winid, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name)
	: wxListCtrl(parent, winid, pos, size, style, validator, name)
{
	InsertColumn(0, _("File name"),	wxLIST_FORMAT_LEFT, 200);
	InsertColumn(1, _("State"),	wxLIST_FORMAT_LEFT, 100);
	InsertColumn(2, _("Size"),	wxLIST_FORMAT_LEFT, 100);
	InsertColumn(3, _("Filehash"),	wxLIST_FORMAT_LEFT, 100);
}

#ifndef __WXMSW__
/* XPM */
static char * convert_xpm[] = {
"16 16 9 1",
" 	c None",
".	c #B20000",
"+	c #FF0000",
"@	c #FF7F7F",
"#	c #008000",
"$	c #33B200",
"%	c #10E500",
"&	c #59FE4C",
"*	c #FFB2B2",
"        .       ",
"       .+.      ",
"      .+@+.     ",
"     .+@+.      ",
".   .+@+.#######",
".. .+@+.  #$%%&#",
".+.+@+.    #$%%#",
".@+@+.    #$%$%#",
".@@+.    #$%$#$#",
".*@@+.  #$%$# ##",
".......#$%$#   #",
"      #$%$#     ",
"     #$%$#      ",
"    #$%$#       ",
"     #$#        ",
"      #         "};
#endif /* ! __WXMSW__ */

// Modeless Dialog Implementation
// CPartFileConvertDlg dialog

BEGIN_EVENT_TABLE(CPartFileConvertDlg, wxDialog)
	EVT_BUTTON(IDC_ADDITEM,		CPartFileConvertDlg::OnAddFolder)
	EVT_BUTTON(IDC_RETRY,		CPartFileConvertDlg::RetrySel)
	EVT_BUTTON(IDC_CONVREMOVE,	CPartFileConvertDlg::RemoveSel)
	EVT_BUTTON(wxID_CANCEL,		CPartFileConvertDlg::OnCloseButton)
	EVT_CLOSE(CPartFileConvertDlg::OnClose)
END_EVENT_TABLE()

CPartFileConvertDlg::CPartFileConvertDlg(wxWindow* parent)
	: wxDialog(parent, -1, _("Import partfiles"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	convertDlg(this, true, true);

	m_joblist = CastChild(IDC_JOBLIST, CConvertListCtrl);
	m_pb_current = CastChild(IDC_CONV_PB_CURRENT, wxGauge);

	SetIcon(wxICON(convert));

	// for some reason, if I try to get the mutex from the dialog
	// it will end up in a deadlock(?) and I have to kill aMule
	CastChild(IDC_RETRY, wxButton)->Enable(false);
	CastChild(IDC_CONVREMOVE, wxButton)->Enable(false);
}

// CPartFileConvertDlg message handlers

void CPartFileConvertDlg::OnAddFolder(wxCommandEvent& WXUNUSED(event))
{
	wxString folder = ::wxDirSelector(_("Please choose a folder to search for temporary downloads! (subfolders will be included)"),
					  GetDocumentsDir());
	if (!folder.IsEmpty()) {
		int reply = wxMessageBox(_("Do you want the source files of succesfully imported downloads be deleted?"),
					 _("Remove sources?"),
					 wxYES_NO | wxCANCEL | wxICON_QUESTION, this);
		if (reply != wxCANCEL) {
			CPartFileConvert::ScanFolderToAdd(folder, (reply == wxYES));
		}
	}
}

void CPartFileConvertDlg::OnClose(wxCloseEvent& WXUNUSED(event))
{
	CPartFileConvert::CloseGUI();
}

void CPartFileConvertDlg::OnCloseButton(wxCommandEvent& WXUNUSED(event))
{
	CPartFileConvert::CloseGUI();
}

void CPartFileConvertDlg::UpdateJobInfo(ConvertJob* job)
{
	if (!job) {
		dynamic_cast<wxStaticBoxSizer*>(IDC_CURJOB)->GetStaticBox()->SetLabel(_("Waiting..."));
		CastChild(IDC_CONV_PROZENT, wxStaticText)->SetLabel(wxEmptyString);
		m_pb_current->SetValue(0);
		CastChild(IDC_CONV_PB_LABEL, wxStaticText)->SetLabel(wxEmptyString);
		return;
	}

	wxString buffer;

	// search jobitem in listctrl
	long itemnr = m_joblist->FindItem(-1, (long)job);
	if (itemnr != -1) {
		m_joblist->SetItem(itemnr, 0, job->filename.IsEmpty() ? job->folder : job->filename );
		m_joblist->SetItem(itemnr, 1, CPartFileConvert::GetReturncodeText(job->state) );
		if (job->size > 0) {
			buffer = CFormat(_("%s (Disk: %s)")) % CastItoXBytes(job->size) % CastItoXBytes(job->spaceneeded);
			m_joblist->SetItem(itemnr, 2, buffer );
		} else {
			m_joblist->SetItem(itemnr, 2, wxEmptyString);
		}
		m_joblist->SetItem(itemnr, 3, job->filehash);

	} else {
//		AddJob(job);	why???
	}
}

void CPartFileConvertDlg::RemoveJob(ConvertJob* job)
{
	long itemnr = m_joblist->FindItem(-1, (long)job);
	if (itemnr != -1) {
		m_joblist->DeleteItem(itemnr);
	}
}

void CPartFileConvertDlg::AddJob(ConvertJob* job)
{
	long ix = m_joblist->InsertItem(m_joblist->GetItemCount(), job->folder);
	if (ix != -1) {
		m_joblist->SetItemData(ix, (long)job);
		m_joblist->SetItem(ix, 1, CPartFileConvert::GetReturncodeText(job->state));
	}
}

void CPartFileConvertDlg::RemoveSel(wxCommandEvent& WXUNUSED(event))
{
	if (m_joblist->GetSelectedItemCount() == 0) return;

	long itemnr = m_joblist->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while (itemnr != -1) {
		ConvertJob* job = (ConvertJob*)m_joblist->GetItemData(itemnr);
		if (job->state != CONV_INPROGRESS) {
			// this will remove the job from both gui and list
			CPartFileConvert::RemoveJob(job);
		}
		itemnr = m_joblist->GetNextItem(itemnr, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
}

void CPartFileConvertDlg::RetrySel(wxCommandEvent& WXUNUSED(event))
{
	if (m_joblist->GetSelectedItemCount() == 0) return;

	long itemnr = m_joblist->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while (itemnr != -1) {
		ConvertJob* job = (ConvertJob*)m_joblist->GetItemData(itemnr);
		wxMutexLocker lock(CPartFileConvert::s_mutex);
		if (job->state != CONV_OK && job->state != CONV_INPROGRESS) {
			job->state = CONV_QUEUE;
			UpdateJobInfo(job);
		}
	}
	wxMutexLocker lock(CPartFileConvert::s_mutex);
	CPartFileConvert::StartThread();
}

#endif
// File_checked_for_headers
