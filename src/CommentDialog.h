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

#ifndef COMMENTDIALOG_H
#define COMMENTDIALOG_H

#include <wx/dialog.h>		// Needed for wxDialog
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/choice.h>		// Needed for wxChoice

#include "resource.h"		// Needed for IDD_COMMENT

class CKnownFile;

// CCommentDialog dialog 

class CCommentDialog : public wxDialog
{ 
public:
	CCommentDialog(wxWindow* pParent, CKnownFile* file);   // standard constructor
	virtual ~CCommentDialog();
	virtual bool OnInitDialog();
	enum { IDD = IDD_COMMENT };

protected:
	DECLARE_EVENT_TABLE()
public:
	void OnBnClickedApply(wxEvent& evt);
	void OnBnClickedClear(wxEvent& evt);
	void OnBnClickedCancel(wxEvent& evt); 
private:
	wxChoice* ratebox;
	CKnownFile* m_file;
};

#endif // COMMENTDIALOG_H
