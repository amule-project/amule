// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Drager
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef EDITSERVERLISTDLG_H
#define EDITSERVERLISTDLG_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "EditServerListDlg.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dialog.h>		// Needed for wxDialog

class wxTextCtrl;

class EditServerListDlg : public wxDialog
{
public:
    EditServerListDlg(wxWindow *parent,
                      const wxString& caption,
                      const wxString& message,
                      const wxString& filename);
    
    virtual ~EditServerListDlg();

    void OnOK(wxCommandEvent& event);

protected:
    wxTextCtrl* m_textctrl;
    wxString    m_file;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(EditServerListDlg)
};

#endif // EDITSERVERLISTDLG_H
