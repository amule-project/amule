//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef PARTFILECONVERTDLG_H
#define PARTFILECONVERTDLG_H

#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/gauge.h>

struct ConvertInfo;

class CPartFileConvertDlg : public wxDialog
{
public:
	CPartFileConvertDlg(wxWindow *parent);

	static void	ShowGUI(wxWindow *parent);
	static void	UpdateProgress(float percent, wxString text = wxEmptyString, wxString header = wxEmptyString);
	static void	UpdateJobInfo(ConvertInfo& info);
	static void	RemoveJobInfo(unsigned id);
	static void	ClearInfo();
	static void	CloseGUI();

protected:
	wxGauge*	m_pb_current;
	wxListCtrl*	m_joblist;

	void	OnAddFolder(wxCommandEvent& event);
	void	OnClose(wxCloseEvent& event);
	void	OnCloseButton(wxCommandEvent& event);
	void	RetrySel(wxCommandEvent& event);
	void	RemoveSel(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()

	static CPartFileConvertDlg*	s_convertgui;
};

#endif /* PARTFILECONVERTDLG_H */
