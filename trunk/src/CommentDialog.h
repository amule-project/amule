//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef COMMENTDIALOG_H
#define COMMENTDIALOG_H

#include <wx/dialog.h>		// Needed for wxDialog
#include <wx/choice.h>		// Needed for wxChoice

class CKnownFile;

// CCommentDialog dialog 

class CCommentDialog : public wxDialog
{ 
public:
	CCommentDialog(wxWindow* pParent, CKnownFile* file);   // standard constructor
	virtual ~CCommentDialog();
	virtual bool OnInitDialog();
protected:
	DECLARE_EVENT_TABLE()
public:
	void OnBnClickedApply(wxCommandEvent& evt);
	void OnBnClickedClear(wxCommandEvent& evt);
	void OnBnClickedCancel(wxCommandEvent& evt); 
private:
	wxChoice* ratebox;
	CKnownFile* m_file;
};

#endif // COMMENTDIALOG_H
// File_checked_for_headers
