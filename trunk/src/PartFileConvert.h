//
// This file is part of the aMule Project.
//
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef PARTFILECONVERT_H
#define PARTFILECONVERT_H

#ifndef CLIENT_GUI

#include <wx/string.h>
#include <wx/thread.h>

#include <list>

#include "Types.h"

#ifndef AMULE_DAEMON
	#include <wx/dialog.h>
#endif

struct ConvertJob;
class CPartFileConvertDlg;

class CPartFileConvert : private wxThread
{
public:
	static int	ScanFolderToAdd(wxString folder, bool deletesource = false);
	static void	ConvertToeMule(wxString folder, bool deletesource = false);
	static void	StartThread();
	static void	StopThread();
#ifndef AMULE_DAEMON
	static void	ShowGUI(wxWindow *parent);
	static void	UpdateGUI(float percent, wxString text, bool fullinfo = false);	// current file information
	static void	UpdateGUI(ConvertJob* job); // listcontrol update
	static void	CloseGUI();
#endif
	static void	RemoveAllJobs();
	static void	RemoveAllSuccJobs();
	static void	RemoveJob(ConvertJob* job);
	static wxString	GetReturncodeText(int ret);

	static wxMutex	s_mutex;

private:
	CPartFileConvert() : wxThread(wxTHREAD_DETACHED) {}

	static int	performConvertToeMule(wxString folder);
	virtual ExitCode Entry();

	static wxThread*		s_convertPfThread;
	static std::list<ConvertJob*>	s_jobs;
	static ConvertJob*		s_pfconverting;
#ifndef AMULE_DAEMON
	static CPartFileConvertDlg*	s_convertgui;
#endif
};
#endif /* ! CLIENT_GUI */

#ifndef AMULE_DAEMON

#include <wx/listctrl.h>

class CConvertListCtrl : public wxListCtrl
{
public:
	CConvertListCtrl(
			 wxWindow* parent,
			 wxWindowID winid = -1,
			 const wxPoint& pos = wxDefaultPosition,
			 const wxSize& size = wxDefaultSize,
			 long style = wxLC_ICON,
			 const wxValidator& validator = wxDefaultValidator,
			 const wxString& name = wxT("convertlistctrl"))
#ifdef CLIENT_GUI
			 {}
#else
			 ;
#endif
};

#ifndef CLIENT_GUI

#include <wx/gauge.h>

class CPartFileConvertDlg : public wxDialog
{
	friend class CPartFileConvert;
public:
	CPartFileConvertDlg(wxWindow *parent);

	void	AddJob(ConvertJob* job);
	void	RemoveJob(ConvertJob* job);
	void	UpdateJobInfo(ConvertJob* job);

protected:
	wxGauge*		m_pb_current;
	CConvertListCtrl*	m_joblist;

	void	OnAddFolder(wxCommandEvent& event);
	void	RetrySel(wxCommandEvent& event);
	void	RemoveSel(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};
#endif /* ! CLIENT_GUI */

#endif /* ! AMULE_DAEMON */

#endif /* PARTFILECONVERT_H */
